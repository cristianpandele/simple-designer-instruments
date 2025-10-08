#include "Majmaa_main.h"

using namespace majmaa;
using namespace MajmaaSynth;
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

void handleAnalogControls(Controls& controls, Engine& engine)
{
    // Get the current values of all analog controls
    controls.ProcessAnalogControls();
    // Smooth Values
    accentKnobVal     = controls.GetAnalogControlValue(Controls::AnalogControlId::Accent);
    brightnessKnobVal = controls.GetAnalogControlValue(Controls::AnalogControlId::Brightness);
    dampingKnobVal    = controls.GetAnalogControlValue(Controls::AnalogControlId::Damping);
    structureKnobVal  = controls.GetAnalogControlValue(Controls::AnalogControlId::Structure);
    reverbFbKnobVal   = controls.GetAnalogControlValue(Controls::AnalogControlId::ReverbFb);
    reverbMixKnobVal  = controls.GetAnalogControlValue(Controls::AnalogControlId::ReverbMix);

    // Only update the engine if any of the values have changed (and thus values are smoothing)
    if (accentKnobVal.isSmoothing() || brightnessKnobVal.isSmoothing() || dampingKnobVal.isSmoothing() ||
        structureKnobVal.isSmoothing() || reverbFbKnobVal.isSmoothing() || reverbMixKnobVal.isSmoothing())
    {
        Engine::Parameters params = {.accent = accentKnobVal.getSmoothVal(),
                                     .brightness = brightnessKnobVal.getSmoothVal(),
                                     .damping = dampingKnobVal.getSmoothVal(),
                                     .structure = structureKnobVal.getSmoothVal(),
                                     .reverbFb = reverbFbKnobVal.getSmoothVal(),
                                     .reverbMix = reverbMixKnobVal.getSmoothVal()};
        engine.SetParameters(params);
    }
}