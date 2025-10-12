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

        void init(const float sampleRate);

        void setParameters(const Parameters& params);

        void triggerNote(const uint8_t instance, const uint8_t pad);

        void processAudioSample(float &outL, float &outR);

    private:
        float sampleRate_;
        float reverbFb = 0.0f;
        float reverbMix = 0.0f;

        Parameters params_;

        // String synth voice
        std::array<Vox, kNumberVoices> strings_;

        void setAccent(const float accent);
        void setBrightness(const float brightness);
        void setDamping(const float damping);
        void setStructure(const float structure);
        void setReverbFeedback(const float reverbFb);
        void setReverbMix(const float reverbMix);

        Engine(const Engine &other) = delete;
        Engine(Engine &&other) = delete;
        Engine& operator=(const Engine &other) = delete;
        Engine& operator=(Engine &&other) = delete;
};

}
}

#endif
