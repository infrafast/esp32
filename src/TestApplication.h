#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

#include "mixerInterfaces.h"

NetworkOSC* networkOSCinterface2 = new NetworkOSC("999.999.999.999",99999,false); // false = not exposed

void _setup(){
    app.init();

    new MixingStation(networkOSCinterface2);    
}

void _loop(){
    app.refresh();
}