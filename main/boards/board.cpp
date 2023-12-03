#include "board.h"

#include <cstdio>
namespace hpde_tools
{

void BoardConfig::set_pin_definition(board_pin_config_t *config)
{
    int *config_raw = (int *)config;
    for (int i = 0; i < sizeof(hpde_tools::board_pin_config_t) / sizeof(int); i++)
    {
        config_raw[i] = -1;
    }
    this->set_pin_definition_derived(config);
}
}