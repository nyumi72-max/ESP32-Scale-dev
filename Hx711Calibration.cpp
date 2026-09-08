#include "Hx711Calibration.h"
#include <math.h>

Hx711Calibration::Hx711Calibration(
    const char* nameSpace)
{
    _ns = nameSpace;

    _count = 0;

    _slope = 1.0;

    _offset = 0.0;

    _r2 = 0.0;

    _tared = false;
}

void Hx711Calibration::clear()
{
    _count = 0;
    _slope = 1.0;
    _offset = 0.0;
    _r2 = 0.0;
    _tared = false;
}

int Hx711Calibration::pointCount() const
{
    return _count;
}

bool Hx711Calibration::isCalibrated() const
{
    return (_count >= 2) && (fabs(_slope) > 1e-9);
}

bool Hx711Calibration::isReady() const
{
    return isCalibrated() || _tared;
}

bool Hx711Calibration::isTared() const
{
    return _tared;
}

bool Hx711Calibration::quickTare(
    double raw)
{
    // weight = slope*raw + offset なので、
    // 現在の生値rawでweight=0になるようoffsetだけを調整する
    _offset = -(_slope * raw);
    _tared = true;
}

bool Hx711Calibration::begin()
{
    return true;
}

bool Hx711Calibration::addPoint(
    double raw,
    double weight)
{
    if (_count >= MAX_POINTS)
    {
        return false;
    }

    _points[_count].raw = raw;
    _points[_count].weight = weight;

    _count++;

    return true;
}

bool Hx711Calibration::calculate()
{
    if (_count < 2)
    {
        return false;
    }

    double sx  = 0.0;
    double sy  = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;

    for (int i = 0; i < _count; i++)
    {
        double x = _points[i].raw;
        double y = _points[i].weight;

        sx  += x;
        sy  += y;
        sxx += x * x;
        sxy += x * y;
    }

    double n = static_cast<double>(_count);

    double denominator =
        (n * sxx) - (sx * sx);

    if (fabs(denominator) < 1e-9)
    {
        return false;
    }

    // 最小二乗法による直線近似
    _slope =
        ((n * sxy) - (sx * sy))
        / denominator;

    _offset =
        (sy - (_slope * sx))
        / n;

    // 決定係数(R²)の計算
    double yMean =
        sy / n;

    double ssRes = 0.0;
    double ssTot = 0.0;

    for (int i = 0; i < _count; i++)
    {
        double x =
            _points[i].raw;

        double y =
            _points[i].weight;

        double predicted =
            (_slope * x)
            + _offset;

        double residual =
            y - predicted;

        double deviation =
            y - yMean;

        ssRes +=
            residual * residual;

        ssTot +=
            deviation * deviation;
    }

    if (ssTot < 1e-12)
    {
        _r2 = 1.0;
    }
    else
    {
        _r2 =
            1.0 -
            (ssRes / ssTot);
    }

    return true;
}

double Hx711Calibration::convert(
    double raw) const
{
    if (!isReady())
    {
        return 0.0;
    }

    double weight =
        (_slope * raw) + _offset;

    return weight;
}

double Hx711Calibration::getR2() const
{
    return _r2;
}


bool Hx711Calibration::save()
{
    if (!_prefs.begin(_ns.c_str(), false))
    {
        return false;
    }

    _prefs.putDouble(
        "slope",
        _slope);

    _prefs.putDouble(
        "offset",
        _offset);

    _prefs.putDouble(
        "r2",
        _r2);

    _prefs.putInt(
        "count",
        _count);

    _prefs.putBool(
        "tared",
        _tared);

    size_t expectedBytes = sizeof(Point) * _count;

    size_t written =
        _prefs.putBytes(
            "points",
            _points,
            expectedBytes);

    _prefs.end();

    // putBytes は実際に書き込めたバイト数を返す。
    // NVS 容量不足などで途中までしか書けなかった場合は失敗として扱う
    if (written != expectedBytes)
    {
        return false;
    }

    return true;
}

bool Hx711Calibration::load()
{
    if (!_prefs.begin(_ns.c_str(), true))
    {
        return false;
    }

    _slope =
        _prefs.getDouble(
            "slope",
            1.0);

    _offset =
        _prefs.getDouble(
            "offset",
            0.0);

    _r2 =
        _prefs.getDouble(
            "r2",
            0.0);

    _count =
        _prefs.getInt(
            "count",
            0);

    _tared =
        _prefs.getBool(
            "tared",
            false);

    // NVS破損等で不正な値が読めた場合に備えてクランプする
    if (_count < 0)
    {
        _count = 0;
    }
    else if (_count > MAX_POINTS)
    {
        _count = MAX_POINTS;
    }

    _prefs.getBytes(
        "points",
        _points,
        sizeof(Point) * _count);

    _prefs.end();

    return true;
}

bool Hx711Calibration::getPoint(
    int index,
    double& raw,
    double& weight) const
{
    if (index < 0 ||
        index >= _count)
    {
        return false;
    }

    raw =
        _points[index].raw;

    weight =
        _points[index].weight;

    return true;
}

double Hx711Calibration::getSlope() const
{
    return _slope;
}

double Hx711Calibration::getOffset() const
{
    return _offset;
}
