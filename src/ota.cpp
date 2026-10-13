#include "ota.h"

#include <ESPmDNS.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

#include "commands.h"
#include "motor.h"
#include "secrets.h"

namespace ota {

constexpr uint32_t CONNECT_TIMEOUT_MS = 25000;
constexpr uint32_t IDLE_TIMEOUT_MS = 5 * 60 * 1000;

static State st = State::Off;
static uint32_t since = 0;
static WebServer* http = nullptr;
static bool uploading = false;

void start() {
    if (st != State::Off) return;
    motor::stop();
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    // SuperMini antenna is badly matched; full TX power often fails to associate
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    st = State::Connecting;
    since = millis();
    commands::reply("ota: connecting to %s\n", WIFI_SSID);
}

void stop() {
    if (st == State::Off) return;
    if (http) {
        http->stop();
        delete http;
        http = nullptr;
    }
    MDNS.end();
    uploading = false;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    st = State::Off;
    commands::reply("ota: off\n");
}

// HTTP upload, same as the remote's OTA: with BLE on, WiFi is forced into modem
// sleep and espota's UDP invitation gets lost, TCP just retransmits.
// curl -F firmware=@firmware.bin http://mid-table.local/update
static void beginOta() {
    http = new WebServer(80);
    http->on("/", HTTP_GET, []() {
        http->send(200, "text/html",
                   "<!doctype html><html><body><h3>mid_table</h3>"
                   "<form method='POST' action='/update' enctype='multipart/form-data'>"
                   "<input type='file' name='firmware'><input type='submit' value='Update'>"
                   "</form></body></html>");
    });
    http->on(
        "/update", HTTP_POST,
        []() {
            const bool ok = !Update.hasError();
            http->send(200, "text/plain", ok ? "OK, rebooting\n" : "Update failed\n");
            Serial.println(ok ? "OTA: done" : "OTA: failed");
            if (ok) {
                delay(300);
                ESP.restart();
            }
            uploading = false;
        },
        []() {
            HTTPUpload& up = http->upload();
            since = millis();
            if (up.status == UPLOAD_FILE_START) {
                motor::emergencyStop();
                uploading = true;
                Serial.printf("OTA: start %s\n", up.filename.c_str());
                Update.begin(UPDATE_SIZE_UNKNOWN);
            } else if (up.status == UPLOAD_FILE_WRITE) {
                Update.write(up.buf, up.currentSize);
            } else if (up.status == UPLOAD_FILE_END) {
                Update.end(true);
            } else if (up.status == UPLOAD_FILE_ABORTED) {
                Update.abort();
                uploading = false;
            }
        });
    http->begin();
    MDNS.begin(OTA_HOSTNAME);
    MDNS.addService("http", "tcp", 80);
}

void update() {
    switch (st) {
        case State::Off:
            break;
        case State::Connecting:
            if (WiFi.status() == WL_CONNECTED) {
                beginOta();
                st = State::Ready;
                since = millis();
                commands::reply("ota: ready %s.local %s\n", OTA_HOSTNAME,
                                WiFi.localIP().toString().c_str());
            } else if (millis() - since > CONNECT_TIMEOUT_MS) {
                commands::reply("ota: wifi failed\n");
                stop();
            }
            break;
        case State::Ready:
            http->handleClient();
            if (WiFi.status() != WL_CONNECTED || millis() - since > IDLE_TIMEOUT_MS) stop();
            break;
    }
}

State state() { return st; }
bool active() { return st != State::Off; }
bool uploadInProgress() { return uploading; }
uint32_t ip() { return st == State::Ready ? static_cast<uint32_t>(WiFi.localIP()) : 0; }

}  // namespace ota
