#include "TimeController.h"

void TimeController::begin()
{
    _unixTime = 0;
    _valid = false;
    _baseUnixTime = 0;
    _baseMillis = 0;
}

bool TimeController::setUnixTime(uint32_t unixTime)
{
    if (unixTime < 1000000000UL)
        return false;

    _baseUnixTime = unixTime;
    _baseMillis = millis();

    _unixTime = unixTime;
    _valid = true;

    return true;
}

uint32_t TimeController::getUnixTime() const
{
    if (!_valid)
        return 0;

    uint32_t elapsed =
        (millis() - _baseMillis) / 1000UL;

    return _baseUnixTime + elapsed;
}

bool TimeController::isValid() const
{
    return _valid;
}

uint32_t TimeController::secondsUntilNextSchedule() const
{
    if (!_valid)
        return 0;

    uint32_t now = getUnixTime();

    uint32_t secondsOfDay =
        now % 86400UL;

    uint32_t targets[] = {
        3UL * 3600UL,
        8UL * 3600UL,
        9UL * 3600UL,
        10UL * 3600UL,
        11UL * 3600UL,
        12UL * 3600UL,
        13UL * 3600UL,
        14UL * 3600UL,
        15UL * 3600UL,
        16UL * 3600UL,
        17UL * 3600UL,
        18UL * 3600UL,
        19UL * 3600UL,
        20UL * 3600UL
    };

    for (uint8_t i = 0;
         i < sizeof(targets) / sizeof(targets[0]);
         i++)
    {
        if (targets[i] > secondsOfDay)
            return targets[i] - secondsOfDay;
    }

    return (86400UL - secondsOfDay) +
           targets[0];
}