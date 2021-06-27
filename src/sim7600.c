// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
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
#include "sim7600.h"

/**
 * @brief This module supports SIM7600 module, which has a very similar interface
 * to the BG96, so it just references most of the handlers from BG96 and implements
 * only those that differ.
 */
static const char *DCE_TAG = "sim7600";

/**
 * @brief Handle response from AT+CBC
 */
static esp_err_t sim7600_handle_cbc(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CBC", strlen("+CBC"))) {
        cbc_ctx_t *cbc = dce->dce_internal->handle_line_ctx;
        int32_t volts = 0, fraction = 0;
        /* +CBC: <voltage in Volts> V*/
        sscanf(line, "+CBC: %d.%dV", &volts, &fraction);
        /* Since the "read_battery_status()" API (besides voltage) returns also values for BCS, BCL (charge status),
         * which are not applicable to this modem, we return -1 to indicate invalid value
         */
        cbc->bcs = -1; // BCS
        cbc->bcl = -1; // BCL
        cbc->battery_status = volts*1000 + fraction;
        err = ESP_OK;
    }
    return err;
}

/**
 * @brief Get battery status
 *
 * @param dce Modem DCE object
 * @param bcs Battery charge status
 * @param bcl Battery connection level
 * @param voltage Battery voltage
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
static esp_err_t sim7600_get_battery_status(modem_dce_t *dce, void *p, void *r)
{
    return esp_modem_dce_generic_command(dce, "AT+CBC\r", 20000,
                                         sim7600_handle_cbc, r);
}
static esp_err_t sim7600_handle_power_down(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_OK;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_NO_CARRIER)) {
        err = ESP_OK;
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

static esp_err_t sim7600_power_down(modem_dce_t *dce, void *p, void *r)
{
    return esp_modem_dce_generic_command(dce, " AT+CPOF\r", MODEM_COMMAND_TIMEOUT_POWEROFF,
                                         sim7600_handle_power_down, NULL);
}

/**
 * @brief Create and initialize SIM7600 object
 *
 */
modem_dce_t *sim7600_create(modem_dte_t *dte, esp_modem_dce_config_t *config)
{
    modem_dce_t *dce = calloc(1, sizeof(modem_dce_t));
    ESP_MODEM_DCE_CHECK(dce, "calloc sim7600_dce failed", err);
    esp_err_t err = sim7600_init(dce, dte, config);
    ESP_MODEM_DCE_CHECK(err == ESP_OK, "sim7600_init has failed", err);
    return dce;
err:
   return NULL;

}

static esp_err_t sim7600_power_up(modem_dce_t* dce)
{
    ESP_MODEM_DCE_CHECK(esp_modem_command(dce, "sync", NULL, NULL)==ESP_OK, "sending sync failed", err);
    ESP_MODEM_DCE_CHECK(esp_modem_command(dce, "set_echo", (void*)false, NULL)==ESP_OK, "set_echo failed", err);
    ESP_MODEM_DCE_CHECK(esp_modem_command(dce, "set_flow_ctrl", (void*)MODEM_FLOW_CONTROL_NONE, NULL)==ESP_OK, "set_flow_ctrl failed", err);
    ESP_MODEM_DCE_CHECK(esp_modem_command(dce, "store_profile", NULL, NULL)==ESP_OK, "store_profile failed", err);
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t sim7600_init(modem_dce_t *dce, modem_dte_t *dte, esp_modem_dce_config_t *config)
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
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_command(dce, "power_down", sim7600_power_down) == ESP_OK, "esp_modem_dce_set_command failed", err);
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_command(dce, "get_battery_status", sim7600_get_battery_status) == ESP_OK, "esp_modem_dce_set_command failed", err);

    /* Initial power up sequence (by default sync and set */
    dce->power_up = sim7600_power_up;
    return ESP_OK;
err:
    return ESP_FAIL;
err_params:
    return ESP_ERR_INVALID_ARG;
}
