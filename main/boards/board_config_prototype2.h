#ifndef __HPDE_TOOLS_BOARD_CONFIG_PROTOTYPE2_H__
#define __HPDE_TOOLS_BOARD_CONFIG_PROTOTYPE2_H__

#include "board.h"

namespace hpde_tools
{

class BoardConfig_Prototype2 : public BoardConfig
{

private:
    char blue_prefix[5] = "ECAN";

public:
    void set_pin_definition_derived(board_pin_config_t *config) override
    {
        config->can_0_rx = 10;
        config->can_0_tx = 20;
        config->can_1_rx = 21;
        config->can_1_tx = 22;
        config->can_sleep = 23;
        config->neopixel = 11;
        config->spi0_mosi = 7;
        config->spi0_miso = 2;
        config->spi0_sck = 6;
        config->spi0_cs0 = 18;
        config->i2c0_sda = 3;
        config->i2c0_scl = 15;
        config->analog_pw = 8;
        config->sw0 = 19;
        config->sw1 = 1;
        config->sw2 = 4;
        config->sw3 = 5;
        config->sw4 = 0;
        config->btn_boot = 9;
    }

    char *get_ble_name()
    {
        return blue_prefix;
    }
};

}

hpde_tools::BoardConfig_Prototype2 boardConfig;

#endif