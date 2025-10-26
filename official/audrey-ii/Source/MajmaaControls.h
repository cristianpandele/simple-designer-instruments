#pragma once
#ifndef MajmaaCONTROLS_H
#define MajmaaCONTROLS_H

#include <daisy.h>
#include <daisy_seed.h>
#include "Constants.h"
#include "MajmaaEngine.h"

namespace majmaa
{
    namespace MajmaaSynth
    {

        class Controls
        {

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
                    ReverbMix,  // 5
                    Volume      // 6
                };

                Controls() = default;
                ~Controls() = default;

                void init(daisy::DaisySeed &hw, Engine &engine);

                void Update(daisy::DaisySeed &hw);

                uint16_t GetMpr121TouchStates(uint8_t instance);

            private:
                static const size_t kNumAdcChannels = 7;

                daisy::AnalogControl controls_[kNumAdcChannels];

                daisy::Mpr121I2C mpr121_[kNumberMprInstances];

                void initADCs(daisy::DaisySeed &hw);
            };
    }
}

#endif
