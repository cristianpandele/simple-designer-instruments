#pragma once
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <array>

namespace majmaa
{
    static constexpr size_t kNumberMprPads      = 12; // Number of pads per voice
    static constexpr size_t kNumberMprInstances = 3;  // Number of MPR instances

    static constexpr size_t kNumberRetriggers = 5; // Number of times a string can be retriggered after being sampled
    static constexpr size_t kMaxBowLengthSec  = 4; // Maximum duration of bowed excitation in seconds

    // Scales (as MIDI notes) for the 3 instrument instances
    const std::array<std::array<uint8_t, kNumberMprPads>, kNumberMprInstances> scales_ =
        {{
            // Instance 0 - A minor Harmonic (Hijaz) - Oud 1
            {57, 59, 60, 62, 64, 65, 68, 69, 71, 72, 74, 76},
            // Instance 1 - D minor - Oud 2
            {62, 64, 65, 67, 69, 70, 72, 74, 76, 77, 79, 81},
            // Instance 2 - C Major Pentatonic - Cellos
            {36, 38, 40, 43, 45, 48, 50, 52, 55, 57, 60, 62},
            // Instance 3 - G Major (Ajam) - Oud
            // {55, 57, 59, 60, 62, 64, 66, 67, 69, 71, 72, 74}
        }};

    struct Parameters
    {
        float accent;     // 0.0 - 1.0
        float brightness; // 0.0 - 1.0
        float damping;    // 0.0 - 1.0
        float structure;  // 0.0 - 1.0
        float reverbFb;   // 0.0 - 1.0
        float reverbMix;  // 0.0 - 1.0
        float volume;     // 0.0 - 1.0
    };

    // Set to 0 after good parameters are found
    #define KNOBS_SET_STRING_PARAMETERS 1

    const std::array<Parameters, kNumberMprInstances> defaultParams_ =
        {{
            // Instance 0
            {.accent = 0.0f,
             .brightness = 0.5f,
             .damping = 0.5f,
             .structure = 0.5f,
             .reverbFb = 0.0f,
             .reverbMix = 0.0f,
             .volume = 0.5f},
            // Instance 1
            {.accent = 0.0f,
             .brightness = 0.5f,
             .damping = 0.5f,
             .structure = 0.5f,
             .reverbFb = 0.0f,
             .reverbMix = 0.0f,
             .volume = 0.5f},
            // Instance 2
            {.accent = 0.1f, // Accent a bit higher for cello - as it is the bow strength
             .brightness = 0.5f,
             .damping = 0.5f,
             .structure = 0.5f,
             .reverbFb = 0.0f,
             .reverbMix = 0.0f,
             .volume = 0.5f},
        }};
}

#endif
