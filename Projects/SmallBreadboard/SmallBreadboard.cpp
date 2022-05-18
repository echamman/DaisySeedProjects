#include <stdio.h>
#include <string.h>
#include "daisysp.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisysp;
using namespace daisy;
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;
using namespace std;

#define MAX_OLED_LINE 12

MyOledDisplay display;

static DaisySeed  hw;
static Oscillator osc;
static Switch button1;
static Switch button2;

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
    wave = 0,
    envelope,
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
        osc.SetAmp(env.Process());
        sig = osc.Process();

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    /** Configure the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = hw.GetPin(12);
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = hw.GetPin(11);
    
    //Declarations for while loop
    float note;
    int wf1 = 0;
    int menuScreen = wave, oldmenu = wave;

    //Menu Variables
    bool k0lock = false, k1lock=false;
    int offset = 0;
    float attack = 0.1f;
    float decay = 0.1f;
    float reverb = 0.0f;
    float delay = 0.0f;
    float drive = 0.0f;

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
    string dLines[5]; //Create an Array of strings for the OLED display


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

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);
    osc.SetAmp(0);


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
    float cvGate;

    //Allow the OLED to start up
    System::Delay(100);
    /** And Initialize */
    display.Init(disp_cfg);
    char strbuff[128];

    while(1) {
        //Debounce buttons together
        button1.Debounce();
        button2.Debounce();
    
        //Get Analog input readings
        K0 = 1.0f - hw.adc.GetFloat(Knob0);
        K1 = 1.0f - hw.adc.GetFloat(Knob1);
        K2 = 1.0f - hw.adc.GetFloat(Knob2);
        K3 = 1.0f - hw.adc.GetFloat(Knob3);
        K4 = 1.0f - hw.adc.GetFloat(Knob4);
        K5 = 1.0f - hw.adc.GetFloat(Knob5);
        cvPitch = hw.adc.GetFloat(CVIN);
        cvGate = hw.adc.GetFloat(CVGATE);

        //Cycle through menu screens
        menuScreen = (fmod(floor(K5*NUM_MENUS),NUM_MENUS));
        if(oldmenu != menuScreen){
            k0lock = true;
            k1lock = true;
            oldmenu = menuScreen;
        }
    
        if(menuScreen == wave){
            dLines[1] = "Wave";
            dLines[4] = "";

            //Adjust offset based on knob, unless locked after menu change
            if(!k0lock){
                offset = (int)floor(K0*100.00f);
            }else{
                if(abs((int)floor(K0*100.00f) - offset) < 2){k0lock = false;}
            }
            dLines[3] = "Offset: " + std::to_string(offset);

            //Wave Selector 
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
                    dLines[2] = "Type: Sin";
                    disp[0].Write(false);
                    disp[1].Write(false);
                    break;
                case 1:
                    osc.SetWaveform(osc.WAVE_SAW);
                    dLines[2] = "Type: Saw";
                    disp[0].Write(true);
                    disp[1].Write(false);
                    break;
                case 2:
                    osc.SetWaveform(osc.WAVE_SQUARE);
                    dLines[2] = "Type: Square";
                    disp[0].Write(false);
                    disp[1].Write(true);
                    break;
                case 3:
                    osc.SetWaveform(osc.WAVE_TRI);
                    dLines[2] = "Type: Triangle";
                    disp[0].Write(true);
                    disp[1].Write(true);
                    break;
            }

        }else if(menuScreen == fx){
            //Display menu
            dLines[1] = "Effects";
            dLines[2] = "Reverb: " + std::to_string((int)floor(K0*100.00f));
            dLines[3] = "Delay: " + std::to_string((int)floor(K1*100.00f));
            dLines[4] = "";
        }else if(menuScreen == envelope){
            //Update envelope attack and decay
            //Adjust offset based on knob, unless locked after menu change
            if(!k0lock){
                attack = K0+0.01;
            }else{
                if(abs(K0+0.01 - attack) < 0.2f){k0lock = false;}
            }

            if(!k1lock){
                decay = K1+0.01;
            }else{
                if(abs(K1+0.01 - decay) < 0.2f){k1lock = false;}
            }

            env.SetTime(ADENV_SEG_ATTACK,attack);
            env.SetTime(ADENV_SEG_DECAY,decay);

            //Take care of display
            dLines[1] = "Envelope";
            dLines[2] = "Attack: " + std::to_string((int)floor(attack*100.0f));
            dLines[3] = "Decay: " + std::to_string((int)floor(decay*100.0f));
            dLines[4] = "";
        }

        //Process notes and key hits
        //Translate CV In to pitch
        note = cvPitch * 5.0f + 1.0f;
        osc.SetFreq(16.35f*(pow(2,note)));    

        if(cvGate < 0.1f && !(env.GetValue() > 0.98f)){
            env.Trigger();
        }
               
        //dLines[4] = std::to_string((int)floor(env.GetValue()*100.0f));

        //Print message to display
        dLines[0] = "Daisy Synth";

        //Print to display
        display.Fill(false);
        display.SetCursor(0, 0);
        sprintf(strbuff, dLines[0].c_str());
        display.WriteString(strbuff, Font_11x18, true);
        for(int d=1; d<5; d++){
            display.SetCursor(0, 8 + d*11);
            sprintf(strbuff, dLines[d].c_str());
            display.WriteString(strbuff, Font_6x8, true);
        }
        display.Update();
    }
}
