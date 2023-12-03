#ifndef __HPDE_TOOLS_TLA2518_H__
#define __HPDE_TOOLS_TLA2518_H__

#include "driver/spi_master.h"

#include <cinttypes>

namespace hpde_tools
{

class TLA2518
{
public:
    enum ERROR
    {
        ERROR_OK = 0,
        ERROR_INVALID_ARG = 1,
        ERROR_SPI_ERROR = 2,
        ERROR_TIMEOUT = 3,
    };

    enum OP_CODE : uint8_t
    {
        NO_OP = 0,
        RD_REG = 0x10,
        WR_REG = 0x08,
        SET_BIT = 0x18,
        CLEAR_BIT = 0x20,
    };

    enum SAMPLE_RATE : uint8_t
    {
        RATE_1000KPS = 0,
        RATE_666_7KPS = 1,
        RATE_500KPS = 2,
        RATE_333_3KPS = 3,
        RATE_250KPS = 4,
        RATE_166_7KPS = 5,
        RATE_125_KPS = 6,
        RATE_83_KPS = 7,
        RATE_62_5_KPS = 8,
        RATE_41_7_KPS = 9,
        RATE_31_3_KPS = 10,
        RATE_20_8_KPS = 11,
        RATE_15_6_KPS = 12,
        RATE_10_4_KPS = 13,
        RATE_7_8_KPS = 14,
        RATE_5_2_KPS = 15,
        RATE_32_25_KPS = 16,
        RATE_20_83_KPS = 17,
        RATE_15_63_KPS = 18,
        RATE_10_42_KPS = 19,
        RATE_7_81_KPS = 20,
        RATE_5_21_KPS = 21,
        RATE_3_91_KPS = 22,
        RATE_2_60_KPS = 23,
        RATE_1_95_KPS = 24,
        RATE_1_3_KPS = 25,
        RATE_0_98_KPS = 26,
        RATE_0_65_KPS = 27,
        RATE_0_49_KPS = 28,
        RATE_0_33_KPS = 29,
        RATE_0_24_KPS = 30,
        RATE_0_16_KPS = 31,
    };

    enum REGISTERS : uint8_t
    {
        REG_SYSTEM_STATUS = 0x00,
        REG_GENERAL_CFG = 0x01,
        REG_DATA_CFG = 0x02,
        REG_OSR_CFG = 0x03,
        REG_OPMODE_CFG = 0x04,
        REG_PIN_CFG = 0x05,
        REG_GPIO_CFG = 0x07,
        REG_GPO_DRIVE_CFG = 0x09,
        REG_GPO_VALUE = 0x0B,
        REG_GPI_VALUE = 0x0D,
        REG_SEQUENCE_CFG = 0x10,
        REG_CHANNEL_SEL = 0x11,
        REG_AUTO_SEQ_CH_SEL = 0x12,
    };

    enum OVERSAMPLE : uint8_t
    {
        OVERSAMPLE_DISABLE = 0x00,
        OVERSAMPLE_2SP = 0x01,
        OVERSAMPLE_4SP = 0x02,
        OVERSAMPLE_8SP = 0x03,
        OVERSAMPLE_16SP = 0x04,
        OVERSAMPLE_32SP = 0x05,
        OVERSAMPLE_64SP = 0x06,
        OVERSAMPLE_128SP = 0x07,
    };

private:
    spi_device_handle_t device_handle;
    uint8_t data_buf[64];
    uint8_t ret_buf[64];
    bool oversample;
    uint16_t vref;

public:
    TLA2518(spi_device_handle_t s) : device_handle(s), oversample(false), vref(50080){};
    ERROR setVRef(uint16_t huv);
    ERROR getSystemStatus(uint8_t *data);
    ERROR softReset();
    ERROR setAllAnalog();
    ERROR offsetCalibrate();
    ERROR clearBOR();
    ERROR enableChID();
    ERROR setOverSampe(uint8_t sample);
    ERROR setSampleRate(uint8_t sampleRate);
    ERROR readADCSingle(uint8_t ch, uint16_t *data);
    ERROR setADCAutoSeq8ch();
    ERROR readADC8Ch(uint16_t *data);

private:
    ERROR spiReadWrite(size_t data_len, size_t buf_len);
    ERROR spiReadRegister(uint8_t reg, uint8_t *data);
    ERROR spiWriteRegister(uint8_t reg, uint8_t data);
    ERROR spiSetRegisterBits(uint8_t reg, uint8_t data);
    ERROR spiClearRegisterBits(uint8_t reg, uint8_t data);
    uint8_t spiDataToADCRaw(uint16_t *data);
    uint8_t spiDataToADCRaw(uint8_t *spi_data, uint16_t *data);
};

}

#endif