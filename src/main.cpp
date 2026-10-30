/**
 * @file main.cpp
 * @brief Selects the firmware entry point.
 *
 * Define INTERVAL_DIAGNOSE in build_flags to run the framework-owned
 * DiagnoseApplication. If it is not defined, MixerApplication is launched.
 */

#include <Arduino.h>

//#define INTERVAL_DIAGNOSE

#if defined(INTERVAL_DIAGNOSE)
    #include <DiagnoseApplication.h>
#else
    #include "MixerApplication.h"
#endif
