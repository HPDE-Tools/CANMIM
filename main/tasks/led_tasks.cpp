#include "tasks.h"
#include "context/context.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "led/led.h"

#include <cstring>

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution,
                                             // 1 tick = 0.1us
                                             // (led strip needs a high resolution)
namespace hpde_tools
{

static const char *TAG = "LED";
static BoardContext *boardContext;

static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static void initLED()
{
    rmt_tx_channel_config_t tx_chan_config = {
        .gpio_num = (gpio_num_t)boardContext->pin_config.neopixel,
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .resolution_hz = 10000000,      // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
        .mem_block_symbols = 64,        // increase the block size can make the LED less flickering
        .trans_queue_depth = 4,         // set the number of transactions that can be pending in the background
        .intr_priority = 0,
        .flags = {
            .invert_out = 0,
            .with_dma = 0,
            .io_loop_back = 0,
            .io_od_mode = 0,
        },
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &led_chan));
    ESP_LOGI(TAG, "Install led encoder");

    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &led_encoder));
    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(led_chan));
}

static uint8_t pixel[3];
static rmt_transmit_config_t tx_config = {
    .loop_count = 0,
    .flags = {},
};
void LEDTask(void *pvParameter)
{
    boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    initLED();
    uint8_t event;
    while (3)
    {
        BaseType_t ret = xQueueReceive(boardContext->led_event_queue, &event, pdMS_TO_TICKS(500));
        if (ret == !pdTRUE)
        {
            continue;
        }
        switch (event)
        {
        case BoardContext::LED_NORMAL_ON:
        {
            pixel[0] = 10; // G
            pixel[1] = 0;  // R
            pixel[2] = 0;  // B
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, pixel, sizeof(pixel), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
        }
        break;
        default:
            memset(pixel, 0, sizeof(pixel));
            ESP_ERROR_CHECK(rmt_transmit(led_chan, led_encoder, pixel, sizeof(pixel), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_chan, portMAX_DELAY));
            break;
        }
    }
}
}