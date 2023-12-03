#ifndef __CAN_OPENER_CAN_TYPES_H__
#define __CAN_OPENER_CAN_TYPES_H__

#include <stdint.h>
#include <string.h>

#include "driver/twai.h"

#define CAN_MAX_DATALEN 8

#define CAN_ID_MASK 0x1FFFFFFFUL  /* omit EFF, RTR, ERR flags */
#define CAN_EFF_FLAG 0x80000000UL /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000UL /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000UL /* error message frame */
#define CAN_SFF_MASK 0x000007FFUL /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFUL /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFUL /* omit EFF, RTR, ERR flags */

// CAN Frame format used in BLE message. Fixed 17 Bytes
typedef struct
{
    uint32_t timestamp; // Time stamp this frame is generated
                        //   in ms since MCU starts.
    uint32_t can_id;    // 32 bit CAN_ID + EFF/RTR/ERR flags

    uint8_t can_dlc;               // frame payload length in byte
                                   //   (0 .. CAN_MAX_DATALEN)
    uint8_t data[CAN_MAX_DATALEN]; // CAN data payload

} ble_can_frame_t;

inline void ConvertTWAItoBLECAN(const twai_message_t *twai_frame, ble_can_frame_t *ble_can_frame, uint32_t timestamp)
{
    ble_can_frame->timestamp = timestamp;
    ble_can_frame->can_id = twai_frame->identifier & CAN_EFF_MASK;
    if (twai_frame->rtr)
        ble_can_frame->can_id |= CAN_RTR_FLAG;
    if (twai_frame->extd)
        ble_can_frame->can_id |= CAN_EFF_FLAG;
    ble_can_frame->can_dlc = twai_frame->data_length_code;
    memcpy(ble_can_frame->data, twai_frame->data, CAN_MAX_DATALEN);
}

inline void ConvertBLECANtoTWAI(const ble_can_frame_t *ble_can_frame, twai_message_t *twai_frame)
{
    memset(twai_frame, 0, sizeof(twai_message_t));
    twai_frame->extd = ble_can_frame->can_id & CAN_EFF_FLAG ? 1 : 0;
    twai_frame->rtr = ble_can_frame->can_id & CAN_RTR_FLAG ? 1 : 0;
    twai_frame->identifier = ble_can_frame->can_id;
    twai_frame->data_length_code = ble_can_frame->can_dlc;
    memcpy(twai_frame->data, ble_can_frame->data, CAN_MAX_DATALEN);
}
#endif