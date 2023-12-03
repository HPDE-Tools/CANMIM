#include "context.h"
#include "can_types.h"
#include "esp_timer.h"

namespace hpde_tools
{

void BoardContext::SetBaseCANID(uint32_t can_id)
{
    this->base_can_id = can_id;
}

inline void BoardContext::sendDataToCAN(uint32_t channel_offset, uint16_t data0, uint16_t data1, uint16_t data2, uint16_t data3)
{
    twai_message_t message;
    ble_can_frame_t ble_can_frame;
    memset(&message, 0, sizeof(twai_message_t));
    message.identifier = (this->base_can_id + channel_offset) & CAN_ID_MASK;
    if (message.identifier > CAN_SFF_MASK)
        message.extd = 1;
    message.data_length_code = 8;
    message.data[0] = data0 & 0xFF;
    message.data[1] = (data0 >> 8) & 0xFF;
    message.data[2] = data1 & 0xFF;
    message.data[3] = (data1 >> 8) & 0xFF;
    message.data[4] = data2 & 0xFF;
    message.data[5] = (data2 >> 8) & 0xFF;
    message.data[6] = data3 & 0xFF;
    message.data[7] = (data3 >> 8) & 0xFF;
    ConvertTWAItoBLECAN(&message, &ble_can_frame, esp_timer_get_time() / 1000);
    twai_transmit_v2(this->can_bus_out, &message, 1);
    this->can_out_count++;
    xQueueSend(this->ble_out_queue, &ble_can_frame, 0);
}

void BoardContext::SendADCwithConversion16bit(uint32_t channel_offset, uint16_t channel_a_converted, uint16_t channel_a_raw, uint16_t channel_b_converted, uint16_t channel_b_raw)
{
    this->sendDataToCAN(channel_offset, channel_a_converted, channel_b_converted, channel_a_raw, channel_b_raw);
}

void BoardContext::SendADCRAW16bit(uint32_t channel_offset, uint16_t channel_a_raw, uint16_t channel_b_raw, uint16_t channel_c_raw, uint16_t channel_d_raw)
{
    this->sendDataToCAN(channel_offset, channel_a_raw, channel_b_raw, channel_c_raw, channel_d_raw);
}

void BoardContext::SetLEDStatus(uint8_t status)
{
    xQueueSend(this->led_event_queue, &status, 1);
}
}