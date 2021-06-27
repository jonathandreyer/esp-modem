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
#include <string.h>
#include "esp_log.h"
#include "esp_modem_dce_internal.h"

/**
 * @brief Macro defined for error checking
 *
 */
static const char *DCE_TAG = "dce_service";
#define ESP_MODEM_DCE_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(DCE_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)


static cmd_item_t s_command_list[] = {
        { .command = "sync", .function = esp_modem_dce_sync },
        { .command = "get_imei_number", .function = esp_modem_dce_get_imei_number },
        { .command = "get_imsi_number", .function = esp_modem_dce_get_imsi_number },
        { .command = "get_module_name", .function = esp_modem_dce_get_module_name },
        { .command = "get_operator_name", .function = esp_modem_dce_get_operator_name },
        { .command = "set_echo", .function = esp_modem_dce_set_echo },
        { .command = "store_profile", .function = esp_modem_dce_store_profile },
        { .command = "set_flow_ctrl", .function = esp_modem_dce_set_flow_ctrl },
        { .command = "set_pdp_context", .function = esp_modem_dce_set_pdp_context },
        { .command = "hang_up", .function = esp_modem_dce_hang_up },
        { .command = "get_signal_quality", .function = esp_modem_dce_get_signal_quality },
        { .command = "set_data_mode", .function = esp_modem_dce_set_data_mode },
        { .command = "set_command_mode", .function = esp_modem_dce_set_command_mode },
        { .command = "get_battery_status", .function = esp_modem_dce_get_battery_status },
        { .command = "power_down", .function = esp_modem_dce_power_down },
        { .command = "reset", .function = esp_modem_dce_reset },
};

static esp_err_t update_internal_command_refs(modem_dce_t *dce)
{
    dce->dce_internal->set_data_mode = esp_modem_dce_find_command(dce, "set_data_mode");
    dce->dce_internal->set_command_mode = esp_modem_dce_find_command(dce, "set_command_mode");
    dce->dce_internal->set_pdp_context = esp_modem_dce_find_command(dce, "set_pdp_context");
    dce->dce_internal->hang_up = esp_modem_dce_find_command(dce, "hang_up");
    dce->dce_internal->set_echo = esp_modem_dce_find_command(dce, "set_echo");

    if (dce->dce_internal->set_data_mode && dce->dce_internal->set_command_mode &&
        dce->dce_internal->set_pdp_context && dce->dce_internal->hang_up &&
        dce->dce_internal->set_echo) {
        return ESP_OK;
    }
    return ESP_FAIL;
}

static inline esp_err_t generic_command_default_handle(modem_dce_t *dce, const char * command)
{
    return esp_modem_dce_generic_command(dce, command, MODEM_COMMAND_TIMEOUT_DEFAULT, esp_modem_dce_handle_response_default, NULL);
}

esp_err_t esp_modem_process_command_done(modem_dce_t *dce, modem_state_t state)
{
    dce->state = state;
    return dce->dte->process_cmd_done(dce->dte);
}

esp_err_t esp_modem_dce_set_default_commands(modem_dce_t *dce)
{
    esp_err_t err = esp_modem_dce_init_command_list(dce, sizeof(s_command_list) / sizeof(cmd_item_t), s_command_list);
    if (err == ESP_OK) {
        return update_internal_command_refs(dce);
    }
    return err;

}

esp_err_t esp_modem_dce_handle_response_default(modem_dce_t *dce, const char *line)
{
    printf(">%s\n", line);
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

esp_err_t esp_modem_dce_handle_reset(modem_dce_t *dce, const char *line)
{
    printf(">%s\n", line);
    esp_err_t err = ESP_OK;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = ESP_OK;
    } else
    if (strstr(line, "PB DONE")) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Handle response from AT+CBC
 */
static esp_err_t esp_modem_dce_common_handle_cbc(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CBC", strlen("+CBC"))) {
        cbc_ctx_t *cbc = dce->dce_internal->handle_line_ctx;
        /* +CBC: <bcs>,<bcl>,<voltage> */
        sscanf(line, "%*s%d,%d,%d", &cbc->bcs, &cbc->bcl, &cbc->battery_status);
        err = ESP_OK;
    }
    return err;
}
/**
 * @brief Handle response from AT+CSQ
 */
static esp_err_t esp_modem_dce_common_handle_csq(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+CSQ", strlen("+CSQ"))) {
        /* store value of rssi and ber */
        csq_ctx_t *csq = dce->dce_internal->handle_line_ctx;
        /* +CSQ: <rssi>,<ber> */
        sscanf(line, "%*s%d,%d", &csq->rssi, &csq->ber);
        err = ESP_OK;
    }
    return err;
}

