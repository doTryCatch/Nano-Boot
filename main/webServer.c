
#include "webServer.h"
#include "esp_log.h"
#include "ledControl.h"
#include "esp_http_server.h"
#include <string.h>

static const char *TAG = "WEB_SERVER";

esp_err_t handle_root(httpd_req_t *req) {
    const char* html = "<h1>ESP32 Home Automation</h1>"
                       "<p><a href=\"/on\">Turn ON</a></p>"
                       "<p><a href=\"/off\">Turn OFF</a></p>";
    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handle_on(httpd_req_t *req) {
    led_on();
    httpd_resp_send(req, "<p>Device ON</p><a href='/'>Back</a>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t handle_off(httpd_req_t *req) {
    led_off();
    httpd_resp_send(req, "<p>Device OFF</p><a href='/'>Back</a>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = handle_root };
        httpd_register_uri_handler(server, &root);

        httpd_uri_t on = { .uri = "/on", .method = HTTP_GET, .handler = handle_on };
        httpd_register_uri_handler(server, &on);

        httpd_uri_t off = { .uri = "/off", .method = HTTP_GET, .handler = handle_off };
        httpd_register_uri_handler(server, &off);
    }
    return server;
}
