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

//#include "esp_modem_dce_service.h"
#include "esp_modem.h"

/**
 * @brief Create and initialize SIM7600 object
 *
 * @param dte Modem DTE object
 * @return modem_dce_t* Modem DCE object
 */
esp_modem_dce_t *esp_sim7600_create(esp_modem_dte_t *dte, esp_modem_dce_config_t *config);

esp_err_t esp_sim7600_init(esp_modem_dce_t *dce, esp_modem_dte_t *dte, esp_modem_dce_config_t *config);

#ifdef __cplusplus
}
#endif
