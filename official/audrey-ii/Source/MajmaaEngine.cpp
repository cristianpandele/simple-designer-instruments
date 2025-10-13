#include "MajmaaEngine.h"
#include "Utils.h"

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisysp;

static DSY_SDRAM_BSS daisysp::ReverbSc reverb_;

void Engine::init(const float sampleRate) {
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

  reverb_.Init(sampleRate);
  reverb_.SetFeedback(0.85f);
  reverb_.SetLpFreq(12000.0f);
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
}

void Engine::setReverbMix(const float reverbMix)
{
  params_.reverbMix = unitclamp(reverbMix);
}

void Engine::triggerNote(const uint8_t instance, const uint8_t pad)
{
  // If the lowest note of the instrument, trigger the note on the first voice
  if (pad == 0)
  {
    // Humanize parameters by adding a small random deviation
    auto humanize = [](float value) {
      float deviation = rand() / float(RAND_MAX);
      deviation = map(deviation, 0.0f, 1.0f, 0.85f, 1.15f);
      return unitclamp(value * deviation);
    };

    float brightness = humanize(params_.brightness);
    float structure = humanize(params_.structure);
    float damping = humanize(params_.damping);
    float accent = humanize(params_.accent);

    strings_[instance].setBrightness(brightness);
    strings_[instance].setStructure(structure);
    strings_[instance].setDamping(damping);
    strings_[instance].NoteOn(mtof(scales_[instance][pad]), accent);
  }
}

void Engine::processAudioSample(float &outL, float &outR) {
  // --- processAudioSample Samples ---
  float sampL, sampR, verbL, verbR;

  for (size_t voice = 0; voice < kNumberVoices; voice++)
  {
    for (size_t pad = 0; pad < 1/*kNumberPads*/; pad++)
    {
      // processAudioSample through KS resonator
      sampL += strings_[voice].processAudioSample();
      sampR += strings_[voice].processAudioSample();
    }
  }

  // ---> Reverb
  reverb_.Process(sampL, sampR, &verbL, &verbR);

  // ---> Output
  outL = lerp(sampL, verbL, reverbMix);
  outR = lerp(sampR, verbR, reverbMix);
}
