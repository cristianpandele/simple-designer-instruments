#include "MajmaaEngine.h"
#include "Utils.h"

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisysp;

static DSY_SDRAM_BSS daisysp::ReverbSc reverb_;

namespace
{
    constexpr size_t kCacheVoices = Engine::kNumberLiveVoices;
    constexpr size_t kCachePads = Engine::kNumberPads;
    constexpr size_t kCacheFrames = Engine::kMaxSampleFrames;

    static DSY_SDRAM_BSS float noteCacheData[kCacheVoices][kCachePads][kCacheFrames];
    static size_t noteCacheLength[kCacheVoices][kCachePads];
    static uint8_t noteCacheMidi[kCacheVoices][kCachePads];
    static bool noteCacheValid[kCacheVoices][kCachePads];
} // namespace

void Engine::init(const float sampleRate)
{
    static_assert(kNumberLiveVoices == kCacheVoices, "Voice cache mismatch");
    static_assert(kNumberPads == kCachePads, "Pad cache mismatch");
    static_assert(kMaxSampleFrames == kCacheFrames, "Frame cache mismatch");

    sampleRate_ = sampleRate;

    // Initialize the voices
    for (auto &s : strings_)
    {
        s.init(sampleRate_);
        s.setBrightness(0.5f);
        s.setStructure(0.5f);
        s.setDamping(0.5f);
        s.SetMult(1.0f);
    }

    for (size_t inst = 0; inst < kCacheVoices; ++inst)
    {
        for (size_t pad = 0; pad < kCachePads; ++pad)
        {
            noteCacheValid[inst][pad] = false;
            noteCacheLength[inst][pad] = 0;
            noteCacheMidi[inst][pad] = scales_[inst][pad];
            if (inst < kNumberLiveVoices && pad < kNumberPads)
            {
                PadPlaybackState &state = padStates_[inst][pad];
                state = {};
                state.playbackBuffer = nullptr;
                state.playbackLength = 0;
            }
        }
    }

    reverb_.Init(sampleRate);
    reverb_.SetLpFreq(8000.0f);
    reverb_.SetFeedback(0.5f);
}

void Engine::setParameters(const Parameters &params)
{
    setAccent(params.accent);
    setBrightness(params.brightness);
    setDamping(params.damping);
    setStructure(params.structure);
    setReverbFeedback(params.reverbFb);
    setReverbMix(params.reverbMix);
    setVolume(params.volume);
}

void Engine::setAccent(const float accent)
{
    params_.accent = unitclamp(accent);
}

void Engine::setBrightness(const float brightness)
{
    params_.brightness = unitclamp(brightness);
}

void Engine::setDamping(const float damping)
{
    params_.damping = unitclamp(damping);
}

void Engine::setStructure(const float structure)
{
    params_.structure = unitclamp(structure);
}

void Engine::setReverbFeedback(const float reverbFb)
{
    params_.reverbFb = unitclamp(reverbFb);
    reverb_.SetFeedback(fmap(params_.reverbFb, 0.5f, 1.0f));
}

void Engine::setReverbMix(const float reverbMix)
{
    params_.reverbMix = unitclamp(reverbMix);
}

void Engine::setVolume(const float volume)
{
    params_.volume = unitclamp(volume);
}

size_t Engine::countLiveNotes() const
{
    size_t liveNotes = 0;
    for (size_t voice = 0; voice < kNumberLiveVoices; ++voice)
    {
        for (size_t pad = 0; pad < kNumberPads; ++pad)
        {
            if (padStates_[voice][pad].recording)
            {
                ++liveNotes;
            }
        }
    }
    return liveNotes;
}

void Engine::triggerNotePlaybackWrapper(const size_t length,
                                        const uint8_t instance,
                                        const uint8_t pad,
                                        PadPlaybackState &state)
{
    // Validate length and indices
    if (length == 0u || instance >= kNumberLiveVoices || pad >= kNumberPads)
    {
        state.active = false;
        state.fromCache = false;
        state.playbackIndex = 0;
        state.playbackBuffer = nullptr;
        state.playbackLength = 0;
        return;
    }

    // Set up pad state for playback from cache
    state.active = true;
    state.age += 1;
    state.fromCache = true;
    state.recording = false;
    state.playbackIndex = 0;
    state.playbackBuffer = noteCacheData[instance][pad];
    state.playbackLength = length;
}

