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

private:
    uint32_t _unixTime = 0;
    bool _valid = false;
};

#endif