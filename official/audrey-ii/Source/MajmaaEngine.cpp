#include "MajmaaEngine.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisysp;

static DSY_SDRAM_BSS daisysp::ReverbSc reverb_;

namespace
{
  constexpr size_t kCacheVoices = Engine::kNumberLiveVoices;
  constexpr size_t kCachePads = Engine::kNumberPads;
  constexpr size_t kCacheFrames = 48000;
  constexpr size_t kSemitoneRadius = 3; // Max pitch shift steps from cached note
  constexpr float kPi = 3.14159265358979323846f;

  static DSY_SDRAM_BSS float noteCacheData[kCacheVoices][kCachePads][kCacheFrames];
  static size_t noteCacheLength[kCacheVoices][kCachePads];
  static uint8_t noteCacheMidi[kCacheVoices][kCachePads];
  static bool noteCacheValid[kCacheVoices][kCachePads];

  std::mt19937 &rng()
  {
    static thread_local std::mt19937 gen(std::random_device{}());
    return gen;
  }

  std::vector<float> buildKernel(size_t numTaps, float normalizedCutoff)
  {
    if (numTaps < 3)
    {
      numTaps = 3;
    }

    if ((numTaps % 2u) == 0u)
    {
      ++numTaps;
    }

    normalizedCutoff = std::clamp(normalizedCutoff, 0.01f, 0.99f);

    std::vector<float> kernel(numTaps, 0.0f);
    const size_t mid = numTaps / 2u;
    float sum = 0.0f;

    for (size_t i = 0; i < numTaps; ++i)
    {
      const float n = static_cast<float>(i) - static_cast<float>(mid);
      const float window = 0.42f - 0.5f * std::cos((2.0f * kPi * static_cast<float>(i)) / static_cast<float>(numTaps - 1u))
                          + 0.08f * std::cos((4.0f * kPi * static_cast<float>(i)) / static_cast<float>(numTaps - 1u));
      const float argument = kPi * n * normalizedCutoff;
      const float sinc = std::abs(argument) < 1e-6f ? 1.0f : std::sin(argument) / argument;
      const float value = window * sinc;
      kernel[i] = value;
      sum += value;
    }

    if (sum != 0.0f)
    {
      for (auto &coeff : kernel)
      {
        coeff /= sum;
      }
    }

    return kernel;
  }

  float lookupKernel(const std::vector<float> &kernel, float position)
  {
    if (kernel.empty())
    {
      return 0.0f;
    }

    const float center = static_cast<float>(kernel.size() - 1u) * 0.5f;
    float index = position + center;
    index = std::clamp(index, 0.0f, static_cast<float>(kernel.size() - 1u));

    const auto lower = static_cast<size_t>(std::floor(index));
    const auto upper = std::min(lower + 1u, kernel.size() - 1u);
    const float frac = index - static_cast<float>(lower);
    return kernel[lower] + (kernel[upper] - kernel[lower]) * frac;
  }

  float computeRms(const std::vector<float> &buffer)
  {
    if (buffer.empty())
    {
      return 0.0f;
    }

    const float energy = std::inner_product(buffer.begin(), buffer.end(), buffer.begin(), 0.0f);
    return std::sqrt(energy / static_cast<float>(buffer.size()));
  }

  void normalizeBuffer(std::vector<float> &buffer, float targetRms)
  {
    if (buffer.empty())
    {
      return;
    }

    const float currentRms = computeRms(buffer);
    if (currentRms <= 0.0f)
    {
      return;
    }

    const float gain = targetRms / currentRms;
    for (auto &sample : buffer)
    {
      sample *= gain;
    }
  }

  void applyOnePoleLowpass(std::vector<float> &buffer, float sampleRate, float cutoff)
  {
    if (buffer.empty() || sampleRate <= 0.0f || cutoff <= 0.0f)
    {
      return;
    }

    cutoff = std::min(cutoff, 0.49f * sampleRate);
    const float alpha = std::exp((-2.0f * kPi * cutoff) / sampleRate);
    const float a0 = 1.0f - alpha;
    const float b1 = alpha;

    float state = 0.0f;
    for (auto &sample : buffer)
    {
      state = a0 * sample + b1 * state;
      sample = state;
    }
  }

