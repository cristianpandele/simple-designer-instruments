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

    static const size_t kNumAdcChannels = 11;

    /// Identifies a parameter of the synth engine
    /// The order here is the same order as the ADC pin configs in the cpp file
    enum class Parameter : uint8_t {
        Frequency           = 0,
        FeedbackGain,       // 1
        FeedbackBody,       // 2
        FeedbackLPFCutoff,  // 3
        FeedbackHPFCutoff,  // 4
        ReverbMix,          // 5
        ReverbDecay,        // 6
        EchoDelaySend,      // 7
        EchoDelayTime,      // 8
        EchoDelayFeedback,  // 9
        OutputVolume        // 10
    };

    using Parameters = ParameterRegistry<Parameter>;

    Parameters params_;

    void initADCs(daisy::DaisySeed &hw);
    void registerParams(Engine &engine);
};

}
}

#endif
