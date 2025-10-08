#pragma once

#ifndef MAJMAA_MAIN_H
#define MAJMAA_MAIN_H

#include <daisy_seed.h>
#include "MajmaaEngine.h"
#include "MajmaaControls.h"
#include "SmoothValue.h"

using namespace majmaa;
using namespace daisy;
using namespace daisysp;

static const auto kSampleRate = SaiHandle::Config::SampleRate::SAI_48KHZ;
static const auto kSamplePeriodMs = 1000.0f / 48000.0f;
static const size_t kBlockSize = 48;

static DaisySeed hw;
static MajmaaSynth::Engine engine;
static MajmaaSynth::Controls controls;
static Limiter limiter[2];

// Controls
static SmoothValue accentKnobVal = SmoothValue(0.0f, 75.0f, kSamplePeriodMs * kBlockSize);
static SmoothValue brightnessKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs * kBlockSize);
static SmoothValue dampingKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs * kBlockSize);
static SmoothValue structureKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs * kBlockSize);

// Get the current values of all analog controls, smooth values and update internal engine state
void handleAnalogControls(MajmaaSynth::Controls &controls, MajmaaSynth::Engine &engine);

#endif