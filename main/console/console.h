#ifndef __HPDE_TOOLS_CONSOLE_H__
#define __HPDE_TOOLS_CONSOLE_H__

#include "context/context.h"

namespace hpde_tools {

class ConfigConsole {

private:
BoardContext *boardContext;

public:
    ConfigConsole(BoardContext *b) : boardContext(b) {}
    ConfigConsole(const ConfigConsole &) = delete;
    ConfigConsole &operator=(const ConfigConsole &) = delete;
    void StartUSBUARTConsole();

};

}

#endif