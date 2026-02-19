#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

#define GROUP_INTERFACE_DMX "DMX"
class DMXInterface : public DeviceInterface{

  public:
    DMXInterface(String _name,String _group,bool _configurable):DeviceInterface(_name,_group,_configurable){}

    virtual bool load(){
      DEBUG_PRINT("\n%s::load() not applicable",interfaceName.c_str());
      return true;
    }

    virtual bool save(){
      DEBUG_PRINT("\n%s::save() not applicable",interfaceName.c_str());
      return true;      
    }    
};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, public NetWorkInterface {

  public:
    String voidArtnetParam = "artnet123";

    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
      attachDeviceInterface(this);
    }

    virtual void monitor(){     
      monitorNetworkInterface();
    }
    
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
class ThreePinDMXInterface: public DMXInterface{
  public:
    String void3pinDMXParam = "";

    ThreePinDMXInterface(bool _configurable):DMXInterface(THREEEPINDMX_INTERFACE,GROUP_INTERFACE_DMX, _configurable){
      icon = "a";
    }

    virtual void monitor(){

    } 

    
};

#endif
