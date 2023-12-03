#include <stdio.h>
#include <cinttypes>

#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "driver/twai.h"

#include "adc/ads1115.h"
#include "adc/adc_raw_conversion.h"

#include "can_types.h"
#include "board_config.h"
#include "context.h"
#include "tasks/tasks.h"
#include "ble.h"

hpde_tools::BoardContext *boardContext;

void fatal_error()
{
    for (int i = 10; i >= 0; i--)
    {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}

char timer_buf[256];
static void tick_timer_cb(void *arg)
{

    // size_t can_in_count;
    // size_t can_out_count;
    size_t ble_out_count = boardContext->ble_out_count;
    boardContext->ble_out_count = 0;
    size_t ble_notified_count = boardContext->ble_notified_count;
    boardContext->ble_notified_count = 0;
    size_t adc_int_count[4];
    for (int i = 0; i < 4; i++)
    {
        adc_int_count[i] = boardContext->adc_int_count[i];
        boardContext->adc_int_count[i] = 0;
        ESP_LOGI("TIMER", "adc%d_intr_count: %u", i, adc_int_count[i]);
    }

    // ESP_LOGI("TIMER", "can_in_count: %zu", can_in_count);
    ESP_LOGI("TIMER", "can_out_count: %zu", (size_t)boardContext->can_out_count);
    boardContext->can_out_count = 0;
    ESP_LOGI("TIMER", "ble_out_count: %zu", ble_out_count);
    ESP_LOGI("TIMER", "ble_notified_count: %zu", ble_notified_count);
    size_t offset = 0;
    for (size_t i = 0; i < sizeof(boardContext->adc_volt) / sizeof(uint16_t); i++)
    {
        offset += sprintf(timer_buf + offset, " CH%u: %f V ", i, static_cast<float>(boardContext->adc_volt[i]) / 10000);
    }
    timer_buf[offset] = '\0';
    ESP_LOGI("TIMER", "%s", timer_buf);
    ESP_LOGI("TIMER", "spitime %d us", (int)boardContext->spi_time);
    ESP_LOGI("TIMER", "");
}

void gpio_init();

void init_can();

void print_mcu_info()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %" PRIi16 ", ", chip_info.revision);

    uint32_t size_flash_chip;
    esp_flash_get_size(NULL, &size_flash_chip);
    printf("%ldMB %s flash\n", size_flash_chip / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %ld bytes\n", esp_get_minimum_free_heap_size());
}

void set_ble_name_from_mac(char *base_name)
{
    uint8_t ble_mac[8];
    memset(ble_mac, 0, sizeof(ble_mac));
    esp_err_t ret = esp_read_mac(ble_mac, ESP_MAC_BT);
    if (ret != ESP_OK)
    {
        ESP_LOGE("MCU", "Failed to retrieve BLE MAC address: %d", ret);
    }
    char ble_name[32];
    memset(ble_name, 0, sizeof(ble_name));
    size_t ble_name_len = strlen(base_name);
    if (ble_name_len > sizeof(ble_name) - 4)
        ble_name_len = sizeof(ble_name) - 4;
    memcpy(ble_name, base_name, ble_name_len);
    int char_count = sprintf(ble_name + ble_name_len, "_%02X%02X", ble_mac[4], ble_mac[5]);
    ble_name_len += char_count;
    ble_name[ble_name_len] = 0;
    set_ble_name(ble_name, ble_name_len);
    ESP_LOGI("BLE", "BLE Device name: %s", ble_name);
}

extern "C" void app_main(void)
{
    ESP_LOGI("MAIN", "app_main");
    hpde_tools::board_pin_config_t pin_config;
    boardConfig.set_pin_definition(&pin_config);
    ESP_LOGI("MAIN", "before boardContext Init");
    boardContext = new hpde_tools::BoardContext(pin_config);
    // Init NVS, which is used by WiFi and BLE stack.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    set_ble_name_from_mac(boardConfig.get_ble_name());
    for (int i = 0; i < 8; i++)
        boardContext->adc_conversion_functions[i] = hpde_tools::ADCConversion150OilSensor;
    boardContext->SetBaseCANID(0x662);
    ESP_LOGI("MAIN", "boardContext Init");
    gpio_init();
    print_mcu_info();
    ESP_LOGI("MAIN", "MCU Info printed");
    init_can();
    ESP_LOGI("MAIN", "CAN Controller Init");

    const esp_timer_create_args_t tick_timer_args = {
        .callback = &tick_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tick_timer",
        .skip_unhandled_events = false,
    };

    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000000));

    ESP_LOGI("MAIN", "Timer Event Started");

    boardContext->ble_out_queue = xQueueCreate(50, sizeof(ble_can_frame_t));
    boardContext->led_event_queue = xQueueCreate(10, sizeof(uint8_t));
    xTaskCreate(hpde_tools::CANInTask, "incomingCANTask", 4096, boardContext, 1, NULL);
    ESP_LOGI("MAIN", "CAN In task started");
    xTaskCreate(hpde_tools::CANOutTask, "outgoingCANTask", 4096, boardContext, 3, NULL);
    ESP_LOGI("MAIN", "CAN Out task started");
    xTaskCreate(hpde_tools::ADCTLA2518Task, "ADCTask", 4096, boardContext, 2, NULL);
    ESP_LOGI("MAIN", "ADC task started");
    xTaskCreate(hpde_tools::BLETask, "bleTask", 4096, boardContext, 2, NULL);
    xTaskCreate(hpde_tools::LEDTask, "ledTask", 4096, boardContext, tskIDLE_PRIORITY, NULL);
    boardContext->SetLEDStatus(hpde_tools::BoardContext::LED_NORMAL_ON);
}