/**
 * @brief Handle response from AT+QPOWD=1
 */
static esp_err_t esp_modem_dce_handle_power_down(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = ESP_OK;
    } else if (strstr(line, "POWERED DOWN")) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    }
    return err;
}

/**
 * @brief Handle response from exiting the PPP mode
 */
esp_err_t esp_modem_dce_handle_exit_data_mode(modem_dce_t *dce, const char *line)
{
    printf(">>> %s\n", line);
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_NO_CARRIER)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}

/**
 * @brief Handle response from entry of the PPP mode
 */
static esp_err_t esp_modem_dce_handle_atd_ppp(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_CONNECT)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    }
    return err;
}


/**
 * @brief Set Working Mode
 *
 * @param dce Modem DCE object
 * @param mode working mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t esp_modem_dce_set_working_mode(modem_dce_t *dce, modem_mode_t mode)
{
    esp_modem_command_retry_strategy_t retry = dce->dce_internal->config.retry_strategy;
    switch (mode) {
        case MODEM_COMMAND_MODE:
            // temporarily suspend recovery, as the modem could already be in command mode and thus not responding
            dce->dce_internal->config.retry_strategy = ESP_MODEM_COMMAND_FAIL_ON_ERROR;
            if (dce->dce_internal->set_command_mode(dce, NULL, NULL) != ESP_OK) {
                esp_modem_dce_sync(dce, NULL, NULL);
            }
            dce->dce_internal->config.retry_strategy = retry;
            dce->mode = MODEM_COMMAND_MODE;
            // switching the echo mode off as command mode
            dce->dce_internal->set_echo(dce, (void*)false, NULL);
            break;
        case MODEM_PPP_MODE:
            // before going to data mode, set the internal PDP data
            dce->dce_internal->set_pdp_context(dce, &dce->dce_internal->pdp_ctx, NULL);
            dce->dce_internal->set_data_mode(dce, NULL, NULL);
            dce->mode = MODEM_PPP_MODE;
            break;
        default:
            ESP_LOGW(DCE_TAG, "unsupported working mode: %d", mode);
            goto err;
    }
    return ESP_OK;
err:
    return ESP_FAIL;
}

/**
 * @brief Handle response from AT+CGSN
 */
static esp_err_t common_handle_string(modem_dce_t *dce, const char *line)
{
    printf("common_handle_string:%s\n", line);
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else {
        common_string_t *result_str = dce->dce_internal->handle_line_ctx;
        assert(result_str->string != NULL && result_str->len != 0);
        int len = snprintf(result_str->string, result_str->len, "%s", line);
        if (len > 2) {
            /* Strip "\r\n" */
            strip_cr_lf_tail(result_str->string, len);
            err = ESP_OK;
        }
    }
    return err;
}



esp_err_t esp_modem_command(modem_dce_t *dce, const char * command, void * param, void* result)
{
    cmd_item_t *item;
    SLIST_FOREACH(item, &dce->dce_internal->command_list, next) {
        if (strcmp(item->command, command) == 0) {
            return item->function(dce, param, result);
        }
    }
    return ESP_ERR_NOT_FOUND;
}


dce_command_t esp_modem_dce_find_command(modem_dce_t *dce, const char * command)
{
    cmd_item_t *item;
    SLIST_FOREACH(item, &dce->dce_internal->command_list, next) {
        if (strcmp(item->command, command) == 0) {
            return item->function;
        }
    }
    return NULL;
}

