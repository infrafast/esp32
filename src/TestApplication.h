#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"
#include "mixerInterfaces.h"

void setup(){
    DEBUG_PRINT("go2");
    app.init();

    //new MixingStation(networkOSCinterface2);    
    //new BehringerXM();
}

void loop(){
    app.refresh();
}