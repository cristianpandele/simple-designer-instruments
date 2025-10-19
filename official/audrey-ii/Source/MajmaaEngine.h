#pragma once
#ifndef MAJMAA_ENGINE_H
#define MAJMAA_ENGINE_H

#include <array>
#include <vector>
#include <daisysp.h>
#include "vox.h"

namespace majmaa {
namespace MajmaaSynth {

class Engine {

    public:
        static constexpr size_t kNumberLiveVoices = 6; // Number of polyphonic voices
        static constexpr size_t kNumberPads = 12;  // Number of pads per voice
        static constexpr size_t kMaxSampleFrames = 48000; // Cached frames per note (~1s @ 48k)

        struct Parameters {
            float accent       = 0.0f;  // 0.0 - 1.0
            float brightness   = 0.5f;  // 0.0 - 1.0
            float damping      = 0.5f;  // 0.0 - 1.0
            float structure    = 0.5f;  // 0.0 - 1.0
            float reverbFb     = 0.0f;  // 0.0 - 1.0
            float reverbMix    = 0.0f;  // 0.0 - 1.0
        };

        Engine() = default;
        ~Engine() = default;

        void init(const float sampleRate);

        void setParameters(const Parameters& params);

        void triggerNote(const uint8_t instance, const uint8_t pad);

        void processAudioSample(float &outL, float &outR);

        std::vector<float> renderResampledNote(const std::vector<float>& noteBuffer,
                                               float srcSampleRate,
                                               float dstSampleRate,
                                               size_t numTaps = 96,
                                               float normalizedCutoff = 0.45f) const;

    private:
        struct PadPlaybackState
        {
            bool active = false;
            bool fromCache = false;
            bool recording = false;
            size_t writeIndex = 0;
            size_t playbackIndex = 0;
            uint8_t basePad = 0;
            float *resampledBuffer = nullptr;
            size_t resampledBufferSize = 0;
        };

        float sampleRate_;
        float reverbFb = 0.0f;
        float reverbMix = 0.0f;

        Parameters params_;

        // String synth voice
        std::array<Vox, kNumberLiveVoices> strings_;

        std::array<std::array<PadPlaybackState, kNumberPads>, kNumberLiveVoices> padStates_;

        // Scales (as MIDI notes) for the 3 instrument instances
        const std::array<std::array<uint8_t, kNumberPads>, kNumberLiveVoices> scales_ =
        {{
            // Instance 0 - A minor Harmonic (Hijaz) - Oud 1
            {57, 59, 60, 62, 64, 65, 68, 69, 71, 72, 74, 76},
            // Instance 1 - D minor - Oud 2
            {62, 64, 65, 67, 69, 70, 72, 74, 76, 77, 79, 81},
            // Instance 2 - C Major Pentatonic - Cellos
            {55, 57, 59, 60, 62, 64, 66, 67, 69, 71, 72, 74}
        }};

        void setAccent(const float accent);
        void setBrightness(const float brightness);
        void setDamping(const float damping);
        void setStructure(const float structure);
        void setReverbFeedback(const float reverbFb);
        void setReverbMix(const float reverbMix);

        size_t countLiveNotes();
        void triggerNoteResampleWrapper(const size_t length,
                                        const uint8_t instance,
                                        const uint8_t targetMidi,
                                        const int closestPad,
                                        PadPlaybackState &state);
        void triggerNoteLiveNoteWrapper(const uint8_t instance,
                                        const uint8_t targetMidi,
                                        const int closestPad,
                                        PadPlaybackState &state);

        Engine(const Engine &other) = delete;
        Engine(Engine &&other) = delete;
        Engine& operator=(const Engine &other) = delete;
        Engine& operator=(Engine &&other) = delete;
};

}
}

#endif
