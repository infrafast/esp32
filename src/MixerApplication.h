#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

#include "mixers/m_mstation.h"
#include "mixers/m_XMbehringer.h"

MixingStation* msMixer;
BehringerXM* xmMixer;

void setup(){
    
    msMixer = new MixingStation();    
    xmMixer = new BehringerXM();

    app.init();
}

void loop(){
    app.refresh();
}