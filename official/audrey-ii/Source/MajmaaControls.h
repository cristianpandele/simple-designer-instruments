#pragma once
#ifndef MajmaaCONTROLS_H
#define MajmaaCONTROLS_H

#include <daisy.h>
#include <daisy_seed.h>
#include "MajmaaEngine.h"
#include "ParameterRegistry.h"

namespace majmaa {
namespace MajmaaSynth {

class Controls {

public:

    Controls() = default;
    ~Controls() = default;

    void Init(daisy::DaisySeed &hw, Engine &engine);

    void Update(daisy::DaisySeed &hw);

    void Process() {
        params_.Process();
    }

private:

    static const size_t kNumAdcChannels  = 4;
    static const size_t kNumMprInstances = 3;

    /// Identifies a parameter of the synth engine
    /// The order here is the same order as the ADC pin configs in the cpp file
    enum class Parameter : uint8_t {
        Accent              = 0,
        Brightness,         // 1
        Damping,            // 2
        Structure           // 3
    };

    using Parameters = ParameterRegistry<Parameter>;

    Parameters params_;

    daisy::Mpr121I2C mpr121_[kNumMprInstances];

    void initADCs(daisy::DaisySeed &hw);
    void registerParams(Engine &engine);
};

}
}

#endif
