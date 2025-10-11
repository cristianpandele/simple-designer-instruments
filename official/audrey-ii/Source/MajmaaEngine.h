#pragma once
#ifndef MAJMAA_ENGINE_H
#define MAJMAA_ENGINE_H

#include <array>
#include <daisysp.h>
#include "vox.h"

namespace majmaa {
namespace MajmaaSynth {

class Engine {

    public:
        struct Parameters {
            float accent       = 0.0f;  // 0.0 - 1.0
            float brightness   = 0.5f;  // 0.0 - 1.0
            float damping      = 0.5f;  // 0.0 - 1.0
            float structure    = 0.5f;  // 0.0 - 1.0
            float reverbFb     = 0.0f;  // 0.0 - 1.0
            float reverbMix    = 0.0f;  // 0.0 - 1.0
        };

        static constexpr size_t kNumberVoices = 3; // Number of polyphonic voices

        Engine() = default;
        ~Engine() = default;

        void Init(const float sample_rate);

        void SetStringPitch(const float nn);

        void SetParameters(const Parameters& params);

        void Process(float &outL, float &outR);

    private:
        float sample_rate_;
        float fb_gain_ = 0.0f;
        float echo_send_ = 0.0f;
        float verb_mix_ = 0.0f;
        float output_level_ = 0.5f;

        float fb_delay_smooth_coef_;
        float fb_delay_samp_ = 1000.f;
        float fb_delay_samp_target_ = 64.f;

        Parameters params_;

        // String synth voice
        std::array<Vox, kNumberVoices> strings_;

        void SetAccent(const float accent);
        void SetBrightness(const float brightness);
        void SetDamping(const float damping);
        void SetStructure(const float structure);
        void SetReverbFeedback(const float reverbFb);
        void SetReverbMix(const float reverbMix);

        Engine(const Engine &other) = delete;
        Engine(Engine &&other) = delete;
        Engine& operator=(const Engine &other) = delete;
        Engine& operator=(Engine &&other) = delete;
};

}
}

#endif
