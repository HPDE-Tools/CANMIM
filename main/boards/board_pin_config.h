#ifndef __HPDE_TOOLS_PIN_CONFIG_H__
#define __HPDE_TOOLS_PIN_CONFIG_H__

namespace hpde_tools
{
typedef struct
{
    int can_0_tx;
    int can_0_rx;
    int can_1_tx;
    int can_1_rx;
    int can_sleep;
    int neopixel;
    int internal_adc0;
    int internal_adc1;
    int ext_adc_int0;
    int ext_adc_int1;
    int ext_adc_int2;
    int ext_adc_int3;
    int i2c0_sda;
    int i2c0_scl;
    int spi0_mosi;
    int spi0_miso;
    int spi0_cs0;
    int spi0_sck;
    int btn_boot;
    int sw0;
    int sw1;
    int sw2;
    int sw3;
    int sw4;
    int analog_pw;
} board_pin_config_t;
}
#endif