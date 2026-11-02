
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "RTOS_CONTROLLER";

#define SENSOR_INTERVAL_MS    500
#define DIAGNOSTIC_INTERVAL   3000
#define TEMP_ALERT_THRESHOLD 30

/* Sensor message */
typedef struct
{
    uint32_t sample_id;
    int temperature;
} sensor_data_t;

/* Shared controller state */
typedef struct
{
    uint32_t processed_samples;
    int latest_temperature;
    int alert_active;
} controller_state_t;

/* RTOS objects */
static QueueHandle_t sensorQueue;
static SemaphoreHandle_t stateMutex;
static SemaphoreHandle_t diagnosticSemaphore;

static TimerHandle_t diagnosticTimer;

static TaskHandle_t alertTaskHandle = NULL;
static TaskHandle_t controllerTaskHandle = NULL;

/* Shared state */
static controller_state_t controllerState = {0};

/* ---------------------------------- */
/* Sensor Simulation Task              */
/* ---------------------------------- */

static void sensor_task(void *parameter)
{
    uint32_t sampleCount = 0;

    while (1)
    {
        sensor_data_t sample;

        sample.sample_id = ++sampleCount;

        /* Simulate temperature from 24 to 34 degrees */
        sample.temperature = 24 + ((sampleCount - 1) % 11);

        if (xQueueSend(sensorQueue, &sample,
                       pdMS_TO_TICKS(100)) == pdPASS)
        {
            ESP_LOGI(TAG,
                     "Sensor sample #%lu: %d C",
                     (unsigned long)sample.sample_id,
                     sample.temperature);
        }
        else
        {
            ESP_LOGW(TAG, "Sensor queue full");
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_INTERVAL_MS));
    }
}

/* ---------------------------------- */
/* Controller Task                     */
/* ---------------------------------- */

static void controller_task(void *parameter)
{
    sensor_data_t receivedData;

    while (1)
    {
        if (xQueueReceive(sensorQueue, &receivedData,
                          portMAX_DELAY) == pdPASS)
        {
            if (xSemaphoreTake(stateMutex,
                               portMAX_DELAY) == pdTRUE)
            {
                controllerState.processed_samples++;
                controllerState.latest_temperature =
                    receivedData.temperature;

                controllerState.alert_active =
                    (receivedData.temperature >
                     TEMP_ALERT_THRESHOLD);

                xSemaphoreGive(stateMutex);
            }

            ESP_LOGI(TAG,
                     "Controller processed sample #%lu",
                     (unsigned long)receivedData.sample_id);

            /* Notify alert task if temperature is high */
            if (receivedData.temperature >
                TEMP_ALERT_THRESHOLD)
            {
                xTaskNotifyGive(alertTaskHandle);
            }
        }
    }
}

/* ---------------------------------- */
/* Alert Task                          */
/* ---------------------------------- */

static void alert_task(void *parameter)
{
    while (1)
    {
        /* Wait for controller notification */
        uint32_t notificationCount =
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGW(TAG,
                 "HIGH TEMPERATURE ALERT! Events: %lu",
                 (unsigned long)notificationCount);
    }
}

/* ---------------------------------- */
/* Timer Callback                      */
/* ---------------------------------- */

static void diagnostic_timer_callback(TimerHandle_t timer)
{
    /*
     * This callback runs in the FreeRTOS timer
     * service task, not in interrupt context.
     */
    xSemaphoreGive(diagnosticSemaphore);
}

/* ---------------------------------- */
/* Diagnostic Task                     */
/* ---------------------------------- */

static void diagnostic_task(void *parameter)
{
    while (1)
    {
        /* Wait for periodic timer signal */
        if (xSemaphoreTake(diagnosticSemaphore,
                          portMAX_DELAY) == pdTRUE)
        {
            controller_state_t snapshot;

            if (xSemaphoreTake(stateMutex,
                              portMAX_DELAY) == pdTRUE)
            {
                snapshot = controllerState;
                xSemaphoreGive(stateMutex);

                ESP_LOGI(TAG, "===== SYSTEM DIAGNOSTICS =====");

                ESP_LOGI(TAG,
                         "Processed Samples : %lu",
                         (unsigned long)snapshot.processed_samples);

                ESP_LOGI(TAG,
                         "Latest Temperature: %d C",
                         snapshot.latest_temperature);

                ESP_LOGI(TAG,
                         "Alert Status      : %s",
                         snapshot.alert_active ? "ACTIVE" : "NORMAL");

                ESP_LOGI(TAG,
                         "Free Heap         : %u bytes",
                         (unsigned int)esp_get_free_heap_size());

                ESP_LOGI(TAG,
                         "Minimum Free Heap : %u bytes",
                         (unsigned int)esp_get_minimum_free_heap_size());

                ESP_LOGI(TAG,
                         "Controller Stack HWM: %u bytes",
                         (unsigned int)uxTaskGetStackHighWaterMark(
                             controllerTaskHandle));

                ESP_LOGI(TAG, "==============================");
            }
        }
    }
}

/* ---------------------------------- */
/* Application Entry Point             */
/* ---------------------------------- */

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Integrated RTOS Controller");

    /* Create queue */
    sensorQueue = xQueueCreate(5, sizeof(sensor_data_t));

    /* Create synchronization objects */
    stateMutex = xSemaphoreCreateMutex();
    diagnosticSemaphore = xSemaphoreCreateBinary();

    if (sensorQueue == NULL ||
        stateMutex == NULL ||
        diagnosticSemaphore == NULL)
    {
        ESP_LOGE(TAG, "RTOS object creation failed");
        return;
    }

    /* Create alert task first so its handle is available */
    if (xTaskCreate(alert_task, "Alert_Task",
                    3072, NULL, 4,
                    &alertTaskHandle) != pdPASS)
    {
        ESP_LOGE(TAG, "Alert task creation failed");
        return;
    }

    /* Create controller task */
    if (xTaskCreate(controller_task, "Controller_Task",
                    4096, NULL, 3,
                    &controllerTaskHandle) != pdPASS)
    {
        ESP_LOGE(TAG, "Controller task creation failed");
        return;
    }

    /* Create diagnostic task */
    if (xTaskCreate(diagnostic_task, "Diagnostic_Task",
                    4096, NULL, 2, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Diagnostic task creation failed");
        return;
    }

    /* Create sensor task */
    if (xTaskCreate(sensor_task, "Sensor_Task",
                    3072, NULL, 2, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Sensor task creation failed");
        return;
    }

    /* Create periodic diagnostic timer */
    diagnosticTimer = xTimerCreate(
        "Diagnostic_Timer",
        pdMS_TO_TICKS(DIAGNOSTIC_INTERVAL),
        pdTRUE,
        NULL,
        diagnostic_timer_callback
    );

    if (diagnosticTimer == NULL)
    {
        ESP_LOGE(TAG, "Timer creation failed");
        return;
    }

    if (xTimerStart(diagnosticTimer, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Timer start failed");
        return;
    }

    ESP_LOGI(TAG, "All RTOS components started successfully");
}
