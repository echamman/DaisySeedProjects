#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Oscillator osc;
static Switch button1;
static Switch button2;
static Switch dip[4];  

static AdEnv env;
static Metro tick;

static GPIO disp[4];

enum AdcChannel {
    pitchKnob = 0,
    octKnob,
    gainKnob,
    NUM_ADC_CHANNELS
};

enum menu {
    basic = 0,
    fx,
    NUM_MENUS
};

int wf1 = 0;
int fCount = 0;
int menuScreen = basic;
bool menuChange = false;
float timeReleased = 0.0f;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    float octave;


    for(size_t i = 0; i < size; i += 2)
    {
        //Debounce buttons together
        button1.Debounce();
        button2.Debounce();
        dip[0].Debounce();
        dip[1].Debounce();
        dip[2].Debounce();
        dip[3].Debounce();

        //Get status of knobs and mute button, set frequency and amplitude
        float knob1 = hw.adc.GetFloat(pitchKnob);
        float knob2 = hw.adc.GetFloat(octKnob);
        float knob3 = hw.adc.GetFloat(gainKnob);

        //Count 0.8seconds hold on button1 to change the menu. Wait 200ms after releasing to allow button to resume function
        if(button1.TimeHeldMs() > 800.0f){
            if(!menuChange){
                menuScreen = (menuScreen + 1) % NUM_MENUS;
                menuChange = true;
            }
            timeReleased = System::GetNow();
        }else if(System::GetNow() > timeReleased + 200.0f){
            menuChange = false;
        }

    
        if(menuScreen == basic){
            
            //Display menu select bits
            disp[2].Write(false);
            disp[3].Write(false);
            
            //DIP 2 toggles freq control and envelope control
            if(!dip[1].Pressed()){
                //Mod the octave knob from 1 to 5
                octave = fmod((knob2 * 5.0f),5) + 1.0f;
                osc.SetFreq((knob1 * 32.70f*(pow(2,octave))) + 32.70f*(pow(2,octave)));
            }else{
                //Update envelope attack and decay
                env.SetTime(ADENV_SEG_ATTACK,knob1+0.01);
                env.SetTime(ADENV_SEG_DECAY,knob2+0.01);
            }

            //Wave Selector 
            if(button1.FallingEdge() && !menuChange){
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

            //Dip 1 toggles mute functionality, DIP 3 toggles whether using envelope
            if(!dip[2].Pressed()){
                if(button2.Pressed() ^ dip[0].Pressed()){
                    osc.SetAmp(0);
                }else{
                    osc.SetAmp(knob3);
                }
            }else{
                //Dip 4 will add an auto trigger
                if(dip[3].Pressed()){
                    tick.SetFreq(knob3*5.0f);
                    if(tick.Process()){
                        env.Trigger();
                        hw.SetLed((fCount++)%2);
                    }
                }
                if(button2.RisingEdge()){
                    env.Trigger();
                }
                osc.SetAmp(env.Process());
            }
        }else if(menuScreen == fx){
            //Display menu select bits
            disp[2].Write(true);
            disp[3].Write(false);
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

    //Initialize Envelope and Metro
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK,0.01);
    env.SetTime(ADENV_SEG_DECAY,0.5);
    env.SetMin(0.0);
    env.SetMax(1.0);
    env.SetCurve(0); // linear
    tick.Init(1.0f, sample_rate);

    //LED Display
    disp[0].Init(Pin(PORTD, 11),GPIO::Mode::OUTPUT); //Pin D26
    disp[1].Init(Pin(PORTA, 0),GPIO::Mode::OUTPUT); //Pin D25
    disp[2].Init(Pin(PORTB, 1),GPIO::Mode::OUTPUT); //Pin D17
    disp[3].Init(Pin(PORTA, 3),GPIO::Mode::OUTPUT); //Pin D16

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
