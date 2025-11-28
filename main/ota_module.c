#include "ota_module.h"

const static char *firmware_update_url = "http://server.com/firmware.bin"

const static char *TAG = "OTA task";

void perform_ota_update(void) {
    esp_http_client_config_t cfg = {
        .url = firmware_update_url,
        .timeout_ms = 5000,
        .skip_cert_common_name_check = true,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &cfg,
    };

    esp_err_t ret = esp_https_ota(&ota_cfg);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful, rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
}