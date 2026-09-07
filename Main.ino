#include <Arduino.h>
#include <esp_system.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <HX711.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEOTA.h>

#include <CommandRouter.h>
#include <Esp32DownloadMode.h>
#include <ScaleService.h>

// ================================================================
// BLEOTA: 既存の動作確認済みライブラリを変更せず使用
// ================================================================
BLEOTAClass BLEOTA;

// ================================================================
// Hardware
// ================================================================
#define HX711_DOUT_PIN 6
#define HX711_SCK_PIN  7
#define LDO_EN_PIN     10

#define BLE_DEVICE_NAME
"ESP32-Scale"
#define SERVICE_UUID
"6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX
"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX
"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic = nullptr;
BLECharacteristic *pRxCharacteristic = nullptr;

bool bleDeviceConnected = false;
String bleRxBuffer;
String rxBuffer;

// Forward declaration
void processCommand(const String &rawCmd);

// ================================================================
// 共通出力
// ================================================================
void bleNotifyRaw(const String &s)
{
    if (!bleDeviceConnected || pTxCharacteristic == nullptr || s.length() == 0)
        return;

    const size_t chunkSize = 180;

    for (size_t i = 0; i < s.length(); i += chunkSize)
    {
        size_t end = (i + chunkSize < s.length()) ? (i + chunkSize) : s.length();
        String chunk = s.substring(i, end);

        pTxCharacteristic->setValue(
            (uint8_t *)chunk.c_str(),
            chunk.length());

        pTxCharacteristic->notify();
        delay(5);
    }
}

void dprint(const String &s)
{
    Serial.print(s);
    bleNotifyRaw(s);
}

void dprintln(const String &s = "")
{
    Serial.println(s);
    bleNotifyRaw(s + "\n");
}

// ================================================================
// ScaleService
// ================================================================
void scaleMessage(const String &msg)
{
    dprintln(msg);
}

ScaleService scaleService("hx711cal", scaleMessage);

// ================================================================
// CommandRouter
// ================================================================
CommandRouter commandRouter;

bool handleBootCommand(const String &cmd)
{
    String upper = cmd;
    upper.trim();
    upper.toUpperCase();

    if (upper == "BOOT" || upper == "B")
    {
        dprintln("Entering UART Download Mode...");
        Serial.flush();
        esp32EnterDownloadMode();
        return true;
    }

    if (upper == "RUN" || upper == "R")
    {
        dprintln("Rebooting normally...");
        Serial.flush();
        delay(100);
        esp_restart();
        return true;
    }

    return false;
}

bool handleScaleCommand(const String &cmd)
{
    return scaleService.handleCommand(cmd);
}

void processCommand(const String &rawCmd)
{
    String cmd = rawCmd;
    cmd.trim();

    if (cmd.length() == 0)
        return;

    dprint("Received: ");
    dprintln(cmd);

    if (!commandRouter.dispatch(cmd))
    {
        dprintln("Unknown command");
        dprintln("Available commands:");
        dprintln("  BOOT / b");
        dprintln("  RUN  / r");
        dprintln("  TARE / t");
        dprintln("  CAL BEGIN");
        dprintln("  CAL ADD <grams>");
        dprintln("  CAL DONE");
        dprintln("  CAL LIST");
        dprintln("  BLE OTA - use the BLEOTA WebApp");
    }
}

// ================================================================
// BLE server callbacks
// ================================================================
class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer) override
    {
        bleDeviceConnected = true;
        Serial.println("BLE client connected");
    }

    void onDisconnect(BLEServer *pServer) override
    {
        bleDeviceConnected = false;
        Serial.println("BLE client disconnected. Restart advertising.");
        BLEDevice::startAdvertising();
    }
};

// ================================================================
// BLE RX callback
// ================================================================
class RxCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic) override
    {
        String value = pCharacteristic->getValue();

        for (char c : value)
        {
            if (c == '\r')
                continue;

            if (c == '\n')
            {
                bleRxBuffer.trim();

                if (bleRxBuffer.length() > 0)
                    processCommand(bleRxBuffer);

                bleRxBuffer = "";
            }
            else
            {
                bleRxBuffer += c;
            }
        }
    }
};

// ================================================================
// BLE setup
// ※ 既存の動作確認済み構成を維持。
// ※ setPreferredConnectionParams() はCore 3.3.11で使用しない。
// ================================================================
void setupBLE()
{
    BLEDevice::init(BLE_DEVICE_NAME);

    // BLEOTA WebAppのMTU probeに合わせる
    BLEDevice::setMTU(517);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY);
    
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR);

    pRxCharacteristic->setCallbacks(new RxCallbacks());

    pService->start();

    // BLEOTAライブラリは変更しない
    BLEOTA.begin(pServer);
    BLEOTA.setModel("ESP32-Scale");
    BLEOTA.setFWVersion("1.0.0");
    BLEOTA.init();

    // WebApp側の検出条件に合わせて0x8018を維持
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));
    pAdvertising->addServiceUUID(BLEUUID((uint16_t)0x8018));

    pAdvertising->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.print("BLE advertising started as: ");
    Serial.println(BLE_DEVICE_NAME);
}

// ================================================================
// setup
// ================================================================
void setup()
{
    Serial.begin(115200);

    // LDO enable
    pinMode(LDO_EN_PIN, OUTPUT);
    digitalWrite(LDO_EN_PIN, HIGH);

    delay(1000);

    // BLE初期化より前に必ずシリアルへ出す
    Serial.println();
    Serial.println("ESP32-C3 Command Monitor");
    Serial.println("Refactored v4");
    Serial.println("GPIO10 HIGH");
    Serial.println("BOOT / b : UART download mode");
    Serial.println("RUN  / r : normal reboot");
    Serial.println("TARE / t : zero scale");
    Serial.println("CAL BEGIN / CAL ADD <g> / CAL DONE / CAL LIST");
    Serial.println("BLE OTA : BLEOTA WebApp");

    // コマンド登録
    commandRouter.addHandler(handleBootCommand);
    commandRouter.addHandler(handleScaleCommand);

    // BLE
    setupBLE();

    // HX711
    scaleService.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
    scaleService.setPrintInterval(500);

    Serial.println("System ready.");
}

// ================================================================
// loop
// ================================================================
void loop()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            rxBuffer.trim();

            if (rxBuffer.length() > 0)
                processCommand(rxBuffer);

            rxBuffer = "";
        }
        else
        {
            rxBuffer += c;
        }
    }

    BLEOTA.process();

    // OTA中はScaleService側の定期出力があっても転送を邪魔しないよう停止
    if (!BLEOTA.isRunning())
        scaleService.update();
}
