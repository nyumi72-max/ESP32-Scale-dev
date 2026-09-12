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
    uint32_t timeoutMs = 5000,
    uint8_t retryCount = 2);

    void sendWeightAndSleep(uint64_t sleepSeconds);

    void setDeviceId(uint32_t id);
uint32_t getDeviceId() const;

    void setSequence(uint32_t sequence);
uint32_t getSequence() const;

bool setUnixTime(uint32_t unixTime);
uint32_t getUnixTime() const;

bool isTimeValid() const;

void sleepUntilNextSchedule();

bool wasTimerWakeup() const;

void setScheduleEnabled(bool enabled);
bool isScheduleEnabled() const;

void setMeasurementEnabled(bool enabled);
bool isMeasurementEnabled() const;

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

    void handleWakeup();

    void processScheduledMeasurement();

    bool _measurementEnabled = true;

};

#endif