#pragma once

#ifndef MAJMAA_MAIN_H
#define MAJMAA_MAIN_H

#include "stopwatch_timer.h"
#include <daisy_seed.h>
#include "MajmaaEngine.h"
#include "MajmaaControls.h"
#include "SmoothValue.h"

using namespace majmaa;
using namespace MajmaaSynth;
using namespace daisy;
using namespace daisysp;

#define PRINT_CPU_LOAD

static const auto kSampleRate = SaiHandle::Config::SampleRate::SAI_48KHZ;
static const auto kSamplePeriodMs = 1000.0f / 48000.0f;
static const size_t kBlockSize = 48;

static const size_t kDebugLogPeriodMs = 500;
static const size_t kTouchRegisterPeriodMs = 100;

static DaisySeed hw;
static Engine engine;
static Controls controls;
static Limiter limiter[2];

// Controls
static SmoothValue accentKnobVal = SmoothValue(0.0f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue brightnessKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue dampingKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue structureKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue reverbFbKnobVal = SmoothValue(0.0f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue reverbMixKnobVal = SmoothValue(0.0f, 75.0f, kSamplePeriodMs *kBlockSize);
static SmoothValue volumeKnobVal = SmoothValue(0.5f, 75.0f, kSamplePeriodMs *kBlockSize);

// Pad touch states
static std::bitset<16> padTouchStates[controls.kNumMprInstances];
static std::bitset<16> padTouchStatesPrev[controls.kNumMprInstances];

daisy::StopwatchTimer log_timer;
daisy::StopwatchTimer touch_timer;

#if DEBUG
void logDebugInfo();
#endif

// Get the current values of all analog controls, smooth values and update internal engine state
void handleAnalogControls(Controls &controls, Engine &engine);
// Get the current values of all digital controls (touch pads)
void handleDigitalControls(Controls &controls);

#endif