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

         //Envelope
        float attack = 0.1f;
        float decay = 0.5f;
        float release = 0.5f;
        float sustain = 0.5f;

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

        float getmlfRes(){
            return mlfRes;
        }
        
        void setmlfRes(float newResonance){
            mlfRes = newResonance;
        }

        float getmlfCutoffCoarse(){
            return mlfCutoffCoarse;
        }
        
        void setmlfCutoffCoarse(float newCoarse){
            mlfCutoffCoarse = newCoarse;
        }

        float getmlfCutoffFine(){
            return mlfCutoffFine;
        }
        
        void setmlfCutoffFine(float newFine){
            mlfCutoffFine = newFine;
        }

        bool getmlfOn(){
            return mlfOn;
        }
        
        void setmlfOn(bool on){
            mlfOn = on;
        }

        void togglemlfOn(){
            mlfOn = mlfOn ? false : true;
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
        
};