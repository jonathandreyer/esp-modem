//
// Created by david on 20.10.20.
//
#include "esp_modem_reset_helper.h"
#include <stdlib.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_modem_dce_command_lib.h"
#include "esp_modem_internal.h"
#include "esp_log.h"

static const char *TAG = "esp_modem_reset_helper";

static void destroy(esp_modem_gpio_t * pin)
{
    free(pin);
}

static void pulse_special(esp_modem_gpio_t * pin, int active_width_ms, int inactive_width_ms)
{
    gpio_set_level(pin->gpio_num, !pin->inactive_level);
    vTaskDelay(active_width_ms / portTICK_PERIOD_MS);
    gpio_set_level(pin->gpio_num, pin->inactive_level);
    vTaskDelay(inactive_width_ms/ portTICK_PERIOD_MS);
}

static void pulse(esp_modem_gpio_t * pin)
{
    gpio_set_level(pin->gpio_num, !pin->inactive_level);
    vTaskDelay(pin->active_width_ms / portTICK_PERIOD_MS);
    gpio_set_level(pin->gpio_num, pin->inactive_level);
    vTaskDelay(pin->inactive_width_ms/ portTICK_PERIOD_MS);
}

static esp_err_t esp_modem_retry_run(esp_modem_retry_t * retry, void *param, void *result)
{
    esp_modem_dce_t *dce = retry->dce;
    int errors = 0;
    int timeouts = 0;
    esp_err_t err = ESP_OK;
    while (timeouts <= retry->retries_after_timeout &&
           errors <= retry->retries_after_error) {
        if (timeouts || errors) {
            // provide recovery action based on the defined strategy
            retry->recover(retry, timeouts, errors);
        }
        ESP_LOGD(TAG, "%s(%d): executing:%s...", __func__, __LINE__, retry->command );
        err = retry->orig_cmd(dce, param, result);
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "%s(%d): Command:%s response timeout", __func__, __LINE__, retry->command );
            timeouts++;
            continue;
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "%s(%d): Command:%s failed", __func__, __LINE__, retry->command );
            errors++;
            continue;
        }
        ESP_LOGD(TAG, "%s(%d): Command:%s succeeded", __func__, __LINE__, retry->command );
        return ESP_OK;
    }
    return ESP_FAIL;

}

esp_modem_retry_t *esp_modem_retry_create(esp_modem_dce_t *dce, dce_command_t orig_cmd, esp_modem_retry_fn_t recover, int max_timeouts, int max_errors)
{
    esp_modem_retry_t * retry = calloc(1, sizeof(esp_modem_retry_t));
    ESP_MODEM_ERR_CHECK(retry, "failed to allocate pin structure", err);
    retry->retries_after_error = max_errors;
    retry->retries_after_timeout = max_timeouts;
    retry->command = "my_new_cmd";
    retry->dce = dce;
    retry->run = esp_modem_retry_run;
    return retry;
err:
    return NULL;
}

esp_modem_gpio_t *esp_modem_gpio_create(int gpio_num, int inactive_level, int active_width_ms, int inactive_width_ms)
{
    gpio_config_t io_config = {
            .pin_bit_mask = BIT64(gpio_num),
            .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io_config);
    gpio_set_level(gpio_num, inactive_level);
    gpio_set_level(gpio_num, inactive_level);

    esp_modem_gpio_t * pin = calloc(1, sizeof(esp_modem_gpio_t));
    ESP_MODEM_ERR_CHECK(pin, "failed to allocate pin structure", err);

    pin->inactive_level = inactive_level;
    pin->active_width_ms = active_width_ms;
    pin->inactive_width_ms = inactive_width_ms;
    pin->gpio_num = gpio_num;
    pin->pulse = pulse;
    pin->pulse_special = pulse_special;
    pin->destroy = destroy;
    return pin;
err:
    return NULL;
}

#if 0
esp_err_t esp_modem_exec_command(esp_modem_dce_t *dce, const char * command, void * params, void *result, esp_modem_command_retry_strategy_t *retry)
{
    int errors = 0;
    int timeouts = 0;
    while (timeouts <= retry->retries_after_timeout &&
           errors <= retry->retries_after_error) {
        if (timeouts || errors) {
            // provide recovery action based on the defined strategy
            switch (retry->retry_mode) {
                case ESP_MODEM_COMMAND_RESET_ON_ERROR:
//                    dce->reset(dce);
                    break;
                case ESP_MODEM_COMMAND_RESEND_ON_ERROR:
                    // no recovery action, just plain resending
                    break;
                case ESP_MODEM_COMMAND_FAIL_ON_ERROR:
                    return ESP_FAIL;
                case ESP_MODEM_COMMAND_RESYNC_ON_ERROR:
                    dce->sync(dce, NULL, NULL);
                    break;
                case ESP_MODEM_COMMAND_POWERCYCLE_ON_ERROR:
//                    dce->power_down(dce);
//                    dce->power_up(dce);
                    break;
            }
        }
        ESP_LOGD(TAG, "%s(%d): executing:%s...", __func__, __LINE__, command );
        esp_err_t err = esp_modem_command_list_run(dce, command, params, result);
        if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "%s(%d): Command:%s response timeout", __func__, __LINE__, command );
            timeouts++;
            continue;
        } else if (err != ESP_OK) {
            ESP_LOGW(TAG, "%s(%d): Command:%s failed", __func__, __LINE__, command );
            errors++;
            continue;
        }
        ESP_LOGD(TAG, "%s(%d): Command:%s succeeded", __func__, __LINE__, command );
        return ESP_OK;
    }
    return ESP_FAIL;
}
#endif