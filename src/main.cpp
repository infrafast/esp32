/**
 * @file main.cpp
 * @brief Selects the firmware entry point.
 *
 * Define INTERVAL_DIAGNOSE in build_flags to run the framework Diagnose class.
 * Otherwise this file registers the product mixer application directly.
 */

#include <Arduino.h>

//#define INTERVAL_DIAGNOSE

#if defined(INTERVAL_DIAGNOSE)
    #include <Interval.h>

    Diagnose diagnose;

    void setup(){
        diagnose.begin();
    }

    void loop(){
        diagnose.loop();
    }
#else
    #define USE_NETWORK

    // Glyphs supplied by data/webapp/user.ttf.gz. These are product-owned
    // icons and therefore remain outside the reusable Interval framework.
    #define ICON_USER_BEHRINGER "!"
    #define ICON_USER_MIXINGSTATION ">"

    #include <Interval.h>

    #include "mixers/m_mstation.h"
    #include "mixers/m_xmseries.h"
    #include "mixers/s_mixer.h"

    MixingStation* msMixer;
    XMSeries* xmMixer;

    /** @brief Register mixers and product strips, then initialize Interval. */
    void setup(){
        msMixer = new MixingStation();
        xmMixer = new XMSeries();

        stripInput2mixbusSkel = new StripSkeleton(STRIP_INPUT2MIXBUS,
                                                  GROUP_INTERFACE_MIXER,
                                                  sendStripConstructorCallback,
                                                  '*');
        stripInput2mixbusSkel->registerDataSetAddress(&addressCHname);
        stripInput2mixbusSkel->registerDataSetAddress(&addressBUSname);

        stripInputGainSkel = new StripSkeleton(STRIP_INPUTGAIN,
                                               GROUP_INTERFACE_MIXER,
                                               gain2chStripConstructorCallback,
                                               '*');
        stripInputGainSkel->registerDataSetAddress(&addressCHname);

        app.init();

    #if defined(USER_I18N_DEMO)
        DEBUG_PRINT("User catalogue: %s", userLang.getString("User specific message")->c_str());
    #endif
    }

    /** @brief Run the product application's non-blocking refresh loop. */
    void loop(){
        app.refresh();
    }
#endif
