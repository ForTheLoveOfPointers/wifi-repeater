idf_component_register(SRCS "http_server.c" "firewall-extender.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    REQUIRES spiffs
                    REQUIRES cjson
                    REQUIRES web
                    )
