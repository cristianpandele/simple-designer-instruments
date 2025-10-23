#include "MajmaaEngine.h"
#include "Utils.h"

#include <cmath>
#include <cstdlib>

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisysp;

static DSY_SDRAM_BSS daisysp::ReverbSc reverb_;

namespace
{
  constexpr size_t kCacheVoices = Engine::kNumberLiveVoices;
  constexpr size_t kCachePads = Engine::kNumberPads;
  constexpr size_t kCacheFrames = Engine::kMaxSampleFrames;
  constexpr size_t kSemitoneRadius = 3; // Max pitch shift steps from cached note
  constexpr float kPi = 3.14159265358979323846f;

  static DSY_SDRAM_BSS float noteCacheData[kCacheVoices][kCachePads][kCacheFrames];
  static size_t noteCacheLength[kCacheVoices][kCachePads];
  static uint8_t noteCacheMidi[kCacheVoices][kCachePads];
  static bool noteCacheValid[kCacheVoices][kCachePads];

  inline float ClampFloat(const float value, const float minValue, const float maxValue)
  {
    if (value < minValue)
    {
      return minValue;
    }
    if (value > maxValue)
    {
      return maxValue;
    }
    return value;
  }

  size_t buildKernel(float* kernel,
                    size_t maxTaps,
                    size_t requestedTaps,
                    float normalizedCutoff)
  {
    if (kernel == nullptr || maxTaps == 0u)
    {
      return 0u;
    }

    if (requestedTaps < 3u)
    {
      requestedTaps = 3u;
    }

    if ((requestedTaps & 1u) == 0u)
    {
      ++requestedTaps;
    }

    if (requestedTaps > maxTaps)
    {
      requestedTaps = maxTaps;
      if ((requestedTaps & 1u) == 0u && requestedTaps > 0u)
      {
        --requestedTaps;
      }
    }

    normalizedCutoff = ClampFloat(normalizedCutoff, 0.01f, 0.99f);

    const size_t mid = requestedTaps / 2u;
    float sum = 0.0f;

    for (size_t i = 0; i < requestedTaps; ++i)
    {
      const float idx = static_cast<float>(i);
      const float n = idx - static_cast<float>(mid);
      const float window = 0.42f - 0.5f * cosf((2.0f * kPi * idx) / static_cast<float>(requestedTaps - 1u))
                          + 0.08f * cosf((4.0f * kPi * idx) / static_cast<float>(requestedTaps - 1u));
      const float argument = kPi * n * normalizedCutoff;
      const float absArgument = argument < 0.0f ? -argument : argument;
      const float sinc = absArgument < 1e-6f ? 1.0f : sinf(argument) / argument;
      const float value = window * sinc;
      kernel[i] = value;
      sum += value;
    }

    if (sum != 0.0f)
    {
      const float invSum = 1.0f / sum;
      for (size_t i = 0; i < requestedTaps; ++i)
      {
        kernel[i] *= invSum;
      }
    }

    return requestedTaps;
  }

  float lookupKernel(const float* kernel, size_t kernelSize, float position)
  {
    if (kernel == nullptr || kernelSize == 0u)
    {
      return 0.0f;
    }

    const float center = static_cast<float>(kernelSize - 1u) * 0.5f;
    float index = position + center;
    const float maxIndex = static_cast<float>(kernelSize - 1u);
    if (index < 0.0f)
    {
      index = 0.0f;
    }
    else if (index > maxIndex)
    {
      index = maxIndex;
    }

    const size_t lower = static_cast<size_t>(index);
    const size_t upper = (lower + 1u) < kernelSize ? lower + 1u : kernelSize - 1u;
    const float frac = index - static_cast<float>(lower);
    return kernel[lower] + (kernel[upper] - kernel[lower]) * frac;
  }

  int findClosestCachedPad(size_t instance, uint8_t midiNote, bool force = false)
  {
    if (instance >= kCacheVoices)
    {
      return -1;
    }

    int bestPad = -1;
    int bestDistance = 127;

    for (size_t candidate = 0; candidate < kCachePads; ++candidate)
    {
      if (!noteCacheValid[instance][candidate])
      {
        continue;
      }

      const int distance = static_cast<int>(noteCacheMidi[instance][candidate]) - static_cast<int>(midiNote);
      const int absDistance = distance < 0 ? -distance : distance;
      if (force || ((absDistance <= static_cast<int>(kSemitoneRadius)) && (absDistance < bestDistance)))
      {
        bestDistance = absDistance;
        bestPad = static_cast<int>(candidate);
        if (absDistance == 0)
        {
          break;
        }
      }
    }

    return bestPad;
  }
} // namespace

