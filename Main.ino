#include <Arduino.h>
#include <ScaleController.h>

ScaleController scaleController;

void setup()
{
    scaleController.begin();
}

void loop()
{
    scaleController.update();
}