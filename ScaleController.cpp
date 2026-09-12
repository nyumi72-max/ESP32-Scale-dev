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

static bool routeSequenceCommand(const String &command)
{
    if (gController == nullptr)
        return false;

    String cmd = command;
    cmd.trim();

    if (!cmd.startsWith("SEQ"))
        return false;

    String value = cmd.substring(3);
    value.trim();

    if (value.length() == 0)
    {
        gController->sendMessage(
            "SEQUENCE: " +
            String(gController->getSequence()));

        return true;
    }

    uint32_t sequence =
        strtoul(value.c_str(), nullptr, 10);

    gController->setSequence(sequence);

    gController->sendMessage(
        "SEQUENCE set: " +
        String(sequence));

    return true;
}

static bool routeTimeCommand(const String &command)
{
    if (gController == nullptr)
        return false;

    String cmd = command;
    cmd.trim();

    if (!cmd.startsWith("TIME"))
        return false;

    String value = cmd.substring(4);
    value.trim();

    if (value.length() == 0)
    {
        if (!gController->isTimeValid())
        {
            gController->sendMessage("TIME: INVALID");
            return true;
        }

        gController->sendMessage(
            "TIME: " +
            String(gController->getUnixTime()));

        return true;
    }

    uint32_t unixTime =
        strtoul(value.c_str(), nullptr, 10);

    if (!gController->setUnixTime(unixTime))
    {
        gController->sendMessage("TIME: INVALID");
        return true;
    }

    gController->sendMessage(
        "TIME set: " +
        String(gController->getUnixTime()));

    return true;
}

static bool routeScheduleCommand(const String &command)
{
    if (gController == nullptr)
        return false;

    String cmd = command;
    cmd.trim();
    cmd.toUpperCase();

    if (cmd != "SCHEDULE")
        return false;

    gController->sleepUntilNextSchedule();

    return true;
}

static bool routeDeviceCommand(const String &command)
{
    if (gController == nullptr)
        return false;

    String cmd = command;
    cmd.trim();

    if (!cmd.startsWith("DEVICE"))
        return false;

    String value = cmd.substring(6);
    value.trim();

    if (value.length() == 0)
    {
        gController->sendMessage(
            "DEVICE ID: " +
            String(gController->getDeviceId()));

        return true;
    }

    uint32_t id =
        strtoul(value.c_str(), nullptr, 10);

    gController->setDeviceId(id);

    gController->sendMessage(
        "DEVICE ID set: " + String(id));

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
        routeDeviceCommand);

    _router.addHandler(
        routeSequenceCommand);
    _router.addHandler(
        routeTimeCommand);

    _router.addHandler(
        routeScheduleCommand);

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
    _ble.println("  SLEEP [seconds]");
    _ble.println("  BLE OTA - use the BLEOTA WebApp");
    _ble.println("  SEQ [number]");
    _ble.println("  TIME [unix]");
    _ble.println("  SCHEDULE");

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

    uint32_t sequence =
        _settings.getSequence();

    sequence++;

    _settings.setSequence(sequence);
    _settings.save();

    uint32_t deviceId =
        _settings.getDeviceId();

    uint32_t unixTime = 0;

    if (_time.isValid())
    {
        unixTime =
            _time.getUnixTime();
    }

    String message = "DATA,";
    message += "ID=";
    message += String(deviceId);
    message += ",SEQ=";
    message += String(sequence);
    message += ",WEIGHT=";
    message += String(weight, 2);
    message += ",TIME=";
    message += String(unixTime);

    _ble.clearAck();

    _ble.println(message);

    uint32_t start = millis();

    while (millis() - start < timeoutMs)
    {
        _ble.update();

        if (_ble.isAckReceived(sequence))
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

void ScaleController::setDeviceId(uint32_t id)
{
    _settings.setDeviceId(id);
    _settings.save();
}

uint32_t ScaleController::getDeviceId() const
{
    return _settings.getDeviceId();
}

void ScaleController::setSequence(uint32_t sequence)
{
    _settings.setSequence(sequence);
    _settings.save();
}

uint32_t ScaleController::getSequence() const
{
    return _settings.getSequence();
}

bool ScaleController::setUnixTime(uint32_t unixTime)
{
    return _time.setUnixTime(unixTime);
}

uint32_t ScaleController::getUnixTime() const
{
    return _time.getUnixTime();
}

bool ScaleController::isTimeValid() const
{
    return _time.isValid();
}

void ScaleController::sleepUntilNextSchedule()
{
    if (!_time.isValid())
    {
        sendMessage("TIME: INVALID");
        return;
    }

    uint32_t seconds =
        _time.secondsUntilNextSchedule();

    if (seconds == 0)
    {
        sendMessage("Schedule calculation failed");
        return;
    }

    sendMessage(
        "Next wake in " +
        String(seconds) +
        " seconds");

    enterPowerSave(seconds);
}

bool ScaleController::wasTimerWakeup() const
{
    return _sleep.wasTimerWakeup();
}