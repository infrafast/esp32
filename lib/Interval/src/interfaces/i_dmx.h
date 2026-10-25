/**
 * @file i_dmx.h
 * @brief DMX interface placeholders for Art-Net and physical output.
 */

#ifndef I_DMX_h
#define I_DMX_h

#include "Interfaces.h"

#define GROUP_INTERFACE_DMX "DMX"
/**
 * @brief Base DMX interface currently reporting not-implemented state.
 */
class DMXInterface : public DeviceInterface{

  public:
    /** @brief Initialize DMXInterface with the supplied configuration and bindings. */
    DMXInterface(String _name,String _group,bool _configurable):DeviceInterface(_name,_group,_configurable){
      state.switchable = false;
    }
    
    /** @brief Run the initialization-specific interface startup path. */
    virtual bool _begin() override{     // called by init()
      DEBUG_INFO("%s::_begin() setStateNotImplemented",interfaceName.c_str());
      setStateNotImplemented();
      return true;
    };        

};

#define ARTNETDMX_INTERFACE "Artnet"
/**
 * @brief Art-Net network interface placeholder; no DMX output is implemented here.
 */
class ArtnetDMXInterface: public DMXInterface, public NetWorkService {

  public:
    String voidArtnetParam = "artnet123";

    /** @brief Initialize ArtnetDMXInterface with the supplied configuration and bindings. */
    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE, GROUP_INTERFACE_DMX, _configurable){
      icon = "b";
      attachDeviceInterface(this);
      state.switchable = false;
    }

    /** @brief Start this interface using its current configuration. */
    virtual bool begin(){
      DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
      return true;
    }

    /** @brief End the interface session and release its active connection. */
    virtual bool end(){
      DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());         
      return true;
    }

    /** @brief Poll the interface connection and process pending data. */
    virtual bool monitor() override{     
      return monitorNetwork();
    }
    
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     */
    virtual bool monitorService() override{return false;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     */
    virtual bool handleConnectService(){return false;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     */
    virtual bool handleDisconnectService(){return false;}
        
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
/**
 * @brief Physical DMX interface placeholder.
 */
class ThreePinDMXInterface: public DMXInterface{
  public:
    String void3pinDMXParam = "";

    /** @brief Initialize ThreePinDMXInterface with the supplied configuration and bindings. */
    ThreePinDMXInterface(bool _configurable):DMXInterface(THREEEPINDMX_INTERFACE,GROUP_INTERFACE_DMX, _configurable){
      icon = "a";
      state.switchable = false;
    }

    /** @brief Start this interface using its current configuration. */
    virtual bool begin(){
      DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
      return true;
    }

    /** @brief End the interface session and release its active connection. */
    virtual bool end(){
      DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());         
      return true;
    }

    /**
     * @brief Placeholder hook; no active processing in this implementation.
     *
     * @return Always false.
     */
    virtual bool monitor() override{
      return false;
    } 
    
};

#endif
