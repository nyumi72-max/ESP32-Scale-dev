#include "ScaleService.h"

ScaleService::ScaleService(
    const char* nvsNamespace,
    ScaleMessageCallback callback)
    : _cal(nvsNamespace)
{
    _callback = callback;
    _calInProgress = false;
    _printIntervalMs = 500;
    _lastPrintMs = 0;
}

void ScaleService::begin(
    uint8_t dataPin,
    uint8_t clockPin)
{
    _scale.begin(dataPin, clockPin);

    if (_scale.is_ready())
    {
        notify("HX711 initialized");
        _cal.load();
    }
    else
    {
        notify("HX711 not found - check wiring");
    }
}

void ScaleService::setPrintInterval(
    unsigned long intervalMs)
{
    _printIntervalMs = intervalMs;
}

void ScaleService::update()
{
    unsigned long now = millis();

    if (now - _lastPrintMs < _printIntervalMs)
    {
        return;
    }

    _lastPrintMs = now;

    printWeight();
}

void ScaleService::notify(
    const String& msg)
{
    if (_callback)
    {
        _callback(msg);
    }
}

bool ScaleService::waitReady(unsigned long timeoutMs)
{
    unsigned long start = millis();
    while (!_scale.is_ready())
    {
        if (millis() - start > timeoutMs) return false;
        delay(1);
    }
    return true;
}
void ScaleService::printWeight()
{
    if (!waitReady(250))
    {
        notify("HX711 not ready (wiring/power check)");
        return;
    }
    
    double raw = _scale.read_average(5);

    if (!_cal.isReady())
    {
        notify("Not tared/calibrated yet. Use TARE, or CAL BEGIN / CAL ADD / CAL DONE.");
        return;
    }

    double grams = _cal.convert(raw);

    notify("Weight: " + String(grams, 2) + " g");
}

void ScaleService::doTare()
{
    if (!_cal.isCalibrated())
        {
            notify("Calibration required before TARE. Use CAL BEGIN / CAL ADD / CAL DONE first.");
            return;
        }
    if (!waitReady(250))
    {
        notify("HX711 not ready");
        return;
    }

    notify("Taring... remove all weight from the scale");
    delay(1000);

    double raw = _scale.read_average(20);

    if (!_cal.quickTare(raw))
    {
        notify("Tare failed.");
        return;
    }

    _cal.save();

    notify("Tare complete. Raw at 0g: " + String(raw, 0));
}

void ScaleService::doCalBegin()
{
    _cal.clear();
    _calInProgress = true;

    notify("Calibration session started.");
    notify("Place a known weight and send: CAL ADD <grams>");
    notify("You can add up to " + String(Hx711Calibration::MAX_POINTS) + " points, then send: CAL DONE");
}

void ScaleService::doCalAdd(
    float knownWeightGrams)
{
    if (!_calInProgress)
    {
        notify("No calibration session in progress. Send CAL BEGIN first.");
        return;
    }

    if (!waitReady(250))
    {
        notify("HX711 not ready");
        return;
    }

    double raw = _scale.read_average(10);

    if (!_cal.addPoint(raw, knownWeightGrams))
    {
        notify("Max calibration points reached. Send CAL DONE.");
        return;
    }

    notify("Point " + String(_cal.pointCount()) + " added: " +
           String(knownWeightGrams, 2) + " g -> raw " + String(raw, 0));
}

void ScaleService::doCalList()
{
    if (_cal.pointCount() == 0)
    {
        notify("No calibration points recorded yet.");
        return;
    }

    for (int i = 0; i < _cal.pointCount(); i++)
    {
        double raw = 0.0;
        double weight = 0.0;
        _cal.getPoint(i, raw, weight);

        notify("  [" + String(i + 1) + "] " + String(weight, 2) +
               " g -> raw " + String(raw, 0));
    }
}

void ScaleService::doCalDone()
{
    if (!_calInProgress)
    {
        notify("No calibration session in progress. Send CAL BEGIN first.");
        return;
    }

    if (_cal.pointCount() < 2)
    {
        notify("Need at least 2 points before CAL DONE. Use CAL ADD <grams>.");
        return;
    }

    if (!_cal.calculate())
    {
        notify("Calibration failed: points are not distinct enough.");
        _calInProgress = false;
        return;
    }

    _calInProgress = false;

    _cal.save();

    notify("Calibration done using " + String(_cal.pointCount()) + " point(s).");
    notify("  slope: " + String(_cal.getSlope(), 6));
    notify("  offset: " + String(_cal.getOffset(), 2));
    notify("  R^2: " + String(_cal.getR2(), 4));
}

bool ScaleService::handleCommand(
    const String& cmd)
{
    String upper = cmd;
    upper.toUpperCase();

    if (upper == "TARE" || upper == "T")
    {
        doTare();
        return true;
    }

    if (upper.startsWith("CAL"))
    {
        String arg = cmd.substring(3);
        arg.trim();

        String argUpper = arg;
        argUpper.toUpperCase();

        if (argUpper == "BEGIN")
        {
            doCalBegin();
        }
        else if (argUpper == "DONE")
        {
            doCalDone();
        }
        else if (argUpper == "LIST")
        {
            doCalList();
        }
        else if (argUpper.startsWith("ADD"))
        {
            String weightStr = arg.substring(3);
            weightStr.trim();
            doCalAdd(weightStr.toFloat());
        }
        else
        {
            notify("Usage: CAL BEGIN | CAL ADD <grams> | CAL DONE | CAL LIST");
        }

        return true;
    }

    return false;
}

bool ScaleService::getWeight(double &weight)
{
    if (!_scale.is_ready())
        return false;

    if (!_cal.isReady())
        return false;

    long raw = _scale.read_average(5);

    weight = _cal.convert((double)raw);

    return true;
}