#include "SleepController.h"

#include <esp_sleep.h>

void SleepController::begin()
{
    _state = AWAKE;
}

void SleepController::prepare()
{
    if (_state == SLEEPING)
        return;

    _state = PREPARING;
}

void SleepController::sleep(uint64_t sleepSeconds)
{
    _state = SLEEPING;

    esp_sleep_enable_timer_wakeup(
        sleepSeconds * 1000000ULL);

    esp_deep_sleep_start();
}

SleepController::State
SleepController::getState() const
{
    return _state;
}

bool SleepController::isSleeping() const
{
    return _state == SLEEPING;
}