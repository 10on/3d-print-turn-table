#include "ble.h"

#include <NimBLEDevice.h>

namespace ble {

static constexpr const char* NUS_SERVICE = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static constexpr const char* NUS_RX      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static constexpr const char* NUS_TX      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
static constexpr const char* TT_SERVICE  = "7ab1e101-0000-4000-8000-00000000cafe";
static constexpr const char* TT_COMMAND  = "7ab1e002-0000-4000-8000-00000000cafe";
static constexpr const char* TT_SPEED    = "7ab1e003-0000-4000-8000-00000000cafe";
static constexpr const char* TT_STATUS   = "7ab1e004-0000-4000-8000-00000000cafe";
static constexpr size_t LINE_LEN = 64;

struct Line {
    char text[LINE_LEN];
};

static NimBLECharacteristic* txChar = nullptr;
static NimBLECharacteristic* statusChar = nullptr;
static QueueHandle_t rxQueue = nullptr;
static volatile bool isConnected = false;
static volatile uint16_t mtu = 23;

static void enqueue(const char* text) {
    Line l;
    strlcpy(l.text, text, sizeof(l.text));
    xQueueSend(rxQueue, &l, 0);
}

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
        isConnected = true;
        mtu = info.getMTU();
        Serial.printf("ble: connected %s\n", info.getAddress().toString().c_str());
    }

    // reason: 0x208 supervision timeout (link lost), 0x213 peer terminated,
    // 0x216 we terminated, 0x23E failed to establish
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
        Serial.printf("ble: disconnected, reason 0x%03X\n", reason);
        isConnected = false;
        mtu = 23;
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t newMtu, NimBLEConnInfo&) override { mtu = newMtu; }
};

// Runs in the NimBLE host task: only split into lines and queue them,
// motor calls happen in loop()
class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
        static Line pending;
        static size_t len = 0;

        const std::string value = c->getValue();
        for (char ch : value) {
            if (ch == '\n' || ch == '\r') {
                if (len == 0) continue;
                pending.text[len] = '\0';
                xQueueSend(rxQueue, &pending, 0);
                len = 0;
            } else if (len < LINE_LEN - 1) {
                pending.text[len++] = ch;
            }
        }
        // Most phone apps send one command per write without a newline
        if (len > 0) {
            pending.text[len] = '\0';
            xQueueSend(rxQueue, &pending, 0);
            len = 0;
        }
    }
};

class TurntableCommandCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
        const std::string value = c->getValue();
        if (value.empty()) return;
        switch (value[0]) {
            case 'F': enqueue("f"); break;
            case 'B': enqueue("b"); break;
            case 'S': enqueue("x"); break;
            case 'R': enqueue("c"); break;
            // Service menu of the remote
            case 'O': enqueue("w"); break;
            case 'o': enqueue("w 0"); break;
            case 'T': enqueue("t"); break;
            case 'Z': enqueue("z"); break;
        }
    }
};

class TurntableSpeedCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
        const std::string value = c->getValue();
        if (value.empty()) return;
        char line[8];
        snprintf(line, sizeof(line), "p %u", static_cast<uint8_t>(value[0]));
        enqueue(line);
    }
};

void begin(const char* name) {
    rxQueue = xQueueCreate(8, sizeof(Line));

    NimBLEDevice::init(name);
    NimBLEDevice::setMTU(185);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* service = server->createService(NUS_SERVICE);
    txChar = service->createCharacteristic(NUS_TX, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* rx =
        service->createCharacteristic(NUS_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rx->setCallbacks(new RxCallbacks());

    NimBLEService* tt = server->createService(TT_SERVICE);
    tt->createCharacteristic(TT_COMMAND, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR)
        ->setCallbacks(new TurntableCommandCallbacks());
    tt->createCharacteristic(TT_SPEED, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR)
        ->setCallbacks(new TurntableSpeedCallbacks());
    statusChar = tt->createCharacteristic(TT_STATUS, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);

    // Only one 128-bit UUID fits the primary packet; the remote matches on
    // the Turntable service, the name goes to the scan response
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setName(name);
    adv->addServiceUUID(TT_SERVICE);
    adv->enableScanResponse(true);
    adv->start();
}

bool connected() { return isConnected; }

bool poll(String& line) {
    Line l;
    if (!rxQueue || xQueueReceive(rxQueue, &l, 0) != pdTRUE) return false;
    line = l.text;
    return true;
}

void send(const char* text) {
    if (!isConnected || !txChar) return;

    const size_t chunk = mtu - 3;
    size_t len = strlen(text);
    while (len > 0) {
        const size_t n = len < chunk ? len : chunk;
        txChar->setValue(reinterpret_cast<const uint8_t*>(text), n);
        txChar->notify();
        text += n;
        len -= n;
    }
}

void setStatus(const uint8_t* data, size_t len) {
    if (!statusChar) return;
    statusChar->setValue(data, len);
    if (isConnected) statusChar->notify();
}

}  // namespace ble
