#ifndef SCALE_CONTROLLER_H
#define SCALE_CONTROLLER_H

#include <Arduino.h>

#include <ScaleService.h>
#include <CommandRouter.h>

#include "PowerController.h"
#include "BLEController.h"
#include "TimeController.h"
#include "SettingsController.h"
#include "SleepController.h"

class ScaleController
{
public:
    ScaleController();

    void begin();
    void update();

    bool handleCommand(const String &command);
    bool handleScaleCommand(const String &command);

    void sendMessage(const String &message);

    void enterPowerSave(uint64_t sleepSeconds);

    bool sendWeightAndWaitAck(
    uint32_t timeoutMs = 5000);

    void sendWeightAndSleep(uint64_t sleepSeconds);

private:
    ScaleService _scale;
    CommandRouter _router;

    PowerController _power;
    BLEController _ble;
    TimeController _time;
    SettingsController _settings;
    SleepController _sleep;

    String _serialBuffer;

    void processSerial();
};

#endif