// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_modem_dce_internal.h"
#include "sim800.h"

#define MODEM_RESULT_CODE_POWERDOWN "POWER DOWN"

/**
 * @brief Macro defined for error checking
 *
 */
static const char *DCE_TAG = "sim800";
#define ESP_MODEM_DCE_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(DCE_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)


/**
 * @brief Handle response from AT+CPOWD=1
 */
static esp_err_t sim800_handle_power_down(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_POWERDOWN)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    return err;
}


/**
 * @brief Set Working Mode
 *
 * @param dce Modem DCE object
 * @param mode woking mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//static esp_err_t sim800_set_working_mode(modem_dce_t *dce, modem_mode_t mode)
//{
//    const char * mode_commands[] = {"+++", "ATD*99##\r"};
//    return esp_modem_dce_set_working_mode(dce, mode_commands, mode);
//}

static esp_err_t sim800_set_command_mode(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "ATD*99##\r", MODEM_COMMAND_TIMEOUT_MODE_CHANGE,
                                         esp_modem_dce_handle_exit_data_mode, NULL);

}

/**
 * @brief Power down
 *
 * @param sim800_dce sim800 object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t sim800_power_down(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "AT+CPOWD=1\r", MODEM_COMMAND_TIMEOUT_POWEROFF,
                                         sim800_handle_power_down, NULL);
}





modem_dce_t *sim800_create(modem_dte_t *dte, esp_modem_dce_config_t *config)
{
    modem_dce_t *dce = calloc(1, sizeof(modem_dce_t));
    ESP_MODEM_DCE_CHECK(dce, "calloc sim7600_dce failed", err);
    esp_err_t err = sim800_init(dce, dte, config);
    ESP_MODEM_DCE_CHECK(err == ESP_OK, "sim800_init has failed", err);
    return dce;
err:
    return NULL;
}


esp_err_t sim800_init(modem_dce_t *dce, modem_dte_t *dte, esp_modem_dce_config_t *config)
{
    /* init the default DCE first */
    ESP_MODEM_DCE_CHECK(dce && dte && config, "failed to init with zero dce, dte or configuration", err_params);
    esp_err_t err = esp_modem_dce_default_init(dce, config);
    ESP_MODEM_DCE_CHECK(err == ESP_OK, "dce default init has failed", err);
    /* Bind DTE with DCE */
    dce->dte = dte;
    dte->dce = dce;
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_default_commands(dce) == ESP_OK, "esp_modem_dce_set_default_commands failed", err);

    /* Update some commands which differ from the defaults */
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_command(dce, "set_command_mode", sim800_set_command_mode) == ESP_OK, "esp_modem_dce_set_command failed", err);
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_command(dce, "power_down", sim800_power_down) == ESP_OK, "esp_modem_dce_set_command failed", err);

    /* Perform the initial sync */
    ESP_ERROR_CHECK(esp_modem_command(dce, "set_echo", (void*)false, NULL));
    ESP_ERROR_CHECK(esp_modem_command(dce, "set_flow_ctrl", (void*)MODEM_FLOW_CONTROL_NONE, NULL));
    ESP_ERROR_CHECK(esp_modem_command(dce, "store_profile", NULL, NULL));

    return ESP_OK;
err:
    return ESP_FAIL;
err_params:
    return ESP_ERR_INVALID_ARG;
}
