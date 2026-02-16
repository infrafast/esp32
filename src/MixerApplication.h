#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

#include "mixerInterfaces.h"

MixingStation* msMixer;
BehringerXM* xmMixer;

void setup(){
    DEBUG_PRINT("go4");
    app.init();

    msMixer = new MixingStation();    
    xmMixer = new BehringerXM();
}

void loop(){
    app.refresh();
}