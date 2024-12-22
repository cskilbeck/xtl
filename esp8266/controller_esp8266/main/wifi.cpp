//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cstddef>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/hw_timer.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "jsmn.h"

#include "util.h"
#include "json.h"
#include "debug.h"
#include "websocket.h"
#include "settings.h"

//////////////////////////////////////////////////////////////////////

namespace
{
    const char *TAG = "wifi_client";

    EventGroupHandle_t wifi_event_group;

    const int CONNECTED_BIT = BIT0;
    const int ESPTOUCH_DONE_BIT = BIT1;

    websocket::client client;

    void smartconfig_task(void *parm);
    void websocket_client_task(void *);

    char const *websocket_server_ip = "wifi_lights.skilbeck.com";
    char const *websock_path = "";
    char const *websocket_server_port = "5000";

    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ESP_LOGI(TAG, "Event Handler!");
        if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            wifi_config_t wifi_config;
            memset(&wifi_config, 0, sizeof(wifi_config));
            esp_err_t r = esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_config);
            if(r == ESP_OK && wifi_config.sta.ssid[0] != 0 && wifi_config.sta.password[0] != 0) {
                ESP_LOGI(TAG, "GOT CONFIG: ssid: %s, password: %s", wifi_config.sta.ssid, wifi_config.sta.password);
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
                ESP_ERROR_CHECK(esp_wifi_connect());
                xTaskCreate(websocket_client_task, "websocket_client_task", 2048, NULL, 1, NULL);
            } else {
                ESP_LOGI(TAG, "NO CONFIG found: %d:%s", r, esp_err_to_name(r));
                xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
            }
        } else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        } else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            ESP_LOGI(TAG, "Got IP");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        } else if(event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
            ESP_LOGI(TAG, "Scan done");
        } else if(event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
            ESP_LOGI(TAG, "Found channel");
        } else if(event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
            ESP_LOGI(TAG, "Got new SSID and password");

            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            wifi_config_t wifi_config;
            uint8_t ssid[33] = { 0 };
            uint8_t password[65] = { 0 };
            uint8_t rvd_data[33] = { 0 };

            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set = evt->bssid_set;

            if(wifi_config.sta.bssid_set == true) {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }

            memcpy(ssid, evt->ssid, sizeof(evt->ssid));
            memcpy(password, evt->password, sizeof(evt->password));
            ESP_LOGI(TAG, "SSID:%s", ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", password);
            if(evt->type == SC_TYPE_ESPTOUCH_V2) {
                ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
                ESP_LOGI(TAG, "RVD_DATA:%s", rvd_data);
            }

            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else if(event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
            ESP_LOGI(TAG, "ESPTOUCH_DONE");
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
        }
    }

    //////////////////////////////////////////////////////////////////////

    void websocket_client_task(void *)
    {
        static char const *TAG = "websocket";

        debug_set_color(debug_color::green);

        settings_t::init();

        client.on_message = [](byte const *msg, int n) {
            ESP_LOGI(TAG, "RECEIVED %d", n);
            settings_t::load(reinterpret_cast<char const *>(msg), n);
        };

        while(true) {
            vTaskDelay(500);
            int frames = 0;
            if(client.open(websocket_server_ip, websocket_server_port, websock_path) >= 0) {
                char const data[] = R"JJ( { "device_id": "123" } )JJ";
                client.send_string(data, countof(data));
                while(client.connected()) {
                    client.update();
                    vTaskDelay(10);
                    frames += 1;
                    if((frames % 600) == 0) {
                        ESP_LOGI(TAG, "websocket is connected (%d)", frames);
                    }
                }
                ESP_LOGW(TAG, "Client disconnected");
            } else {
                ESP_LOGW(TAG, "Connect failed!");
            }
        }
    }

    //////////////////////////////////////////////////////////////////////

    void smartconfig_task(void *parm)
    {
        ESP_LOGI(TAG, "smartconfig_task!");
        EventBits_t uxBits;
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
        while(1) {
            uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
            if(uxBits & CONNECTED_BIT) {
                ESP_LOGI(TAG, "WiFi Connected to ap");
            }
            if(uxBits & ESPTOUCH_DONE_BIT) {
                ESP_LOGI(TAG, "smartconfig over");
                esp_smartconfig_stop();
                xTaskCreate(websocket_client_task, "websocket_client_task", 2048, NULL, 1, NULL);
                vTaskDelete(NULL);
            }
        }
    }

}    // namespace

//////////////////////////////////////////////////////////////////////

void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    wifi_config_t saved;
    esp_wifi_get_config(WIFI_IF_STA, &saved);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, (esp_event_handler_t)event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, (esp_event_handler_t)event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, (esp_event_handler_t)event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

__attribute((noreturn)) void wifi_reset()
{
    esp_wifi_stop();
    nvs_flash_erase();
    esp_restart();
}