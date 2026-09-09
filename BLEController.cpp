#include "BLEController.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEOTA.h>

#define BLE_DEVICE_NAME "ESP32-Scale"

#define SERVICE_UUID \
    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

#define CHARACTERISTIC_UUID_RX \
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define CHARACTERISTIC_UUID_TX \
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

static BLEServer *pServer = nullptr;
static BLECharacteristic *pTxCharacteristic = nullptr;
static BLECharacteristic *pRxCharacteristic = nullptr;

static bool bleDeviceConnected = false;
static String bleRxBuffer;

static BLEController *gController = nullptr;

static BLEOTAClass BLEOTA;

class ServerCallbacks : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *server) override
    {
        bleDeviceConnected = true;
        Serial.println("BLE client connected");
    }

    void onDisconnect(BLEServer *server) override
    {
        bleDeviceConnected = false;

        Serial.println(
            "BLE client disconnected. Restart advertising.");

        BLEDevice::startAdvertising();
    }
};

class RxCallbacks : public BLECharacteristicCallbacks
{
public:
    void onWrite(BLECharacteristic *characteristic) override
    {
        String value = characteristic->getValue();

        for (char c : value)
        {
            if (c == '\r')
                continue;

            if (c == '\n')
            {
                bleRxBuffer.trim();

                if (bleRxBuffer.length() > 0 &&
                    gController != nullptr)
                {
                    gController->receiveCommand(
                        bleRxBuffer);
                }

                bleRxBuffer = "";
            }
            else
            {
                bleRxBuffer += c;
            }
        }
    }
};

void BLEController::begin()
{
    gController = this;

    BLEDevice::init(BLE_DEVICE_NAME);

    BLEDevice::setMTU(517);

    pServer = BLEDevice::createServer();

    pServer->setCallbacks(
        new ServerCallbacks());

    BLEService *pService =
        pServer->createService(SERVICE_UUID);

    pTxCharacteristic =
        pService->createCharacteristic(
            CHARACTERISTIC_UUID_TX,
            BLECharacteristic::PROPERTY_NOTIFY);

    pRxCharacteristic =
        pService->createCharacteristic(
            CHARACTERISTIC_UUID_RX,
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR);

    pRxCharacteristic->setCallbacks(
        new RxCallbacks());

    pService->start();

    BLEOTA.begin(pServer);
    BLEOTA.setModel("ESP32-Scale");
    BLEOTA.setFWVersion("1.0.0");
    BLEOTA.init();

    BLEAdvertising *pAdvertising =
        BLEDevice::getAdvertising();

    pAdvertising->addServiceUUID(
        BLEUUID(SERVICE_UUID));

    pAdvertising->addServiceUUID(
        BLEUUID((uint16_t)0x8018));

    pAdvertising->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.print(
        "BLE advertising started as: ");

    Serial.println(BLE_DEVICE_NAME);
}

void BLEController::update()
{
    BLEOTA.process();
}

void BLEController::notify(
    const String &message)
{
    if (!bleDeviceConnected ||
        pTxCharacteristic == nullptr ||
        message.length() == 0)
    {
        return;
    }

    const size_t chunkSize = 180;

    for (size_t i = 0;
         i < message.length();
         i += chunkSize)
    {
        size_t end =
            (i + chunkSize < message.length())
                ? i + chunkSize
                : message.length();

        String chunk =
            message.substring(i, end);

        pTxCharacteristic->setValue(
            (uint8_t *)chunk.c_str(),
            chunk.length());

        pTxCharacteristic->notify();

        delay(5);
    }
}

void BLEController::print(
    const String &message)
{
    Serial.print(message);
    notify(message);
}

void BLEController::println(
    const String &message)
{
    Serial.println(message);
    notify(message + "\n");
}

void BLEController::setCommandCallback(
    CommandCallback callback)
{
    _commandCallback = callback;
}

void BLEController::receiveCommand(
    const String &command)
{
    if (command.equalsIgnoreCase("ACK"))
    {
        _ackReceived = true;
    }

    if (_commandCallback != nullptr)
    {
        _commandCallback(command);
    }
}

bool BLEController::isConnected() const
{
    return bleDeviceConnected;
}

bool BLEController::isOtaRunning() const
{
    return BLEOTA.isRunning();
}

void BLEController::clearAck()
{
    _ackReceived = false;
}

bool BLEController::isAckReceived() const
{
    return _ackReceived;
}

void BLEController::stop()
{
    BLEDevice::stopAdvertising();

    if (pServer != nullptr)
    {
        pServer->disconnect(
            pServer->getConnId());
    }

    bleDeviceConnected = false;
}

bool ScaleController::sendWeightAndWaitAck(
    uint32_t timeoutMs)
{
    double weight;

    if (!_ble.isConnected())
    {
        sendMessage("BLE not connected");
        return false;
    }

    if (!_scale.getWeight(weight))
    {
        sendMessage("Weight measurement failed");
        return false;
    }

    _ble.clearAck();

    String message = "WEIGHT:";
    message += String(weight, 2);

    _ble.println(message);

    uint32_t start = millis();

    while (millis() - start < timeoutMs)
    {
        _ble.update();

        if (_ble.isAckReceived())
        {
            sendMessage("ACK received");
            return true;
        }

        delay(10);
    }

    sendMessage("ACK timeout");

    return false;
}