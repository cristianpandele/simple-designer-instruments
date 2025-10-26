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

    for (size_t i = 0; i < size; i++)
    {
        engine.processAudioSample(OUT_L[i], OUT_R[i]);
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

    engine.init(hw.AudioSampleRate());
    controls.init(hw, engine);

    for (auto &lim : limiter)
    {
        lim.Init();
    }

#if DEBUG
    Log::StartLog(false);
    log_timer.Init();
    touch_timer.Init();
    loadMeter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

    Log::PrintLine("Initializing the individual pad sounds...");
#endif

    // Pre-trigger every pad to warm caches before audio starts
    for (uint8_t voice = 0; voice < Engine::kNumberLiveVoices; ++voice)
    {
        for (uint8_t pad = 0; pad < Engine::kNumberPads; ++pad)
        {
            engine.triggerNote(voice, pad);
        }
    }

    hw.StartAudio(AudioCallback);
}

int main(void)
{
    // Initialize the hardware and the engine
    init();

    while (1)
    {
        // Debug logging
        if (log_timer.HasPassedMs(kDebugLogPeriodMs))
        {
#if DEBUG
            logDebugInfo();
#endif
            log_timer.Restart();
        }

        if (touch_timer.HasPassedMs(kTouchRegisterPeriodMs))
        {
            // Touch pad registration
            handleDigitalControls(controls);
            touch_timer.Restart();
        }
    }
}

void handleAnalogControls(Controls &controls, Engine &engine)
{
    // Lambda function: Leave a margin at the low and high ends of the pots
    auto mapControlValue = [](float value)
    {
        return unitclamp(map(value, 0.05f, 0.93f, 0.0f, 1.0f));
    };

    // Smooth Values
    accentKnobVal = mapControlValue(hw.adc.GetFloat(0));
    brightnessKnobVal = mapControlValue(hw.adc.GetFloat(1));
    dampingKnobVal = mapControlValue(hw.adc.GetFloat(2));
    structureKnobVal = mapControlValue(hw.adc.GetFloat(3));
    reverbFbKnobVal = mapControlValue(hw.adc.GetFloat(4));
    reverbMixKnobVal = mapControlValue(hw.adc.GetFloat(5));
    volumeKnobVal = mapControlValue(hw.adc.GetFloat(6));

    // Only update the engine if any of the values have changed (and thus values are smoothing)
    if (accentKnobVal.isSmoothing() || brightnessKnobVal.isSmoothing() || dampingKnobVal.isSmoothing() ||
        structureKnobVal.isSmoothing() || reverbFbKnobVal.isSmoothing() || reverbMixKnobVal.isSmoothing() ||
        volumeKnobVal.isSmoothing())
    {
        Engine::Parameters params = {.accent = accentKnobVal.getSmoothVal(),
                                     .brightness = brightnessKnobVal.getSmoothVal(),
                                     .damping = dampingKnobVal.getSmoothVal(),
                                     .structure = structureKnobVal.getSmoothVal(),
                                     .reverbFb = reverbFbKnobVal.getSmoothVal(),
                                     .reverbMix = reverbMixKnobVal.getSmoothVal(),
                                     .volume = volumeKnobVal.getSmoothVal()};
        engine.setParameters(params);
    }
}

void handleDigitalControls(Controls &controls)
{
    for (size_t instance = 0; instance < Controls::kNumMprInstances; instance++)
    {
        padTouchStates[instance] = controls.GetMpr121TouchStates(instance);

        for (size_t i = 0; i < Controls::kNumMprPads; i++)
        {
            if (hasTouchStateChangedToPressed(padTouchStates[instance], padTouchStatesPrev[instance], i))
            {
#if DEBUG
                Log::PrintLine("Pad %d on instance %d pressed", i, instance);
#endif
                engine.triggerNote(instance, i);
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
    Log::PrintLine("Volume          : " FLT_FMT(5), FLT_VAR(5, volumeKnobVal.getSmoothVal()));
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