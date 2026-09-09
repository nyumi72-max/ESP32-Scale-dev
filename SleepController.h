#ifndef SLEEP_CONTROLLER_H
#define SLEEP_CONTROLLER_H

#include <Arduino.h>

class SleepController
{
public:
    enum State
    {
        AWAKE,
        PREPARING,
        SLEEPING
    };

    void begin();

    void prepare();

    void sleep(uint64_t sleepSeconds);

    State getState() const;

    bool isSleeping() const;

private:
    State _state = AWAKE;
};

#endif