esp_err_t esp_modem_dce_delete_all_commands(modem_dce_t *dce)
{
    while (!SLIST_EMPTY(&dce->dce_internal->command_list)) {
        cmd_item_t *item = SLIST_FIRST(&dce->dce_internal->command_list);
        SLIST_REMOVE_HEAD(&dce->dce_internal->command_list, next);
        if (item < s_command_list || item >= s_command_list + sizeof(s_command_list)) {
            // only free those items allocated additionally
            free(item);
        }
    }
    return ESP_OK;
}
esp_err_t esp_modem_dce_delete_command(modem_dce_t *dce, const char * command_id)
{
    cmd_item_t *item;
    SLIST_FOREACH(item, &dce->dce_internal->command_list, next) {
        if (strcmp(item->command, command_id) == 0) {
            SLIST_REMOVE(&dce->dce_internal->command_list, item, cmd_item_, next);
            if (item < s_command_list || item >= s_command_list + sizeof(s_command_list)) {
                // only free those items allocated additionally
                free(item);
            }
            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND;
}

esp_err_t esp_modem_dce_set_command(modem_dce_t *dce, const char * command_id, dce_command_t command)
{
    cmd_item_t *item;
    SLIST_FOREACH(item, &dce->dce_internal->command_list, next) {
        if (strcmp(item->command, command_id) == 0) {
            item->function = command;
            return update_internal_command_refs(dce);
        }
    }
    cmd_item_t *new_item = calloc(1, sizeof(struct cmd_item_));
    new_item->command = command_id;
    new_item->function = command;
    SLIST_INSERT_HEAD(&dce->dce_internal->command_list, new_item, next);
    return update_internal_command_refs(dce);;

}

static esp_err_t common_get_operator_after_mode_format(modem_dce_t *dce, const char *line)
{
    esp_err_t err = ESP_FAIL;
    if (strstr(line, MODEM_RESULT_CODE_SUCCESS)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_SUCCESS);
    } else if (strstr(line, MODEM_RESULT_CODE_ERROR)) {
        err = esp_modem_process_command_done(dce, MODEM_STATE_FAIL);
    } else if (!strncmp(line, "+COPS", strlen("+COPS"))) {
        common_string_t *result_str = dce->dce_internal->handle_line_ctx;
        assert(result_str->string != NULL && result_str->len != 0);
        /* there might be some random spaces in operator's name, we can not use sscanf to parse the result */
        /* strtok will break the string, we need to create a copy */
        char *line_copy = strdup(line);
        /* +COPS: <mode>[, <format>[, <oper>]] */
        char *str_ptr = NULL;
        char *p[3];
        uint8_t i = 0;
        /* strtok will broke string by replacing delimiter with '\0' */
        p[i] = strtok_r(line_copy, ",", &str_ptr);
        while (p[i]) {
            p[++i] = strtok_r(NULL, ",", &str_ptr);
        }
        if (i >= 3) {
            int len = snprintf(result_str->string, result_str->len, "%s", p[2]);
            if (len > 2) {
                /* Strip "\r\n" */
                strip_cr_lf_tail(result_str->string, len);
                err = ESP_OK;
            }
        }
        free(line_copy);
    }
    return err;
}

static esp_err_t common_get_common_string(modem_dce_t *dce, void *ctx)
{
    common_string_t * param_str = ctx;
    return esp_modem_dce_generic_command(dce, param_str->command, MODEM_COMMAND_TIMEOUT_DEFAULT, common_handle_string, ctx);
}

esp_err_t esp_modem_dce_get_imei_number(modem_dce_t *dce, void *param, void *result)
{
    common_string_t common_str = { .command = "AT+CGSN\r", .string = result, .len = (size_t)param };
    return common_get_common_string(dce, &common_str);
}

esp_err_t esp_modem_dce_get_imsi_number(modem_dce_t *dce, void *param, void *result)
{
    common_string_t common_str = { .command = "AT+CIMI\r", .string = result, .len = (size_t)param };
    return common_get_common_string(dce, &common_str);
}

esp_err_t esp_modem_dce_get_module_name(modem_dce_t *dce, void *param, void *result)
{
    common_string_t common_str = { .command = "AT+CGMM\r", .string = result, .len = (size_t)param };
    return common_get_common_string(dce, &common_str);
}

esp_err_t esp_modem_dce_get_operator_name(modem_dce_t *dce, void *param, void *result)
{
    common_string_t common_str = { .command = "AT+COPS?\r", .string = result, .len = (size_t)param };
    return esp_modem_dce_generic_command(dce, common_str.command, MODEM_COMMAND_TIMEOUT_OPERATOR,
                                         common_get_operator_after_mode_format, &common_str);
}

esp_err_t esp_modem_dce_set_echo(modem_dce_t *dce, void *param, void *result)
{
    bool echo_on = (bool)param;
    if (echo_on) {
        return generic_command_default_handle(dce, "ATE1\r");
    } else {
        return generic_command_default_handle(dce, "ATE0\r");
    }
}

esp_err_t esp_modem_dce_reset(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce,  "AT+CRESET\r", MODEM_COMMAND_TIMEOUT_RESET, esp_modem_dce_handle_reset, NULL);
}

esp_err_t esp_modem_dce_sync(modem_dce_t *dce, void *param, void *result)
{
    return generic_command_default_handle(dce, "AT\r");
}

esp_err_t esp_modem_dce_store_profile(modem_dce_t *dce, void *param, void *result)
{
    return generic_command_default_handle(dce, "AT&W\r");
}

