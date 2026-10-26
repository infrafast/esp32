/**
 * @file main.cpp
 * @brief Selects the firmware entry point.
 *
 * INTERVAL_APP_ENTRY can be set in build_flags:
 *   INTERVAL_ENTRY_MIXER (0) = normal MixerApplication (default)
 *   INTERVAL_ENTRY_DIAGNOSE (1) = DiagnoseApplication
 */

#include <Arduino.h>

#define INTERVAL_ENTRY_MIXER    0
#define INTERVAL_ENTRY_DIAGNOSE 1

#ifndef INTERVAL_APP_ENTRY
  #define INTERVAL_APP_ENTRY INTERVAL_ENTRY_MIXER
#endif

#if INTERVAL_APP_ENTRY == INTERVAL_ENTRY_DIAGNOSE

#include "DiagnoseApplication.h"

#else

#include "MixerApplication.h"

#endif
