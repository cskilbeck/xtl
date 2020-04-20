#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClientSecureAxTLS.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
#include <WiFiServerSecureAxTLS.h>
#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>

#include <WiFiManager.h>

#include <WiFiClient.h>
#include <WiFiServer.h>

//////////////////////////////////////////////////////////////////////

#include <dummy.h>

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiManager.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
//#include <WiFiManager.h>
#include <ESPAsyncTCP.h>
#include "asyncHTTPRequest.h"
#include <FS.h>
#include <ArduinoJson.h>

#include "types.h"
#include "spi.h"
#include "message.h"

using namespace message;

//////////////////////////////////////////////////////////////////////

char const *ssid = "xmas tree lights";
// char const *mdns_hostname = "XTL_442E8CEB157A43E1ADEF24B2153CD8FA";
char const *mdns_hostname = "foo";

char const *server_api = "52.200.210.232:3000/state";

int const max_strings = 6;
int const max_color_parameters = 8;
int const max_speed_parameters = 8;
int const size_of_params = 8 + max_color_parameters * 6 + max_speed_parameters * 4;

int debug_led = 0;

//////////////////////////////////////////////////////////////////////

char const *data_filename = "led_data.";

MDNSResponder mdns;
WiFiClient wifi_client;
ESP8266WebServer web_server(80);

asyncHTTPrequest http_request;
bool http_in_flight = false;

enum
{
    disconnected = 0,
    connecting = 1,
    connected = 2
};

String ipAddress;
int connects = 0;
int wifi_status = disconnected;

spi_data_buffer spi_tx;
spi_data_buffer spi_rx;

int spi_requests = 0;
int spi_receive_errors = 0;

int blue_led = 0;

//////////////////////////////////////////////////////////////////////

int last_index = 0;
int brightness = 100;
int speed = 128;

//////////////////////////////////////////////////////////////////////

tracked_state<led_state> led_state0;
tracked_state<led_state> led_state1;
tracked_state<led_state> led_state2;
tracked_state<led_state> led_state3;
tracked_state<led_state> led_state4;
tracked_state<led_state> led_state5;
tracked_state<stm_state> stm_state0;
tracked_state<global_state> global_state0;

//////////////////////////////////////////////////////////////////////

enum
{
    num_led_channels = 6
};

static state_handler *handlers[] = { &led_state0, &led_state1, &led_state2, &led_state3, &led_state4, &led_state5, &stm_state0, &global_state0 };

//////////////////////////////////////////////////////////////////////

struct effect_name
{
    char const *name;
    int effect;
};

effect_name effect_names[] = { { "glow", 17 }, { "twinkle", 16 }, { "sparkle", 15 }, { "flash", 14 }, { "alternate", 14 }, { "pulse", 12 }, { "fade", 12 } };

//////////////////////////////////////////////////////////////////////

template <typename T, std::size_t N> constexpr std::size_t countof(T const (&)[N]) noexcept
{
    return N;
}

//////////////////////////////////////////////////////////////////////

int handle_packet(data_buffer &din, data_buffer &dout)
{
    dout.clear();
    message_header m;
    while(din.position < din.length) {
        din.take(&m);
        handlers[m.address]->handle(m, din, dout);
    }
    return dout.position;
}

//////////////////////////////////////////////////////////////////////

void on_root()
{
    String s = "The IP Address is ";
    s += ipAddress;
    s += "\nThe MAC Address is ";
    s += WiFi.macAddress();
    web_server.send(200, "text/plain", s.c_str());
}

//////////////////////////////////////////////////////////////////////

void on_not_found()
{
    web_server.send(404, "text/plain", "NOT FOUND!!!");
}

//////////////////////////////////////////////////////////////////////

