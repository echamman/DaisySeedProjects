#include <stdio.h>
#include <string.h>
#include "daisysp.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"
#include "Parameters.cpp"
#include "LittleOLED.cpp"

using namespace daisysp;
using namespace daisy;
using namespace std;

static DaisySeed  hw;
static Oscillator osc, subosc, lfo1, lfo2;

//Holds all values, can be accessed from main and audio func
static dd22Params params;
static lilOled screen;

static Adsr env;
static Metro tick;

//Effects
static ReverbSc verb;
static Overdrive oDrive;
static WhiteNoise noise;

//Filters
static Svf filt;

enum AdcChannel {
    Knob0 = 0,
    Knob1,
    Knob2,
    Knob3,
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
    pitchLFO,
    attackLFO,
    decayLFO,
    cutoffLFO,
    resonanceLFO,
    NUM_LFO_SENDS
};

static Switch button[NUM_BUTTONS];

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;
    bool gate;
    float oscTotal;
    float resTotal;

    for(size_t i = 0; i < size; i += 2)
    { 
        //Set process of lfo1 for use in main
        params.setLFO1Process(lfo1.Process());

        if(params.getLFO1Send() == pitchLFO){
            osc.SetFreq(16.35f*(pow(2,params.getNote()+params.getLFO1Process())));
            subosc.SetFreq(16.35f*(pow(2,params.getSubNote()+params.getLFO1Process())));
        }else{
            osc.SetFreq(16.35f*(pow(2,params.getNote())));
            subosc.SetFreq(16.35f*(pow(2,params.getSubNote())));
        }

        if(params.getLFO1Send() == cutoffLFO){
            filt.SetFreq(params.getLFO1Process() * params.getFiltFreq() + params.getFiltFreq());
        }else if(params.getLFO1Send() == resonanceLFO){
            resTotal = params.getRes() * params.getLFO1Process() + params.getRes();
            if(resTotal < 0.0f){
                resTotal = 0.0f;
            }else if(resTotal > 1.0f){
                resTotal = 1.0f;
            }
            filt.SetRes(resTotal);
        }

        //Gates, and processes
        gate = hw.adc.GetFloat(CVGATE) < 0.1f;
        params.setEnvProc(env.Process(gate));
        oscTotal = osc.Process() + subosc.Process() + noise.Process();

        //Create signal
        sig = params.getEnvProc() * oscTotal + params.getEnvProc() * oDrive.Process(oscTotal);
        filt.Process(sig);
        switch (params.getFilter()){
            case 1:
                sig = filt.Low();
                break;
            case 2:
                sig = filt.High();
                break;
            case 3: 
                sig = filt.Band();
        }

        //Reverb add
        verb.Process(sig, sig, &out[i], &out[i + 1]);

        //left out
        out[i] += sig;

        // right out
        out[i + 1] += sig;
    }
}

