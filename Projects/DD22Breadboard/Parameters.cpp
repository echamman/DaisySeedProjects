/*
This file contains all the parameters for the DD-22
*/

class dd22Params {
    private:
        //Oscillator Variables
        float note, subNote;
        float offset = 0.0f;
        float octave = 1.0f;
        int subOct = 0;
        float oscAmp = 0.5;
        float subOscAmp = 0.5;

         //Envelope
        float attack = 0.1f;
        float decay = 0.5f;
        float release = 0.5f;
        float sustain = 0.5f;

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
};