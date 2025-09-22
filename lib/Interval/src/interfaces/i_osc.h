#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"

class OSCInterface: public DeviceInterface{
  #define GROUP_INTERFACE_OSC "OSC"
  public:
    OSCInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_OSC,_configurable){}

    virtual String getProviderName(){
      return GROUP_INTERFACE_OSC;        // the provider is common to all MIDI strips
    }    
};

#define NETWORKOSC_INTERFACE "Network(OSC)"
class NetworkOSC: public OSCInterface,NetworkInterface{
  #define CONFIG_OSCIP  "oscIP"
  #define CONFIG_OSCPORT  "oscPort"
  public:
    String OSC_IPaddress;
    int OSC_port;

    NetworkOSC(String _OSC_IPaddress,int _OSC_port,bool _configurable=false):OSC_IPaddress(_OSC_IPaddress),OSC_port(_OSC_port), OSCInterface(_configurable),NetworkInterface(this){
      interfaceName = NETWORKOSC_INTERFACE;
      icon = "g";     
    }

    virtual void derivedConfig(JsonObject& config){   
      //{"oscIP":"5.6.7.8","oscPort":3000}
      config[CONFIG_OSCIP] = OSC_IPaddress;
      config[CONFIG_OSCPORT] = OSC_port;         
    }

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_OSCIP)){
        OSC_IPaddress = config[CONFIG_OSCIP].as<String>();
        changed = true;
        DEBUG_PRINT("\n%s param: %s =>%s",interfaceName.c_str(),CONFIG_OSCIP, OSC_IPaddress.c_str());
      }
      if (config.containsKey(CONFIG_OSCPORT)){
        OSC_port = config[CONFIG_OSCPORT];        
        changed = true;
        DEBUG_PRINT("\n%s param: %s =>%i",interfaceName.c_str(),CONFIG_OSCPORT, OSC_port);        
      }
      return changed;
    }        

    virtual void monitor(){}

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    virtual void end(){}


};



#endif