#include "ScaleController.h"

#include <esp_system.h>
#include <Esp32DownloadMode.h>

static constexpr uint8_t HX711_DOUT_PIN = 6;
static constexpr uint8_t HX711_SCK_PIN  = 7;

static ScaleController *gController = nullptr;

static void scaleMessage(
    const String &message)
{
    if (gController != nullptr)
    {
        gController->sendMessage(message);
    }
}

static bool handleBootCommand(
    const String &command)
{
    if (gController == nullptr)
        return false;

    String upper = command;

    upper.trim();
    upper.toUpperCase();

    if (upper == "BOOT" ||
        upper == "B")
    {
        gController->sendMessage(
            "Entering UART Download Mode...");

        Serial.flush();

        esp32EnterDownloadMode();

        return true;
    }

    if (upper == "RUN" ||
        upper == "R")
    {
        gController->sendMessage(
            "Rebooting normally...");

        Serial.flush();

        delay(100);

        esp_restart();

        return true;
    }

    return false;
}

static bool routeSleepCommand(
    const String &command)
{
    if (gController == nullptr)
        return false;

    String cmd = command;
    cmd.trim();
    cmd.toUpperCase();

    if (!cmd.startsWith("SLEEP"))
        return false;

    uint64_t seconds = 3600;

    if (cmd.length() > 5)
    {
        String value =
            cmd.substring(5);

        value.trim();

        if (value.length() > 0)
        {
            seconds =
                strtoull(
                    value.c_str(),
                    nullptr,
                    10);
        }
    }

    if (seconds == 0)
    {
        gController->sendMessage(
            "Invalid sleep time");

        return true;
    }

    gController->sendMessage(
        "Sleep command accepted");

    gController->enterPowerSave(seconds);

    return true;
}

static bool routeScaleCommand(
    const String &command)
{
    if (gController == nullptr)
        return false;

    return gController->handleScaleCommand(command);
}

ScaleController::ScaleController()
    : _scale("hx711cal", scaleMessage)
{
}

void ScaleController::begin()
{
    gController = this;

    Serial.begin(115200);

    _power.begin();

    _time.begin();

    _settings.begin();

    _sleep.begin();

    delay(1000);

    Serial.println();
    Serial.println(
        "ESP32-C3 Command Monitor");

    Serial.println(
        "Refactored v5");

    Serial.println(
        "GPIO10 HIGH");

    Serial.println(
        "BOOT / b : UART download mode");

    Serial.println(
        "RUN  / r : normal reboot");

    Serial.println(
        "TARE / t : zero scale");

    Serial.println(
        "CAL BEGIN / CAL ADD <g> / CAL DONE / CAL LIST");

    Serial.println(
        "BLE OTA : BLEOTA WebApp");

    _router.addHandler(
        handleBootCommand);

    _router.addHandler(
        routeSleepCommand);

    _router.addHandler(
        routeScaleCommand);

    _ble.setCommandCallback(
        [](const String &command) -> bool
        {
            return gController->handleCommand(
                command);
        });

    _ble.begin();

    _scale.begin(
        HX711_DOUT_PIN,
        HX711_SCK_PIN);

    _scale.setPrintInterval(500);

    Serial.println("System ready.");
}

void ScaleController::update()
{
    processSerial();

    _ble.update();

    if (!_ble.isOtaRunning())
    {
        _scale.update();
    }
}

bool ScaleController::handleCommand(
    const String &command)
{
    String cmd = command;

    cmd.trim();

    if (cmd.length() == 0)
        return true;

    _ble.print("Received: ");
    _ble.println(cmd);

    if (_router.dispatch(cmd))
        return true;

    _ble.println("Unknown command");
    _ble.println("Available commands:");
    _ble.println("  BOOT / b");
    _ble.println("  RUN  / r");
    _ble.println("  TARE / t");
    _ble.println("  CAL BEGIN");
    _ble.println("  CAL ADD <grams>");
    _ble.println("  CAL DONE");
    _ble.println("  CAL LIST");
    _ble.println(
        "  BLE OTA - use the BLEOTA WebApp");

    return false;
}

bool ScaleController::handleScaleCommand(
    const String &command)
{
    return _scale.handleCommand(command);
}

void ScaleController::sendMessage(
    const String &message)
{
    _ble.println(message);
}

void ScaleController::processSerial()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            _serialBuffer.trim();

            if (_serialBuffer.length() > 0)
            {
                handleCommand(
                    _serialBuffer);
            }

            _serialBuffer = "";
        }
        else
        {
            _serialBuffer += c;
        }
    }
}

void ScaleController::enterPowerSave(
    uint64_t sleepSeconds)
{
    _sleep.prepare();

    _ble.println(
        "Entering power save mode");

    _ble.stop();

    delay(50);

    _power.disable4V5();

    delay(50);

    _sleep.sleep(sleepSeconds);
}

bool ScaleController::sendWeightAndWaitAck(uint32_t timeoutMs)
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

void ScaleController::sendWeightAndSleep(
    uint64_t sleepSeconds)
{
    if (sendWeightAndWaitAck())
    {
        sendMessage("Transmission complete");

        delay(100);

        enterPowerSave(sleepSeconds);
    }
    else
    {
        sendMessage(
            "Transmission failed. Stay awake.");
    }
}