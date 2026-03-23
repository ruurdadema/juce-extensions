#pragma once

#include <cstdint>

class LevelMeterConstants
{
public:
    /// The default minus infinity.
    static constexpr double kDefaultMinusInfinityDb = -100.0;

    /// The return rate in decibels per second.
    static constexpr double kDefaultReturnRate = 11.7; // Decibels per second, equals 20dB in 1.7 seconds.

    /// The refresh rate of the meter.
    static constexpr int kRefreshRateHz = 30;

    /// The amount of time in milliseconds the peak hold has to wait before declining.
    static constexpr uint32_t kPeakHoldDefaultValueTimeMs = 2000;

    /// The level which triggers the overload indication
    static constexpr float kOverloadTriggerLevel = 1.001f;
};
