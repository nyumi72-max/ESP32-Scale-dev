#ifndef TIME_CONTROLLER_H
#define TIME_CONTROLLER_H

#include <Arduino.h>

class TimeController
{
public:
    void begin();

    bool setUnixTime(uint32_t unixTime);
    uint32_t getUnixTime() const;

    bool isValid() const;

    uint32_t secondsUntilNextSchedule() const;

private:
    uint32_t _unixTime = 0;
    uint32_t _baseUnixTime = 0;
    uint32_t _baseMillis = 0;
    bool _valid = false;
};

#endif