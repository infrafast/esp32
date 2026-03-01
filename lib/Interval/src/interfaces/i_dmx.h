#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

#define GROUP_INTERFACE_DMX "DMX"
class DMXInterface : public DeviceInterface{

  public:
    DMXInterface(String _name,String _group,bool _configurable):DeviceInterface(_name,_group,_configurable){
      state.switchable = false;
    }
    
    virtual bool begin(){     // called by init()
      DEBUG_PRINT("\n%s::begin() setStateNotImplemented",interfaceName.c_str());
      setStateNotImplemented();
      return true;
    };        

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, public NetWorkService {

  public:
    String voidArtnetParam = "artnet123";

    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
      attachDeviceInterface(this);
      state.switchable = false;
    }

    virtual void monitor(){     
      monitorNetwork();
    }
    
    virtual void monitorService(){}
    virtual bool handleConnectService(){return false;}
    virtual bool handleDisconnectService(){return false;}
        
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
class ThreePinDMXInterface: public DMXInterface{
  public:
    String void3pinDMXParam = "";

    ThreePinDMXInterface(bool _configurable):DMXInterface(THREEEPINDMX_INTERFACE,GROUP_INTERFACE_DMX, _configurable){
      icon = "a";
      state.switchable = false;
    }

    virtual void monitor(){
    } 

    
};

#endif
