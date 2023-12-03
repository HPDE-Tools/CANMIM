#include "tasks.h"
#include "context/context.h"
#include "ble/ble.h"

namespace hpde_tools
{
void BLETask(void *pvParameter)
{
    BoardContext *boardContext = reinterpret_cast<BoardContext *>(pvParameter);
    ble_init();
    ble_can_frame_t frame;
    while (3)
    {
        if (xQueueReceive(boardContext->ble_out_queue, &frame, portMAX_DELAY) == pdTRUE)
        {
            if (uxQueueMessagesWaiting(boardContext->ble_out_queue) > 25)
            {
                for (int i = 0; i < 10; i++)
                {
                    xQueueReceive(boardContext->ble_out_queue, &frame, portMAX_DELAY);
                }
                ESP_LOGI("BLE", "10 message skipped.");
            }
            boardContext->ble_out_count++;
            if (ble_notify(&frame))
            {
                boardContext->ble_notified_count++;
            }
        }
    }
}
}