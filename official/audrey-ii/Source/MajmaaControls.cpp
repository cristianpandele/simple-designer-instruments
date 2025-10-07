#include "MajmaaControls.h"
#include <functional>

using namespace majmaa;
using namespace majmaa::MajmaaSynth;
using namespace daisy;

////////////// SIMPLE X DAISY PINOUT CHEATSHEET ///////////////

// 3v3           29  |       |   20    AGND
// D15 / A0      30  |       |   19    OUT 01
// D16 / A1      31  |       |   18    OUT 00
// D17 / A2      32  |       |   17    IN 01
// D18 / A3      33  |       |   16    IN 00
// D19 / A4      34  |       |   15    D14
// D20 / A5      35  |       |   14    D13
// D21 / A6      36  |       |   13    D12
// D22 / A7      37  |       |   12    D11
// D23 / A8      38  |       |   11    D10
// D24 / A9      39  |       |   10    D9
// D25 / A10     40  |       |   09    D8
// D26           41  |       |   08    D7
// D27           42  |       |   07    D6
// D28 / A11     43  |       |   06    D5
// D29           44  |       |   05    D4
// D30           45  |       |   04    D3
// 3v3 Digital   46  |       |   03    D2
// VIN           47  |       |   02    D1
// DGND          48  |       |   01    D0

// TODO: Add footprint numbers to these

static constexpr Pin kAccentKnobAdcPin           = seed::A0;
static constexpr Pin kBrightnessKnobAdcPin       = seed::A1;
static constexpr Pin kDampingKnobAdcPin          = seed::A2;
static constexpr Pin kStructureKnobAdcPin        = seed::A3;
static constexpr Pin kI2CSdaPin                  = seed::D12;
static constexpr Pin kI2CSclPin                  = seed::D11;

void Controls::Init(DaisySeed &hw, Engine &engine) {
    params_.Init(hw.AudioSampleRate() / hw.AudioBlockSize());

    initADCs(hw);
    registerParams(engine);
}

void Controls::Update(DaisySeed &hw) {
    params_.UpdateNormalized(Parameter::Frequency,          1.0f - hw.adc.GetFloat(0));
    params_.UpdateNormalized(Parameter::FeedbackGain,       1.0f - hw.adc.GetFloat(1));
    params_.UpdateNormalized(Parameter::FeedbackBody,       1.0f - hw.adc.GetFloat(2));
    params_.UpdateNormalized(Parameter::FeedbackLPFCutoff,  1.0f - hw.adc.GetFloat(3));
    params_.UpdateNormalized(Parameter::FeedbackHPFCutoff,  1.0f - hw.adc.GetFloat(4));
    params_.UpdateNormalized(Parameter::ReverbMix,          1.0f - hw.adc.GetFloat(5));
    // Special mapping for reverb feedback/decay (anti-exponential tension curve)
    params_.UpdateNormalized(Parameter::ReverbDecay,        ftension(1.0f - hw.adc.GetFloat(6), -3.0f));
    params_.UpdateNormalized(Parameter::EchoDelaySend,      1.0f - hw.adc.GetFloat(7));

    float delay_norm = 1.0f - hw.adc.GetFloat(8);
    float delay_scale = /* del_sw_.Pressed() ? 0.5f : */ 1.0f;
    params_.UpdateNormalized(Parameter::EchoDelayTime, delay_norm * delay_scale);
    params_.UpdateNormalized(Parameter::EchoDelayFeedback,  1.0f - hw.adc.GetFloat(9));
    params_.UpdateNormalized(Parameter::OutputVolume,       1.0f - hw.adc.GetFloat(10));
}

void Controls::initADCs(DaisySeed &hw) {
    AdcChannelConfig config[kNumAdcChannels];

    config[0].InitSingle(kAccentKnobAdcPin);
    config[1].InitSingle(kBrightnessKnobAdcPin);
    config[2].InitSingle(kDampingKnobAdcPin);
    config[3].InitSingle(kStructureKnobAdcPin);

    hw.adc.Init(config, kNumAdcChannels);
    hw.adc.Start();
}

void Controls::registerParams(Engine &engine) {
    using namespace std::placeholders;

    // String freq/pitch as note number
    params_.Register(Parameter::Frequency, 40.0f, 16.0f, 72.0f,
        std::bind(&Engine::SetStringPitch, &engine, _1), 0.2f);

    // Feedback Gain in dbFS
    params_.Register(Parameter::FeedbackGain, -60.0f, -60.0f, 12.0f,
        std::bind(&Engine::SetFeedbackGain, &engine, _1));

    // Feedback body/delay in seconds
    params_.Register(Parameter::FeedbackBody, 0.001f, 0.001f, 0.1f,
        std::bind(&Engine::SetFeedbackDelay, &engine, _1), 1.0f, daisysp::Mapping::EXP);

    // Feedback filter cutoffs in hz
    params_.Register(Parameter::FeedbackLPFCutoff, 18000.0f, 100.0f, 18000.0f,
        std::bind(&Engine::SetFeedbackLPFCutoff, &engine, _1), 0.05f, daisysp::Mapping::LOG);
    params_.Register(Parameter::FeedbackHPFCutoff, 250.0f, 10.0f, 4000.0f,
        std::bind(&Engine::SetFeedbackHPFCutoff, &engine, _1), 0.05f, daisysp::Mapping::LOG);

    // Reverb Mix
    params_.Register(Parameter::ReverbMix, 0.0f, 0.0f, 1.0f,
        std::bind(&Engine::SetReverbMix, &engine, _1));

    // Reverb Feedback (input is mapped to anti-exponential on ADC read)
    params_.Register(Parameter::ReverbDecay, 0.2f, 0.2f, 1.0f,
        std::bind(&Engine::SetReverbFeedback, &engine, _1));

    // Echo Delay send
    params_.Register(Parameter::EchoDelaySend, 0.0f, 0.0f, 1.0f,
        std::bind(&Engine::SetEchoDelaySendAmount, &engine, _1), 0.05f, daisysp::Mapping::EXP);

    // Echo Delay time in s
    params_.Register(Parameter::EchoDelayTime, 0.5f, 0.05f, 5.0f,
        std::bind(&Engine::SetEchoDelayTime, &engine, _1), 0.1f, daisysp::Mapping::EXP);

    // Echo Delay feedback
    params_.Register(Parameter::EchoDelayFeedback, 0.0f, 0.0f, 1.5f,
        std::bind(&Engine::SetEchoDelayFeedback, &engine, _1));

    // Output level
    params_.Register(Parameter::OutputVolume, 0.5f, 0.0f, 1.0f,
        std::bind(&Engine::SetOutputLevel, &engine, _1), 0.05f, daisysp::Mapping::EXP);
}
