#pragma once

#include "daisysp.h"

using namespace daisysp;

class Vox {
public:
  struct BowParameters {
    float bowSeconds;
    float bowStrength;

    BowParameters(float seconds = 0.0f, float strength = 1.0f)
        : bowSeconds(seconds), bowStrength(strength) {}
  };

  Vox()
      : _sample_rate{0.f}, _freq_mult{1.f}, _bow_samples_remaining{0},
        _bow_sustain_level{0.f}, _bow_gate{false}, _bowed{false} {}
  ~Vox() {}

  void init(float sample_rate) {
    _sample_rate = sample_rate;
    _osc.Init(sample_rate);
    _osc.SetSustain(false);
    _osc.SetAccent(0.0f);
    _bow_env.Init(sample_rate);
    _bow_env.SetSustainLevel(0.0f);
    _bow_samples_remaining = 0;
    _bow_sustain_level = 0.f;
    _bow_gate = false;
    _bowed = false;
  }

  void setBrightness(const float value) {
    // With high brightness and pitch the osc can get unstable.
    _osc.SetBrightness(value * 0.5f);
  }

  void setStructure(const float value) { _osc.SetStructure(fmap(value, 0.f, 0.8f)); }

  void setDamping(const float value) { _osc.SetDamping(value); }

  void NoteOn(float freq, float acc = 1.0f, BowParameters bow = BowParameters()) {
    _osc.SetFreq(freq * _freq_mult);
    _osc.SetAccent(acc);
    _bowed = (bow.bowSeconds > 0.f) && (bow.bowStrength > 0.f);
    if (!_bowed) {
      _bow_samples_remaining = 0u;
      _bow_sustain_level = 0.f;
      _bow_gate = false;
      _bow_env.SetSustainLevel(_bow_sustain_level);
      _osc.SetAccent(acc);
      _osc.Trig();
      return;
    }

    const float duration_s = bow.bowSeconds;
    const float sustain_level = fclamp(bow.bowStrength, 0.f, 1.f);

    // Calculate total samples for bow duration
    uint32_t total_samples = static_cast<uint32_t>(duration_s * _sample_rate);

    // Set up bow envelope
    _bow_samples_remaining = total_samples;
    _bow_sustain_level = sustain_level;
    const float attack_time = duration_s * 0.10f;
    const float decay_time = duration_s * 0.05f;
    const float release_time = duration_s * 0.20f;

    _bow_env.SetAttackTime(attack_time);
    _bow_env.SetDecayTime(decay_time);
    _bow_env.SetSustainLevel(_bow_sustain_level);
    _bow_env.SetReleaseTime(release_time);
    _bow_env.Retrigger(true);
    _bow_gate = true;

    _osc.SetSustain(true);
    _osc.SetAccent(0.f);
    _osc.Trig();
  }

  void SetMult(const float value) { _freq_mult = value; }

  float processAudioSample() {
    if (_bowed && _bow_samples_remaining > 0u) {
      float env = _bow_env.Process(_bow_gate);
      env = fclamp(env, 0.f, 1.f);
      _osc.SetAccent(env);
    }

    const float sample = _osc.Process();

    if (_bowed && _bow_samples_remaining > 0u) {
      --_bow_samples_remaining;
      if (_bow_samples_remaining == 0u) {
        _osc.SetSustain(false);
        _osc.SetAccent(0.f);
        _bow_env.SetSustainLevel(0.0f);
        _bow_gate = false;
        _bowed = false;
        _bow_sustain_level = 0.f;
      }
    }

    return sample;
  }

private:
  float       _sample_rate;
  float       _freq_mult;
  uint32_t    _bow_samples_remaining;
  float       _bow_sustain_level;
  bool        _bow_gate;
  Adsr        _bow_env;
  bool        _bowed;
  StringVoice _osc;
};
