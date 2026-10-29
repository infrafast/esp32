/**
 * @file MixerApplication.h
 * @brief Registers mixer backends and strip types, then starts the application.
 */

#define USE_NETWORK

// Glyphs supplied by data/webapp/user.ttf.gz: The generic DeviceInterface API
// still selects interval.ttf from the icon value alone; only the mixer-specific
// renderer override routes these identity icons to user.ttf. A future per-icon
// source selection is tracked in the roadmap for user-defined interfaces.
#define ICON_USER_BEHRINGER "!"
#define ICON_USER_MIXINGSTATION ">"

#include "Interval.h"

#include "mixers/m_mstation.h"
#include "mixers/m_xmseries.h"
#include "mixers/s_mixer.h"

MixingStation* msMixer;
XMSeries* xmMixer;

/** @brief Register application devices and strip types, then initialize the framework. */
void setup(){
    
    msMixer = new MixingStation();    // a mixer has to be notified when webapp is started so it starts synchronisationo of data
    xmMixer = new XMSeries();

    stripInput2mixbusSkel = new StripSkeleton(STRIP_INPUT2MIXBUS,GROUP_INTERFACE_MIXER,sendStripConstructorCallback,'*');
        stripInput2mixbusSkel->registerDataSetAddress(&addressCHname);
        stripInput2mixbusSkel->registerDataSetAddress(&addressBUSname);
    
    stripInputGainSkel = new StripSkeleton(STRIP_INPUTGAIN,GROUP_INTERFACE_MIXER,gain2chStripConstructorCallback,'*');
        stripInputGainSkel->registerDataSetAddress(&addressCHname);

    app.init();

#if defined(USER_I18N_DEMO)
    // demonstrate usage of user specific language catalogue
    DEBUG_PRINT("User catalogue: %s", userLang.getString("User specific message")->c_str());
#endif

}

/** @brief Run the application refresh loop. */
void loop(){
    app.refresh();
}
