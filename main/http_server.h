#pragma once
#include <string.h>
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "esp_spiffs.h"
#include "firewall_rules.h"

#define AP_SSID "ESP32_wifi"
#define AP_PASS "h@llo_wifi_2025"

typedef struct {
    char sta_ssid[32];
    char sta_pass[64];
} router_config_t;


httpd_handle_t start_webserver(void);

esp_err_t load_config(router_config_t *cfg);
void save_config(const router_config_t *cfg);