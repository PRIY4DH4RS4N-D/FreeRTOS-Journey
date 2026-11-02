
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

// Queue configuration
#define QUEUE_LENGTH 5

static const char *TAG = "QUEUE_SYSTEM";

// Data structure transferred between tasks
typedef struct
{
    int sensor_id;
    int sequence;
    float temperature;
    TickType_t timestamp;

} sensor_data_t;

// Global queue handle
static QueueHandle_t sensorQueue = NULL;


// PRODUCER TASK
void producerTask(void *pvParameters)
{
    sensor_data_t sensorData;
    int counter = 0;

    while (1)
    {
        // Simulate temperature sensor readings
        sensorData.sensor_id = 1;
        sensorData.sequence = counter++;

        sensorData.temperature =
            25.0f + (sensorData.sequence % 150) * 0.1f;

        sensorData.timestamp = xTaskGetTickCount();

        ESP_LOGI(TAG,
                 "[PRODUCER] Generated reading #%d | Temperature: %.2f C",
                 sensorData.sequence,
                 sensorData.temperature);

        // Send data to the queue
        if (xQueueSend(sensorQueue,
                       &sensorData,
                       pdMS_TO_TICKS(1000)) == pdPASS)
        {
            ESP_LOGI(TAG, "[PRODUCER] Data successfully queued");
        }
        else
        {
            ESP_LOGW(TAG, "[PRODUCER] Queue full! Data not sent");
        }

        vTaskDelay(pdMS_TO_TICKS(700));
    }
}


// CONSUMER TASK
void consumerTask(void *pvParameters)
{
    sensor_data_t receivedData;

    while (1)
    {
        // Wait until data becomes available
        if (xQueueReceive(sensorQueue,
                          &receivedData,
                          portMAX_DELAY) == pdPASS)
        {
            ESP_LOGI(TAG,
                     "[CONSUMER] Received #%d | Sensor: %d | Temperature: %.2f C | Tick: %lu",
                     receivedData.sequence,
                     receivedData.sensor_id,
                     receivedData.temperature,
                     (unsigned long)receivedData.timestamp);

            // Simulate processing time
            vTaskDelay(pdMS_TO_TICKS(1200));

            ESP_LOGI(TAG,
                     "[CONSUMER] Processing completed for #%d",
                     receivedData.sequence);
        }
    }
}


// APPLICATION ENTRY POINT
void app_main(void)
{
    ESP_LOGI(TAG, "FreeRTOS Queue Communication Started");

    // Create queue
    sensorQueue = xQueueCreate(
        QUEUE_LENGTH,
        sizeof(sensor_data_t)
    );

    if (sensorQueue == NULL)
    {
        ESP_LOGE(TAG, "Queue creation failed!");
        return;
    }

    ESP_LOGI(TAG, "Queue created: %d items", QUEUE_LENGTH);

    // Create producer task
    BaseType_t producerResult = xTaskCreate(
        producerTask,
        "Producer_Task",
        3072,
        NULL,
        2,
        NULL
    );

    // Create consumer task
    BaseType_t consumerResult = xTaskCreate(
        consumerTask,
        "Consumer_Task",
        3072,
        NULL,
        2,
        NULL
    );

    if (producerResult != pdPASS || consumerResult != pdPASS)
    {
        ESP_LOGE(TAG, "Task creation failed!");
    }
}