void Engine::triggerNoteLiveNoteWrapper(const uint8_t instance,
                                        const uint8_t targetNote,
                                        const int pad,
                                        PadPlaybackState &state)
{
    auto humanize = [](float value)
    {
        float deviation = rand() / float(RAND_MAX);
        deviation = map(deviation, 0.0f, 1.0f, 0.85f, 1.15f);
        return unitclamp(value * deviation);
    };

    const float brightness = humanize(params_.brightness);
    const float structure = humanize(params_.structure);
    const float damping = humanize(params_.damping);
    const float accent = humanize(params_.accent);
    const float bowLength = humanize(1.0f) * kMaxBowLengthSec;

    strings_[instance].setBrightness(brightness);
    strings_[instance].setStructure(structure);
    strings_[instance].setDamping(damping);

    Vox::BowParameters bow{};
    // Only enable bowed excitation for the last live voice.
    if (instance == kNumberLiveVoices - 1)
    {
        bow.bowSeconds = bowLength;
        bow.bowStrength = accent;
    }

    strings_[instance].NoteOn(mtof(targetNote), accent, bow);

    // Set up pad state for recording
    state.active = true;
    state.age = 0;
    state.recording = true;
    state.fromCache = false;
    state.writeIndex = 0;
    state.playbackIndex = 0;
    state.playbackBuffer = nullptr;
    state.playbackLength = 0;
    // Invalidate previous cache entry
    noteCacheValid[instance][pad] = false;
    noteCacheLength[instance][pad] = 0;
    noteCacheMidi[instance][pad] = targetNote;
}

void Engine::triggerNote(const uint8_t instance, const uint8_t pad)
{
    if (instance >= kNumberLiveVoices || pad >= kNumberPads)
    {
        // Invalid instance or pad
        return;
    }

    auto &state = padStates_[instance][pad];
    const uint8_t previousAge = state.age;
    state = {};
    state.age = previousAge;
    state.playbackBuffer = nullptr;
    state.playbackLength = 0;

    const uint8_t targetNote = scales_[instance][pad];

    // If too many live notes, or cached note available, use cache
    const bool cacheReady = ((countLiveNotes() >= kNumberLiveVoices) ||
                             (noteCacheValid[instance][pad] &&
                              noteCacheMidi[instance][pad] == targetNote &&
                              state.age < kNumberRetriggers)
                            );

    if (cacheReady)
    {
        const size_t length = noteCacheLength[instance][pad];
        // If cache entry valid, trigger playback from cache
        if (length > 0U)
        {
            triggerNotePlaybackWrapper(length, instance, pad, state);
            return;
        }
    }

    // Fall back to live synthesis and capture the note if above criteria not met
    triggerNoteLiveNoteWrapper(instance, targetNote, pad, state);
}

void Engine::processAudioSample(float &outL, float &outR)
{
    float dryL = 0.0f;
    float dryR = 0.0f;

    for (size_t voice = 0; voice < kNumberMprInstances; ++voice)
    {
        for (size_t pad = 0; pad < kNumberPads; ++pad)
        {
            auto &state = padStates_[voice][pad];
            float sampleValue = 0.0f;

            if (state.recording)
            {
                // Recording live synthesized note into cache
                if (state.writeIndex < kCacheFrames)
                {
                    const float liveSample = strings_[voice].processAudioSample();
                    noteCacheData[voice][pad][state.writeIndex] = liveSample;
                    ++state.writeIndex;
                    noteCacheLength[voice][pad] = state.writeIndex;
                    sampleValue = liveSample;
                    state.playbackBuffer = noteCacheData[voice][pad];
                    state.playbackLength = state.writeIndex;
                }
                // Finished recording, mark cache as valid
                else
                {
                    noteCacheValid[voice][pad] = true;
                    state.recording = false;
                    state.active = false;
                    state.fromCache = false;
                    state.playbackIndex = 0;
                    state.playbackBuffer = nullptr;
                    state.playbackLength = 0;
                }
            }
            // Playback from cache
            else if (state.active && state.fromCache)
            {
                if (state.playbackBuffer != nullptr)
                {
                    if (state.playbackIndex < state.playbackLength)
                    {
                        sampleValue = state.playbackBuffer[state.playbackIndex++];
                    }
                    // Finished playback
                    else
                    {
                        state.active = false;
                        state.fromCache = false;
                        state.playbackBuffer = nullptr;
                        state.playbackLength = 0;
                        state.playbackIndex = 0;
                    }
                }
                // Invalid playback buffer
                else
                {
                    state.active = false;
                    state.fromCache = false;
                    state.playbackBuffer = nullptr;
                    state.playbackLength = 0;
                    state.playbackIndex = 0;
                }
            }

            // Mix the sample if active
            if (state.active && sampleValue != 0.0f)
            {
                dryL += sampleValue;
                dryR += sampleValue;
            }
        }
    }

    float verbL = 0.0f;
    float verbR = 0.0f;

    // Process reverb
    reverb_.Process(dryL, dryR, &verbL, &verbR);

    // Apply reverb mix
    outL = lerp(dryL, verbL, params_.reverbMix);
    outR = lerp(dryR, verbR, params_.reverbMix);

    // Apply volume
    outL *= params_.volume;
    outR *= params_.volume;
}
