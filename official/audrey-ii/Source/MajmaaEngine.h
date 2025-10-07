#pragma once
#ifndef IFS_FEEDBACK_SYNTH_ENGINE_H
#define IFS_FEEDBACK_SYNTH_ENGINE_H

#include <memory>
#include <daisysp.h>
#include "BiquadFilters.h"
#include "EchoDelay.h"
#include "KarplusString.h"

#ifdef __arm__
#include <dev/sdram.h>
#endif

namespace majmaa {
namespace MajmaaSynth {

class Engine {

    public:
        Engine() = default;
        ~Engine() = default;

        void Init(const float sample_rate);

        void SetStringPitch(const float nn);

        void SetAccent(const float accent);

        void SetBrightness(const float brightness);
        void SetDamping(const float damping);
        void SetStructure(const float structure);

        void Process(float &outL, float &outR);

    private:
        // long enough for 250ms at 48kHz
        static constexpr size_t kMaxFeedbackDelaySamp = 12000;
        // long enough for 5s at 48kHz
        static constexpr size_t kMaxEchoDelaySamp = 48000 * 5;

        float sample_rate_;
        float fb_gain_ = 0.0f;
        float echo_send_ = 0.0f;
        float verb_mix_ = 0.0f;
        float output_level_ = 0.5f;

        float fb_delay_smooth_coef_;
        float fb_delay_samp_ = 1000.f;
        float fb_delay_samp_target_ = 64.f;

        majmaa::KarplusString strings_[2];
        daisysp::WhiteNoise noise_;
        daisysp::DelayLine<float, kMaxFeedbackDelaySamp> fb_delayline_[2];
        daisysp::Overdrive overdrive_[2];

        LPF12 fb_lpf_;
        HPF12 fb_hpf_;

        using VerbPtr = std::unique_ptr<daisysp::ReverbSc>;
        VerbPtr verb_;

        using EchoDelayPtr = std::unique_ptr<EchoDelay<kMaxEchoDelaySamp>>;
        EchoDelayPtr echo_delay_[2];

        Engine(const Engine &other) = delete;
        Engine(Engine &&other) = delete;
        Engine& operator=(const Engine &other) = delete;
        Engine& operator=(Engine &&other) = delete;
};

}
}

#endif
