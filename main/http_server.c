#include "http_server.h"


// Linker symbols for embedded files!
extern const uint8_t _binary_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t _binary_index_html_end[]   asm("_binary_index_html_end");

extern const uint8_t _binary_style_css_start[] asm("_binary_style_css_start");
extern const uint8_t _binary_style_css_end[]   asm("_binary_style_css_end");

extern const uint8_t _binary_script_js_start[] asm("_binary_script_js_start");
extern const uint8_t _binary_script_js_end[]   asm("_binary_script_js_end");

extern const unsigned char _binary_favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const unsigned char _binary_favicon_ico_end[]   asm("_binary_favicon_ico_end");

/*
    CONFIG LOADING AND SAVING
*/
esp_err_t load_config(router_config_t *cfg) {
    nvs_handle_t nvs;
    if(nvs_open("router", NVS_READONLY, &nvs) != ESP_OK) {
        return ESP_FAIL;
    }

    size_t size = sizeof(cfg->sta_ssid);
    nvs_get_str(nvs, "sta_ssid", cfg->sta_ssid, &size);

    size = sizeof(cfg->sta_pass);
    nvs_get_str(nvs, "sta_pass", cfg->sta_pass, &size);

    nvs_close(nvs);

    return ESP_OK;
}

void save_config(const router_config_t *cfg) {
    nvs_handle_t nvs;
    nvs_open("router", NVS_READWRITE, &nvs);

    nvs_set_str(nvs, "sta_ssid", cfg->sta_ssid);
    nvs_set_str(nvs, "sta_pass", cfg->sta_pass);

    nvs_commit(nvs);
    nvs_close(nvs);
}



/*
    ROUTING
*/

static esp_err_t load_config_handler(httpd_req_t *req) {
    char res[256];
    
    httpd_resp_set_type(req, "application/json");
    router_config_t cfg;
    
    load_config(&cfg);

    snprintf(res, sizeof(res), 
            "{\"sta_ssid\":\"%s\"}", cfg.sta_ssid);
    

    httpd_resp_sendstr(req, res);
    return ESP_OK;
}

static httpd_uri_t load_config_uri = {
            .uri       = "/config",
            .method    = HTTP_GET,
            .handler   = load_config_handler,
            .user_ctx  = NULL
};


static esp_err_t save_config_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");

    char buf[256];
    int len = httpd_req_recv(req, buf, sizeof(buf));
    buf[len] = 0;

    router_config_t cfg;
    cJSON *json = cJSON_Parse(buf);
    
    strcpy(cfg.sta_ssid, cJSON_GetObjectItem(json, "sta_ssid")->valuestring);
    strcpy(cfg.sta_pass, cJSON_GetObjectItem(json, "sta_pass")->valuestring);
    save_config(&cfg);

    cJSON_Delete(json);
    httpd_resp_sendstr(req, "OK");

    esp_restart();
    return ESP_OK;
}

static httpd_uri_t save_config_uri = {
            .uri       = "/save",
            .method    = HTTP_POST,
            .handler   = save_config_handler,
            .user_ctx  = NULL
};


static esp_err_t favicon_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "image/x-icon");

    httpd_resp_send(
        req,
        (const char*) _binary_favicon_ico_start,
        _binary_favicon_ico_end - _binary_favicon_ico_start
    );

    return ESP_OK;
}

static httpd_uri_t favicon_uri = {
            .uri       = "/favicon.ico",
            .method    = HTTP_GET,
            .handler   = favicon_handler,
            .user_ctx  = NULL
};

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    

    httpd_resp_send(
        req,
        (const char *)_binary_index_html_start,
        _binary_index_html_end - _binary_index_html_start
    );

    return ESP_OK;
}


static httpd_uri_t root_uri = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = index_handler,
            .user_ctx  = NULL
};





httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t server_cfg = HTTPD_DEFAULT_CONFIG();    

    #if CONFIG_IDF_TARGET_LINUX
    // Setting port as 8001 when building for Linux. Port 80 can be used only by a privileged user in linux.
    // So when a unprivileged user tries to run the application, it throws bind error and the server is not started.
    // Port 8001 can be used by an unprivileged user as well. So the application will not throw bind error and the
    // server will be started.
    config.server_port = 8001;
    #endif // !CONFIG_IDF_TARGET_LINUX

    server_cfg.lru_purge_enable = true;

    ESP_ERROR_CHECK(httpd_start(&server, &server_cfg));

    httpd_register_uri_handler(server, &root_uri);
    httpd_register_uri_handler(server, &favicon_uri);
     httpd_register_uri_handler(server, &load_config_uri);
    httpd_register_uri_handler(server, &save_config_uri);

    return server;
}