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
        stripCh2BusSkel->registerDataSetAddress(&addressCHname);
        stripCh2BusSkel->registerDataSetAddress(&addressBUSname);
    
    stripChGainSkel = new StripSkeleton(STRIP_CHGAIN,GROUP_INTERFACE_MIXER,gain2chStripConstructorCallback,'*');
        stripChGainSkel->registerDataSetAddress(&addressCHname);

    app.init();

}

void loop(){
    app.refresh();
}