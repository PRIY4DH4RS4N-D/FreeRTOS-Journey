
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "RTOS_MONITOR";

static TaskHandle_t workerTaskHandle = NULL;

/* Simulated application workload */
static void worker_task(void *parameter)
{
    volatile uint8_t buffer[256];

    while (1)
    {
        /* Simulate temporary stack usage */
        for (int i = 0; i < sizeof(buffer); i++)
        {
            buffer[i] = (uint8_t)i;
        }

        ESP_LOGI(TAG, "Worker task is running");

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Runtime resource monitoring */
static void monitor_task(void *parameter)
{
    while (1)
    {
        /* Heap monitoring */
        size_t freeHeap = esp_get_free_heap_size();
        size_t minimumHeap = esp_get_minimum_free_heap_size();

        /* Worker task stack monitoring */
        UBaseType_t workerStack =
            uxTaskGetStackHighWaterMark(workerTaskHandle);

        /* Monitor task stack monitoring */
        UBaseType_t monitorStack =
            uxTaskGetStackHighWaterMark(NULL);

        /* Task priority and state */
        UBaseType_t workerPriority =
            uxTaskPriorityGet(workerTaskHandle);

        eTaskState workerState =
            eTaskGetState(workerTaskHandle);

        ESP_LOGI(TAG, "========== RESOURCE REPORT ==========");

        ESP_LOGI(TAG, "Free Heap       : %u bytes",
                 (unsigned int)freeHeap);

        ESP_LOGI(TAG, "Minimum Heap    : %u bytes",
                 (unsigned int)minimumHeap);

        ESP_LOGI(TAG, "Worker Stack HWM: %u bytes",
                 (unsigned int)workerStack);

        ESP_LOGI(TAG, "Monitor Stack HWM: %u bytes",
                 (unsigned int)monitorStack);

        ESP_LOGI(TAG, "Worker Priority : %u",
                 (unsigned int)workerPriority);

        ESP_LOGI(TAG, "Worker State    : %d",
                 (int)workerState);

        ESP_LOGI(TAG, "=====================================");

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting FreeRTOS Runtime Monitoring");

    /* Create worker task */
    BaseType_t result = xTaskCreate(
        worker_task,
        "Worker_Task",
        2048,
        NULL,
        2,
        &workerTaskHandle
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Worker task creation failed");
        return;
    }

    /* Create monitoring task */
    result = xTaskCreate(
        monitor_task,
        "Monitor_Task",
        3072,
        NULL,
        3,
        NULL
    );

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Monitor task creation failed");
        vTaskDelete(workerTaskHandle);
        return;
    }
}