  int findClosestCachedPad(uint8_t instance, uint8_t midiNote)
  {
    int bestPad = -1;
    int bestDistance = 127;

    if (instance >= kCacheVoices)
    {
      return bestPad;
    }

    for (size_t candidate = 0; candidate < kCachePads; ++candidate)
    {
      if (!noteCacheValid[instance][candidate])
      {
        continue;
      }

      const int distance = std::abs(static_cast<int>(noteCacheMidi[instance][candidate]) - static_cast<int>(midiNote));
      if (distance <= static_cast<int>(kSemitoneRadius) && distance < bestDistance)
      {
        bestDistance = distance;
        bestPad = static_cast<int>(candidate);
        if (distance == 0)
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
      padStates_[inst][pad] = {};
      if (inst < kNumberLiveVoices && pad < kNumberPads)
      {
        padStates_[inst][pad] = {};
        padStates_[inst][pad].basePad = static_cast<uint8_t>(pad);
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

size_t Engine::countLiveNotes()
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
                                        const uint8_t targetMidi,
                                        const int closestPad,
                                        PadPlaybackState &state)
{
  std::vector<float> source(length);
  std::copy_n(noteCacheData[instance][closestPad], length, source.begin());
  const float sourceFreq = mtof(noteCacheMidi[instance][closestPad]);
  const float targetFreq = mtof(targetMidi);
  const float pitchRatio = (sourceFreq <= 0.0f) ? 1.0f : (targetFreq / sourceFreq);
  std::vector<float> resampled = {}; // renderResampledNote(source, sampleRate_, sampleRate_ * pitchRatio);
  std::copy(resampled.begin(), resampled.end(), state.resampledBuffer);
  state.active = !state.resampledBufferSize;
  state.fromCache = state.active;
  state.recording = false;
  state.playbackIndex = 0;
  state.basePad = static_cast<uint8_t>(closestPad);
}

void Engine::triggerNoteLiveNoteWrapper(const uint8_t instance,
                                        const uint8_t targetMidi,
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
  strings_[instance].NoteOn(mtof(targetMidi), accent);

  state.active = true;
  state.recording = true;
  state.fromCache = false;
  state.writeIndex = 0;
  state.playbackIndex = 0;
  noteCacheValid[instance][pad] = false;
  noteCacheLength[instance][pad] = 0;
  noteCacheMidi[instance][pad] = targetMidi;
}

void Engine::triggerNote(const uint8_t instance, const uint8_t pad)
{
  if (instance >= kNumberLiveVoices || pad >= kNumberPads)
  {
    return;
  }

  auto &state = padStates_[instance][pad];
  state = {};
  state.basePad = pad;

  const uint8_t targetMidi = scales_[instance][pad];
  const int closestPad = findClosestCachedPad(instance, targetMidi);

  if (closestPad >= 0)
  {
    const size_t length = noteCacheLength[instance][closestPad];
    if (length > 0U)
    {
      Log::PrintLine("Resampling note cached for pad %d on instance %d!", closestPad, instance);
      triggerNoteResampleWrapper(length, instance, targetMidi, closestPad, state);
      return;
    }
  }

  if (countLiveNotes() >=  kNumberLiveVoices)
  {
    Log::PrintLine("Cannot trigger note for pad %d on instance %d: maximum live notes reached!", pad, instance);
    return;
  }

  // Fall back to live synthesis and capture the note
  Log::PrintLine("Synthesizing note for %d on instance %d!", pad, instance);
  triggerNoteLiveNoteWrapper(instance, targetMidi, pad, state);
}

void Engine::processAudioSample(float &outL, float &outR) {
  // --- processAudioSample Samples ---
  float dryL = 0.0f;
  float dryR = 0.0f;

  for (size_t voice = 0; voice < kNumberLiveVoices; ++voice)
  {
    bool needsLiveSample = false;
    for (size_t pad = 0; pad < kNumberPads; ++pad)
    {
      if (padStates_[voice][pad].recording)
      {
        needsLiveSample = true;
        break;
      }
    }

    const float liveSample = needsLiveSample ? strings_[voice].processAudioSample() : 0.0f;

    for (size_t pad = 0; pad < kNumberPads; ++pad)
    {
      auto &state = padStates_[voice][pad];
      float sampleValue = 0.0f;

      if (state.recording)
      {
        if (state.writeIndex < kMaxSampleFrames)
        {
          noteCacheData[voice][pad][state.writeIndex] = liveSample;
          ++state.writeIndex;
          noteCacheLength[voice][pad] = state.writeIndex;
          sampleValue = liveSample;
        }

        if (state.writeIndex >= kMaxSampleFrames)
        {
          noteCacheValid[voice][pad] = true;
          state.recording = false;
          state.fromCache = true;
          state.playbackIndex = 0;
          // state.resampledBuffer = &noteCacheData[voice][pad][0];
          // state.resampledBufferSize = noteCacheLength[voice][pad];
        }
      }
      else if (state.active && state.fromCache)
      {
        if (state.playbackIndex < state.resampledBufferSize)
        {
          sampleValue = state.resampledBuffer[state.playbackIndex++];
        }
        else
        {
          state.active = false;
          state.resampledBufferSize = 0;
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

  const float mix = reverbMix;
  outL = lerp(dryL, verbL, mix);
  outR = lerp(dryR, verbR, mix);
}

std::vector<float> Engine::renderResampledNote(const std::vector<float> &noteBuffer,
                                               float srcSampleRate,
                                               float dstSampleRate,
                                               size_t numTaps,
                                               float normalizedCutoff) const
{
  if (noteBuffer.empty() || srcSampleRate <= 0.0f || dstSampleRate <= 0.0f)
  {
    return {};
  }

  auto kernel = buildKernel(numTaps, normalizedCutoff);
  const size_t halfTaps = kernel.size() / 2u;
  if (halfTaps == 0u || noteBuffer.size() <= halfTaps)
  {
    return noteBuffer;
  }

  const float pitchRatio = dstSampleRate / srcSampleRate;
  std::vector<float> output(noteBuffer.size(), 0.0f);

  std::uniform_real_distribution<float> phaseDistribution(0.0f, 1.0f);
  float phase = phaseDistribution(rng());
  const float maxPhase = static_cast<float>(noteBuffer.size() - halfTaps - 1u);

  for (size_t n = 0; n < output.size(); ++n)
  {
    const size_t indexInt = static_cast<size_t>(phase);
    const float frac = phase - static_cast<float>(indexInt);

    float acc = 0.0f;
    for (int tap = -static_cast<int>(halfTaps); tap <= static_cast<int>(halfTaps); ++tap)
    {
      int sampleIndex = static_cast<int>(indexInt) + tap;
      sampleIndex = std::clamp(sampleIndex, 0, static_cast<int>(noteBuffer.size() - 1u));
      const float weight = lookupKernel(kernel, static_cast<float>(tap) - frac);
      acc += noteBuffer[static_cast<size_t>(sampleIndex)] * weight;
    }

    output[n] = acc;
    phase += pitchRatio;

    if (phase >= maxPhase)
    {
      output.resize(n + 1u);
      break;
    }
  }

  const float targetRms = computeRms(noteBuffer);
  if (targetRms > 0.0f)
  {
    normalizeBuffer(output, targetRms);
  }

  const float nyquist = 0.5f * std::min(srcSampleRate, dstSampleRate);
  applyOnePoleLowpass(output, dstSampleRate, nyquist * normalizedCutoff);

  return output;
}
