#ifndef SCALESERVICE_H
#define SCALESERVICE_H

#include <Arduino.h>
#include <HX711.h>
#include "Hx711Calibration.h"

typedef void (*ScaleMessageCallback)(const String& msg);

// HX711の読み取りとHx711Calibrationを橋渡しし、
// TARE/CALコマンドの処理と定期的な重量通知をまとめて担当する。
class ScaleService
{
public:

    ScaleService(
        const char* nvsNamespace,
        ScaleMessageCallback callback = nullptr);

    // HX711/Hx711Calibrationを内部に持つため、意図しないコピーを禁止
    ScaleService(const ScaleService&) = delete;
    ScaleService& operator=(const ScaleService&) = delete;

    void begin(
        uint8_t dataPin,
        uint8_t clockPin);

    // loop()から毎回呼ぶ。setPrintInterval()の間隔ごとに重量を通知する
    void update();

    void setPrintInterval(
        unsigned long intervalMs);

    // TARE / CAL BEGIN / CAL ADD <g> / CAL DONE / CAL LIST を処理する。
    // 認識できないコマンドならfalseを返す
    bool handleCommand(const String& cmd);

    bool getWeight(double &weight);

private:

    void notify(const String& msg);

    void doTare();
    void doCalBegin();
    void doCalAdd(float knownWeightGrams);
    void doCalList();
    void doCalDone();
    void printWeight();

    HX711 _scale;

    Hx711Calibration _cal;

    ScaleMessageCallback _callback;

    bool waitReady(unsigned long timeoutMs);

    bool _calInProgress;

    unsigned long _printIntervalMs;

    unsigned long _lastPrintMs;
};

#endif
