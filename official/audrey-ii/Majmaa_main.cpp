#include "Majmaa_main.h"

using namespace majmaa;
using namespace daisy;
using namespace daisysp;

#if DEBUG
CpuLoadMeter loadMeter;
#endif

void AudioCallback(AudioHandle::InputBuffer /* in */, AudioHandle::OutputBuffer out, size_t size)
{
#if DEBUG
    loadMeter.OnBlockStart();
#endif
    // Get, smooth and set the current values of all analog controls
    handleAnalogControls(controls, engine);

    for (size_t i=0; i<size; i++) {
        engine.Process(OUT_L[i], OUT_R[i]);
    }
    limiter[0].ProcessBlock(OUT_L, size, 0.7f);
    limiter[1].ProcessBlock(OUT_R, size, 0.7f);
#if DEBUG
    loadMeter.OnBlockEnd();
#endif
}

void init()
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

void handleDigitalControls(Controls &controls)
{
    for (size_t instance = 0; instance < Controls::kNumMprInstances; instance++)
    {
        padTouchStates[instance] = controls.GetMpr121TouchStates(instance);

        for (size_t i = 0; i < 16; i++)
        {
            if (hasTouchStateChangedToPressed(padTouchStates[instance], padTouchStatesPrev[instance], i))
            {
                Log::PrintLine("Pad %d on instance %d pressed", i, instance);
            }
        }
        // Update the previous touch states
        padTouchStatesPrev[instance] = padTouchStates[instance];
    }
}

#if DEBUG
void logDebugInfo()
{
    Log::PrintLine("Accent          : " FLT_FMT(5), FLT_VAR(5, accentKnobVal.getSmoothVal()));
    Log::PrintLine("Damping         : " FLT_FMT(5), FLT_VAR(5, dampingKnobVal.getSmoothVal()));
    Log::PrintLine("Brightness      : " FLT_FMT(5), FLT_VAR(5, brightnessKnobVal.getSmoothVal()));
    Log::PrintLine("Structure       : " FLT_FMT(5), FLT_VAR(5, structureKnobVal.getSmoothVal()));
    Log::PrintLine("Reverb Fb       : " FLT_FMT(5), FLT_VAR(5, reverbFbKnobVal.getSmoothVal()));
    Log::PrintLine("Reverb Mix      : " FLT_FMT(5), FLT_VAR(5, reverbMixKnobVal.getSmoothVal()));
    // Log::PrintLine("String Freq   : " FLT_FMT(5), FLT_VAR(5, engine.getStringFreq()));

#ifdef PRINT_CPU_LOAD
    // get the current load (smoothed value and peak values)
    const float avgLoad = loadMeter.GetAvgCpuLoad();
    const float maxLoad = loadMeter.GetMaxCpuLoad();
    const float minLoad = loadMeter.GetMinCpuLoad();
    // print it to the serial connection (as percentages)
    Log::PrintLine("Processing Load (%%):");
    Log::PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
    Log::PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
    Log::PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
#endif
}
#endif