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
static constexpr Pin kReverbFbKnobAdcPin         = seed::A4;
static constexpr Pin kReverbMixKnobAdcPin        = seed::A5;
static constexpr Pin kI2CSdaPin                  = seed::D12;
static constexpr Pin kI2CSclPin                  = seed::D11;

void Controls::Init(DaisySeed &hw, Engine &engine) {
    // --- MPR121 (I2C) ---
    Mpr121I2C::Config mpr_cfg;
    const uint8_t mprAddr[kNumMprInstances] = {0x5A, 0x5C, 0x5B};
    mpr_cfg.transport_config.periph = I2CHandle::Config::Peripheral::I2C_1;
    mpr_cfg.transport_config.scl = kI2CSclPin;
    mpr_cfg.transport_config.sda = kI2CSdaPin;
    mpr_cfg.transport_config.speed = I2CHandle::Config::Speed::I2C_400KHZ;
    for (size_t i = 0; i < kNumMprInstances; i++)
    {
        if (i == 0)
        {
            mpr_cfg.transport_config.mode = I2CHandle::Config::Mode::I2C_MASTER;
        }
        else
        {
            mpr_cfg.transport_config.mode = I2CHandle::Config::Mode::I2C_SLAVE;
        }
        mpr_cfg.transport_config.dev_addr = mprAddr[i];
        if (mpr121_[i].Init(mpr_cfg) != Mpr121I2C::Result::OK)
        {
            while (1)
                ;
        }
    }

    initADCs(hw);
}

uint16_t Controls::GetMpr121TouchStates(uint8_t instance)
{
    return mpr121_[instance].Touched();
}

void Controls::initADCs(DaisySeed &hw) {
    AdcChannelConfig config[kNumAdcChannels];

    config[0].InitSingle(kAccentKnobAdcPin);
    config[1].InitSingle(kBrightnessKnobAdcPin);
    config[2].InitSingle(kDampingKnobAdcPin);
    config[3].InitSingle(kStructureKnobAdcPin);
    config[4].InitSingle(kReverbFbKnobAdcPin);
    config[5].InitSingle(kReverbMixKnobAdcPin);

    hw.adc.Init(config, kNumAdcChannels);
    hw.adc.Start();
}
