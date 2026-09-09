#ifndef POWER_CONTROLLER_H
#define POWER_CONTROLLER_H

#include <Arduino.h>

class PowerController
{
public:
    void begin();

    void enable4V5();
    void disable4V5();

    bool is4V5Enabled() const;

private:
    static constexpr uint8_t LDO_EN_PIN = 10;
    bool _enabled = false;
};

#endif