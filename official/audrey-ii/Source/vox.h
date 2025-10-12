#pragma once

#include "daisysp.h"

using namespace daisysp;

class Vox {
public:
  Vox() : _freq_mult{1.f} {}
  ~Vox() {}

  void init(float sample_rate) { _osc.Init(sample_rate); }

  void setBrightness(const float value) {
    // With high brightness and pitch the osc can get unstable.
    _osc.SetBrightness(value * 0.5f);
  }

  void setStructure(const float value) { _osc.SetStructure(fmap(value, 0.f, 0.8f)); }

  void setDamping(const float value) { _osc.SetDamping(value); }

  void NoteOn(float freq, float acc = 1.0f) {
    _osc.SetFreq(freq * _freq_mult);
    _osc.SetAccent(acc);
    _osc.Trig();
  }

  void SetMult(const float value) { _freq_mult = value; }

  float processAudioSample() { return _osc.Process(); }

private:
  float       _freq_mult;
  StringVoice _osc;
};
