/*
This file contains all the parameters for the DD-22
*/
#include <vector>
#include <algorithm>

#define numFloats 17
#define numInts 6

using namespace std;

class dd22Params {
    private:
        //Oscillator Variables
        float note, subNote;
        vector<float> heldNotes;
        vector<float>::iterator toDelete;
        float offset = 0.0f;
        float octave = 0.0f;
        int subOct = 0;
        float oscAmp = 0.5;
        float subOscAmp = 0.5;
        int wave = 0;
        int subWave = 0;

         //Envelope
        float attack = 0.1f;
        float decay = 0.5f;
        float release = 0.5f;
        float sustain = 0.5f;
        bool gate = false;

        //Effects
        float reverb = 0.0f;
        float delay = 0.0f;
        float drive = 0.0f;
        float noise = 0.0f;

        //Filters
        float filtFreq = 1000.0f;
        int filterType = 0;
        float filterRes = 0.0f;
        float filterDrive = 0.0f;

        //LFO
        float lfo1Amount = 0.0f;
        float lfo1Freq = 0.0f;
        int lfo1wave = 0;
        int lfo1send = 0;

        //Processes
        float lfo1Process = 0;
        float envProcess = 0;

        //Patch Saving
        float savedFloats[numFloats];
        int savedInts[numInts];

    public:
        //Getters and Setters
        float getNote(){
            return note;
        }

        void setNote(float newNote){
            note = newNote;
        }

        float getSubNote(){
            return subNote;
        }
        
        void setSubNote(float newSubNote){
            subNote = newSubNote;
        }

        float getOffset(){
            return offset;
        }
        
        void setOffset(float newOffset){
            offset = newOffset;
        }
        
        float getOctave(){
            return octave;
        }
        
        void setOctave(float newOctave){
            octave = newOctave;
        }

        int getSubOctave(){
            return subOct;
        }
        
        void setSubOctave(int newSubOctave){
            subOct = newSubOctave;
        }

        float getOscAmp(){
            return oscAmp;
        }
        
        void setOscAmp(float newOscAmp){
            oscAmp = newOscAmp;
        }

        float getSubOscAmp(){
            return subOscAmp;
        }
        
        void setSubOscAmp(float newSubOscAmp){
            subOscAmp = newSubOscAmp;
        }

        void setWave(int newWave){
            wave = newWave;
        }

        void incWave(int MAX_WAVES){
            wave = wave < MAX_WAVES-1 ? wave + 1: 0;
        }

        int getWave(){
            return wave;
        }

        void setSubWave(int newWave){
            subWave = newWave;
        }

        void incSubWave(int MAX_WAVES){
            subWave = subWave < MAX_WAVES-1 ? subWave + 1: 0;
        }

        int getSubWave(){
            return subWave;
        }

        float getAttack(){
            return attack;
        }
        
        void setAttack(float newAttack){
            attack = newAttack;
        }

        float getRelease(){
            return release;
        }
        
        void setRelease(float newRelease){
            release = newRelease;
        }
        
        float getSustain(){
            return sustain;
        }
        
        void setSustain(float newSustain){
            sustain = newSustain;
        }

        float getDecay(){
            return decay;
        }
        
        void setDecay(float newDecay){
            decay = newDecay;
        }

        float getReverb(){
            return reverb;
        }
        
        void setReverb(float newReverb){
            reverb = newReverb;
        }

        float getDelay(){
            return delay;
        }
        
        void setDelay(float newDelay){
            delay = newDelay;
        }

        float getDrive(){
            return drive;
        }
        
        void setDrive(float newDrive){
            drive = newDrive;
        }

        float getLFO1Amount(){
            return lfo1Amount;
        }
        
        void setLFO1Amount(float newAmount){
            lfo1Amount = newAmount;
        }

        float getLFO1Freq(){
            return lfo1Freq;
        }
        
        void setLFO1Freq(float newFreq){
            lfo1Freq = newFreq;
        }
        
        int getLFO1Send(){
            return lfo1send;
        }
        
        void setLFO1Send(int newSend){
            lfo1send = newSend;
        }

