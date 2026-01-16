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

    virtual void restoreProperty(const char* property){
      DEBUG_PRINT("\n%s::restoreProperty() not applicable");
    }     

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface, public NetWorkInterface {

  public:
    String voidArtnetParam = "artnet123";
    NetworkInterfaceMonitor netHelper = NetworkInterfaceMonitor(this,this);

    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
    }

    virtual void restoreProperty(const char* property){
      DEBUG_PRINT("\n%s::restoreProperty() not applicable");
    }  

    virtual void monitor(){     
      netHelper.monitor();
    }

  
    virtual void* read(){return nullptr;}

    virtual bool begin(){
      return false;
    }
    
    virtual bool end(){
      return false;      
    }

    virtual void derivedNetworkMonitor(){}

    virtual bool connectNetwork(){
      return false;
    }

    virtual bool disconnectNetwork() {
      return false;
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

    virtual void restoreProperty(const char* property){
      DEBUG_PRINT("\n%s::restoreProperty() not applicable");
    }  

    virtual void* read(){return nullptr;}

    virtual bool begin(){
      return false;
    }

    virtual bool end(){
      return false;
    }

    
};

#endif