#ifndef HX711CALIBRATION_H
#define HX711CALIBRATION_H

#include <Arduino.h>
#include <Preferences.h>

class Hx711Calibration
{
public:

    static constexpr int MAX_POINTS = 20;

    Hx711Calibration(
        const char* nameSpace);

    // Preferences ハンドルを内部に持つため、意図しないコピーを禁止
    Hx711Calibration(const Hx711Calibration&) = delete;
    Hx711Calibration& operator=(const Hx711Calibration&) = delete;

    bool begin();

    void clear();

    bool addPoint(
        double raw,
        double weight);

    // 多点校正をやり直さず、スロープは据え置きで現在の生値を0gとして
    // オフセットだけを更新する(いわゆる簡易TARE)
    void quickTare(
        double raw);

    bool getPoint(
        int index,
        double& raw,
        double& weight) const;

    bool calculate();

    double convert(
        double raw) const;

    bool save();

    bool load();

    int pointCount() const;

    bool isCalibrated() const;

    // convert()が意味のある値を返せるか(多点校正済み、またはquickTare済み)
    bool isReady() const;

    bool isTared() const;

    double getSlope() const;

    double getOffset() const;

    double getR2() const;

private:

    struct Point
    {
        double raw;
        double weight;
    };

    Point _points[MAX_POINTS];

    int _count;

    double _slope;

    double _offset;

    double _r2;

    bool _tared;

    // 呼び出し元の一時バッファ(例: String::c_str())破棄によるダングリングを避けるため
    // 値でコピー保持する
    String _ns;

    Preferences _prefs;
};

#endif
