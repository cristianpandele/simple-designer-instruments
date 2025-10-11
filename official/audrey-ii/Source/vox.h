#pragma once

#include "daisysp.h"

using namespace daisysp;

class Vox {
public:
  Vox() : _freq_mult{1.f} {}
  ~Vox() {}

  void Init(float sample_rate) { _osc.Init(sample_rate); }

  void SetBrightness(const float value) {
    // With high brightness and pitch the osc can get unstable.
    _osc.SetBrightness(value * 0.5f);
  }

  void SetStructure(const float value) { _osc.SetStructure(fmap(value, 0.f, 0.8f)); }

  void SetDamping(const float value) { _osc.SetDamping(value); }

  void NoteOn(float freq, float acc = 1.0f) {
    _osc.SetFreq(freq * _freq_mult);
    _osc.SetAccent(acc);
    _osc.Trig();
  }

  void SetMult(const float value) { _freq_mult = value; }

  float Process() { return _osc.Process(); }

private:
  float       _freq_mult;
  StringVoice _osc;
};
