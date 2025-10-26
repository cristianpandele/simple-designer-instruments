#pragma once
#ifndef MAJMAA_ENGINE_H
#define MAJMAA_ENGINE_H

#include <array>
#include <daisysp.h>
#include "vox.h"
#include "Constants.h"

namespace majmaa
{
    namespace MajmaaSynth
    {

        class Engine
        {

            public:
                static constexpr size_t kNumberLiveStrings = 3;                       // Number of live synthesized strings
                static constexpr size_t kMaxCacheSamples  = kMaxBowLengthSec * 48000; // Cached samples per note (~2s @ 48k)

                struct Parameters
                {
                    float accent = 0.0f;     // 0.0 - 1.0
                    float brightness = 0.5f; // 0.0 - 1.0
                    float damping = 0.5f;    // 0.0 - 1.0
                    float structure = 0.5f;  // 0.0 - 1.0
                    float reverbFb = 0.0f;   // 0.0 - 1.0
                    float reverbMix = 0.0f;  // 0.0 - 1.0
                    float volume = 0.5f;     // 0.0 - 1.0
                };

                Engine() = default;
                ~Engine() = default;

                void init(const float sampleRate);

                void setParameters(const Parameters &params);

                void triggerNote(const uint8_t instance, const uint8_t pad);

                void processAudioSample(float &outL, float &outR);

            private:
                struct PadPlaybackState
                {
                    bool active = false;             // Is the pad currently active (playing or recording)
                    bool fromCache = false;          // Is the pad playback from cache
                    bool recording = false;          // Is the pad currently being recorded
                    uint8_t age = 0;                 // Age of the pad (for retriggering)
                    size_t writeIndex = 0;           // Write index for recording
                    size_t playbackIndex = 0;        // Playback index for playing back
                    float *playbackBuffer = nullptr; // Buffer for playback
                    size_t playbackLength = 0;       // Length of the playback buffer
                };

                float sampleRate_;
                Parameters params_;

                // String synth voice
                std::array<Vox, kNumberMprInstances> strings_;

                // Pad playback states
                std::array<std::array<PadPlaybackState, kNumberMprPads>, kNumberMprInstances> padStates_;

                void setAccent(const float accent);
                void setBrightness(const float brightness);
                void setDamping(const float damping);
                void setStructure(const float structure);
                void setReverbFeedback(const float reverbFb);
                void setReverbMix(const float reverbMix);
                void setVolume(const float volume);

                size_t countLiveNotes() const;
                void triggerNotePlaybackWrapper(size_t length,
                                                uint8_t instance,
                                                uint8_t pad,
                                                PadPlaybackState &state);
                void triggerNoteLiveNoteWrapper(uint8_t instance,
                                                uint8_t targetMidi,
                                                int pad,
                                                PadPlaybackState &state);

                Engine(const Engine &other) = delete;
                Engine(Engine &&other) = delete;
                Engine &operator=(const Engine &other) = delete;
                Engine &operator=(Engine &&other) = delete;
            };
    }
}

#endif
