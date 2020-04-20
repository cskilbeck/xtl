//////////////////////////////////////////////////////////////////////

extern "C" {

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_wifi_default.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "esp_websocket_client.h"
}
#include "util.h"
#include "debug.h"
#include "wifi.h"
#include "settings.h"

//////////////////////////////////////////////////////////////////////

static const char *TAG = "WIFI";

static EventGroupHandle_t s_wifi_event_group;
static EventGroupHandle_t s_websocket_event_group;

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;

static const int WS_CONNECTED_BIT = BIT0;
static const int WS_DISCONNECTED_BIT = BIT1;

static const char *WEBSOCKET_ENDPOINT = "ws://52.200.210.232:5000";

static esp_websocket_client_handle_t client = null;

static void wifi_task(void *parm);

enum class opcode : byte
{
    continuation = 0,
    text = 1,
    binary = 2,
    connection = 8,
    ping = 9,
    pong = 10,
};

//////////////////////////////////////////////////////////////////////

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    char const *TAG = "WIFI_EVENT";

    if(event_base == WIFI_EVENT) {

        switch(event_id) {

        case WIFI_EVENT_STA_START:
            xTaskCreatePinnedToCore(wifi_task, "wifi_task", 4096, null, 3, null, wifi_core);    // my wifi task on same core as wifi driver
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
            xEventGroupSetBits(s_websocket_event_group, WS_DISCONNECTED_BIT);
            esp_wifi_connect();
            xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
            debug_led.set(debug_led.blink, debug_led.red);
            break;
        }
    }

    else if(event_base == IP_EVENT) {

        switch(event_id) {

        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
            xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
            break;
        }
    }

    else if(event_base == SC_EVENT) {

        switch(event_id) {

        case SC_EVENT_SCAN_DONE:
            debug_led.set(debug_led.solid, debug_led.yellow);
            ESP_LOGI(TAG, "Scan done");
            break;

        case SC_EVENT_FOUND_CHANNEL:
            debug_led.set(debug_led.solid, debug_led.cyan);
            ESP_LOGI(TAG, "Found channel");
            break;

        case SC_EVENT_GOT_SSID_PSWD: {
            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
            wifi_config_t wifi_config;
            bzero(&wifi_config, sizeof(wifi_config_t));
            memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
            memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
            wifi_config.sta.bssid_set = evt->bssid_set;
            if(wifi_config.sta.bssid_set) {
                memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
            }
            ESP_LOGI(TAG, "SmartConfig: SSID:%s, PASSWORD:%s", evt->ssid, evt->password);
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
            debug_led.set(debug_led.solid, debug_led.white);
        } break;

        case SC_EVENT_SEND_ACK_DONE:
            xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////

void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    char const *TAG = "WEBSOCKET";

    // esp_websocket_client_handle_t client = (esp_websocket_client_handle_t)handler_args;
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch(event_id) {

    case WEBSOCKET_EVENT_CONNECTED: {
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        char const data[] = "{ \"device_id\": \"123\" }";
        int len = countof(data) - 1;
        if(esp_websocket_client_send(client, data, len, portMAX_DELAY) != len) {
            ESP_LOGW(TAG, "HUH!? ws_send returned %d", len);
        }
        debug_led.set(debug_led.solid, debug_led.white);
    } break;

    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        debug_led.set(debug_led.pulse, debug_led.blue);
        debug_led.set_for(500, debug_led.flash, debug_led.red);
        break;

    case WEBSOCKET_EVENT_DATA: {
        ESP_LOGV(TAG, "WEBSOCKET_EVENT_DATA");
        int len = data->data_len;
        char *msg = (char *)data->data_ptr;
        if(len > 0 && msg[len - 1] == '\n') {
            len -= 1;
        }
        opcode o = (opcode)(data->op_code);
        ESP_LOGV(TAG, "OPCODE: %d", (int)o);
        if(o == opcode::text) {
            settings_t::load(msg, len);
            debug_led.set_for(250, debug_led.flash, debug_led.yellow);
        }
    } break;

    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        debug_led.set_for(500, debug_led.flash, debug_led.red);
        break;
    }
}

/////////////////////////////////////////////////////////////////////

static void wifi_task(void *parm)
{
    ESP_LOGI(TAG, "WIFI_TASK starts");

    EventBits_t uxBits;

    wifi_config_t conf;

    // if SSID/PASSWORD are saved in eeprom, try that
    if(esp_wifi_get_config(ESP_IF_WIFI_STA, &conf) == ESP_OK && conf.sta.ssid[0] != 0 && conf.sta.password != 0) {

        ESP_LOGI(TAG, "got stored credentials: ssid %s, password %s", conf.sta.ssid, conf.sta.password);
        ESP_ERROR_CHECK(esp_wifi_connect());

    } else {

        // otherwise kick off smartconfig
        ESP_LOGI(TAG, "no stored credentials, smartconfig begins");
        ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
        smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    }

    // then wait for events (wifi got connected or smartconfig completed)
    while(1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {

            ESP_LOGI(TAG, "Connected");
            debug_led.set(debug_led.pulse, debug_led.blue);
            xEventGroupSetBits(s_websocket_event_group, WS_CONNECTED_BIT);
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {

            ESP_LOGI(TAG, "SmartConfig complete");
            esp_smartconfig_stop();
        }
    }
}

//////////////////////////////////////////////////////////////////////

static void websocket_task(void *)
{
    char const *TAG = "WEBSOCKET_TASK";

    ESP_LOGI(TAG, "Task starts");

    debug_led.set(debug_led.blink, debug_led.magenta);

    bool got_credentials = false;
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif != null);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = 1;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, null));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, null));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, null));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "esp_wifi_start called...");

    while(true) {
        EventBits_t uxBits = xEventGroupWaitBits(s_websocket_event_group, WS_CONNECTED_BIT | WS_DISCONNECTED_BIT, true, false, portMAX_DELAY);

        if(uxBits & WS_CONNECTED_BIT) {

            if(client != null) {
                ESP_LOGI(TAG, "WS_CONNECTED_BIT!");
                esp_websocket_client_stop(client);
                esp_websocket_client_destroy(client);
                client = null;
            }

            ESP_LOGI(TAG, "Connecting to %s...", WEBSOCKET_ENDPOINT);
            const esp_websocket_client_config_t websocket_cfg = { .uri = WEBSOCKET_ENDPOINT };
            client = esp_websocket_client_init(&websocket_cfg);
            esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
            esp_websocket_client_start(client);
        } else if(uxBits & WS_DISCONNECTED_BIT) {
            ESP_LOGI(TAG, "WS_DISCONNECTED_BIT!");
        }
    }
}

//////////////////////////////////////////////////////////////////////

void initialise_wifi()
{
    ESP_LOGI(TAG, "initialise_wifi");
    nvs_flash_init();

    s_websocket_event_group = xEventGroupCreate();
    xTaskCreatePinnedToCore(websocket_task, "websocket_task", 4096, null, 3, null, wifi_core);    // websocket task on same core as wifi
}
