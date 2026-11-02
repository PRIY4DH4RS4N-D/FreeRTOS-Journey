
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

static const char *TAG = "SOFTWARE_TIMER";

static TimerHandle_t periodicTimer;
static TimerHandle_t oneShotTimer;

static uint32_t periodicCount = 0;

/* Executes every 2 seconds */
static void periodic_timer_callback(TimerHandle_t timer)
{
    periodicCount++;

    ESP_LOGI(TAG,
             "Periodic timer fired: %lu",
             (unsigned long)periodicCount);
}

/* Executes only once after 5 seconds */
static void one_shot_timer_callback(TimerHandle_t timer)
{
    ESP_LOGI(TAG, "One-shot timer expired!");

    ESP_LOGI(TAG, "One-shot operation completed");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FreeRTOS Software Timers");

    /* Create periodic timer: 2 seconds */
    periodicTimer = xTimerCreate(
        "Periodic_Timer",
        pdMS_TO_TICKS(2000),
        pdTRUE,
        NULL,
        periodic_timer_callback
    );

    /* Create one-shot timer: 5 seconds */
    oneShotTimer = xTimerCreate(
        "OneShot_Timer",
        pdMS_TO_TICKS(5000),
        pdFALSE,
        NULL,
        one_shot_timer_callback
    );

    if (periodicTimer == NULL || oneShotTimer == NULL)
    {
        ESP_LOGE(TAG, "Timer creation failed");
        return;
    }

    /* Start both timers */
    if (xTimerStart(periodicTimer, 0) != pdPASS ||
        xTimerStart(oneShotTimer, 0) != pdPASS)
    {
        ESP_LOGE(TAG, "Timer start command failed");
        return;
    }

    ESP_LOGI(TAG, "Both timers started");

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