int main(void)
{
    //Declarations for while loop
    float lastnote = 1;
    int oscWave = 0;
    int subOscWave = 0;
    int menuScreen = wave;

    //Menu Variables
    bool klock[4] = {true};

    //LFO
    int lfo1wave = 0;
    string lfoNames[NUM_LFO_SENDS] = {"None","Pitch","Attack","Decay","Cutoff","Resonance"};

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
    subosc.Init(sample_rate);
    subosc.SetWaveform(subosc.WAVE_SIN);
    subosc.SetFreq(440);
    subosc.SetAmp(0);

    //Initialize LFOs
    lfo1.Init(sample_rate);
    lfo1.SetWaveform(osc.WAVE_SIN);
    lfo1.SetFreq(params.getLFO1Freq());
    lfo1.SetAmp(params.getLFO1Amount());

    //Initialize Envelope and Metro
    env.Init(sample_rate);
    env.SetTime(ADSR_SEG_ATTACK,params.getAttack());
    env.SetTime(ADSR_SEG_DECAY,params.getDecay());
    env.SetTime(ADSR_SEG_RELEASE,params.getRelease());
    env.SetSustainLevel(params.getSustain());
    bool keyHeld = false;
    tick.Init(1.0f, sample_rate);

    //Initialize Effects
    verb.Init(sample_rate);
    verb.SetFeedback(params.getReverb());
    verb.SetLpFreq(18000.0f);
    oDrive.Init();
    oDrive.SetDrive(params.getDrive());
    noise.Init();
    noise.SetAmp(params.getNoise());

    //Initialize Filters
    filt.Init(sample_rate);
    filt.SetFreq(params.getFiltFreq());
    filt.SetRes(params.getRes());
    filt.SetDrive(params.getFilterDrive());

    string dLines[6]; //Create an Array of strings for the OLED display

    //Analog Inputs
    AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
    adcConfig[Knob0].InitSingle(hw.GetPin(25));
    adcConfig[Knob1].InitSingle(hw.GetPin(24));
    adcConfig[Knob2].InitSingle(hw.GetPin(21));
    adcConfig[Knob3].InitSingle(hw.GetPin(20));
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
    float kVal[4];
    float kLockVals[4] = {0}; //For parameter locking
    float cvPitch;
    float cvGate;

    //Allow the OLED to start up
    System::Delay(100);
    /** And Initialize */
    screen.Init(&hw);
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
        cvPitch = hw.adc.GetFloat(CVIN);
        cvGate = hw.adc.GetFloat(CVGATE);

        //Cycle through menu screens and parameter lock knobs on each cycle
        if(button[bright].FallingEdge()){
            menuScreen = (menuScreen + 1) % NUM_MENUS;
            for(int kl = 0; kl < 4; kl++){ 
                klock[kl] = true;
                kLockVals[kl] = kVal[kl];
            }
        }else if(button[bleft].FallingEdge()){
            menuScreen = menuScreen == 0 ? NUM_MENUS - 1: menuScreen - 1;
            for(int kl = 0; kl < 4; kl++){ 
                klock[kl] = true;
                kLockVals[kl] = kVal[kl];
            }
        }
    
        if(menuScreen == wave){
            dLines[0] = "Wave";

            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                params.setOffset(kVal[0]);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }
            dLines[2] = "1|Offset: " + std::to_string((int)floor(params.getOffset()*100.0f));

            //Octave adjust
            if(!klock[1]){
                params.setOctave(floor(kVal[1] * 3.0f));
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            if(!klock[2]){
                params.setSubOctave(floor(kVal[2] * 3.0f));
            }else{
                if(abs(kVal[2] - kLockVals[2]) > 0.15f){klock[2] = false;}
            }

            dLines[3] = "2|Octave: " + std::to_string((int)floor(params.getOctave()));

            switch(params.getSubOctave()){
                case 0: 
                    dLines[5] = "3|Sub Octave: Off";
                    subosc.SetAmp(0.0f);
                    break;
                case 1:
                    dLines[5] = "3|Sub Octave: -1";
                    subosc.SetAmp(0.5f);
                    break;
                case 2:
                    dLines[5] = "3|Sub Octave: -2";
                    subosc.SetAmp(0.5f);
                    break;
                case 3:
                    dLines[5] = "3|Sub Octave: -3";
                    subosc.SetAmp(0.5f);
            }

            //Wave Selector 
            if(button[bup].FallingEdge()){
                oscWave = oscWave < 3 ? oscWave + 1 : 0;
            }

            switch(oscWave){
                case 0:
                    osc.SetWaveform(osc.WAVE_SIN);
                    dLines[1] = "^|Type: Sin";
                    break;
                case 1:
                    osc.SetWaveform(osc.WAVE_SAW);
                    dLines[1] = "^|Type: Saw";
                    break;
                case 2:
                    osc.SetWaveform(osc.WAVE_SQUARE);
                    dLines[1] = "^|Type: Square";
                    break;
                case 3:
                    osc.SetWaveform(osc.WAVE_TRI);
                    dLines[1] = "^|Type: Triangle";
                    break;
            }

            //Sub Wave Selector 
            if(button[bdown].FallingEdge()){
                subOscWave = subOscWave < 3 ? subOscWave + 1 : 0;
            }

            switch(subOscWave){
                case 0:
                    subosc.SetWaveform(subosc.WAVE_SIN);
                    dLines[4] = "v|Sub Type: Sin";
                    break;
                case 1:
                    subosc.SetWaveform(subosc.WAVE_SAW);
                    dLines[4] = "v|Sub Type: Saw";
                    break;
                case 2:
                    subosc.SetWaveform(subosc.WAVE_SQUARE);
                    dLines[4] = "v|Sub Type: Square";
                    break;
                case 3:
                    subosc.SetWaveform(subosc.WAVE_TRI);
                    dLines[4] = "v|Sub Type: Triangle";
                    break;
            }

        }else if(menuScreen == fx){
            //Display menu
            dLines[0] = "Effects";
            dLines[5] = "";

            //Reverb adjust
            if(!klock[0]){
                params.setReverb(kVal[0]);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }
            verb.SetFeedback(params.getReverb());

            //OD adjust
            if(!klock[1]){
                params.setDrive(kVal[1]);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            //Noise adjust
            if(!klock[2]){
                params.setNoise(kVal[2] * 0.5f);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > 0.15f){klock[2] = false;}
            }

            oDrive.SetDrive(params.getDrive());
            noise.SetAmp(params.getNoise());

            dLines[1] = "1|Reverb: " + std::to_string((int)floor(params.getReverb()*100.00f));
            dLines[2] = "2|Overdrive: " + std::to_string((int)floor(params.getDrive()*100.00f));
            dLines[3] = "3|Noise: " + std::to_string((int)floor(params.getNoise()*100.00f));
            dLines[4] = "";

        }else if(menuScreen == envelope){
            //Update envelope attack and decay
            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                params.setAttack(kVal[0]+0.01);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                params.setDecay(kVal[1]+0.01);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            if(!klock[2]){
                params.setSustain(kVal[2]);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > 0.15f){klock[2] = false;}
            }

            if(!klock[3]){
                params.setRelease(kVal[3]+0.01);
            }else{
                if(abs(kVal[3] - kLockVals[3]) > 0.15f){klock[3] = false;}
            }

            env.SetTime(ADSR_SEG_ATTACK,params.getAttack());
            env.SetTime(ADSR_SEG_DECAY,params.getDecay());
            env.SetTime(ADSR_SEG_RELEASE,params.getRelease());
            env.SetSustainLevel(params.getSustain());

            //Take care of display
            dLines[0] = "Envelope";
            dLines[1] = "1|Attack: " + std::to_string((int)floor(params.getAttack()*100.0f));
            dLines[2] = "2|Decay: " + std::to_string((int)floor(params.getDecay()*100.0f));
            dLines[3] = "3|Sustain: " + std::to_string((int)floor(params.getSustain()*100.0f));
            dLines[4] = "4|Relase: " + std::to_string((int)floor(params.getRelease()*100.0f));
            dLines[5] = "";
        }else if(menuScreen == filter){

            //Toggle Filter
            if(button[bsel].FallingEdge()){
                params.incFilter();
            }

            dLines[0] = "Filter";
            switch(params.getFilter()){
                case 0:
                    dLines[1] = "o|Filter: Off";
                    break;
                case 1:
                    dLines[1] = "o|Filter: LPF";
                    break;
                case 2:
                    dLines[1] = "o|Filter: HPF";
                    break;
                case 3:
                    dLines[1] = "o|Filter: BPF";
            }

            if(!klock[0]){
                params.setFiltFreq(floor(kVal[0]*100.0f) * 100.0f);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                params.setRes(kVal[1]);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            if(!klock[2]){
                params.setFilterDrive(kVal[2]);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > 0.15f){klock[2] = false;}
            }
            
            filt.SetFreq(params.getFiltFreq());
            filt.SetRes(params.getRes());
            filt.SetDrive(params.getFilterDrive());

            dLines[2] = "1|Cutoff: " + std::to_string((int)floor(params.getFiltFreq()));
            dLines[3] = "2|Resonance: " + std::to_string((int)floor(params.getRes()*100.0f));
            dLines[4] = "3|Drive: " + std::to_string((int)floor(params.getFilterDrive()*100.0f));
            dLines[5] = "";

        }else if(menuScreen == LFO1){
            dLines[0] = "LFO 1";

            //LFO 1 Send Selector 
            if(button[bsel].FallingEdge()){
                params.incLFO1Send(NUM_LFO_SENDS);
            }
            //LFO1 selection
            dLines[1] = "o|Send: " + lfoNames[params.getLFO1Send()];

            //Set amounts
            if(!klock[0]){
                params.setLFO1Amount(kVal[0]);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > 0.15f){klock[0] = false;}
            }

            if(!klock[1]){
                params.setLFO1Freq(floor(kVal[1]*100.0f));
            }else{
                if(abs(kVal[1] - kLockVals[1]) > 0.15f){klock[1] = false;}
            }

            lfo1.SetFreq(params.getLFO1Freq());
            lfo1.SetAmp(params.getLFO1Amount());

            dLines[2] = "1|Amount: " + std::to_string((int)floor(params.getLFO1Amount()*100.0f));
            dLines[3] = "2|Frequency: " + std::to_string((int)params.getLFO1Freq());
            dLines[5] = "";

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
            params.setNote(params.getOctave());
        }else{
            params.setNote(cvPitch * 3.33f + params.getOctave() + params.getOffset());
        }

        params.setSubNote(params.getNote() - static_cast<float>(params.getSubOctave()));

        if(cvGate < 0.1f && (!keyHeld || abs(lastnote - params.getNote()) > 0.08f)){
            env.Retrigger(false);
            lastnote = params.getNote();
            keyHeld = true;
        }else if(cvGate > 0.1f){
            keyHeld = false;
        }
               
        //dLines[5] = std::to_string((int)floor(params.getLFO1Process()*100.0f));
        //Print to display
        screen.print(dLines, 6);
    }
}
