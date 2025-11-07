#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"

#define GROUP_INTERFACE_OSC "OSC"
class OSCInterface{
  public:
};

class NetworkOSC: public OSCInterface, public NetworkDevice{
  #define CONFIG_OSCIP  "oscIP"
  #define CONFIG_OSCPORT  "oscPort"
  public:
    String OSC_IPaddress;
    int OSC_port;

    String getFilename(){
      return interfaceName+"_if.cfg";
    }

    NetworkOSC(String name, String _OSC_IPaddress,int _OSC_port,bool _configurable=false):OSC_IPaddress(_OSC_IPaddress),OSC_port(_OSC_port), NetworkDevice(name, GROUP_INTERFACE_OSC,_configurable){
      interfaceName = name;
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
        String OSCIP = config[CONFIG_OSCIP].as<String>();
        if (OSC_IPaddress != OSCIP) changed = true;
        OSC_IPaddress = OSCIP;
        DEBUG_PRINT("\n%s param: %s =>%s",interfaceName.c_str(),CONFIG_OSCIP, OSC_IPaddress.c_str());
      }
      if (config.containsKey(CONFIG_OSCPORT)){
        int OSCport = config[CONFIG_OSCPORT];        
        if (OSC_port != OSCport) changed = true;
        OSC_port = OSCport;
        DEBUG_PRINT("\n%s param: %s =>%i",interfaceName.c_str(),CONFIG_OSCPORT, OSC_port);        
      }
      return changed;
    }        

    virtual void derivedMonitor(){}

    virtual void* read(){return nullptr;}

    virtual bool derivedBegin(){
      return false;
    }

    virtual bool end(){
      return false;
    }

    virtual bool connect(){
      return false;
    }

    virtual bool disconnect(){
      return false;
    }


};



#endif