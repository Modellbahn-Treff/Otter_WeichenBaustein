// Copyright (C) 2026 Modellbahn-Treff for Kids GmbH
// SPDX-License-Identifier: GPL-3.0-or-later

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "wm.h"
#include "otter.h"
#include "settings.h"
#include <string.h>

static const char *TAG = "WM";

bool    AmSchalten[8]         = {0};
int64_t AusSchaltZeitStempel[8] = {0};

void WM_Trigger(uint8_t number, const char *richtung) {
    if (strcmp(richtung, "0") == 0) {
        ESP_LOGI(TAG, "Weiche %u auf '0' geschaltet.", number);
        gpio_set_level((gpio_num_t)Pin[2 * number],     0);
        gpio_set_level((gpio_num_t)Pin[2 * number + 1], 1);
        AmSchalten[number]          = 1;
        AusSchaltZeitStempel[number] = esp_timer_get_time() / 1000;
    } else if (strcmp(richtung, "1") == 0) {
        ESP_LOGI(TAG, "Weiche %u auf '1' geschaltet.", number);
        gpio_set_level((gpio_num_t)Pin[2 * number],     1);
        gpio_set_level((gpio_num_t)Pin[2 * number + 1], 0);
        AmSchalten[number]          = 1;
        AusSchaltZeitStempel[number] = esp_timer_get_time() / 1000;
    } else {
        ESP_LOGW(TAG, "Richtung existiert nicht");
    }
}

void WM_Loop(uint8_t number) {
    if (AmSchalten[number] == 1) {
        int64_t elapsed = (esp_timer_get_time() / 1000) - AusSchaltZeitStempel[number];
        if (elapsed >= AusSchaltZeitWeiche) {
            AmSchalten[number] = 0;
            gpio_set_level((gpio_num_t)Pin[2 * number],     0);
            gpio_set_level((gpio_num_t)Pin[2 * number + 1], 0);
            ESP_LOGI(TAG, "W%u aus", number);
        }
    }
}

void WM_Start(uint8_t number) {
    gpio_set_direction((gpio_num_t)Pin[2 * number],     GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)Pin[2 * number + 1], GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)Pin[2 * number],     0);
    gpio_set_level((gpio_num_t)Pin[2 * number + 1], 0);
}