void Engine::init(const float sampleRate) {
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
        state.basePad = static_cast<uint8_t>(pad);
        state.playbackBuffer = nullptr;
        state.playbackLength = 0;
      }
    }
  }

  reverb_.Init(sampleRate);
  reverb_.SetLpFreq(8000.0f);
  reverb_.SetFeedback(0.5f);
}

void Engine::setParameters(const Parameters &params) {
  setAccent(params.accent);
  setBrightness(params.brightness);
  setDamping(params.damping);
  setStructure(params.structure);
  setReverbFeedback(params.reverbFb);
  setReverbMix(params.reverbMix);
  setVolume(params.volume);
}

void Engine::setAccent(const float accent) {
  params_.accent = unitclamp(accent);
}

void Engine::setBrightness(const float brightness) {
  params_.brightness = unitclamp(brightness);
}

void Engine::setDamping(const float damping) {
  params_.damping = unitclamp(damping);
}

void Engine::setStructure(const float structure) {
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

void Engine::triggerNoteResampleWrapper(const size_t length,
                                        const uint8_t instance,
                                        const uint8_t targetNote,
                                        const int closestPad,
                                        PadPlaybackState &state)
{
  PadPlaybackState::ResampleState &resample = state.resample;
  resample = {};

  resample.source = noteCacheData[instance][closestPad];
  resample.sourceLength = length;

  const float sourceFreq = mtof(noteCacheMidi[instance][closestPad]);
  const float targetFreq = mtof(targetNote);
  const float rawPitch = (sourceFreq <= 0.0f) ? 1.0f : (targetFreq / sourceFreq);
  const float safePitch = rawPitch <= 0.0f ? 1.0f : rawPitch;

  resample.pitchRatio = safePitch;
  resample.kernelSize = buildKernel(resample.kernel,
                                    kMaxKernelTaps,
                                    kResampleKernelTaps,
                                    kResampleCutoff);

  if (resample.kernelSize == 0u || resample.source == nullptr)
  {
    state.active = false;
    state.fromCache = false;
    state.recording = false;
    state.playbackIndex = 0;
    state.playbackBuffer = nullptr;
    state.playbackLength = 0;
    state.age = 0;
    return;
  }

  const size_t halfKernel = resample.kernelSize / 2u;
  if (length <= (halfKernel + 1u))
  {
    state.active = false;
    state.fromCache = false;
    state.recording = false;
    state.playbackIndex = 0;
    state.playbackBuffer = nullptr;
    state.playbackLength = 0;
    resample.active = false;
    return;
  }

  resample.phase = static_cast<float>(rand() & 0x3FF) / 1024.0f;
  resample.maxPhase = static_cast<float>(length - halfKernel - 1u);
  if (resample.phase > resample.maxPhase)
  {
    resample.phase = 0.0f;
  }

  const float effectiveRate = sampleRate_ * safePitch;
  const float minRate = (effectiveRate < sampleRate_) ? effectiveRate : sampleRate_;
  const float nyquist = 0.5f * (minRate > 0.0f ? minRate : sampleRate_);
  float cutoff = nyquist * kResampleCutoff;
  const float maxCutoff = 0.49f * sampleRate_;
  if (cutoff > maxCutoff)
  {
    cutoff = maxCutoff;
  }

  if (cutoff <= 0.0f)
  {
    resample.lowpassAlpha = 0.0f;
    resample.lowpassA0 = 1.0f;
    resample.lowpassState = 0.0f;
  }
  else
  {
    const float alpha = expf((-2.0f * kPi * cutoff) / sampleRate_);
    resample.lowpassAlpha = alpha;
    resample.lowpassA0 = 1.0f - alpha;
    resample.lowpassState = 0.0f;
  }

  resample.active = true;

  state.playbackBuffer = nullptr;
  state.playbackLength = length;
  state.active = true;
  state.age += 1;
  state.fromCache = true;
  state.recording = false;
  state.playbackIndex = 0;
  state.basePad = static_cast<uint8_t>(closestPad);
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

  strings_[instance].setBrightness(brightness);
  strings_[instance].setStructure(structure);
  strings_[instance].setDamping(damping);
  strings_[instance].NoteOn(mtof(targetNote), accent);

  state.active = true;
  state.age = 0;
  state.recording = true;
  state.fromCache = false;
  state.writeIndex = 0;
  state.playbackIndex = 0;
  state.playbackBuffer = nullptr;
  state.playbackLength = 0;
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
  state.basePad = pad;
  state.playbackBuffer = nullptr;
  state.playbackLength = 0;

  const uint8_t targetNote = scales_[instance][pad];
  int closestPad = -1;
  bool forceSearch = false;

  if (countLiveNotes() >= kNumberLiveVoices)
  {
    // Log::PrintLine("Cannot synthesize note for pad %d on instance %d: maximum live notes reached!", pad, instance);
    forceSearch = true;
  }

  closestPad = findClosestCachedPad(static_cast<size_t>(instance), targetNote, forceSearch);
  if ((closestPad >= 0) && (state.age < kNumberRetriggers))
  {
    const size_t length = noteCacheLength[instance][closestPad];
    if (length > 0U)
    {
      // Log::PrintLine("Resampling note cached for pad %d on instance %d, age: %d!", closestPad, instance, state.age);
      triggerNoteResampleWrapper(length, instance, targetNote, closestPad, state);
      return;
    }
  }

  // Fall back to live synthesis and capture the note
  // Log::PrintLine("Synthesizing note for %d on instance %d!", pad, instance);
  if (countLiveNotes() < kNumberLiveVoices)
  {
    triggerNoteLiveNoteWrapper(instance, targetNote, pad, state);
  }
}

void Engine::processAudioSample(float &outL, float &outR) {
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
      else if (state.active && state.fromCache)
      {
        if (state.resample.active)
        {
          float resampledSample = 0.0f;
          if (renderResampledNote(state, resampledSample))
          {
            sampleValue = resampledSample;
          }
          else
          {
            state.active = false;
            state.playbackBuffer = nullptr;
            state.playbackLength = 0;
          }
        }
        else if (state.playbackBuffer != nullptr)
        {
          if (state.playbackIndex < state.playbackLength)
          {
            sampleValue = state.playbackBuffer[state.playbackIndex++];
          }
          else
          {
            state.active = false;
            state.playbackBuffer = nullptr;
            state.playbackLength = 0;
          }
        }
        else
        {
          state.active = false;
          state.playbackBuffer = nullptr;
          state.playbackLength = 0;
        }
      }

      if (state.active && sampleValue != 0.0f)
      {
        dryL += sampleValue;
        dryR += sampleValue;
      }
    }
  }

  float verbL = 0.0f;
  float verbR = 0.0f;
  reverb_.Process(dryL, dryR, &verbL, &verbR);

  // Apply reverb mix
  outL = lerp(dryL, verbL, params_.reverbMix);
  outR = lerp(dryR, verbR, params_.reverbMix);

  // Apply volume
  outL *= params_.volume;
  outR *= params_.volume;
}

