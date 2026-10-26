/**
 * @file DiagnoseApplication.h
 * @brief Minimal hardware diagnostic application.
 */

#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"

Diagnose diagnose;

void setup(){
    diagnose.begin();
}

void loop(){
    diagnose.loop();
}