bool handle_wifi()
{
    if(wifi_status != connecting) {
        if(WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi connecting...");
            wifi_status = connecting;
            WiFi.disconnect();
            WiFi.mode(WIFI_STA);
            WiFi.begin("3 Byfeld Gardens", "titwp2012");
            Serial.println("WiFi connection started...");
        }
    }

    if(wifi_status == connecting && WiFi.status() == WL_CONNECTED) {

        wifi_status = connected;
        connects += 1;

        ipAddress = WiFi.localIP().toString();

        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        Serial.println("Connected!?");
        Serial.println();

        if(connects == 1) {
            MDNS.addService("http", "tcp", 80);
            if(MDNS.begin(mdns_hostname)) {
                Serial.println("\n\nMDNS responder started\n");
            }

            web_server.on("/", on_root);
            web_server.onNotFound(on_not_found);
            web_server.begin();
            Serial.println("HTTP web_server started");
        }
        MDNS.notifyAPChange();
    }
    return wifi_status == connected;
}

//////////////////////////////////////////////////////////////////////

uint32 scale_color(uint32 color, int percent)
{
    percent = percent * 256 / 100;
    int r = (color >> 16) & 0xff;
    int g = (color >> 8) & 0xff;
    int b = (color >> 0) & 0xff;
    r = (r * percent) >> 8;
    g = (g * percent) >> 8;
    b = (b * percent) >> 8;
    return (r << 16) | (g << 8) | b;
}

//////////////////////////////////////////////////////////////////////

void set_led_string(led_state &led, uint32 color[], uint32 effect)
{
    for(int i = 0; i < 6; ++i) {
        uint32 color_low = color[i];
        uint32 color_high = color[i];
        if((color[i] & 0x80000000) != 0) {
            color_low = 0;
            color_high = 0xffffff;
        }
        led.color_ranges[i].low.set(color_low);
        led.color_ranges[i].high.set(color_high);
    }
    led.effect_index = effect;
}

//////////////////////////////////////////////////////////////////////

