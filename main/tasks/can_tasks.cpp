#include "tasks.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/twai.h"

#include "context/context.h"
#include "context/can_types.h"

namespace hpde_tools
{

void CANInTask(void *pvParameter)
{
    BoardContext *boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    while (3)
    {
        twai_message_t message;
        esp_err_t ret_code;
        ret_code = twai_receive_v2(boardContext->can_bus_in, &message, portMAX_DELAY);
        switch (ret_code)
        {
        case ESP_OK:
            if (boardContext->IsCANIDColliding(message.identifier))
                break;    
            else if (!(message.rtr))
            {
                ble_can_frame_t ble_can_frame;
                ConvertTWAItoBLECAN(&message, &ble_can_frame, esp_timer_get_time() / 1000);
                twai_transmit_v2(boardContext->can_bus_out, &message, 1);
                boardContext->can_out_count++;
                xQueueSend(boardContext->ble_out_queue, &ble_can_frame, 0);
            }
            break;
        case ESP_ERR_TIMEOUT:
            // Timeout is normal when there is no CAN communication.
            // Ignore it.
            break;
        default:
            // TODO: handle failure and bus recovery.
            ESP_LOGE("ESPCAN", "Failed when receiving IN port due to %s", esp_err_to_name(ret_code));
        }
    }
}

void CANOutTask(void *pvParameter)
{
    BoardContext *boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    while (3)
    {
        twai_message_t message;
        esp_err_t ret_code;
        ret_code = twai_receive_v2(boardContext->can_bus_out, &message, portMAX_DELAY);
        switch (ret_code)
        {
        case ESP_OK:
            break;
        case ESP_ERR_TIMEOUT:
            // Timeout is normal when there is no CAN communication.
            // Ignore it.
            break;
        default:
            // TODO: handle failure and bus recovery.
            ESP_LOGE("ESPCAN", "Failed when receiving OUT port due to %s", esp_err_to_name(ret_code));
        }
    }
}
}