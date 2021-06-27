set(srcs "src/esp_modem.c"
        "src/esp_modem_dce_command_lib.c"
        "src/esp_modem_dce_common_commands.c"
        "src/esp_modem_dce.c"
        "src/esp_modem_netif.c"
        "src/esp_modem_reset_helper.c"
        "src/esp_sim800.c"
        "src/esp_sim7600.c"
        "src/esp_bg96.c")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS include
                    PRIV_INCLUDE_DIRS private_include
                    REQUIRES driver)
