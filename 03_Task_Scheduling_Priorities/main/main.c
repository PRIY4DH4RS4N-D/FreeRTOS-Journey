#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

static const char *TAG = "SCHEDULER";

// Task priorities
#define HIGH_PRIORITY    3
#define MEDIUM_PRIORITY  2
#define LOW_PRIORITY     1

// High-priority task
void highPriorityTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "[HIGH] Critical event processing");

        // Simulate short processing
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Medium-priority task
void mediumPriorityTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "[MEDIUM] Sensor data processing");

        // Simulate periodic processing
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Low-priority task
void lowPriorityTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "[LOW] Background data logging");

        // Simulate background processing
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "FreeRTOS Scheduling Demonstration Started");

    // Create high-priority task
    xTaskCreate(
        highPriorityTask,
        "High_Task",
        2048,
        NULL,
        HIGH_PRIORITY,
        NULL
    );

    // Create medium-priority task
    xTaskCreate(
        mediumPriorityTask,
        "Medium_Task",
        2048,
        NULL,
        MEDIUM_PRIORITY,
        NULL
    );

    // Create low-priority task
    xTaskCreate(
        lowPriorityTask,
        "Low_Task",
        2048,
        NULL,
        LOW_PRIORITY,
        NULL
    );

    ESP_LOGI(TAG, "All three tasks created successfully");
}
