#ifndef __HPDE_TOOLS_BOARD_H__
#define __HPDE_TOOLS_BOARD_H__

#include "board_pin_config.h"

namespace hpde_tools
{
class BoardConfig
{

public:
    void set_pin_definition(board_pin_config_t *config);
    virtual void set_pin_definition_derived(board_pin_config_t *config);
    virtual char *get_ble_name();
};
}
#endif
