#define USE_NETWORK
#include "Interval.h"

#include "mixers/m_mstation.h"
#include "mixers/m_xmseries.h"
#include "mixers/s_mixer.h"

MixingStation* msMixer;
XMSeries* xmMixer;



void setup(){
    
    msMixer = new MixingStation();    
    xmMixer = new XMSeries();

    stripCh2BusSkel = new StripSkeleton(STRIP_CH2BUS,GROUP_INTERFACE_MIXER,sendStripConstructorCallback,'*');
    /*dsOSCMessages = new OSCLib(new DropDownAddress(ADDRESS_OSCMSG));        // this dataset is based on a gzip file therefore it needs to be created after littleFS init and reloaded each time a saveLibrary is saved
    oscSkel->registerDataSetAddress(dsOSCMessages->address);*/

    app.init();

}

void loop(){
    app.refresh();
}