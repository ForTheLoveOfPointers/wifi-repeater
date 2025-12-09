#pragma once
#include <stdbool.h>
#include "esp_log.h"
#include "esp_https_ota.h"
void perform_ota_update(void * pvParameters);