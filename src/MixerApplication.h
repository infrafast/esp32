#define USE_NETWORK

#include "Interval.h"

#include "mixers/m_mstation.h"
#include "mixers/m_xmseries.h"
#include "mixers/s_mixer.h"

MixingStation* msMixer;
XMSeries* xmMixer;

void setup(){
    
    msMixer = new MixingStation();    // a mixer has to be notified when webapp is started so it starts synchronisationo of data
    xmMixer = new XMSeries();

    stripInput2mixbusSkel = new StripSkeleton(STRIP_INPUT2MIXBUS,GROUP_INTERFACE_MIXER,sendStripConstructorCallback,'*');
        stripInput2mixbusSkel->registerDataSetAddress(&addressCHname);
        stripInput2mixbusSkel->registerDataSetAddress(&addressBUSname);
    
    stripInputGainSkel = new StripSkeleton(STRIP_INPUTGAIN,GROUP_INTERFACE_MIXER,gain2chStripConstructorCallback,'*');
        stripInputGainSkel->registerDataSetAddress(&addressCHname);

    app.init();

}

void loop(){
    app.refresh();
}
