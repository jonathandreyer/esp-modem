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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_modem_dce.h"

/**
* @brief Macro defined for error checking
*
*/
#define ESP_MODEM_DCE_CHECK(a, str, goto_tag, ...)                                    \
    do                                                                                \
    {                                                                                 \
        if (!(a))                                                                     \
        {                                                                             \
            ESP_LOGE(DCE_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                            \
        }                                                                             \
    } while (0)



typedef esp_err_t (*dce_command_t)(modem_dce_t *dce, void *param, void *result);

typedef struct cmd_item_ {
    /**
     * Command name (statically allocated by application)
     */
    const char *command;                   //!< optional pointer to arg table
    dce_command_t function;                 //!< optional pointer to arg table
    SLIST_ENTRY(cmd_item_) next;    //!< next command in the list
} cmd_item_t;


/**
 * @brief Strip the tailed "\r\n"
 *
 * @param str string to strip
 * @param len length of string
 */
static inline void strip_cr_lf_tail(char *str, uint32_t len)
{
    if (str[len - 2] == '\r') {
        str[len - 2] = '\0';
    } else if (str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }
}

/**
 * @brief Default handler for response
 * Some responses for command are simple, commonly will return OK when succeed of ERROR when failed
 *
 * @param dce Modem DCE object
 * @param line line string
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t esp_modem_dce_handle_response_default(modem_dce_t *dce, const char *line);

/**
 * @brief Syncronization
 *
 * @param dce Modem DCE object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_sync(modem_dce_t *dce);

/**
 * @brief Enable or not echo mode of DCE
 *
 * @param dce Modem DCE object
 * @param on true to enable echo mode, false to disable echo mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_echo(modem_dce_t *dce, bool on);

/**
 * @brief Store current parameter setting in the user profile
 *
 * @param dce Modem DCE object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_store_profile(modem_dce_t *dce);

/**
 * @brief Set flow control mode of DCE in data mode
 *
 * @param dce Modem DCE object
 * @param flow_ctrl flow control mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_set_flow_ctrl(modem_dce_t *dce, modem_flow_ctrl_t flow_ctrl);

/**
 * @brief Define PDP context
 *
 * @param dce Modem DCE object
 * @param cid PDP context identifier
 * @param type Protocol type
 * @param apn Access point name
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_set_pdp_context(modem_dce_t *dce, uint32_t cid, const char *type, const char *apn);

/**
 * @brief Hang up
 *
 * @param dce Modem DCE object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_hang_up(modem_dce_t *dce);

/**
 * @brief Get signal quality
 *
 * @param dce Modem DCE object
 * @param rssi received signal strength indication
 * @param ber bit error ratio
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_dce_get_signal_quality(modem_dce_t *dce, uint32_t *rssi, uint32_t *ber);


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
//esp_err_t esp_modem_dce_get_battery_status(modem_dce_t *dce, uint32_t *bcs, uint32_t *bcl, uint32_t *voltage);

/**
 * @brief Set Working Mode
 *
 * @param dce Modem DCE object
 * @param mode_commands Array of commands for switching between AT/PPP modes
 *          mode_commands[0]: command to exit PPP mode
 *          mode_commands[1]: AT command to enter PPP mode
 * @param mode working mode
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t esp_modem_dce_set_working_mode(modem_dce_t *dce, modem_mode_t mode);


esp_err_t esp_modem_dce_init_command_list(modem_dce_t *dce, size_t commands, cmd_item_t *command_list);

/**
 * @brief Indicate that processing current command has done
 *
 * @param dce Modem DCE object
 * @param state Modem state after processing
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
//esp_err_t esp_modem_process_command_done(modem_dce_t *dce, modem_state_t state);

dce_command_t esp_modem_dce_find_command(modem_dce_t *dce, const char * command);

esp_err_t esp_modem_dce_set_command(modem_dce_t *dce, const char * command_id, dce_command_t command);

esp_err_t esp_modem_dce_delete_command(modem_dce_t *dce, const char * command_id);

esp_err_t esp_modem_dce_delete_all_commands(modem_dce_t *dce);

esp_err_t esp_modem_dce_set_default_commands(modem_dce_t *dce);

typedef struct esp_modem_dce_pdp_ctx_s {
    size_t cid;
    const char *type;
    const char *apn;
} esp_modem_dce_pdp_ctx_t;


struct modem_dce_internal {
    esp_modem_dce_pdp_ctx_t pdp_ctx;
    void *handle_line_ctx;                                              /*!< DCE context reserved for handle_line
                                                                             processing */
    SLIST_HEAD(cmd_list_, cmd_item_) command_list;

    esp_modem_dce_config_t config;
    // list of frequently used commands
    dce_command_t hang_up;
    dce_command_t set_pdp_context;
    dce_command_t set_data_mode;
    dce_command_t set_command_mode;
    dce_command_t set_echo;

};

/**
 * @brief Get DCE module IMEI number
 *
 * @param sim800_dce sim800 object
 * @return esp_err_t
 *      - ESP_OK on success
 *      - ESP_FAIL on error
 */
esp_err_t esp_modem_dce_get_imei_number(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_get_imsi_number(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_get_module_name(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_get_operator_name(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_set_echo(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_sync(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_store_profile(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_set_flow_ctrl(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_set_pdp_context(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_hang_up(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_get_signal_quality(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_get_battery_status(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_set_data_mode(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_set_command_mode(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_power_down(modem_dce_t *dce, void *param, void *result);
esp_err_t esp_modem_dce_reset(modem_dce_t *dce, void *param, void *result);

esp_err_t esp_modem_dce_handle_exit_data_mode(modem_dce_t *dce, const char *line);
esp_err_t esp_modem_dce_default_init(modem_dce_t *dce, esp_modem_dce_config_t* config);

#ifdef __cplusplus
}
#endif
