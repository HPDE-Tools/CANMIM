#include "console/console.h"
#include "esp_console.h"
#include "stdio.h"
#include "esp_log.h"

#include <string>
namespace hpde_tools {

static BoardContext *boardContext;

static int cmd_debug_func(int argc, char** argv) {
    if (argc != 2) {
        printf("invalid command\n");
        return 1;
    }
    std::string cmd(argv[1]);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
        [](unsigned char c){ return std::tolower(c); });
    if (cmd == "on") {
        boardContext->debug = true;
        esp_log_set_level_master(ESP_LOG_INFO);
        printf("Debug Print Out ON\n");
    } else if (cmd == "off") {
        boardContext->debug = false;
        esp_log_set_level_master(ESP_LOG_ERROR);
        printf("Debug Print Out OFF\n");
    } else {
        printf("invalid command\n");
        return 1;
    }
    return 0;
}



void ConfigConsole::StartUSBUARTConsole() {
    hpde_tools::boardContext = this->boardContext;
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = ">";
    repl_config.max_cmdline_length = 256;
    esp_console_register_help_command();
    esp_console_cmd_t cmd_debug = {
        .command = "debug",
        .help = "set debug output ON/OFF",
        .hint = NULL,
        .func = cmd_debug_func,
        .argtable = NULL,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_debug));
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}


}