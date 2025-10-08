#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

#define GROUP_INTERFACE_DMX "DMX"
class DMXInterface{

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, NetworkDevice{
  #define CONFIG_ARTNET "voidArtnetparam" 
  public:
    String voidArtnetParam = "artnet123";

    String getFilename(){
      return ARTNETDMX_INTERFACE"_if.cfg";
    }    

    ArtnetDMXInterface(bool _configurable):NetworkDevice(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
    }

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_ARTNET] = voidArtnetParam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_ARTNET)){
        voidArtnetParam = config[CONFIG_ARTNET].as<String>();
        changed = true;
      }
      return changed;
    }  

    virtual void* read(){return nullptr;}

    virtual bool derivedBegin(){
      return false;
    }
    
    virtual bool end(){
      return false;      
    }

    virtual void derivedMonitor(){}

    virtual bool connect(){
      return false;
    }

    virtual bool disconnect(){
      return false;
    }

    
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
class ThreePinDMXInterface: public DMXInterface, public DeviceInterface{
  #define CONFIG_3PINDMX "void3PinDMXparam" 
  public:
    String void3pinDMXParam = "3pin123";

    ThreePinDMXInterface(bool _configurable):DeviceInterface(THREEEPINDMX_INTERFACE,GROUP_INTERFACE_DMX, _configurable){
      icon = "a";
    }

    String getFilename(){
      return THREEEPINDMX_INTERFACE"_if.cfg";
    }   

    virtual void monitor(){

    } 

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_3PINDMX] = void3pinDMXParam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_3PINDMX)){
        void3pinDMXParam = config[CONFIG_3PINDMX].as<String>();
        changed = true;
      }
      return changed;
    }  

    virtual void* read(){return nullptr;}

    virtual bool derivedBegin(){
      return false;
    }

    virtual bool end(){
      return false;
    }

    
};

#endif