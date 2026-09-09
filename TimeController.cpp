#include "TimeController.h"

void TimeController::begin()
{
    _unixTime = 0;
    _valid = false;
}

bool TimeController::setUnixTime(uint32_t unixTime)
{
    if (unixTime < 1000000000UL)
        return false;

    _unixTime = unixTime;
    _valid = true;

    return true;
}

uint32_t TimeController::getUnixTime() const
{
    return _unixTime;
}

bool TimeController::isValid() const
{
    return _valid;
}