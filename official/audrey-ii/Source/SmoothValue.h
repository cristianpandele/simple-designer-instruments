#pragma once
#ifndef SMOOTHED_VALUE_H
#define SMOOTHED_VALUE_H

#include <daisysp.h>
#include "DSPUtils.h"

namespace majmaa {

// SmoothValue class for smoothing parameter changes
class SmoothValue
{
public:
    SmoothValue() : SmoothValue(0.0f, 0.0f, 0.0f) {};

    SmoothValue(float smoothTimeMs, float updatePeriodMs) : SmoothValue(0.0f, smoothTimeMs, updatePeriodMs) {}

    SmoothValue(float currentValue, float smoothTimeMs, float updatePeriodMs)
    {
        smoothing_ = false;
        currentValue_ = currentValue;
        targetValue_ = currentValue;

        // coeff = 100.0 / (time * sample_rate), where time is in seconds
        float updateRate = 1000.0f / updatePeriodMs;
        filterCoeff_ = 100.0f / ((smoothTimeMs / 1000.0f) * updateRate);
    }

    // Overload assignment to set targetValue
    SmoothValue &operator=(float v)
    {
        if (std::abs(v - targetValue_) > 0.015f)
        {
            targetValue_ = v;
            // Check if the value is undergoing smoothing
            setSmoothing(targetValue_, currentValue_);
        }
        return *this;
    }

    // Overload the += operator to add to the target value
    SmoothValue &operator+=(float v)
    {
        targetValue_ += v;
        // Check if the value is undergoing smoothing
        setSmoothing(targetValue_, currentValue_);
        return *this;
    }

    // Get the smoothed value
    float getSmoothVal()
    {
        daisysp::fonepole(currentValue_, targetValue_, filterCoeff_);
        // Check if the value is undergoing smoothing
        setSmoothing(targetValue_, currentValue_);
        return currentValue_;
    }

    // Get the target value
    float getTargetVal() const { return targetValue_; }

    // Check if the value has smoothing
    bool isSmoothing() const { return smoothing_; }

private:
    bool smoothing_;
    float currentValue_;
    float targetValue_;
    float filterCoeff_;

    // Determine if the value has smoothing
    void setSmoothing(float oldValue, float currentValue)
    {
        // If the current value is more that 1% away from the old value, mark as smoothing
        if (std::abs((currentValue - oldValue) / oldValue) > 0.01f)
        {
            smoothing_ = true;
        }
        else
        {
            smoothing_ = false;
        }
    }
};

}

#endif