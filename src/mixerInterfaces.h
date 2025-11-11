#ifndef MixerInterface_h
#define MixerInterface_h

#include "Interfaces.h"


#define GROUP_INTERFACE_MIXER "Mixer"   
#define ADDRESS_MIXERMODEL  "/mixer/model/%s"
#define ADDRESS_MIXERNAME   "/mixer/name/%s"

class MixerInterface: public DeviceInterface{

  public:
    String mixerModel;
    String mixerName;

    MixerInterface(String _name, String _model):mixerModel(_model),DeviceInterface(_name,GROUP_INTERFACE_MIXER){}
    
   
    virtual const String& getProviderName(){
      return interfaceName;        // mixers do not belong to the group dataset interface name but have their own provider as MixingStation dropdown will not be the same as X32
    }      
   
    virtual bool restoreProperty(const char* property){
      String _tmp;
      if (sscanf(property, ADDRESS_MIXERNAME, &_tmp)) {
        if (_tmp != mixerName) configChanged = true;
        mixerName = _tmp;       
      }
      if (sscanf(property, ADDRESS_MIXERMODEL, &_tmp)) {
        if (mixerModel != _tmp) configChanged = true;
        mixerModel = _tmp;       
      }
      return configChanged;   
    }
};

class OSCMixerInterface : public MixerInterface{
  public:
    OSCInterface* osc;
    OSCMixerInterface(String _name, String _model, OSCInterface*_osc):osc(_osc),MixerInterface(_name,_model){
    }
          
    virtual void monitor(){}

    virtual void* read(){return nullptr;}

};

#define MIXINGSTATION_INTERFACE "MixingStation"
class MixingStation: public OSCMixerInterface{
  #define MS_CONFIG_AUTOCONNECT "/autoConnect/%d"
  #define MS_CONFIG_AUTOCONNECT_MIXERID "/mixerID/%d"
  #define MS_CONFIG_AUTOCONNECT_APIPORT "/restAPIport/%d"
  public:
    uint8_t autoConnectMixerID = 1;
    int RESTAPI_port = 8080;
    String mixerIPaddress = "1.1.1.1";
    bool autoConnect = false;    

    MixingStation(OSCInterface* osc):OSCMixerInterface("MixingStation","virtual",osc){
      interfaceName = MIXINGSTATION_INTERFACE;
    }

    String getFilename(){
      return MIXINGSTATION_INTERFACE"_if.cfg";
    }

    virtual bool restoreProperty(const char* property){
      bool changed = false;
      if (config.containsKey(MS_CONFIG_AUTOCONNECT_MIXERID)){ 
        autoConnectMixerID = config[MS_CONFIG_AUTOCONNECT_MIXERID];
         changed = true;
      }
      if (config.containsKey(MS_CONFIG_AUTOCONNECT_MIXERID)){ 
        RESTAPI_port = config[MS_CONFIG_AUTOCONNECT_MIXERID];
         changed = true;
      }
      if (config.containsKey(MS_CONFIG_AUTOCONNECT)){ 
        autoConnect = config[MS_CONFIG_AUTOCONNECT];
         changed = true;
      }
      if (config.containsKey(MS_CONFIG_AUTOCONNECT)){
         mixerIPaddress = config[MS_CONFIG_AUTOCONNECT].as<String>();   
         changed = true;
      }
      OSCMixerInterface::restore(config);
      return changed;
    }            

    virtual bool derivedBegin(){
      return false;
    }    

    virtual bool end(){
      return false;
    }         
};

class BehringerXMR: public OSCMixerInterface{
  #define BEHRINGER_CONFIG_DISCOVERY "discovery"
  public:

    bool discovery = false;

    BehringerXMR(OSCInterface* osc):OSCMixerInterface("X32Emul","X32",osc){
      interfaceName = "X+Mseries-"; //+osc->interfaceName;
    }  

    virtual bool restoreProperty(const char* property){
      bool changed = false;
      if (config.containsKey(BEHRINGER_CONFIG_DISCOVERY)){
        discovery = config[BEHRINGER_CONFIG_DISCOVERY];
        changed = true;
      }
      changed |= OSCMixerInterface::restore(config);
      return changed;
    }            
};

#endif