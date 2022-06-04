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
static MidiUsbHandler midi;

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
    NUM_ADC_CHANNELS
};

enum menu {
    oscillatorMenu = 0,
    suboscillatorMenu,
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
    bsave,
    bload,
    NUM_BUTTONS
};

enum LFOsends {
    none = 0,
    pitchLFO,
    attackLFO,
    decayLFO,
    sustainLFO,
    releaseLFO,
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
    float oscTotal;
    float resTotal;
    float envTotal;
    float note;
    float subNote;

    for(size_t i = 0; i < size; i += 2)
    { 
        //Set process of lfo1 for use in main
        params.setLFO1Process(lfo1.Process());

        //LFO for Pitch
        if(params.getLFO1Send() == pitchLFO){
            note = (params.getNote() *  pow(2, params.getOctave()));
            note += note * params.getOffset() + note * params.getLFO1Process();

            subNote = params.getSubNote() *  pow(2, params.getOctave());
            subNote += subNote * params.getOffset() + subNote * params.getLFO1Process();
        }else{
            note = (params.getNote() *  pow(2, params.getOctave()));
            note += note * params.getOffset();

            subNote = params.getSubNote() *  pow(2, params.getOctave());
            subNote += subNote * params.getOffset();
        }

        //Set frequencies
        osc.SetFreq(note);
        subosc.SetFreq(subNote);

        //LFO for Filter
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

        //LFO for Envelope
        if(params.getLFO1Send() == attackLFO){
            envTotal = params.getAttack() * params.getLFO1Process() + params.getAttack();
            if(envTotal < 0.01f){
                envTotal = 0.01f;
            }
            env.SetTime(ADSR_SEG_ATTACK,envTotal);
        }else if(params.getLFO1Send() == decayLFO){
            envTotal = params.getDecay() * params.getLFO1Process() + params.getDecay();
            if(envTotal < 0.01f){
                envTotal = 0.01f;
            }
            env.SetTime(ADSR_SEG_DECAY,envTotal);
        }else if(params.getLFO1Send() == sustainLFO){
            envTotal = params.getSustain() * params.getLFO1Process() + params.getSustain();
            if(envTotal < 0.01f){
                envTotal = 0.01f;
            }else if(envTotal > 1.0f){
                envTotal = 1.0f;
            }
            env.SetSustainLevel(envTotal);
        }else if(params.getLFO1Send() == releaseLFO){
            envTotal = params.getRelease() * params.getLFO1Process() + params.getRelease();
            if(envTotal < 0.01f){
                envTotal = 0.01f;
            }
            env.SetTime(ADSR_SEG_RELEASE,envTotal);
        }

        //Gates, and processes
        params.setEnvProc(env.Process(params.getEnvGate()));
        oscTotal = osc.Process() + subosc.Process() + noise.Process();

        //Create signal
        sig = params.getEnvProc() * oscTotal + params.getEnvProc() * oDrive.Process(oscTotal);
        //sig = oscTotal;
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

        //left out goes to Delay circuit
        out[i] += sig;

        //right out to output
        out[i + 1] += (1.0f - params.getDelay()) * sig + params.getDelay() * in[i];
    }
}

//Translate MIDI messages to notes
void HandleMidiMessage(MidiEvent m){

    switch(m.type){
        case NoteOn: {
            NoteOnEvent p = m.AsNoteOn();

            params.noteOn(mtof(p.note));

            params.setNote(mtof(p.note)); 
            env.Retrigger(false);
            params.setEnvGate(true);

        }
        break;
        case NoteOff: {
            NoteOffEvent o = m.AsNoteOff();

            params.noteOff(mtof(o.note));

            //If no highest, no notes pressed
            if(params.notesHeld()){
                params.setNote(params.getLastNote());
            }else{
                params.setEnvGate(false);
            }
        }
        break;
        default: 
            break;
    }
}

