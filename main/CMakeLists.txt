idf_component_register(SRCS "ota_module.c" "firewall_rules.c" "http_server.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    REQUIRES cjson
                    REQUIRES web
                    )
