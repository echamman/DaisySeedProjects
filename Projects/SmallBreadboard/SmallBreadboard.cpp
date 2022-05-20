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

static AdEnv env;
static Metro tick;

//Effects
static ReverbSc verb;
static Overdrive oDrive;

//Filters
static MoogLadder mlf;

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
    filter,
    fx,
    LFO,
    NUM_MENUS
};

enum buttons {
    bleft = 0,
    bdown,
    bright,
    bup,
    bsel,
    NUM_BUTTONS
};

static Switch button[NUM_BUTTONS];

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;

    for(size_t i = 0; i < size; i += 2)
    { 
        sig = mlf.Process(env.Process() * osc.Process()) + (env.Process() * oDrive.Process(osc.Process()));

        //Reverb add
        verb.Process(sig, sig, &out[i], &out[i + 1]);

        //left out
        out[i] += sig;// + vOut[i];

        // right out
        out[i + 1] += sig;// + vOut[i + 1];
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
    bool klock[6] = {false};
    float offset = 0.0f;
    float octave = 1.0f;
    //Envelope
    float attack = 0.1f;
    float decay = 0.5f;
    //Effects
    float reverb = 0.0f;
    float delay = 0.0f;
    float drive = 0.0f;
    //MoogLadder
    float mlfRes = 0.0f;
    float mlfCutoffCoarse = 10000.0f;
    float mlfCutoffFine = 1000.0f;
    bool mlfOn = false;

    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);

    //Initialize Envelope and Metro
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK,attack);
    env.SetTime(ADENV_SEG_DECAY,decay);
    env.SetMin(0.0);
    env.SetMax(1.0);
    env.SetCurve(0); // linear
    tick.Init(1.0f, sample_rate);

    //Initialize Effects
    verb.Init(sample_rate);
    verb.SetFeedback(reverb);
    verb.SetLpFreq(18000.0f);
    oDrive.Init();
    oDrive.SetDrive(drive);

    //Initialize Filters
    mlf.Init(sample_rate);
    mlf.SetFreq(20000.0f);
    mlf.SetRes(mlfRes);

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
    button[bleft].Init(hw.GetPin(8),1000);
    button[bdown].Init(hw.GetPin(9),1000);
    button[bright].Init(hw.GetPin(6),1000);
    button[bup].Init(hw.GetPin(10),1000);
    button[bsel].Init(hw.GetPin(7),1000);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);

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

    //Display startup Screen
    display.Fill(false);
    display.SetCursor(0, 18);
    sprintf(strbuff, "Eazaudio");
    display.WriteString(strbuff, Font_11x18, true);
    display.SetCursor(0, 36);
    sprintf(strbuff, "DD-22");
    display.WriteString(strbuff, Font_7x10, true);
    display.Update();
    System::Delay(2000);

    while(1) {
        //Debounce buttons together
        for(int b = 0; b < NUM_BUTTONS; b++){
            button[b].Debounce();
        }
    
        //Get Analog input readings
        K0 = 1.0f - hw.adc.GetFloat(Knob0);
        K1 = 1.0f - hw.adc.GetFloat(Knob1);
        K2 = 1.0f - hw.adc.GetFloat(Knob2);
        K3 = 1.0f - hw.adc.GetFloat(Knob3);
        K4 = 1.0f - hw.adc.GetFloat(Knob4);
        K5 = 1.0f - hw.adc.GetFloat(Knob5);
        cvPitch = hw.adc.GetFloat(CVIN);
        cvGate = hw.adc.GetFloat(CVGATE);

        //Cycle through menu screens and parameter lock knobs on each cycle
        if(button[bright].FallingEdge()){
            menuScreen = (menuScreen + 1) % NUM_MENUS;
            for(int kl = 0; kl < 6; kl++){ klock[kl] = true;}
        }else if(button[bleft].FallingEdge()){
            menuScreen = menuScreen == 0 ? NUM_MENUS - 1: menuScreen - 1;
            for(int kl = 0; kl < 6; kl++){ klock[kl] = true;}
        }
    
        if(menuScreen == wave){
            dLines[0] = "Wave";
            dLines[4] = "";

            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                offset = K0;
            }else{
                if(abs((int)floor(K0*100.00f) - (int)floor(offset*100.0f)) < 2){klock[0] = false;}
            }
            dLines[2] = "1|Offset: " + std::to_string((int)floor(offset*100.0f));

            //Octave adjust
            if(button[bup].FallingEdge()){
                octave = octave > 2.9f ? 3.0f : octave + 1.0f;
            }else if(button[bdown].FallingEdge()){
                octave = octave < 0.1f ? 0.0f : octave - 1.0f;
            }
            dLines[3] = "^|Octave: " + std::to_string((int)floor(octave));

            //Wave Selector 
            if(button[bsel].FallingEdge()){
                if(wf1 < 3){
                    wf1++;
                }else{
                    wf1 = 0;
                }
            }

            switch(wf1){
                case 0:
                    osc.SetWaveform(osc.WAVE_SIN);
                    dLines[1] = "o|Type: Sin";
                    break;
                case 1:
                    osc.SetWaveform(osc.WAVE_SAW);
                    dLines[1] = "o|Type: Saw";
                    break;
                case 2:
                    osc.SetWaveform(osc.WAVE_SQUARE);
                    dLines[1] = "o|Type: Square";
                    break;
                case 3:
                    osc.SetWaveform(osc.WAVE_TRI);
                    dLines[1] = "o|Type: Triangle";
                    break;
            }

        }else if(menuScreen == fx){
            //Display menu
            dLines[0] = "Effects";

            //Reverb adjust
            if(!klock[0]){
                reverb = K0;
            }else{
                if(abs(K0 - reverb) < 0.2f){klock[0] = false;}
            }
            verb.SetFeedback(reverb);

            //OD adjust
            if(!klock[1]){
                drive = K1;
            }else{
                if(abs(K1 - drive) < 0.2f){klock[1] = false;}
            }
            oDrive.SetDrive(drive);
            dLines[1] = "1|Reverb: " + std::to_string((int)floor(reverb*100.00f));
            dLines[2] = "2|Overdrive: " + std::to_string((int)floor(drive*100.00f));
            dLines[3] = "";
            dLines[4] = "";

        }else if(menuScreen == envelope){
            //Update envelope attack and decay
            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                attack = K0+0.01;
            }else{
                if(abs(K0+0.01 - attack) < 0.2f){klock[0] = false;}
            }

            if(!klock[1]){
                decay = K1+0.01;
            }else{
                if(abs(K1+0.01 - decay) < 0.2f){klock[1] = false;}
            }

            env.SetTime(ADENV_SEG_ATTACK,attack);
            env.SetTime(ADENV_SEG_DECAY,decay);

            //Take care of display
            dLines[0] = "Envelope";
            dLines[1] = "1|Attack: " + std::to_string((int)floor(attack*100.0f));
            dLines[2] = "2|Decay: " + std::to_string((int)floor(decay*100.0f));
            dLines[3] = "";
            dLines[4] = "";
        }else if(menuScreen == filter){

            //Toggle Filter
            if(button[bsel].FallingEdge()){
                mlfOn = mlfOn ? false : true;
            }

            dLines[0] = "Filter";
            if(mlfOn){
                dLines[1] = "o|Enable: On";
            }else{
                dLines[1] = "o|Enable: Off";
            }

            if(!klock[0]){
                mlfCutoffCoarse = floor(K0*100.0f) * 100.0f;
            }else{
                if(abs(K0*10000.0f - mlfCutoffCoarse) < 20.0f){klock[0] = false;}
            }

            if(!klock[1]){
                mlfCutoffFine = floor(K1*100.0f) * 10.0f;
            }else{
                if(abs(K1*1000.0f - mlfCutoffFine) < 20.0f){klock[1] = false;}
            }

            if(!klock[2]){
                mlfRes = K2 > 0.9f ? 0.9f : K2; //Limit resonance at 0.9
            }else{
                if(abs(K2 - mlfRes) < 0.1f){klock[2] = false;}
            }

            if(mlfOn){
                mlf.SetFreq(mlfCutoffCoarse + mlfCutoffFine);
                mlf.SetRes(mlfRes);
            }else{
                mlf.SetFreq(20000.0f);
                mlf.SetRes(0.0f);
            }
            

            dLines[2] = "1|Cutoff Coarse";
            dLines[3] = "2|Cutoff Fine: " + std::to_string((int)floor(mlfCutoffCoarse + mlfCutoffFine));
            dLines[4] = "3|Resonance: " + std::to_string((int)floor(mlfRes*100.0f));

        }else if(menuScreen == LFO){
            dLines[0] = "LFO";
            dLines[1] = "";
            dLines[2] = "";
            dLines[3] = "";
            dLines[4] = "";
        }

        //Process notes and key hits
        //Translate CV In to pitch
        if(cvPitch * 3.33f < 0.1f){
            note = octave;
        }else{
            note = cvPitch * 3.33f + octave + offset;
        }
        osc.SetFreq(16.35f*(pow(2,note)));

        if(cvGate < 0.1f && !(env.GetValue() > 0.98f)){
            env.Trigger();
        }
               
        //dLines[4] = std::to_string((int)floor(note*100.0f));

        //Print to display
        display.Fill(false);
        display.SetCursor(44, 0);
        sprintf(strbuff, dLines[0].c_str());
        display.WriteString(strbuff, Font_6x8, true);
        for(int d=1; d<5; d++){
            display.SetCursor(0, d*11);
            sprintf(strbuff, dLines[d].c_str());
            display.WriteString(strbuff, Font_6x8, true);
        }
        display.Update();
    }
}
