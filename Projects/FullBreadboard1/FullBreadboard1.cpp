#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Oscillator osc;
static Switch button1;
static Switch button2;
static Switch dip[6];  

static AdEnv env;
static Metro tick;

static GPIO disp[4];

enum AdcChannel {
    Knob0 = 0,
    Knob1,
    Knob2,
    Knob3,
    Knob4,
    Knob5,
    CVIN,
    CVGATE,
    NUM_ADC_CHANNELS
};

enum menu {
    basic = 0,
    fx,
    NUM_MENUS
};

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;

    for(size_t i = 0; i < size; i += 2)
    {     
        sig = osc.Process();

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{

    //Declarations for while loop
    float new_oct, octave;
    int wf1 = 0;
    float fCount = 0;
    int menuScreen = basic;
    bool menuChange = false;
    float timeReleased = 0.0f;

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
    disp[0].Init(Pin(PORTC, 8),GPIO::Mode::OUTPUT); //Pin D4
    disp[1].Init(Pin(PORTC, 9),GPIO::Mode::OUTPUT); //Pin D3
    disp[2].Init(Pin(PORTB, 6),GPIO::Mode::OUTPUT); //Pin D6
    disp[3].Init(Pin(PORTD, 2),GPIO::Mode::OUTPUT); //Pin D5

    //Analog Inputs
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
    adcConfig[Knob0].InitSingle(hw.GetPin(25));
    adcConfig[Knob1].InitSingle(hw.GetPin(24));
    adcConfig[Knob2].InitSingle(hw.GetPin(23));
    adcConfig[Knob3].InitSingle(hw.GetPin(22));
    adcConfig[Knob4].InitSingle(hw.GetPin(21));
    adcConfig[Knob5].InitSingle(hw.GetPin(20));
    adcConfig[CVIN].InitSingle(hw.GetPin(15));
    adcConfig[CVGATE].InitSingle(hw.GetPin(16));
    hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);

    //buttons
    button1.Init(hw.GetPin(7),1000); //Green button
    button2.Init(hw.GetPin(8),1000); //Purple button

    //DIPSwitch
    dip[0].Init(hw.GetPin(13),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[1].Init(hw.GetPin(12),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[2].Init(hw.GetPin(14),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[3].Init(hw.GetPin(11),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[4].Init(hw.GetPin(10),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);
    dip[5].Init(hw.GetPin(9),1000,Switch::Type::TYPE_TOGGLE,Switch::Polarity::POLARITY_INVERTED,Switch::Pull::PULL_UP);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0.25);


    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    //Analog input vars
    float K0;
    float K1;
    float K2;
    float K3;
    float K4;
    float K5;
    float cvPitch;



    while(1) {
        //Debounce buttons together
        button1.Debounce();
        button2.Debounce();
        for(int i=0; i<6; i++){
            dip[i].Debounce();
        }
    
        //Get Analog input readings
        K0 = 1.0f - hw.adc.GetFloat(Knob0);
        K1 = 1.0f - hw.adc.GetFloat(Knob1);
        K2 = 1.0f - hw.adc.GetFloat(Knob2);
        K3 = 1.0f - hw.adc.GetFloat(Knob3);
        K4 = 1.0f - hw.adc.GetFloat(Knob4);
        K5 = 1.0f - hw.adc.GetFloat(Knob5);
        cvPitch = hw.adc.GetFloat(CVIN);

        //Count 0.8 seconds hold on button1 to change the menu. Wait 200ms after releasing to allow button to resume function
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
                //octave = fmod((K2 * 5.0f),5) + 1.0f;
                //osc.SetFreq((K1 * 32.70f*(pow(2,octave))) + 32.70f*(pow(2,octave)));
                octave = cvPitch * 5.0f + 1.0f;
                osc.SetFreq(32.70f*(pow(2,octave)));
            }else{
                //Update envelope attack and decay
                env.SetTime(ADENV_SEG_ATTACK,K1*5.0f+0.01);
                env.SetTime(ADENV_SEG_DECAY,K2*5.0f+0.01);
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
                    osc.SetAmp(100);
                }
            }else{
                //Dip 4 will add an auto trigger
                if(dip[3].Pressed()){
                    tick.SetFreq(K3+0.2f);
                    if(tick.Process()){
                        env.Trigger();
                        hw.SetLed(true);
                        fCount = System::GetNow();
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

        //LED blink on auto-trigger
        if(dip[3].Pressed() && System::GetNow() > fCount + 100.0f){
            hw.SetLed(false);
        }
    }
}
