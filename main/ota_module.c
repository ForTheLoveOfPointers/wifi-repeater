#include "ota_module.h"

const static char *firmware_update_url = "http://server.com/firmware.bin";

const static char *TAG = "OTA task";

extern const uint8_t cert_pem_start[] asm("_binary_certs_cert_pem_start");
extern const uint8_t cert_pem_end[]   asm("_binary_certs_cert_pem_end");

void perform_ota_update(void * pvParameters) {
    esp_http_client_config_t cfg = {
        .url = firmware_update_url,
        .client_cert_len = cert_pem_end - cert_pem_start,
        .cert_pem = (char *)cert_pem_start,
        .keep_alive_enable = true
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