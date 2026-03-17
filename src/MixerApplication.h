#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

#include "mixers/m_mstation.h"
#include "mixers/m_xmseries.h"

MixingStation* msMixer;
XMSeries* xmMixer;

void setup(){
    
    msMixer = new MixingStation();    
    xmMixer = new XMSeries();

    app.init();
}

void loop(){
    app.refresh();
}