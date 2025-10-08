#pragma once
#ifndef DSPUTILS_H
#define DSPUTILS_H

#include <cmath>
#include <Utility/dsp.h>

#ifdef __arm__
#include <arm_math.h>
#endif

#define MIN(in, mn) (in < mn ? in : mn)
#define MAX(in, mx) (in > mx ? in : mx)
#define CLAMP(in, mn, mx) MIN(MAX(in, mn), mx)

namespace majmaa {

inline float dbfs2lin(float dbfs) {
    return daisysp::pow10f(dbfs * 0.05f);
}

inline float lin2dbfs(float lin) {
    return daisysp::fastlog10f(lin) * 20.0f;
}

constexpr float unitclamp(float in)
{
    return CLAMP(in, 0.0f, 1.0f);
}

template <typename T>
inline T map(const T &x, const T &in_min, const T &in_max, const T &out_min, const T &out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Coefficient for one pole smoothing filter based on Tau time constant for `time_s`
inline float onepole_coef(float time_s, float sample_rate) {
    if (time_s <= 0.0f || sample_rate <= 0.0f) { return 1.0f; }
    return daisysp::fmin(1.0f / (time_s * sample_rate), 1.0f);
}

inline float onepole_coef_t60(float time_s, float sample_rate)
{
	return onepole_coef(time_s * 0.1447597f, sample_rate);
}

inline float ftension(const float in, const float factor)
{
    if (factor == 0.0f) return in;
    const float denom = expm1f(factor);
    return expm1f(in * factor) / denom;
}

inline float tanf(const float x)
{
#ifdef __arm__
    return std::tan(x);
    // float s, c;
    // arm_sin_cos_f32(x, &s, &c);
    // return s / c;
#else
    return std::tanf(x);
#endif
}

}

#endif
