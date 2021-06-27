//
// Created by david on 19.10.20.
//
#include "esp_modem_dce.h"
#include "esp_modem_dce_internal.h"
#include "esp_log.h"
//#include "esp_modem_dte.h"

static const char *DCE_TAG = "esp_modem_dce";

esp_err_t esp_modem_dce_generic_command(modem_dce_t *dce, const char * command, uint32_t timeout, esp_modem_dce_handle_line_t handle_line, void *ctx)
{
    modem_dte_t *dte = dce->dte;
    int errors = 0;
    int timeouts = 0;
    while (timeouts <= dce->dce_internal->config.retries_after_timeout &&
           errors <= dce->dce_internal->config.retries_after_error) {
        if (timeouts || errors) {
            // provide recovery action based on the defined strategy
            switch (dce->dce_internal->config.retry_strategy) {
                case ESP_MODEM_COMMAND_RESET_ON_ERROR:
                    dce->reset(dce);
                    break;
                case ESP_MODEM_COMMAND_RESEND_ON_ERROR:
                    // no recovery action, just plain resending
                    break;
                case ESP_MODEM_COMMAND_FAIL_ON_ERROR:
                    return ESP_FAIL;
                case ESP_MODEM_COMMAND_RESYNC_ON_ERROR:
                    esp_modem_dce_sync(dce, NULL, NULL);
                    break;
                case ESP_MODEM_COMMAND_POWERCYCLE_ON_ERROR:
                    dce->power_down(dce);
                    dce->power_up(dce);
                    break;
            }
        }
        ESP_LOGD(DCE_TAG, "%s(%d): Sending command:%s...", __func__, __LINE__, command );
        dce->handle_line = handle_line;
        dce->dce_internal->handle_line_ctx = ctx;
        if (dte->send_cmd(dte, command, timeout) != ESP_OK) {
            ESP_LOGW(DCE_TAG, "%s(%d): Command:%s response timeout", __func__, __LINE__, command );
            timeouts++;
            continue;
        }
        if (dce->state == MODEM_STATE_FAIL) {
            ESP_LOGW(DCE_TAG, "%s(%d): Command:%s failed", __func__, __LINE__, command );
            errors++;
            continue;
        }
        ESP_LOGD(DCE_TAG, "%s(%d): Command:%s succeeded", __func__, __LINE__, command );
        return ESP_OK;
    }
    return ESP_FAIL;
}


esp_err_t esp_modem_dce_default_deinit(modem_dce_t *dce)
{
    if (dce->dte) {
        dce->dte->dce = NULL;
    }
    esp_modem_dce_delete_all_commands(dce);
    free(dce->dce_internal);
    return ESP_OK;
}