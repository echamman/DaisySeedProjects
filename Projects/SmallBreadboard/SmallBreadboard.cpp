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
static Oscillator osc, lfo1, lfo2;

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
    LFO1,
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

enum LFOsends {
    none = 0,
    otherLFO,
    pitch,
    attack,
    decay,
    cutoff,
    resonance,
    NUM_LFO_SENDS
};

static Switch button[NUM_BUTTONS];

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;

    for(size_t i = 0; i < size; i += 2)
    { 
        sig = mlf.Process(env.Process() * osc.Process() + env.Process() * oDrive.Process(osc.Process()));

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
    int menuScreen = wave;

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
    //LFO
    float lfo1Amount = 0.0f;
    float lfo1Freq = 0.0f;
    int lfo1wave = 0;
    int lfo1send = 0;
    float lfo2Amount = 0.0f;
    float lfo2Freq = 0.0f;
    int lfo2wave = 0;
    int lfo2send = 0;
    string lfoNames[NUM_LFO_SENDS] = {"None", "LFO","Pitch","Attack","Decay","Cutoff","Resonance"};

    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    // Set parameters for oscillator
    osc.Init(sample_rate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(440);

    //Initialize LFOs
    lfo1.Init(sample_rate);
    lfo1.SetWaveform(osc.WAVE_SIN);
    lfo1.SetFreq(10);
    lfo2.Init(sample_rate);
    lfo2.SetWaveform(osc.WAVE_SIN);
    lfo2.SetFreq(10);


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

    // start callback
    hw.adc.Start();
    hw.StartAudio(AudioCallback);

    //Analog input vars
    float kVal[6];
    float kLockVals[6] = {0}; //For parameter locking
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
        kVal[0] = 1.0f - hw.adc.GetFloat(Knob0);
        kVal[1] = 1.0f - hw.adc.GetFloat(Knob1);
        kVal[2] = 1.0f - hw.adc.GetFloat(Knob2);
        kVal[3] = 1.0f - hw.adc.GetFloat(Knob3);
        kVal[4] = 1.0f - hw.adc.GetFloat(Knob4);
        kVal[5] = 1.0f - hw.adc.GetFloat(Knob5);
        cvPitch = hw.adc.GetFloat(CVIN);
        cvGate = hw.adc.GetFloat(CVGATE);

        //Cycle through menu screens and parameter lock knobs on each cycle
        if(button[bright].FallingEdge()){
            menuScreen = (menuScreen + 1) % NUM_MENUS;
            for(int kl = 0; kl < 6; kl++){ 
                klock[kl] = true;
                kLockVals[kl] = kVal[kl];
            }
        }else if(button[bleft].FallingEdge()){
            menuScreen = menuScreen == 0 ? NUM_MENUS - 1: menuScreen - 1;
            for(int kl = 0; kl < 6; kl++){ 
                klock[kl] = true;
                kLockVals[kl] = kVal[kl];
            }
        }
    
        if(menuScreen == wave){
            dLines[0] = "Wave";
            dLines[4] = "";

            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                offset = kVal[0];
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
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
                reverb = kVal[0];
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }
            verb.SetFeedback(reverb);

            //OD adjust
            if(!klock[1]){
                drive = kVal[1];
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
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
                attack = kVal[0]+0.01;
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                decay = kVal[1]+0.01;
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
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
                mlfCutoffCoarse = floor(kVal[0]*100.0f) * 100.0f;
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                mlfCutoffFine = floor(kVal[1]*100.0f) * 10.0f;
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            if(!klock[2]){
                mlfRes = kVal[2] > 0.9f ? 0.9f : kVal[2]; //Limit resonance at 0.9
            }else{
                if(abs(kVal[2] - kLockVals[2]) > 0.15f){klock[2] = false;}
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

        }else if(menuScreen == LFO1){
            dLines[0] = "LFO 1";

            //LFO 1 Send Selector 
            if(button[bsel].FallingEdge()){
                lfo1send = lfo1send < NUM_LFO_SENDS-1 ? lfo1send + 1: 0;
            }
            //LFO1 selection
            dLines[1] = "o|Send: " + lfoNames[lfo1send];

            //Set amounts
            if(!klock[0]){
                lfo1Amount = floor(kVal[0]*100.0f);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                lfo1Freq = floor(kVal[1]*100.0f);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            lfo1.SetFreq(lfo1Freq);
            lfo1.SetAmp(lfo1Amount);
            
            dLines[2] = "1|Amount: " + std::to_string((int)lfo1Amount);
            dLines[3] = "2|Frequency: " + std::to_string((int)lfo1Freq);

            //LFO 1 Wave Selector 
            if(button[bup].FallingEdge()){
                lfo1wave = lfo1wave < 3 ? lfo1wave + 1: 0;
            }

            switch(lfo1wave){
                case 0:
                    lfo1.SetWaveform(lfo1.WAVE_SIN);
                    dLines[4] = "^|Wave: Sin";
                    break;
                case 1:
                    lfo1.SetWaveform(lfo1.WAVE_SAW);
                    dLines[4] = "^|Wave: Saw";
                    break;
                case 2:
                    lfo1.SetWaveform(lfo1.WAVE_SQUARE);
                    dLines[4] = "^|Wave: Square";
                    break;
                case 3:
                    lfo1.SetWaveform(lfo1.WAVE_TRI);
                     dLines[4] = "^|Wave: Triangle";
                    break;
            }
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
