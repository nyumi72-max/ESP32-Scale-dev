#include "SettingsController.h"

void SettingsController::begin()
{
    load();
}

void SettingsController::setDeviceId(uint32_t id)
{
    _deviceId = id;
}

uint32_t SettingsController::getDeviceId() const
{
    return _deviceId;
}

void SettingsController::setSequence(uint32_t sequence)
{
    _sequence = sequence;
}

uint32_t SettingsController::getSequence() const
{
    return _sequence;
}

void SettingsController::save()
{
    if (!_preferences.begin(NVS_NAMESPACE, false))
        return;

    _preferences.putUInt("device_id", _deviceId);
    _preferences.putUInt("sequence", _sequence);

    _preferences.end();
}

void SettingsController::load()
{
    if (!_preferences.begin(NVS_NAMESPACE, true))
        return;

    _deviceId =
        _preferences.getUInt("device_id", 0);

    _sequence =
        _preferences.getUInt("sequence", 0);

    _preferences.end();
}