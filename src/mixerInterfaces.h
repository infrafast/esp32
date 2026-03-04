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
/*class MixingStation: public OSCMixerInterface{
  public:
    MixingStation():OSCMixerInterface(INTERFACE_MIXINGSTATION,8889,"192.168.0.1",10023,"/info"){
      //destination->host = "192.168.99.99";
    }     
};*/

#define INTERFACE_BEHRINGERXM "BehringerXM"
class BehringerXM: public NetworkOSC, public MixerInterface{
  public:
    BehringerXM():NetworkOSC(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,"/xinfo",true,GROUP_INTERFACE_MIXER){
      //destination->host = "192.168.0.1";
    }     
};

#endif