bool Engine::renderResampledNote(PadPlaybackState &state,
                                 float &outSample,
                                 size_t numTaps,
                                 float normalizedCutoff) const
{
  PadPlaybackState::ResampleState &resample = state.resample;

  if (!resample.active || resample.source == nullptr || resample.sourceLength == 0u)
  {
    resample.active = false;
    return false;
  }

  if (resample.kernelSize == 0u)
  {
    resample.kernelSize = buildKernel(resample.kernel,
                                      kMaxKernelTaps,
                                      numTaps,
                                      normalizedCutoff);
    if (resample.kernelSize == 0u)
    {
      resample.active = false;
      return false;
    }

    const size_t halfKernel = resample.kernelSize / 2u;
    if (resample.sourceLength <= (halfKernel + 1u))
    {
      resample.active = false;
      return false;
    }

    resample.maxPhase = static_cast<float>(resample.sourceLength - halfKernel - 1u);
    if (resample.phase > resample.maxPhase)
    {
      resample.phase = 0.0f;
    }
  }

  if (resample.phase > resample.maxPhase)
  {
    resample.active = false;
    return false;
  }

  const size_t halfKernel = resample.kernelSize / 2u;
  const size_t indexInt = static_cast<size_t>(resample.phase);
  const float frac = resample.phase - static_cast<float>(indexInt);
  float acc = 0.0f;

  for (int tap = -static_cast<int>(halfKernel); tap <= static_cast<int>(halfKernel); ++tap)
  {
    int sampleIndex = static_cast<int>(indexInt) + tap;
    if (sampleIndex < 0)
    {
      sampleIndex = 0;
    }
    else if (sampleIndex >= static_cast<int>(resample.sourceLength))
    {
      sampleIndex = static_cast<int>(resample.sourceLength) - 1;
    }

    const float weight = lookupKernel(resample.kernel,
                                      resample.kernelSize,
                                      static_cast<float>(tap) - frac);
    acc += resample.source[static_cast<size_t>(sampleIndex)] * weight;
  }

  float filtered = acc;
  if (resample.lowpassA0 != 1.0f || resample.lowpassAlpha != 0.0f)
  {
    filtered = resample.lowpassA0 * acc + resample.lowpassAlpha * resample.lowpassState;
    resample.lowpassState = filtered;
  }

  resample.phase += resample.pitchRatio;
  if (resample.phase > resample.maxPhase)
  {
    resample.active = false;
  }

  outSample = filtered;
  return true;
}
