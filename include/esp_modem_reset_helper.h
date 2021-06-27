//
// Created by david on 20.10.20.
//

#ifndef PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H
#define PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H

#include "esp_err.h"
#include "esp_modem_dce.h"

/**
 * @brief DCE(Data Communication Equipment)
 *
 */
//
//typedef enum esp_modem_command_retry_mode_e {
//    ESP_MODEM_COMMAND_FAIL_ON_ERROR = 0,
//    ESP_MODEM_COMMAND_RESEND_ON_ERROR,
//    ESP_MODEM_COMMAND_RESYNC_ON_ERROR,
//    ESP_MODEM_COMMAND_RESET_ON_ERROR,
//    ESP_MODEM_COMMAND_POWERCYCLE_ON_ERROR,
//} esp_modem_command_retry_mode_t;
//
//typedef struct esp_modem_command_retry_strategy_s {
//    int retries_after_timeout;                      //!< Retry strategy: numbers of resending the same command on timeout
//    int retries_after_error;                        //!< Retry strategy: numbers of resending the same command on error
//    esp_modem_command_retry_mode_t retry_mode;      //!< Retry strategy mode
//} esp_modem_command_retry_strategy_t;


typedef struct esp_modem_gpio_s {
    int gpio_num;
    int inactive_level;
    int active_width_ms;
    int inactive_width_ms;
    void (*pulse)(struct esp_modem_gpio_s *pin);
    void (*pulse_special)(struct esp_modem_gpio_s *pin, int active_width_ms, int inactive_width_ms);
    void (*destroy)(struct esp_modem_gpio_s *pin);
} esp_modem_gpio_t;

typedef struct esp_modem_retry_s esp_modem_retry_t;

typedef esp_err_t (*esp_modem_retry_fn_t)(esp_modem_retry_t *retry_cmd, int timeouts, int errors);

struct esp_modem_retry_s {
    const char *command;
    esp_err_t (*run)(struct esp_modem_retry_s *retry, void *param, void *result);
    dce_command_t orig_cmd;
    esp_modem_retry_fn_t recover;
    esp_modem_dce_t *dce;
    int retries_after_timeout;                      //!< Retry strategy: numbers of resending the same command on timeout
    int retries_after_error;                        //!< Retry strategy: numbers of resending the same command on error
} ;

esp_modem_retry_t *esp_modem_retry_create(esp_modem_dce_t *dce, dce_command_t orig_cmd, esp_modem_retry_fn_t recover, int max_timeouts, int max_errors);

esp_modem_gpio_t *esp_modem_gpio_create(int gpio_num, int inactive_level, int active_width_ms, int inactive_width_ms);

//esp_err_t esp_modem_exec_command(esp_modem_dce_t *dce, const char * command, void * params, void *result, esp_modem_command_retry_strategy_t *retry);

#define DEFINE_RETRY_CMD(name, retry, super_type) \
esp_err_t name(esp_modem_dce_t *dce, void *param, void *result) \
{ \
    super_type *super = __containerof(dce, super_type, parent); \
    return super->retry->run(super->retry, param, result);      \
}


#endif //PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H
