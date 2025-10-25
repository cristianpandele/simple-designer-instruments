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

  Vox() : _sample_rate{0.f}, _freq_mult{1.f}, _bow_samples_remaining{0}, _bowed{false} {}
  ~Vox() {}

  void init(float sample_rate) {
    _sample_rate = sample_rate;
    _osc.Init(sample_rate);
    _osc.SetSustain(false);
    _osc.SetAccent(0.0f);
    _bow_samples_remaining = 0;
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
      _osc.SetSustain(false);
      _osc.SetAccent(bow.bowStrength);
      _osc.Trig();
      return;
    }

    const float duration_s = bow.bowSeconds;
    const float sustain_level = fclamp(bow.bowStrength, 0.f, 1.f);

    _osc.SetSustain(true);
    _osc.SetAccent(sustain_level);

    const float total_samples = duration_s * _sample_rate;
    _bow_samples_remaining = total_samples > 0.f ? static_cast<uint32_t>(total_samples) : 0u;
    if (_bow_samples_remaining == 0u && total_samples > 0.f) {
      _bow_samples_remaining = 1u;
    }

    _osc.Trig();
  }

  void SetMult(const float value) { _freq_mult = value; }

  float processAudioSample() {
    const float sample = _osc.Process();

    if (_bowed && _bow_samples_remaining > 0u) {
      --_bow_samples_remaining;
      if (_bow_samples_remaining == 0u) {
        _osc.SetSustain(false);
        _osc.SetAccent(0.f);
        _bowed = false;
      }
    }

    return sample;
  }

private:
  float       _sample_rate;
  float       _freq_mult;
  uint32_t    _bow_samples_remaining;
  bool        _bowed;
  StringVoice _osc;
};
