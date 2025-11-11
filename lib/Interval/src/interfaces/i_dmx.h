#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

#define GROUP_INTERFACE_DMX "DMX"
class DMXInterface{

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, NetworkDevice{

  public:
    String voidArtnetParam = "artnet123";

    String getFilename(){
      return ARTNETDMX_INTERFACE"_if.cfg";
    }    

    ArtnetDMXInterface(bool _configurable):NetworkDevice(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
    }

    virtual bool restoreProperty(const char* property){

      String configArtNet;
      if (sscanf(property, ADDRESS_CHANGETOPARAMETERNAME, &configArtNet)) {
        if (voidArtnetParam != configArtNet) configChanged = true;
        voidArtnetParam = configArtNet;       
      }

      return configChanged;      

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
  public:
    String void3pinDMXParam = "";

    ThreePinDMXInterface(bool _configurable):DeviceInterface(THREEEPINDMX_INTERFACE,GROUP_INTERFACE_DMX, _configurable){
      icon = "a";
    }

    String getFilename(){
      return THREEEPINDMX_INTERFACE"_if.cfg";
    }   

    virtual void monitor(){

    } 

    virtual bool restoreProperty(const char* property){
      /*bool changed = false;
      if (config.containsKey(CONFIG_3PINDMX)){
        String config3pin = config[CONFIG_3PINDMX].as<String>();
        if (void3pinDMXParam != config3pin) changed = true;
        void3pinDMXParam = config3pin;        
      }
      return changed;*/

      String config3pin;
      if (sscanf(property, ADDRESS_CHANGETOPARAMETERNAME, &config3pin)) {
        if (void3pinDMXParam != config3pin) configChanged = true;
        void3pinDMXParam = config3pin;       
      }

      return configChanged;      
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