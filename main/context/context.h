#ifndef __HPDE_TOOLS_CONTEXT_H__
#define __HPDE_TOOLS_CONTEXT_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/twai.h"

#include <functional>

#include "board_pin_config.h"
#include "conversion/conversion.h"

namespace hpde_tools
{

class BoardContext
{
public:
    enum CAN_MODE
    {
        CAN_MODE_IN_ONLY = 0,
        CAN_MODE_IN_TO_OUT = 1,
        CAN_MODE_IN_OUT_BIDIRECTION = 2,
    };

    enum ANALOG_DATA_MODE
    {
        ANALOG_DATA_MODE_CAN_ONLY = 0,
        ANALOG_DATA_MODE_BLE_ONLY = 1,
        ANALOG_DATA_MODE_CAN_BLE = 2,
    };

    enum LED_EVENT : uint8_t
    {
        LED_OFF = 0,
        LED_NORMAL_ON = 1,
        LED_ERROR = 2,
        LED_LOW_POWER = 3,
    };

public:
    volatile size_t adc_int_count[4] = {};
    volatile uint16_t adc_volt[8] = {};
    volatile uint32_t spi_time = 0;
    volatile uint32_t ble_out_count = 0;
    volatile uint32_t ble_notified_count = 0;
    volatile uint32_t can_out_count = 0;
    volatile bool debug;

public:
    QueueHandle_t ble_out_queue;
    QueueHandle_t led_event_queue;
    twai_handle_t can_bus_in;
    twai_handle_t can_bus_out;
    const board_pin_config_t pin_config;
    const uint16_t vref = 50000;
    //std::function<void(uint16_t *, uint16_t)> adc_conversion_functions[8];
    ADC_Conversion* adc_conversion_functions[8];

private:
    uint32_t base_can_id;

private:
    inline void sendDataToCAN(uint32_t channel_offset, uint16_t data0, uint16_t data1, uint16_t data2, uint16_t data3);

public:
    BoardContext(board_pin_config_t pin_config_data) : pin_config(pin_config_data) {}
    BoardContext(const BoardContext &) = delete;
    BoardContext &operator=(const BoardContext &) = delete;

    void SetBaseCANID(uint32_t can_id);
    void SendADCwithConversion16bit(uint32_t channel_offset, uint16_t channel_a_converted, uint16_t channel_a_raw, uint16_t channel_b_converted, uint16_t channel_b_raw);
    void SendADCRAW16bit(uint32_t channel_offset, uint16_t channel_a_raw, uint16_t channel_b_raw, uint16_t channel_c_raw, uint16_t channel_d_raw);
    void SetLEDStatus(uint8_t status);
};

}
#endif