void http_data_arrived(void *, asyncHTTPrequest *request, size_t available)
{
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, request->responseText());
    if(error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    // Get the root object in the document
    JsonObject root = doc.as<JsonObject>();
    String status = root["status"];    // "ok"
    if(status == "ok") {
        debug_led = 2;
        int index = root["index"];
        if(index == last_index) {
        } else {
            last_index = index;
            int powerstate = root["powerstate"];    // 1
            String effect = root["effect"];         // "solid"
            JsonArray colors = root["colors"];

            // get the effect (13 is solid, and special)

            int effect_index = 13;    // solid
            for(int i = 0; i < countof(effect_names); ++i) {
                if(effect == effect_names[i].name) {
                    effect_index = effect_names[i].effect;
                    break;
                }
            }

            int num_colors = colors.size();

            uint32 color[6];

            if(num_colors > 3) {
                num_colors = 3;
            }
            if(num_colors != 0) {

                int index[3][6] = { { 0, -1, 0, -1, 0, -1 }, { 0, 1, 0, 1, 0, 1 }, { 0, 1, 2, 0, 1, 2 } };

                bool single_color = false;

                if((effect_index == 15 || effect_index == 14 || effect_index == 17) && num_colors == 1 && colors[0].as<int>() == 0x80000000) {
                    single_color = true;
                }
                if(effect_index == 13 && num_colors == 1) {
                    single_color = true;
                }

                if(single_color) {
                    index[0][1] = 0;
                    index[0][3] = 0;
                    index[0][5] = 0;
                }

                for(int i = 0; i < 6; ++i) {
                    int idx = index[num_colors - 1][i];
                    if(idx < 0) {
                        color[i] = 0;
                    } else {
                        color[i] = colors[idx].as<int>();
                    }
                }
            }

            speed = root["speed"];
            brightness = root["brightness"];

            // setup global state for sending to stm103
            if(powerstate) {
                global_state0.state.flags |= global_power;
            } else {
                global_state0.state.flags &= ~global_power;
            }
            global_state0.state.flags |= global_new_state;
            global_state0.state.brightness = brightness;
            global_state0.state.speed = speed;

            Serial.printf("\nStatus: %s, State: %d, Effect:%s (%d), brightness: %d, speed: %d, num_colors = %d, Color0:%08x, Color1:%08x, Color2: %08x\n", status.c_str(),
                          powerstate, effect.c_str(), effect_index, brightness, speed, num_colors, colors[0].as<int>(), colors[1].as<int>(), colors[2].as<int>());

            for(int i = 0; i < 6; ++i) {
                Serial.printf("%d: %08x\n", i, color[i]);
                tracked_state<led_state> *s = (tracked_state<led_state> *)handlers[i];
                set_led_string(s->state, color, effect_index);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////

void setup()
{
    Serial.begin(115200);

    pinMode(2, OUTPUT);
    pinMode(16, INPUT);    // request from stm
    SPI.setHwCs(true);
    SPI.begin();

    SPIFFS.begin();    // Start the SPI Flash File System

    handle_wifi();

    if(SPIFFS.exists("/test.txt")) {
        File file = SPIFFS.open("/test.txt", "r");
        char t[1024];
        int n = file.read((byte *)t, 1024);
        t[n] = 0;
        String s(t);
        Serial.print("STRING:");
        Serial.println(s);
        file.close();
    }

    Serial.print("Sketch size: ");
    Serial.println(ESP.getSketchSize());
    Serial.print("Free size: ");
    Serial.println(ESP.getFreeSketchSpace());

    // TODO: load these from the file system (and save them when idle for > ? seconds)
    for(int i = 0; i < num_led_channels; ++i) {
        tracked_state<led_state> *s = (tracked_state<led_state> *)handlers[i];
        s->state.string_index = i;
        s->state.effect_index = 9;
        s->state.color_ranges[0].low.r = 0;
        s->state.color_ranges[0].low.g = 0;
        s->state.color_ranges[0].low.b = 0;
        s->state.color_ranges[0].high.r = 255;
        s->state.color_ranges[0].high.g = 255;
        s->state.color_ranges[0].high.b = 255;
        s->state.speed_ranges[0].low = 48;
        s->state.speed_ranges[0].high = 104;
        s->state.speed_ranges[1].low = 48;
        s->state.speed_ranges[1].high = 104;
    }
    http_request.setDebug(false);
    http_request.onData(http_data_arrived);
    http_request.setTimeout(1);
}

//////////////////////////////////////////////////////////////////////

void handle_spi()
{
    if(digitalRead(16) == LOW) {
        spi_requests += 1;
        data_buffer spi_buffer(spi_tx.data, spi_data_buffer::packet_size);
        for(int i = 0; i < num_led_channels; ++i) {
            handlers[i]->dump(i, spi_buffer);    // all messages with their payloads get sent over SPI each frame, what comes back? some acks presumably
        }

        if(debug_led != 0) {
            global_state0.state.flags |= message::global_debug_led;
            debug_led -= 1;
        } else {
            global_state0.state.flags &= ~message::global_debug_led;
        }

        global_state0.dump(global_channel, spi_buffer);
        global_state0.state.flags &= ~global_new_state;    // TODO (chs): RACE CONDITION HERE....
        spi_tx.init(spi_buffer.position);
        SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
        SPI.transferBytes(spi_tx.transfer_buffer(), spi_rx.transfer_buffer(), 1024);
        if(!spi_rx.is_valid()) {
            spi_receive_errors += 1;
        }
    }
}

//////////////////////////////////////////////////////////////////////

int loops = 0;
int last_http_issued_at = 0;

//////////////////////////////////////////////////////////////////////

void loop(void)
{
    char stat = 'N';

    if(handle_wifi()) {

        web_server.handleClient();

        auto state = http_request.readyState();

        if(state == asyncHTTPrequest::readyStateUnsent || state == asyncHTTPrequest::readyStateDone) {
            if((millis() - last_http_issued_at) > 1500) {
                blue_led = 5000;
                loops = 0;
                last_http_issued_at = millis();
                if(http_request.open("GET", server_api)) {
                    http_request.send();
                }
            }
        }
        stat = http_request.readyState() + '0';
    }
    handle_spi();

    if(blue_led > 0) {
        blue_led -= 1;
    }
    digitalWrite(2, !blue_led);

    if(loops == 0) {
        Serial.printf(" F: %d E: %d M: %08x C: %d", spi_requests, spi_receive_errors, ESP.getFreeHeap(), connects);
    }

    if((loops & 0x7fff) == 0) {
        if((loops & 0x1ffff) == 0) {
            Serial.println();
        }
        Serial.printf("%c", stat);
    }
    loops += 1;
}
