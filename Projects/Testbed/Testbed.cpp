#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Oscillator osc;
Switch button1;

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

    for(size_t i = 0; i < size; i += 2)
    {
        float pKnob = hw.adc.GetFloat(pitchKnob);
        float oKnob = hw.adc.GetFloat(octKnob) * 440.0f;
        float gKnob = hw.adc.GetFloat(gainKnob);
        osc.SetFreq((pKnob * 440.0f) + oKnob);
        osc.SetAmp(gKnob);

        button1.Debounce();
        //osc.SetAmp(button1.Pressed());
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
                hw.SetLed(false);
                break;
            case 1:
                osc.SetWaveform(osc.WAVE_SAW);
                hw.SetLed(true);
                break;
            case 2:
                osc.SetWaveform(osc.WAVE_SQUARE);
                hw.SetLed(false);
                break;
            case 3:
                osc.SetWaveform(osc.WAVE_TRI);
                hw.SetLed(true);
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

    //knob
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
    adcConfig[pitchKnob].InitSingle(hw.GetPin(21));
    adcConfig[octKnob].InitSingle(hw.GetPin(20));
    adcConfig[gainKnob].InitSingle(hw.GetPin(19));
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    //button
    button1.Init(hw.GetPin(28),1000);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(880);
    osc.SetAmp(0.25);


    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);


    while(1) {}
}
