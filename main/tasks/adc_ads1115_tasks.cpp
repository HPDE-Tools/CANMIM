#include "tasks.h"
#include "context/context.h"
#include "adc/ads1115.h"
#include "adc/tla2518.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

namespace hpde_tools
{

enum ADC_EVENT
{
    ADC_INT_0 = 0,
    ADC_INT_1 = 1,
    ADC_INT_2 = 2,
    ADC_INT_3 = 3,
    ADC_SEND = 4,
};

static QueueHandle_t adc_event_queue;

static void adc_timer_cb(void *arg)
{
    int event = ADC_SEND;
    xQueueSendFromISR(adc_event_queue, &event, NULL);
}

static void IRAM_ATTR adc_int_handler(void *arg)
{
    int event = (int)arg + ADC_INT_0;
    if (event > ADC_INT_3)
        return;
    xQueueSendFromISR(adc_event_queue, &event, NULL);
}

static void SetUpSingleGPIOINTR(BoardContext *boardContext, int pin_num, int intr_num)
{
    if (pin_num == -1)
        return;
    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(gpio_config_t));
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = 1ULL << pin_num;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_isr_handler_add((gpio_num_t)pin_num, adc_int_handler, (void *)intr_num);
}

static void SetUpGPIOINTR(BoardContext *boardContext)
{
    SetUpSingleGPIOINTR(boardContext, boardContext->pin_config.ext_adc_int0, 0);
    SetUpSingleGPIOINTR(boardContext, boardContext->pin_config.ext_adc_int1, 1);
    SetUpSingleGPIOINTR(boardContext, boardContext->pin_config.ext_adc_int2, 2);
    SetUpSingleGPIOINTR(boardContext, boardContext->pin_config.ext_adc_int3, 3);
}

static esp_err_t StartI2C(i2c_port_t port_num, int pin_scl, int pin_sda)
{
    i2c_config_t conf;
    memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = pin_sda;
    conf.scl_io_num = pin_scl;
    conf.sda_pullup_en = 0;
    conf.scl_pullup_en = 0;
    conf.master.clk_speed = 400000;
    i2c_param_config(port_num, &conf);
    return i2c_driver_install(port_num, conf.mode, 0, 0, 0);
}

ESP_ADS1X15 ads1115_adc[4];
uint64_t ads1115_channel_sum[4];
size_t ads1115_channel_cv_count[4];
uint16_t adc_raw[4];
uint16_t adc_converted[4];
void ADS1115ADCTask(void *pvParameter)
{
    BoardContext *boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &adc_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "adc_send_timer",
        .skip_unhandled_events = false,
    };

    esp_timer_handle_t adc_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &adc_timer));

    adc_event_queue = xQueueCreate(20, sizeof(int));
    SetUpGPIOINTR(boardContext);
    ESP_ERROR_CHECK(StartI2C(I2C_NUM_0, boardContext->pin_config.i2c0_scl, boardContext->pin_config.i2c0_sda));
    for (int i = 0; i < 4; i++)
    {
        ads1115_adc[i].begin(I2C_NUM_0, ADS1X15_ADDRESS + i);
        ads1115_adc[i].setDataRate(RATE_ADS1115_250SPS);
        ads1115_adc[i].setGain(GAIN_TWOTHIRDS);
    }
    for (int i = 0; i < 3; i++)
    {
        ads1115_adc[i].startADCReading(MUX_BY_CHANNEL[0], true);
    }
    memset(ads1115_channel_sum, 0, sizeof(uint64_t) * 4);
    memset(ads1115_channel_cv_count, 0, sizeof(size_t) * 4);
    ads1115_adc[3].startADCReading(MUX_BY_CHANNEL[3], true);
    int event = 0;
    ESP_ERROR_CHECK(esp_timer_start_periodic(adc_timer, 50000));
    int16_t adc_value;
    while (3)
    {
        BaseType_t ret = xQueueReceive(adc_event_queue, &event, portMAX_DELAY);
        if (ret != pdTRUE)
            continue;
        switch (event)
        {
        case ADC_SEND:
        {
            for (int i = 0; i < 4; i++)
            {
                uint64_t result = 0;
                if (ads1115_channel_cv_count[i] > 0)
                    result = ((uint64_t)61440) * ads1115_channel_sum[i] / ads1115_channel_cv_count[i];
                if (result % 32767 > 32767 / 2)
                    result = result / 32767 + 1;
                else
                    result = result / 32767;
                adc_raw[i] = (uint16_t)result;
                ads1115_channel_sum[i] = 0;
                ads1115_channel_cv_count[i] = 0;
                boardContext->adc_conversion_functions[i]->Convert(&adc_converted[i], adc_raw[i]);
                boardContext->adc_volt[i] = adc_raw[i];
            }
            boardContext->SendADCwithConversion16bit(0, adc_converted[0], adc_raw[0], adc_converted[1], adc_raw[1]);
            boardContext->SendADCwithConversion16bit(1, adc_converted[2], adc_raw[2], adc_converted[3], adc_raw[3]);
        }
        break;
        case ADC_INT_0:
        case ADC_INT_1:
        case ADC_INT_2:
        case ADC_INT_3:
        {
            int i = event - ADC_INT_0;
            boardContext->adc_int_count[i]++;
            ESP_ADS1X15::ERROR err = ads1115_adc[i].getLastConversionResults(&adc_value);
            if (err != ESP_ADS1X15::ERROR_OK)
            {
                ESP_LOGE("ADC", "error while reading adc value from %d, code: %d", i, (int)err);
                continue;
            }
            if (adc_value < 0)
                adc_value = 0;
            ads1115_channel_sum[i] += adc_value;
            ads1115_channel_cv_count[i]++;
        }
        break;
        default:
            break;
        }
    }
}

}