#ifndef _MSTATION_h
#define _MSTATION_h

#include "mixer.h"

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation: public NetworkDevice, public MixerInterface{
  public:
  
    MixingStation():NetworkDevice(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,8887,8088,true){
      //destination->host = "192.168.99.99";
      destination = new PingEcho("0.0.0.0");      
    }

     virtual void monitorService(){

      // time to ping the device to see if it's still alive after certain time or if we have no valid UDP, we reconnect
      #define STATE_PING_DELAY 3000
      // delay expired
      if (millis()-destination->lastQueryTime > STATE_PING_DELAY){        

      }                                                                   
    }

    virtual bool handleConnectService(){return false;}

    virtual bool handleDisconnectService(){return false;}    
};


#endif