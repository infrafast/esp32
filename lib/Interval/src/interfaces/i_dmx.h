#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

class DMXInterface: public DeviceInterface{
  #define GROUP_INTERFACE_DMX "DMX"
  public:
  
    DMXInterface(const String& name, bool _configurable):DeviceInterface(name,GROUP_INTERFACE_DMX,_configurable){}

    virtual String getProviderName(){
      return GROUP_INTERFACE_DMX;        // the provider is common to all MIDI strips
    }    

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, NetworkInterface{
  #define CONFIG_ARTNET "voidArtnetparam" 
  public:
    String voidArtnetParam = "artnet123";

    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE,_configurable), NetworkInterface(this){
      icon = "b";
    }

    virtual void monitor(){}    

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_ARTNET] = voidArtnetParam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidArtnetParam = config[CONFIG_ARTNET].as<String>();
    }  

    virtual void* read(){return nullptr;}

    virtual void begin(){}
    virtual void end(){}

    
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
class ThreePinDMXInterface: public DMXInterface{
  #define CONFIG_3PINDMX "void3PinDMXparam" 
  public:
    String void3pinDMXParam = "3pin123";

    ThreePinDMXInterface(bool _configurable):DMXInterface(THREEEPINDMX_INTERFACE,_configurable){
      icon = "a";
    }

    virtual void monitor(){

    } 

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_3PINDMX] = void3pinDMXParam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      void3pinDMXParam = config[CONFIG_3PINDMX].as<String>();
    }  

    virtual void* read(){return nullptr;}

    virtual void begin(){}
    virtual void end(){}

    
};

#endif