        void incLFO1Send(int MAX_SENDS){
            lfo1send = lfo1send < MAX_SENDS-1 ? lfo1send + 1: 0;
        }

        void setLFO1Wave(int newWave){
            lfo1wave = newWave;
        }

        void incLFO1Wave(int MAX_WAVES){
            lfo1wave = lfo1wave < MAX_WAVES-1 ? lfo1wave + 1: 0;
        }

        int getLFO1Wave(){
            return lfo1wave;
        }

        float getLFO1Process(){
            return lfo1Process;
        }

        void setLFO1Process(float newLFO1Process){
            lfo1Process = newLFO1Process;
        }
        
        float getEnvProc(){
            return envProcess;
        }

        void setEnvProc(float newEnvProc){
            envProcess = newEnvProc;
        }

        bool getEnvGate(){
            return gate;
        }

        void setEnvGate(bool newGate){
            gate = newGate;
        }

        float getFiltFreq(){
            return filtFreq;
        }

        void setFiltFreq(float newFreq){
            filtFreq = newFreq;
        }

        int getFilter(){
            return filterType;
        }

        void incFilter(){
            filterType = filterType < 3 ? filterType + 1 : 0;
        }

        void setFilter(int fType){
            filterType = fType;
        }

        float getRes(){
            return filterRes;
        }
        void setRes(float newResonance){
            filterRes = newResonance;
        }

        float getFilterDrive(){
            return filterDrive;
        }
        void setFilterDrive(float newDrive){
            filterDrive = newDrive;
        }

        float getNoise(){
            return noise;
        }

        void setNoise(float newNoise){
            noise = newNoise;
        }

        //Handling held notes from midi
        //Add note to vector
        void noteOn(float newNote){
            heldNotes.push_back(newNote);
        }

        //Delete note from vector
        void noteOff(float newNote){
            for(int i = 0; i < heldNotes.size(); i++){
                if(abs(heldNotes.at(i) - newNote) < 0.01f){
                    toDelete = heldNotes.begin() + i;
                    heldNotes.erase(toDelete);
                    break;
                }
            }
        }

        float getLastNote(){
            return *(heldNotes.end()-1);
        }

        bool notesHeld(){
            return !heldNotes.empty();
        }
        //End midi handling

        //Patch saving and restoring functions
        void savePatch(){
            savedFloats[0] = offset;
            savedFloats[1] = octave;
            savedFloats[2] = oscAmp;
            savedFloats[3] = subOscAmp;
            savedFloats[4] = attack;
            savedFloats[5] = decay;
            savedFloats[6] = release;
            savedFloats[7] = sustain;
            savedFloats[8] = reverb;
            savedFloats[9] = delay;
            savedFloats[10] = drive;
            savedFloats[11] = noise;
            savedFloats[12] = filtFreq;
            savedFloats[13] = filterRes;
            savedFloats[14] = filterDrive;
            savedFloats[15] = lfo1Amount;
            savedFloats[16] = lfo1Freq;

            savedInts[0] = subOct;
            savedInts[1] = filterType;
            savedInts[2] = lfo1wave;
            savedInts[3] = lfo1send;
            savedInts[4] = wave;
            savedInts[5] = subWave;

        }

        void restorePatch(){
            setOffset(savedFloats[0]);
            setOctave(savedFloats[1]);
            setOscAmp(savedFloats[2]);
            setSubOscAmp(savedFloats[3]);
            setAttack(savedFloats[4]);
            setDecay(savedFloats[5]);
            setRelease(savedFloats[6]);
            setSustain(savedFloats[7]);
            setReverb(savedFloats[8]);
            setDelay(savedFloats[9]);
            setDrive(savedFloats[10]);
            setNoise(savedFloats[11]);
            setFiltFreq(savedFloats[12]);
            setRes(savedFloats[13]);
            setFilterDrive(savedFloats[14]);
            setLFO1Amount(savedFloats[15]);
            setLFO1Freq(savedFloats[16]);

            setSubOctave(savedInts[0]);
            setFilter(savedInts[1]);
            setLFO1Wave(savedInts[2]);
            setLFO1Send(savedInts[3]);
            setWave(savedInts[4]);
            setSubWave(savedInts[5]);
        }
};