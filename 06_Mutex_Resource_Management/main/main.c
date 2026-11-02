
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static const char *TAG = "MUTEX_SYSTEM";

static SemaphoreHandle_t resourceMutex;

static int sharedCounter = 0;


// Simulated shared resource
void accessSharedResource(const char *taskName)
{
    ESP_LOGI(TAG, "%s: Waiting for shared resource", taskName);

    if (xSemaphoreTake(resourceMutex, portMAX_DELAY) == pdTRUE)
    {
        ESP_LOGI(TAG, "%s: LOCK ACQUIRED", taskName);

        sharedCounter++;

        ESP_LOGI(TAG, "%s: Updating shared counter = %d",
                 taskName, sharedCounter);

        // Simulate resource access
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGI(TAG, "%s: Releasing shared resource", taskName);

        xSemaphoreGive(resourceMutex);
    }
}


// HIGH PRIORITY TASK
void highPriorityTask(void *pvParameters)
{
    while (1)
    {
        accessSharedResource("HIGH PRIORITY");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


// MEDIUM PRIORITY TASK
void mediumPriorityTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "MEDIUM: Performing background processing");

        vTaskDelay(pdMS_TO_TICKS(700));
    }
}


// LOW PRIORITY TASK
void lowPriorityTask(void *pvParameters)
{
    while (1)
    {
        accessSharedResource("LOW PRIORITY");

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}


// APPLICATION ENTRY POINT
void app_main(void)
{
    ESP_LOGI(TAG, "Mutex Resource Management Started");

    // Create mutex
    resourceMutex = xSemaphoreCreateMutex();

    if (resourceMutex == NULL)
    {
        ESP_LOGE(TAG, "Mutex creation failed");
        return;
    }

    ESP_LOGI(TAG, "Shared resource mutex created");

    // High priority task
    BaseType_t result1 = xTaskCreate(
        highPriorityTask,
        "High_Task",
        3072,
        NULL,
        3,
        NULL
    );

    // Medium priority task
    BaseType_t result2 = xTaskCreate(
        mediumPriorityTask,
        "Medium_Task",
        3072,
        NULL,
        2,
        NULL
    );

    // Low priority task
    BaseType_t result3 = xTaskCreate(
        lowPriorityTask,
        "Low_Task",
        3072,
        NULL,
        1,
        NULL
    );

    if (result1 != pdPASS ||
        result2 != pdPASS ||
        result3 != pdPASS)
    {
        ESP_LOGE(TAG, "Task creation failed");
    }
}