int main(void)
{
    //Declarations for while loop
    int oscWave = 0;
    int subOscWave = 0;
    int menuScreen = oscillatorMenu;
    float lockThresh = 0.05f;
    bool firstBoot = true;

    //LFO
    int lfo1wave = 0;
    string lfoNames[NUM_LFO_SENDS] = {"None","Pitch","Attack","Decay","Sustain","Release","Cutoff","Resonance"};

    // initialize seed hardware and oscillator daisysp module
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    //Initialize Midi port
    MidiUsbHandler::Config MidiConfig;
    midi.Init(MidiConfig);
    System::Delay(250);

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
    midi.StartReceive();

    //Allow the OLED to start up
    System::Delay(100);
    /** And Initialize */
    screen.Init(&hw);
    System::Delay(2000);

    //Analog input vars
    float kVal[4];
    bool klock[4] = {true};
    float kLockVals[4];
    for(int k = 0; k < 4; k++){
        kLockVals[k] = 1.0f - hw.adc.GetFloat(k);
    }

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
    
        if(menuScreen == oscillatorMenu){
            dLines[0] = "Oscillator";
            dLines[5] = "";

            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0] && !firstBoot){
                params.setOffset(kVal[0]);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false; firstBoot = false;}
            }
            dLines[2] = "1|Offset: " + std::to_string((int)floor(params.getOffset()*100.0f));

            //Octave adjust
            if(!klock[1] && !firstBoot){
                params.setOctave(floor(kVal[1] * 3.0f));
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false; firstBoot = false;}
            }

            //Amplitude adjust
            if(!klock[2] && !firstBoot){
                params.setOscAmp(kVal[2] * 0.5f);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > lockThresh){klock[2] = false; firstBoot = false;}
            }

            osc.SetAmp(params.getOscAmp());

            dLines[3] = "2|Octave: " + std::to_string((int)floor(params.getOctave()));
            dLines[4] = "3|Amplitude: "  + std::to_string((int)floor(params.getOscAmp() * 200.0f));

            //Wave Selector 
            if(button[bsel].FallingEdge()){
                oscWave = oscWave < 3 ? oscWave + 1 : 0;
            }

            switch(oscWave){
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

        }else if(menuScreen == suboscillatorMenu){
            dLines[0] = "Sub Oscillator";
            dLines[4] = "";
            dLines[5] = "";

            if(!klock[0]){
                params.setSubOctave(floor(kVal[0] * 3.0f));
            }else{
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false;}
            }

            //Amplitude adjust
            if(!klock[1]){
                params.setSubOscAmp(kVal[1] * 0.5f);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false;}
            }

            subosc.SetAmp(params.getSubOscAmp());
            dLines[3] = "2|Amplitude: "  + std::to_string((int)floor(params.getSubOscAmp() * 200.0f));
            //Sub Wave Selector 
            if(button[bsel].FallingEdge()){
                subOscWave = subOscWave < 3 ? subOscWave + 1 : 0;
            }

            switch(subOscWave){
                case 0:
                    subosc.SetWaveform(subosc.WAVE_SIN);
                    dLines[1] = "o|Sub Type: Sin";
                    break;
                case 1:
                    subosc.SetWaveform(subosc.WAVE_SAW);
                    dLines[1] = "o|Sub Type: Saw";
                    break;
                case 2:
                    subosc.SetWaveform(subosc.WAVE_SQUARE);
                    dLines[1] = "o|Sub Type: Square";
                    break;
                case 3:
                    subosc.SetWaveform(subosc.WAVE_TRI);
                    dLines[1] = "o|Sub Type: Triangle";
                    break;
            }

            switch(params.getSubOctave()){
                case 0: 
                    dLines[2] = "1|Sub Octave: Off";
                    subosc.SetAmp(0.0f);
                    break;
                case 1:
                    dLines[2] = "1|Sub Octave: -1";
                    break;
                case 2:
                    dLines[2] = "1|Sub Octave: -2";
                    break;
                case 3:
                    dLines[2] = "1|Sub Octave: -3";
            }

        }else if(menuScreen == fx){
            //Display menu
            dLines[0] = "Effects";
            dLines[5] = "";

            //Reverb adjust
            if(!klock[0]){
                params.setReverb(kVal[0]);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false;}
            }
            verb.SetFeedback(params.getReverb());

            //OD adjust
            if(!klock[1]){
                params.setDrive(kVal[1]);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false;}
            }

            //Delay adjust
            if(!klock[2]){
                params.setDelay(kVal[2]);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > lockThresh){klock[2] = false;}
            }

            //Noise adjust
            if(!klock[3]){
                params.setNoise(kVal[3] * 0.5f);
            }else{
                if(abs(kVal[3] - kLockVals[3]) > lockThresh){klock[3] = false;}
            }

            oDrive.SetDrive(params.getDrive());
            noise.SetAmp(params.getNoise());

            dLines[1] = "1|Reverb: " + std::to_string((int)floor(params.getReverb()*100.00f));
            dLines[2] = "2|Overdrive: " + std::to_string((int)floor(params.getDrive()*100.00f));
            dLines[3] = "3|Delay: " + std::to_string((int)floor(params.getDelay()*100.00f));
            dLines[4] = "4|Noise: " + std::to_string((int)floor(params.getNoise()*100.00f));

        }else if(menuScreen == envelope){
            //Update envelope attack and decay
            //Adjust offset based on knob, unless locked after menu change
            if(!klock[0]){
                params.setAttack(kVal[0]+0.01);
            }else{
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false;}
            }

            if(!klock[1]){
                params.setDecay(kVal[1]+0.01);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false;}
            }

            if(!klock[2]){
                params.setSustain(kVal[2]);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > lockThresh){klock[2] = false;}
            }

            if(!klock[3]){
                params.setRelease(kVal[3]+0.01);
            }else{
                if(abs(kVal[3] - kLockVals[3]) > lockThresh){klock[3] = false;}
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
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false;}
            }

            if(!klock[1]){
                params.setRes(kVal[1]);
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false;}
            }

            if(!klock[2]){
                params.setFilterDrive(kVal[2]);
            }else{
                if(abs(kVal[2] - kLockVals[2]) > lockThresh){klock[2] = false;}
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
                if(abs(kVal[0] - kLockVals[0]) > lockThresh){klock[0] = false;}
            }

            if(!klock[1]){
                
                //1 to 50
                if(kVal[1] > 0.5f){
                    params.setLFO1Freq(floor(kVal[1]*50.0f-24.0f));
                }else{
                    params.setLFO1Freq(1.0f / ceil(10.0f - kVal[1]*20.0f));
                }
                
            }else{
                if(abs(kVal[1] - kLockVals[1]) > lockThresh){klock[1] = false;}
            }

            lfo1.SetFreq(params.getLFO1Freq());
            lfo1.SetAmp(params.getLFO1Amount());

            dLines[2] = "1|Amount: " + std::to_string((int)floor(params.getLFO1Amount()*100.0f));
            if(params.getLFO1Freq() < 1.0f && params.getLFO1Freq() > 0.0f){
                dLines[3] = "2|Frequency: 1/" + std::to_string((int)(1.0f / params.getLFO1Freq()));
            }else{
                dLines[3] = "2|Frequency: " + std::to_string((int)params.getLFO1Freq());
            }
            dLines[5] = "";

            //LFO 1 Wave Selector 
            if(button[bup].FallingEdge()){
                lfo1wave = lfo1wave < 4 ? lfo1wave + 1: 0;
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
                    lfo1.SetWaveform(lfo1.WAVE_RAMP);
                     dLines[4] = "^|Wave: Ramp";
                    break;
                case 4:
                    lfo1.SetWaveform(lfo1.WAVE_TRI);
                     dLines[4] = "^|Wave: Triangle";
                    break;
            }
        }

        //Cool new MIDI note handling
        midi.Listen();
        while(midi.HasEvents()){
            HandleMidiMessage(midi.PopEvent());
        }

        params.setSubNote(params.getNote() / pow(2.0f, static_cast<float>(params.getSubOctave())));
               
        //dLines[5] = std::to_string((int)floor(params.getLFO1Process()*100.0f));
        //Print to display
        screen.print(dLines, 6);
    }
}
