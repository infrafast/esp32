#ifndef MixerInterface_h
#define MixerInterface_h

#include "Interfaces.h"


#define GROUP_INTERFACE_MIXER "Mixer"   
#define ADDRESS_MIXERMODEL  "/mixer/model/%s"
#define ADDRESS_MIXERNAME   "/mixer/name/%s"

class OSCMixerInterface : public NetworkOSC{
  public:

    OSCMixerInterface(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_remotePort, String ping, bool _configurable=true): NetworkOSC(name, _OSC_localPort, _OSC_IPaddress,_OSC_remotePort, ping, _configurable){
    }       
};

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation: public OSCMixerInterface{
  public:
    MixingStation():OSCMixerInterface(INTERFACE_MIXINGSTATION,8889,"192.168.0.1",10023,"/info"){
      //destination->host = "192.168.99.99";
    }     
};

#define INTERFACE_BEHRINGERXM "BehringerXM"
class BehringerXM: public OSCMixerInterface{
  public:
    BehringerXM():OSCMixerInterface(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,"/xinfo"){
      //destination->host = "192.168.0.1";
    }     
};

#endif