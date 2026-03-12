#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

class MixerInterface{
  public:
    String mixerName = "mixerName";
    String mixerModel = "mixerModel";
    IPAddress mixerIP = IPAddress(0,0,0,0);      
};  

#endif