//
// Created by david on 20.10.20.
//
#include "esp_modem_reset_helper.h"
#include <stdlib.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_modem_dce_internal.h"
#include "esp_log.h"

static const char *DCE_TAG = "esp_modem_reset_helper";

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
    ESP_MODEM_DCE_CHECK(pin, "failed to allocate pin structure", err);

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

