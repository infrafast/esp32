/**
 * @file main.cpp
 * @brief Selects the firmware entry point.
 *
 * Define INTERVAL_DIAGNOSE in build_flags to run the framework Diagnose class.
 * If it is not defined, MixerApplication is launched.
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
    #include "MixerApplication.h"
#endif
