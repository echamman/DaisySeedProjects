/* Class for the OLED display
*/
#define MAX_OLED_LINE 12

#include <stdio.h>
#include <string.h>
#include "daisysp.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

using namespace daisysp;
using namespace daisy;
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;
using namespace std;

class lilOled{
    MyOledDisplay display;
    DaisySeed  hw;
    char strbuff[128];

    public: void Init(DaisySeed *seed){
        hw = *seed;
        /** Configure the Display */
        MyOledDisplay::Config disp_cfg;
        disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = hw.GetPin(12);
        disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = hw.GetPin(11);

        display.Init(disp_cfg);

        //Display startup Screen
        display.Fill(false);
        display.SetCursor(0, 18);
        sprintf(strbuff, "Eazaudio");
        display.WriteString(strbuff, Font_11x18, true);
        display.SetCursor(0, 36);
        sprintf(strbuff, "DD-22");
        display.WriteString(strbuff, Font_7x10, true);
        display.Update();
    }

    //Print to Display
    public: void print(string dLines[], int max){
        display.Fill(false);
        display.SetCursor(44, 0);
        sprintf(strbuff, dLines[0].c_str());
        display.WriteString(strbuff, Font_6x8, true);
        for(int d=1; d<max; d++){
            display.SetCursor(0, d*11);
            sprintf(strbuff, dLines[d].c_str());
            display.WriteString(strbuff, Font_6x8, true);
        }
        display.Update();
    }

};