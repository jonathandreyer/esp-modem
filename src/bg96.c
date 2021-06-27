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
#include "bg96.h"
#include "esp_modem_dce_internal.h"


static const char *DCE_TAG = "bg96";

modem_dce_t *bg96_create(modem_dte_t *dte, esp_modem_dce_config_t *config)
{
    modem_dce_t *dce = calloc(1, sizeof(modem_dce_t));
    ESP_MODEM_DCE_CHECK(dce, "calloc sim7600_dce failed", err);
    esp_err_t err = bg96_init(dce, dte, config);
    ESP_MODEM_DCE_CHECK(err == ESP_OK, "bg96_init has failed", err);
    return dce;
err:
    return NULL;
}

esp_err_t bg96_init(modem_dce_t *dce, modem_dte_t *dte, esp_modem_dce_config_t *config)
{
    /* init the default DCE first */
    ESP_MODEM_DCE_CHECK(dce && dte && config, "failed to init with zero dce, dte or configuration", err_params);
    esp_err_t err = esp_modem_dce_default_init(dce, config);
    ESP_MODEM_DCE_CHECK(err == ESP_OK, "dce default init has failed", err);
    /* Bind DTE with DCE */
    dce->dte = dte;
    dte->dce = dce;

//    ESP_MODEM_DCE_CHECK(esp_modem_dce_init_command_list(bg96_dce, sizeof(s_command_list) / sizeof(cmd_item_t), s_command_list) == ESP_OK, "init cmd list failed", err_io);
    ESP_MODEM_DCE_CHECK(esp_modem_dce_set_default_commands(dce) == ESP_OK, "esp_modem_dce_set_default_commands failed", err);

    // update the commands


    /* Bind methods */
//    bg96_dce->define_pdp_context = set_current_pdp_context;
//    bg96_dce->hang_up = current_hang_up;


//    bg96_dce->sync = esp_modem_dce_sync;
//    bg96_dce->echo_mode = esp_modem_dce_echo;
//    bg96_dce->store_profile = esp_modem_dce_store_profile;
//    bg96_dce->set_flow_ctrl = esp_modem_dce_set_flow_ctrl;
//    bg96_dce->define_pdp_context = esp_modem_dce_set_pdp_context;
//    bg96_dce->get_signal_quality = esp_modem_dce_get_signal_quality;
//    bg96_dce->get_battery_status = esp_modem_dce_get_battery_status;
//    bg96_dce->set_working_mode = esp_modem_dce_set_working_mode;
//    bg96_dce->power_down = bg96_power_down;
//    bg96_dce->deinit = esp_modem_dce_default_deinit;
    /* Sync between DTE and DCE */
//    ESP_MODEM_DCE_CHECK(esp_modem_dce_sync(bg96_dce) == ESP_OK, "sync failed", err_io);
    /* Close echo */
//    ESP_MODEM_DCE_CHECK(esp_modem_dce_echo(bg96_dce, false) == ESP_OK, "close echo mode failed", err_io);
    ESP_ERROR_CHECK(esp_modem_command(dce, "sync", NULL, NULL));
    ESP_ERROR_CHECK(esp_modem_command(dce, "set_echo", (void*)false, NULL));
    ESP_ERROR_CHECK(esp_modem_command(dce, "set_flow_ctrl", (void*)MODEM_FLOW_CONTROL_NONE, NULL));
    ESP_ERROR_CHECK(esp_modem_command(dce, "store_profile", NULL, NULL));

    return ESP_OK;
err:
    return ESP_FAIL;
err_params:
    return ESP_ERR_INVALID_ARG;
}
