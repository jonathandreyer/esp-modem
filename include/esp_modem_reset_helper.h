//
// Created by david on 20.10.20.
//

#ifndef PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H
#define PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H

/**
 * @brief DCE(Data Communication Equipment)
 *
 */

typedef struct esp_modem_gpio_s {
    int gpio_num;
    int inactive_level;
    int active_width_ms;
    int inactive_width_ms;
    void (*pulse)(struct esp_modem_gpio_s *pin);
    void (*pulse_special)(struct esp_modem_gpio_s *pin, int active_width_ms, int inactive_width_ms);
    void (*destroy)(struct esp_modem_gpio_s *pin);
} esp_modem_gpio_t;

esp_modem_gpio_t *esp_modem_gpio_create(int gpio_num, int inactive_level, int active_width_ms, int inactive_width_ms);

#endif //PPPOS_CLIENT_ESP_MODEM_RESET_HELPER_H
