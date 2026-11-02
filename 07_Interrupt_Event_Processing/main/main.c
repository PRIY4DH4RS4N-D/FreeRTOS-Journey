
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_log.h"

static const char *TAG = "INTERRUPT_DEMO";

static TaskHandle_t processingTaskHandle = NULL;
static volatile uint32_t interruptCount = 0;

/* Hardware timer ISR callback */
static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;

    interruptCount++;

    vTaskNotifyGiveFromISR(
        processingTaskHandle,
        &higherPriorityTaskWoken
    );

    return higherPriorityTaskWoken == pdTRUE;
}

/* Deferred event processing task */
static void processing_task(void *parameter)
{
    uint32_t processedCount = 0;

    while (1)
    {
        /* Block until timer ISR notifies this task */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        processedCount++;

        ESP_LOGI(TAG,
                 "Event processed: %lu",
                 (unsigned long)processedCount);
    }
}

/* Configure GPTimer */
static void initialize_timer(void)
{
    gptimer_handle_t timer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_callback,
    };

    ESP_ERROR_CHECK(
        gptimer_register_event_callbacks(timer, &callbacks, NULL)
    );

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 1000000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(
        gptimer_set_alarm_action(timer, &alarm_config)
    );

    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));

    ESP_LOGI(TAG, "Hardware timer started: 1-second interval");
}

void app_main(void)
{
    xTaskCreate(
        processing_task,
        "Processing_Task",
        3072,
        NULL,
        5,
        &processingTaskHandle
    );

    initialize_timer();
}
