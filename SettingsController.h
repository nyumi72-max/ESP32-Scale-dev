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

private:
    Preferences _preferences;

    uint32_t _deviceId = 0;
    uint32_t _sequence = 0;

    static constexpr const char *NVS_NAMESPACE = "scale";
};

    bool _scheduleEnabled = true;

#endif