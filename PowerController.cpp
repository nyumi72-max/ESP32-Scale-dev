#include "PowerController.h"

void PowerController::begin()
{
    pinMode(LDO_EN_PIN, OUTPUT);
    enable4V5();
}

void PowerController::enable4V5()
{
    digitalWrite(LDO_EN_PIN, HIGH);
    _enabled = true;
}

void PowerController::disable4V5()
{
    digitalWrite(LDO_EN_PIN, LOW);
    _enabled = false;
}

bool PowerController::is4V5Enabled() const
{
    return _enabled;
}