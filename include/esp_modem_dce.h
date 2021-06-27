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

#include "esp_types.h"
#include "esp_err.h"
#include "esp_modem_dte.h"

typedef struct modem_dce modem_dce_t;
typedef struct modem_dte modem_dte_t;

/**
 * @brief Result Code from DCE
 *
 */
#define MODEM_RESULT_CODE_SUCCESS "OK"              /*!< Acknowledges execution of a command */
#define MODEM_RESULT_CODE_CONNECT "CONNECT"         /*!< A connection has been established */
#define MODEM_RESULT_CODE_RING "RING"               /*!< Detect an incoming call signal from network */
#define MODEM_RESULT_CODE_NO_CARRIER "NO CARRIER"   /*!< Connection termincated or establish a connection failed */
#define MODEM_RESULT_CODE_ERROR "ERROR"             /*!< Command not recognized, command line maximum length exceeded, parameter value invalid */
#define MODEM_RESULT_CODE_NO_DIALTONE "NO DIALTONE" /*!< No dial tone detected */
#define MODEM_RESULT_CODE_BUSY "BUSY"               /*!< Engaged signal detected */
#define MODEM_RESULT_CODE_NO_ANSWER "NO ANSWER"     /*!< Wait for quiet answer */

/**
 * @brief Specific Length Constraint
 *
 */
#define MODEM_MAX_NAME_LENGTH (32)     /*!< Max Module Name Length */
#define MODEM_MAX_OPERATOR_LENGTH (32) /*!< Max Operator Name Length */
#define MODEM_IMEI_LENGTH (15)         /*!< IMEI Number Length */
#define MODEM_IMSI_LENGTH (15)         /*!< IMSI Number Length */

/**
 * @brief Specific Timeout Constraint, Unit: millisecond
 *
 */
#define MODEM_COMMAND_TIMEOUT_DEFAULT (500)      /*!< Default timeout value for most commands */
#define MODEM_COMMAND_TIMEOUT_OPERATOR (75000)   /*!< Timeout value for getting operator status */
#define MODEM_COMMAND_TIMEOUT_RESET (60000)   /*!< Timeout value for reset command */
#define MODEM_COMMAND_TIMEOUT_MODE_CHANGE (5000) /*!< Timeout value for changing working mode */

#define MODEM_COMMAND_TIMEOUT_POWEROFF (1000)    /*!< Timeout value for power down */

/**
 * @brief Working state of DCE
 *
 */
typedef enum {
    MODEM_STATE_PROCESSING, /*!< In processing */
    MODEM_STATE_SUCCESS,    /*!< Process successfully */
    MODEM_STATE_FAIL        /*!< Process failed */
} modem_state_t;



typedef struct common_string_s {
    const char * command;
    char * string;
    size_t len;
} common_string_t;


typedef struct cbc_ctx_s {
    int battery_status;     //!< current status in mV
    int bcs;                //!< charge status (-1-Not available, 0-Not charging, 1-Charging, 2-Charging done)
    int bcl;                //!< 1-100% battery capacity, -1-Not available
} cbc_ctx_t;

typedef struct csq_ctx_s {
    int rssi;             //!< Signal strength indication
    int ber;              //!< Channel bit error rate
} csq_ctx_t;

typedef esp_err_t (*esp_modem_dce_handle_line_t)(modem_dce_t *dce, const char *line);

esp_err_t esp_modem_dce_generic_command(modem_dce_t *dce, const char * command, uint32_t timeout, esp_modem_dce_handle_line_t handle_line, void *ctx);


struct modem_dce_internal;

typedef enum esp_modem_command_retry_strategy_e {
    ESP_MODEM_COMMAND_FAIL_ON_ERROR = 0,
    ESP_MODEM_COMMAND_RESEND_ON_ERROR,
    ESP_MODEM_COMMAND_RESYNC_ON_ERROR,
    ESP_MODEM_COMMAND_RESET_ON_ERROR,
    ESP_MODEM_COMMAND_POWERCYCLE_ON_ERROR,
} esp_modem_command_retry_strategy_t;

typedef struct esp_modem_dce_config_s {
    const char *apn;            //!< Access Ponit Name, a logical name to choose data network
    int retries_after_timeout;  //!< Retry strategy: numbers of resending the same command on timeout
    int retries_after_error;    //!< Retry strategy: numbers of resending the same command on error
    esp_modem_command_retry_strategy_t retry_strategy;        //!< Retry strategy: calls

} esp_modem_dce_config_t;

#define ESP_MODEM_DCE_DEFAULT_CFG(APN) \
    {                                  \
        .apn = APN,                    \
        .retries_after_timeout = 1,    \
        .retries_after_error = 0,      \
        .retry_strategy = ESP_MODEM_COMMAND_POWERCYCLE_ON_ERROR, \
    }

/**
 * @brief DCE(Data Communication Equipment)
 *
 */
struct modem_dce {
    modem_state_t state;                                                              /*!< Modem working state */
    modem_mode_t mode;                                                                /*!< Working mode */
    modem_dte_t *dte;                                                                 /*!< DTE which connect to DCE */
    struct modem_dce_internal *dce_internal;
    esp_modem_dce_handle_line_t handle_line;                                                        /*!< Handle line strategy */
    esp_err_t (*sync)(modem_dce_t *dce);                                              /*!< Synchronization */
    esp_err_t (*set_working_mode)(modem_dce_t *dce, modem_mode_t mode); /*!< Set working mode */
    esp_err_t (*power_down)(modem_dce_t *dce);                          /*!< Normal power down */
    esp_err_t (*deinit)(modem_dce_t *dce);                              /*!< Deinitialize */
    esp_err_t (*power_up)(modem_dce_t *dce);                            /*!< Power up sequence */
    esp_err_t (*reset)(modem_dce_t *dce);                               /*!< Reset sequence */
};

esp_err_t esp_modem_command(modem_dce_t *dce, const char * command, void * param, void* result);

esp_err_t esp_modem_dce_default_deinit(modem_dce_t *dce);

esp_err_t esp_modem_process_command_done(modem_dce_t *dce, modem_state_t state);

//esp_err_t common_get_common_string(modem_dce_t *dce, void *ctx);



#ifdef __cplusplus
}
#endif
