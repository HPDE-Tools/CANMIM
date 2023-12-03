#include "tasks.h"
#include "context/context.h"
#include "adc/tla2518.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include <cstring>

namespace hpde_tools
{

enum ADC_EVENT
{
    ADC_READ = 0,
    ADC_SEND = 10,
};

static QueueHandle_t adc_event_queue;
static size_t count = 0;

static void adc_timer_cb(void *arg)
{
    count++;
    int event = ADC_READ;
    xQueueSendFromISR(adc_event_queue, &event, NULL);
    if (count >= 10)
    {
        count = 0;
        event = ADC_SEND;
        xQueueSendFromISR(adc_event_queue, &event, NULL);
    }
}

static TLA2518 *tla2518;
static void initADC(BoardContext *boardContext)
{
    spi_bus_config_t s_bus_config;
    spi_device_handle_t spi_tla2518;
    memset(&s_bus_config, 0, sizeof(spi_bus_config_t));
    s_bus_config.mosi_io_num = boardContext->pin_config.spi0_mosi;
    s_bus_config.miso_io_num = boardContext->pin_config.spi0_miso;
    s_bus_config.sclk_io_num = boardContext->pin_config.spi0_sck;
    s_bus_config.quadwp_io_num = -1;
    s_bus_config.quadhd_io_num = -1;
    s_bus_config.data4_io_num = -1;
    s_bus_config.data5_io_num = -1;
    s_bus_config.data6_io_num = -1;
    s_bus_config.data7_io_num = -1;
    spi_device_interface_config_t s_dev_config;
    memset(&s_dev_config, 0, sizeof(spi_device_interface_config_t));
    s_dev_config.clock_speed_hz = 8 * 1000 * 1000; // 8Mhz SPI
    s_dev_config.mode = 0;
    s_dev_config.spics_io_num = boardContext->pin_config.spi0_cs0;
    s_dev_config.queue_size = 24;
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &s_bus_config, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &s_dev_config, &spi_tla2518));
    tla2518 = new TLA2518(spi_tla2518);
}

static esp_timer_handle_t adc_timer;
static void setupTimer()
{
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &adc_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "adc_send_timer",
        .skip_unhandled_events = false,
    };
    adc_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &adc_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(adc_timer, 5000));
}

static void resetADC()
{
    TLA2518::ERROR err;
    err = tla2518->softReset();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "softreset failed: %d", (int)err);
    }
    err = tla2518->setAllAnalog();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "set analog failed: %d", (int)err);
    }
    err = tla2518->clearBOR();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "failed to clear BOR bit: %d", (int)err);
    }
    err = tla2518->setSampleRate(TLA2518::RATE_1000KPS);
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "sample rate set failed: %d", (int)err);
    }
    err = tla2518->offsetCalibrate();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "offset calibration failed: %d", (int)err);
    }
    err = tla2518->setOverSampe(TLA2518::OVERSAMPLE_8SP);
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "failed to set oversample: %d", (int)err);
    }
    err = tla2518->enableChID();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "failed to enable CH ID: %d", (int)err);
    }
    err = tla2518->setADCAutoSeq8ch();
    if (err != TLA2518::ERROR_OK)
    {
        ESP_LOGI("ADC", "failed to enable CH ID: %d", (int)err);
    }
}

uint64_t adc_channel_sum[8];
size_t adc_channel_cv_count;
uint16_t adc_raw[8];
uint16_t adc_volt[8];
uint16_t adc_converted[8];
void ADCTLA2518Task(void *pvParameter)
{
    BoardContext *boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    TLA2518::ERROR err;
    vTaskDelay(1000);
    initADC(boardContext);
    resetADC();
    adc_event_queue = xQueueCreate(20, sizeof(int));
    memset(adc_channel_sum, 0, sizeof(adc_channel_sum));
    adc_channel_cv_count = 0;
    memset(adc_raw, 0, sizeof(adc_raw));
    memset(adc_volt, 0, sizeof(adc_volt));
    memset(adc_converted, 0, sizeof(adc_converted));
    setupTimer();
    int event = 0;
    while (3)
    {
        BaseType_t ret = xQueueReceive(adc_event_queue, &event, portMAX_DELAY);
        if (ret != pdTRUE)
            continue;
        switch (event)
        {
        case ADC_READ:
        {
            int64_t start = esp_timer_get_time();
            err = tla2518->readADC8Ch(adc_raw);
            int64_t end = esp_timer_get_time();
            if (err != TLA2518::ERROR_OK)
            {
                ESP_LOGI("ADC", "readADC8Ch returned %d", (int)err);
                continue;
            }
            boardContext->spi_time = end - start;
            for (size_t i = 0; i < 8; i++)
            {
                adc_channel_sum[i] += adc_raw[i];
            }
            adc_channel_cv_count++;
        }
            break;
        case ADC_SEND:
        {
            if (adc_channel_cv_count == 0)
            {
                memset(adc_channel_sum, 0, sizeof(adc_channel_sum));
            }
            else
            {
                for (size_t i = 0; i < 8; i++)
                {
                    adc_raw[i] = adc_channel_sum[i] / adc_channel_cv_count;
                    adc_volt[i] = static_cast<uint64_t>(boardContext->vref) * adc_raw[i] / 65536;
                    boardContext->adc_volt[i] = adc_volt[i];
                    boardContext->adc_conversion_functions[i](&adc_converted[i], adc_volt[i]);
                }
                for (size_t i = 0; i < 4; i++)
                {
                    boardContext->SendADCwithConversion16bit(i, adc_converted[i * 2], adc_volt[i * 2], adc_converted[i * 2 + 1], adc_volt[i * 2 + 1]);
                }
                boardContext->adc_int_count[0] = adc_channel_cv_count;
                memset(adc_channel_sum, 0, sizeof(adc_channel_sum));
                adc_channel_cv_count = 0;
            }
        }
            break;
        default:
            break;
        }
    }
}
} // namespace hpde_tools