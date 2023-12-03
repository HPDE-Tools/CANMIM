#include "board_config_prototype1.h"

namespace hpde_tools
{
void BoardConfig_Prototype1::set_pin_definition_derived(board_pin_config_t *config)
{
    config->can_0_rx = 18;
    config->can_0_tx = 19;
    config->can_1_rx = 20;
    config->can_1_tx = 21;
    config->can_sleep = 22;
    config->neopixel = 11;
    config->ext_adc_int0 = 5;
    config->ext_adc_int1 = 6;
    config->ext_adc_int2 = 7;
    config->ext_adc_int3 = 0;
    config->i2c0_sda = 8;
    config->i2c0_scl = 10;
    config->analog_pw = 4;
    config->sw0 = 2;
    config->sw1 = 3;
    config->sw2 = 15;
    config->sw3 = 23;
    config->sw4 = 1;
    config->btn_boot = 9;
}

char ble_prefix[] = "ECAN";
char *BoardConfig_Prototype1::get_ble_name()
{
    return ble_prefix;
}
}