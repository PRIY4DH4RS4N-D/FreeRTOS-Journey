
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

static const char *TAG = "SEMAPHORE";

// Semaphore handles
static SemaphoreHandle_t eventSemaphore;
static SemaphoreHandle_t countingSemaphore;


// TASK 1: EVENT GENERATOR
void eventGeneratorTask(void *pvParameters)
{
    int eventCount = 0;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(2000));

        eventCount++;

        ESP_LOGI(TAG, "[GENERATOR] Event %d detected",
                 eventCount);

        // Signal event handler
        if (xSemaphoreGive(eventSemaphore) == pdTRUE)
        {
            ESP_LOGI(TAG, "[GENERATOR] Binary semaphore given");
        }
        else
        {
            ESP_LOGW(TAG, "[GENERATOR] Binary semaphore already available");
        }

        // Generate counting semaphore signals
        if (eventCount % 3 == 0)
        {
            ESP_LOGI(TAG, "[GENERATOR] Generating 3 counting signals");

            for (int i = 0; i < 3; i++)
            {
                if (xSemaphoreGive(countingSemaphore) != pdTRUE)
                {
                    ESP_LOGW(TAG, "[GENERATOR] Counting semaphore full");
                    break;
                }
            }
        }
    }
}


// TASK 2: BINARY SEMAPHORE HANDLER
void binaryHandlerTask(void *pvParameters)
{
    while (1)
    {
        ESP_LOGI(TAG, "[BINARY HANDLER] Waiting for event...");

        // Block until binary semaphore is available
        if (xSemaphoreTake(eventSemaphore, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI(TAG, "[BINARY HANDLER] Event received!");

            ESP_LOGI(TAG, "[BINARY HANDLER] Processing event...");

            vTaskDelay(pdMS_TO_TICKS(500));

            ESP_LOGI(TAG, "[BINARY HANDLER] Event processing completed");
        }
    }
}


// TASK 3: COUNTING SEMAPHORE HANDLER
void countingHandlerTask(void *pvParameters)
{
    int processedCount = 0;

    while (1)
    {
        // Wait for one available count
        if (xSemaphoreTake(countingSemaphore, portMAX_DELAY) == pdTRUE)
        {
            processedCount++;

            ESP_LOGI(TAG,
                     "[COUNTING HANDLER] Processing pending signal %d",
                     processedCount);

            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}


// APPLICATION ENTRY POINT
void app_main(void)
{
    ESP_LOGI(TAG, "FreeRTOS Semaphore Demonstration Started");

    // Create binary semaphore
    eventSemaphore = xSemaphoreCreateBinary();

    // Create counting semaphore
    countingSemaphore = xSemaphoreCreateCounting(5, 0);

    if (eventSemaphore == NULL || countingSemaphore == NULL)
    {
        ESP_LOGE(TAG, "Semaphore creation failed!");
        return;
    }

    ESP_LOGI(TAG, "Binary and counting semaphores created");

    // Create event generator
    BaseType_t result1 = xTaskCreate(
        eventGeneratorTask,
        "Event_Generator",
        3072,
        NULL,
        2,
        NULL
    );

    // Create binary semaphore handler
    BaseType_t result2 = xTaskCreate(
        binaryHandlerTask,
        "Binary_Handler",
        3072,
        NULL,
        3,
        NULL
    );

    // Create counting semaphore handler
    BaseType_t result3 = xTaskCreate(
        countingHandlerTask,
        "Counting_Handler",
        3072,
        NULL,
        2,
        NULL
    );

    if (result1 != pdPASS ||
        result2 != pdPASS ||
        result3 != pdPASS)
    {
        ESP_LOGE(TAG, "Task creation failed!");
    }
}