static void SetUpSingleGPIOINTR(int pin_num, int intr_num)
{
    if (pin_num == -1)
        return;
    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(gpio_config_t));
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = 1ULL << pin_num;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    // gpio_isr_handler_add((gpio_num_t)pin_num, adc_int_handler, (void*)intr_num);
}

void gpio_init()
{
    gpio_install_isr_service(0);
    gpio_config_t io_conf;
    memset(&io_conf, 0, sizeof(gpio_config_t));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask |= 1ULL << boardContext->pin_config.analog_pw;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)boardContext->pin_config.analog_pw, 1);
    memset(&io_conf, 0, sizeof(gpio_config_t));
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask |= 1ULL << boardContext->pin_config.can_sleep;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    SetUpSingleGPIOINTR(boardContext->pin_config.sw0, 0);
    SetUpSingleGPIOINTR(boardContext->pin_config.sw1, 1);
    SetUpSingleGPIOINTR(boardContext->pin_config.sw2, 2);
    SetUpSingleGPIOINTR(boardContext->pin_config.sw3, 3);
    SetUpSingleGPIOINTR(boardContext->pin_config.sw4, 4);
}

void init_can()
{
    twai_mode_t mode = TWAI_MODE_NO_ACK;
    // twai_general_config_t g_config_in = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)boardContext->pin_config.can_0_tx /* TX */, (gpio_num_t)boardContext->pin_config.can_0_rx /* RX */, mode);
    twai_general_config_t g_config_in;
    memset(&g_config_in, 0, sizeof(twai_general_config_t));
    g_config_in.mode = mode;
    g_config_in.tx_io = (gpio_num_t)boardContext->pin_config.can_0_tx;
    g_config_in.rx_io = (gpio_num_t)boardContext->pin_config.can_0_rx;
    g_config_in.clkout_io = TWAI_IO_UNUSED;
    g_config_in.bus_off_io = TWAI_IO_UNUSED;
    g_config_in.tx_queue_len = 10;
    g_config_in.rx_queue_len = 5;
    g_config_in.alerts_enabled = TWAI_ALERT_NONE;
    g_config_in.clkout_divider = 0;
    g_config_in.intr_flags = ESP_INTR_FLAG_LEVEL1;
    g_config_in.controller_id = 0;
    twai_timing_config_t t_config_in = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config_in = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // Install TWAI driver
    esp_err_t err = twai_driver_install_v2(&g_config_in, &t_config_in, &f_config_in, &boardContext->can_bus_in);
    if (err == ESP_OK)
    {
        ESP_LOGW("ESPCAN", "Can bus 0 driver installed");
    }
    else
    {
        ESP_LOGE("ESPCAN", "Failed to install driver for CAN 0, code: %d ", (int)err);
        fatal_error();
    }
    if (twai_start_v2(boardContext->can_bus_in) == ESP_OK)
    {
        ESP_LOGW("ESPCAN", "Can 0 bus started");
    }
    else
    {
        ESP_LOGE("ESPCAN", "CAN 0 Failed to start");
        fatal_error();
    }

    twai_general_config_t g_config_out;
    memset(&g_config_out, 0, sizeof(twai_general_config_t));
    g_config_out.mode = mode;
    g_config_out.tx_io = (gpio_num_t)boardContext->pin_config.can_1_tx;
    g_config_out.rx_io = (gpio_num_t)boardContext->pin_config.can_1_rx;
    g_config_out.clkout_io = TWAI_IO_UNUSED;
    g_config_out.bus_off_io = TWAI_IO_UNUSED;
    g_config_out.tx_queue_len = 10;
    g_config_out.rx_queue_len = 5;
    g_config_out.alerts_enabled = TWAI_ALERT_NONE;
    g_config_out.clkout_divider = 0;
    g_config_out.intr_flags = ESP_INTR_FLAG_LEVEL1;
    g_config_out.controller_id = 1;
    twai_timing_config_t t_config_out = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config_out = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    // Install TWAI driver
    if (twai_driver_install_v2(&g_config_out, &t_config_out, &f_config_out, &boardContext->can_bus_out) == ESP_OK)
    {
        ESP_LOGW("ESPCAN", "Can bus driver installed");
    }
    else
    {
        ESP_LOGE("ESPCAN", "Failed to install driver");
        fatal_error();
    }
    if (twai_start_v2(boardContext->can_bus_out) == ESP_OK)
    {
        ESP_LOGW("ESPCAN", "Can bus started");
    }
    else
    {
        ESP_LOGE("ESPCAN", "Failed to start");
        fatal_error();
    }
}
