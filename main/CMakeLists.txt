idf_component_register(SRCS "firewall_rules.c" "http_server.c" "firewall-extender.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    REQUIRES cjson
                    REQUIRES web
                    )
