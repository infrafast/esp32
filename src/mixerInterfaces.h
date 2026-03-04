#ifndef MixerInterface_h
#define MixerInterface_h

#include "Interfaces.h"


#define GROUP_INTERFACE_MIXER "Mixer"   
#define ADDRESS_MIXERMODEL  "/mixer/model/%s"
#define ADDRESS_MIXERNAME   "/mixer/name/%s"

class MixerInterface {
  public:

    MixerInterface(){
    }       
};

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation: public NetworkDevice, public MixerInterface{
  public:
  
    MixingStation():NetworkDevice(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,8887,10024,true){
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

#define INTERFACE_BEHRINGERXM "BehringerXM"
class BehringerXM: public NetworkOSC, public MixerInterface{
  public:
    BehringerXM():NetworkOSC(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,"/xinfo",true,GROUP_INTERFACE_MIXER){
      //destination->host = "192.168.0.1";
    }     
};

#endif