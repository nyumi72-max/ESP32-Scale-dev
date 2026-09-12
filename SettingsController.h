#ifndef SETTINGS_CONTROLLER_H
#define SETTINGS_CONTROLLER_H

#include <Arduino.h>
#include <Preferences.h>

class SettingsController
{
public:
    void begin();

    void setDeviceId(uint32_t id);
    uint32_t getDeviceId() const;

    void setSequence(uint32_t sequence);
    uint32_t getSequence() const;

    void save();
    void load();

    void setScheduleEnabled(bool enabled);
    bool isScheduleEnabled() const;

    void setMeasurementEnabled(bool enabled);
    bool isMeasurementEnabled() const;

private:
    Preferences _preferences;

    uint32_t _deviceId = 0;
    uint32_t _sequence = 0;

    bool _scheduleEnabled = true;
    bool _measurementEnabled = true;

    static constexpr const char *NVS_NAMESPACE = "scale";
};

#endif