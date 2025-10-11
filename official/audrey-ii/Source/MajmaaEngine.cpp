#include "MajmaaEngine.h"
#include "Utils.h"

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisysp;

static DSY_SDRAM_BSS daisysp::ReverbSc reverb_;

void Engine::Init(const float sample_rate) {
  sample_rate_ = sample_rate;

  for (unsigned int i = 0; i < 2; i++) {

    strings_[i].Init(sample_rate);
    strings_[i].SetBrightness(0.98f);
    // strings_[i].SetFreq(mtof(40.0f));
    strings_[i].SetDamping(0.4f);
  }

  reverb_.Init(sample_rate);
  reverb_.SetFeedback(0.85f);
  reverb_.SetLpFreq(12000.0f);
}

void Engine::SetStringPitch(const float nn) {
  const auto freq = mtof(nn);
  // strings_[0].SetFreq(freq);
  // strings_[1].SetFreq(freq);
}

void Engine::SetParameters(const Parameters &params) {
  SetAccent(params.accent);
  SetBrightness(params.brightness);
  SetDamping(params.damping);
  SetStructure(params.structure);
  SetReverbFeedback(params.reverbFb);
  SetReverbMix(params.reverbMix);
}

void Engine::SetAccent(const float accent) {
  params_.accent = unitclamp(accent);
}

void Engine::SetBrightness(const float brightness) {
  params_.brightness = unitclamp(brightness);
}

void Engine::SetDamping(const float damping) {
  params_.damping = unitclamp(damping);
}

void Engine::SetStructure(const float structure) {
  params_.structure = unitclamp(structure);
}

void Engine::SetReverbFeedback(const float reverbFb)
{
  params_.reverbFb = unitclamp(reverbFb);
}

void Engine::SetReverbMix(const float reverbMix)
{
  params_.reverbMix = unitclamp(reverbMix);
}

void Engine::Process(float &outL, float &outR) {
  // --- Process Samples ---

  float sampL, sampR, echoL, echoR, verbL, verbR;

  // Process through KS resonator
  sampL = strings_[0].Process();
  sampR = strings_[1].Process();

  // ---> Reverb

  reverb_.Process(sampL, sampR, &verbL, &verbR);

  //       (sampL * (1.0f - verb_mix_)) + verbL * verb_mix_;
  //       sampL - sampL * verb_mix + verbL * verb_mix_;
  sampL -= (sampL - verbL) * verb_mix_;
  sampR -= (sampR - verbR) * verb_mix_;

  sampL = 0.5f * (sampL + echoL);
  sampR = 0.5f * (sampR + echoR);

  // ---> Output
  outL = sampL * output_level_;
  outR = sampR * output_level_;
}
