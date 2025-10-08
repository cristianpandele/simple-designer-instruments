#include "Majmaa_main.h"

using namespace majmaa;
using namespace daisy;
using namespace daisysp;

void AudioCallback(AudioHandle::InputBuffer /* in */, AudioHandle::OutputBuffer out, size_t size)
{
    // Get, smooth and set the current values of all analog controls
    handleAnalogControls(controls, engine);

    for (size_t i=0; i<size; i++) {
        engine.Process(OUT_L[i], OUT_R[i]);
    }
    limiter[0].ProcessBlock(OUT_L, size, 0.7f);
    limiter[1].ProcessBlock(OUT_R, size, 0.7f);
}

int main(void)
{
    hw.Init();

    hw.SetAudioSampleRate(kSampleRate);
    hw.SetAudioBlockSize(kBlockSize);

    engine.Init(hw.AudioSampleRate());
    controls.Init(hw, engine);

    for(auto& lim : limiter)
    {
        lim.Init();
    }

    hw.StartAudio(AudioCallback);

    while(1) {}
}

void handleAnalogControls(MajmaaSynth::Controls& controls, MajmaaSynth::Engine& engine)
{
    // Get the current values of all analog controls
    controls.ProcessAnalogControls();
    // Smooth Values
    accentKnobVal     = controls.GetAnalogControlValue(MajmaaSynth::Controls::AnalogControlId::Accent);
    brightnessKnobVal = controls.GetAnalogControlValue(MajmaaSynth::Controls::AnalogControlId::Brightness);
    dampingKnobVal    = controls.GetAnalogControlValue(MajmaaSynth::Controls::AnalogControlId::Damping);
    structureKnobVal  = controls.GetAnalogControlValue(MajmaaSynth::Controls::AnalogControlId::Structure);

    // Only update the engine if any of the values have changed (and thus values are smoothing)
    if (accentKnobVal.isSmoothing() || brightnessKnobVal.isSmoothing() || dampingKnobVal.isSmoothing() ||
        structureKnobVal.isSmoothing())
    {
        engine.SetAccent(accentKnobVal.getSmoothVal());
        engine.SetBrightness(brightnessKnobVal.getSmoothVal());
        engine.SetDamping(dampingKnobVal.getSmoothVal());
        engine.SetStructure(structureKnobVal.getSmoothVal());
    }
}