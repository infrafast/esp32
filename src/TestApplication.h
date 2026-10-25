/**
 * @file TestApplication.h
 * @brief Alternative minimal application; not selected by main.cpp.
 */

#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

/**
 * @brief Initialize the framework without registering mixer-specific strips.
 */
void setup(){
    app.init();
}

/** @brief Run the application refresh loop. */
void loop(){
    app.refresh();
}