esp_err_t esp_modem_dce_set_flow_ctrl(modem_dce_t *dce, void *param, void *result)
{
    modem_dte_t *dte = dce->dte;
    modem_flow_ctrl_t flow_ctrl = (modem_flow_ctrl_t)param;
    char *command;
    int len = asprintf(&command, "AT+IFC=%d,%d\r", dte->flow_ctrl, flow_ctrl);
    if (len <= 0) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = generic_command_default_handle(dce, command);
    free(command);
    return err;
}

esp_err_t esp_modem_dce_set_pdp_context(modem_dce_t *dce, void *param, void *result)
{
    esp_modem_dce_pdp_ctx_t *pdp = param;
    char *command;
    int len = asprintf(&command, "AT+CGDCONT=%d,\"%s\",\"%s\"\r", pdp->cid, pdp->type, pdp->apn);
    if (len <= 0) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t err = generic_command_default_handle(dce, command);
    free(command);
    return err;
}

#define MODEM_COMMAND_TIMEOUT_HANG_UP (90000)    /*!< Timeout value for hang up */

esp_err_t esp_modem_dce_hang_up(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "ATH\r", MODEM_COMMAND_TIMEOUT_HANG_UP,
                                         esp_modem_dce_handle_response_default, NULL);
}

esp_err_t esp_modem_dce_get_signal_quality(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "AT+CSQ\r", MODEM_COMMAND_TIMEOUT_DEFAULT,
                                         esp_modem_dce_common_handle_csq, result);
}

esp_err_t esp_modem_dce_get_battery_status(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "AT+CBC\r", MODEM_COMMAND_TIMEOUT_DEFAULT,
                                         esp_modem_dce_common_handle_cbc, result);

}

static esp_err_t esp_modem_power_down(modem_dce_t *dce)
{
    return esp_modem_command(dce, "power_down", NULL, NULL);
}

static esp_err_t esp_modem_power_up(modem_dce_t *dce)
{
    return esp_modem_dce_sync(dce, NULL, NULL);
}

static esp_err_t esp_modem_reset(modem_dce_t *dce)
{
    return esp_modem_dce_reset(dce, NULL, NULL);
}

static esp_err_t esp_modem_dce_default_destroy(modem_dce_t *dce)
{
    ESP_MODEM_DCE_CHECK(esp_modem_dce_default_deinit(dce) == ESP_OK, "failed", err);
    free(dce);
    return ESP_OK;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_default_init(modem_dce_t *dce, esp_modem_dce_config_t* config)
{
    ESP_MODEM_DCE_CHECK(dce && config, "dce object or configuration is NULL", err);
    dce->dce_internal = calloc(1, sizeof(struct modem_dce_internal));
    ESP_MODEM_DCE_CHECK(dce->dce_internal, "Allocation of dce internal object has failed", err_malloc);
    memcpy(&dce->dce_internal->config, config, sizeof(esp_modem_dce_config_t));
    dce->dce_internal->pdp_ctx.cid = 1;
    dce->dce_internal->pdp_ctx.type = "IP";
    dce->dce_internal->pdp_ctx.apn = config->apn;
    // save the config

    // create default handlers
    dce->set_working_mode = esp_modem_dce_set_working_mode;
    dce->power_down = esp_modem_power_down;
    dce->power_up = esp_modem_power_up;
    dce->reset = esp_modem_reset;
    dce->deinit = esp_modem_dce_default_destroy;
    return ESP_OK;

err_malloc:
    free(dce);
    return ESP_ERR_NO_MEM;
err:
    return ESP_FAIL;
}

esp_err_t esp_modem_dce_init_command_list(modem_dce_t *dce, size_t commands, cmd_item_t *command_list)
{
    if (commands < 1 || command_list == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    SLIST_INIT(&dce->dce_internal->command_list);

    for (int i=0; i < commands; ++i) {
        SLIST_INSERT_HEAD(&dce->dce_internal->command_list, &(command_list[i]), next);
    }
    return ESP_OK;
}

esp_err_t esp_modem_dce_set_data_mode(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "ATD*99***1#\r", MODEM_COMMAND_TIMEOUT_MODE_CHANGE,
                                         esp_modem_dce_handle_atd_ppp, NULL);
}

esp_err_t esp_modem_dce_set_command_mode(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "+++", MODEM_COMMAND_TIMEOUT_MODE_CHANGE,
                                         esp_modem_dce_handle_exit_data_mode, NULL);
}

esp_err_t esp_modem_dce_power_down(modem_dce_t *dce, void *param, void *result)
{
    return esp_modem_dce_generic_command(dce, "AT+QPOWD=1\r", MODEM_COMMAND_TIMEOUT_POWEROFF,
                                         esp_modem_dce_handle_power_down, NULL);
}
