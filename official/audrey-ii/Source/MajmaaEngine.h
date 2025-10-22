#pragma once
#ifndef MAJMAA_ENGINE_H
#define MAJMAA_ENGINE_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <daisysp.h>
#include "vox.h"

namespace majmaa {
namespace MajmaaSynth {

class Engine {

    public:
        static constexpr size_t kNumberLiveVoices = 3; // Number of polyphonic voices
        static constexpr size_t kNumberRetriggers = 5; // Number of times a voice can be retriggered after being sampled
        static constexpr size_t kNumberPads = 12;  // Number of pads per voice
        static constexpr size_t kNumberMprInstances = 3;  // Number of MPR instances
        static constexpr size_t kMaxSampleFrames = 2 * 48000; // Cached frames per note (~2s @ 48k)
        static constexpr size_t kMaxKernelTaps = 128;     // FIR taps used for resampling

        struct Parameters {
            float accent       = 0.0f;  // 0.0 - 1.0
            float brightness   = 0.5f;  // 0.0 - 1.0
            float damping      = 0.5f;  // 0.0 - 1.0
            float structure    = 0.5f;  // 0.0 - 1.0
            float reverbFb     = 0.0f;  // 0.0 - 1.0
            float reverbMix    = 0.0f;  // 0.0 - 1.0
            float volume       = 0.5f;  // 0.0 - 1.0
        };

        Engine() = default;
        ~Engine() = default;

        void init(const float sampleRate);

        void setParameters(const Parameters& params);

        void triggerNote(const uint8_t instance, const uint8_t pad);

        void processAudioSample(float &outL, float &outR);

        size_t renderResampledNote(const float* source,
                                   size_t sourceLength,
                                   float pitchRatio,
                                   float* destination,
                                   size_t destinationCapacity,
                                   size_t numTaps = 96,
                                   float normalizedCutoff = 0.45f) const;

    private:
        struct PadPlaybackState {
            bool active = false;
            bool fromCache = false;
            bool recording = false;
            uint8_t age = 0;
            size_t writeIndex = 0;
            size_t playbackIndex = 0;
            uint8_t basePad = 0;
            float* playbackBuffer = nullptr;
            size_t playbackLength = 0;
            float* resampleBuffer = nullptr;
        };

        float sampleRate_;

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
        void setVolume(const float volume);

    size_t countLiveNotes() const;
        void triggerNoteResampleWrapper(size_t length,
                                        uint8_t instance,
                                        uint8_t targetMidi,
                                        int closestPad,
                                        PadPlaybackState &state);
        void triggerNoteLiveNoteWrapper(uint8_t instance,
                                        uint8_t targetMidi,
                                        int pad,
                                        PadPlaybackState &state);

        Engine(const Engine &other) = delete;
        Engine(Engine &&other) = delete;
        Engine& operator=(const Engine &other) = delete;
        Engine& operator=(Engine &&other) = delete;
};

}
}

#endif
