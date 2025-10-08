#pragma once
#ifndef MajmaaCONTROLS_H
#define MajmaaCONTROLS_H

#include <daisy.h>
#include <daisy_seed.h>
#include "MajmaaEngine.h"

namespace majmaa {
namespace MajmaaSynth {

class Controls {

public:
    // Identifies a parameter of the synth engine
    /// The order here is the same order as the ADC pin configs in the cpp file
    enum AnalogControlId
    {
        Accent = 0,
        Brightness, // 1
        Damping,    // 2
        Structure,  // 3
        ReverbFb,   // 4
        ReverbMix   // 5
    };

    Controls() = default;
    ~Controls() = default;

    void Init(daisy::DaisySeed &hw, Engine &engine);

    void Update(daisy::DaisySeed &hw);

    // Read and smooth all analog controls
    void ProcessAnalogControls();

    // Get the current value of an analog control in the range 0.0 - 1.0
    float GetAnalogControlValue(AnalogControlId id);

private:
    static const size_t kNumAdcChannels  = 6;
    static const size_t kNumMprInstances = 3;

    daisy::AnalogControl controls_[kNumAdcChannels];

    daisy::Mpr121I2C mpr121_[kNumMprInstances];

    void initADCs(daisy::DaisySeed &hw);
};

}
}

#endif
