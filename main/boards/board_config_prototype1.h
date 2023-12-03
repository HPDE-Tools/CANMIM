#ifndef __HPDE_TOOLS_BOARD_CONFIG_PROTOTYPE1_H__
#define __HPDE_TOOLS_BOARD_CONFIG_PROTOTYPE1_H__

#include "board.h"

namespace hpde_tools
{

class BoardConfig_Prototype1 : public BoardConfig
{
public:
    void set_pin_definition_derived(board_pin_config_t *config) override;
    char *get_ble_name() override;
};

}

hpde_tools::BoardConfig_Prototype1 boardConfig;

#endif