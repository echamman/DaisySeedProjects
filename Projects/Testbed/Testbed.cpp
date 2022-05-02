#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Oscillator osc;
static Switch button1;
static Switch button2;
static Switch dip[4];

static GPIO disp[2];

enum AdcChannel {
    pitchKnob = 0,
    octKnob,
    gainKnob,
    NUM_ADC_CHANNELS
};

int wf1 = 0;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    float octave;
    for(size_t i = 0; i < size; i += 2)
    {
        //Get status of knobs and mute button, set frequency and amplitude
        button2.Debounce();
        dip[0].Debounce();
        float pKnob = hw.adc.GetFloat(pitchKnob);
        float oKnob = hw.adc.GetFloat(octKnob);
        float gKnob = hw.adc.GetFloat(gainKnob);

        //Mod the octave knob from 1 to 5
        octave = fmod((oKnob * 5.0f),5) + 1.0f;
        osc.SetFreq((pKnob * 32.70f*(pow(2,octave))) + 32.70f*(pow(2,octave)));

        //Dip 1 toggles mute functionality
        if(button2.Pressed() ^ dip[0].Pressed()){
            osc.SetAmp(0);
        }else{
            osc.SetAmp(gKnob);
        }

        //Wave Selector 
        button1.Debounce();
        if(button1.FallingEdge()){
            if(wf1 < 3){
                wf1++;
            }else{
                wf1 = 0;
            }
        }

        switch(wf1){
            case 0:
                osc.SetWaveform(osc.WAVE_SIN);
                disp[0].Write(false);
                disp[1].Write(false);
                break;
            case 1:
                osc.SetWaveform(osc.WAVE_SAW);
                disp[0].Write(true);
                disp[1].Write(false);
                break;
            case 2:
                osc.SetWaveform(osc.WAVE_SQUARE);
                disp[0].Write(false);
                disp[1].Write(true);
                break;
            case 3:
                osc.SetWaveform(osc.WAVE_TRI);
                disp[0].Write(true);
                disp[1].Write(true);
                break;
        }


        sig = osc.Process();

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);

    //LED Display
    disp[0].Init(Pin(PORTD, 11),GPIO::Mode::OUTPUT); //Pin D26
    disp[1].Init(Pin(PORTA, 0),GPIO::Mode::OUTPUT); //Pin D25

    //knobs
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
    adcConfig[pitchKnob].InitSingle(hw.GetPin(21));
    adcConfig[octKnob].InitSingle(hw.GetPin(20));
    adcConfig[gainKnob].InitSingle(hw.GetPin(19));
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    //buttons
    button1.Init(hw.GetPin(28),1000);
    button2.Init(hw.GetPin(27),1000);

    //DIPSwitch
    dip[0].Init(hw.GetPin(24),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[1].Init(hw.GetPin(23),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_NORMAL,Switch::Pull::PULL_UP);
    dip[2].Init(hw.GetPin(22),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[3].Init(hw.GetPin(18),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_NORMAL,Switch::Pull::PULL_UP);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(880);
    osc.SetAmp(0.25);


    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);


    while(1) {}
}
