#ifndef MixerInterface_h
#define MixerInterface_h

#include "Interfaces.h"


#define GROUP_INTERFACE_MIXER "Mixer"   
class MixerInterface: public DeviceInterface{
  #define CONFIG_MIXERNAME "mixerName"
  #define CONFIG_MIXERMODEL "model"    
  public:
    String mixerModel;
    MixerInterface(String _name, String _model):mixerModel(_model),DeviceInterface(_name,GROUP_INTERFACE_MIXER){}
    
   
    virtual String getProviderName(){
      return interfaceName;        // mixers have their own provider as MixingStation dropdown will not be the same as X32
    }      

    virtual void derivedConfig(JsonObject& config){
      //{"mixerName":"software","model":"virtual"}      
      config[CONFIG_MIXERNAME] = interfaceName;
      config[CONFIG_MIXERMODEL] = mixerModel;
    }   
    
    virtual void restore(const JsonObject& config){
      //mixerName = config[CONFIG_MIXERNAME].as<String>(); // since mixerName is dynamically retrieved from mixer, we should not consider it
      mixerModel = config[CONFIG_MIXERMODEL].as<String>();
    }      
};

class OSCMixerInterface : public MixerInterface{
  public:
    OSCInterface* osc;
    OSCMixerInterface(String _name, String _model, OSCInterface*_osc):osc(_osc),MixerInterface(_name,_model){
    }
          
    virtual void derivedConfig(JsonObject& config){  
      MixerInterface::derivedConfig(config);
      //{"mixerName":"software","model":"virtual"}

      // we retrieve the config of the OSCInterface instance which is specific
      String usesConf = osc->serializeConfig(false); // false : we partially serialize 
      JsonDocument oscConf;

      // convert it back to an object that can be attached to the parent config 
      DeserializationError error = deserializeJson(oscConf, usesConf);
      if (error) {
        DEBUG_PRINT("\n%s serialize %s",interfaceName.c_str(),error.c_str());
        return;
      }
      oscConf.shrinkToFit();

      //config[INTERFACE_USES] = oscConf;
      for (JsonPair kv : oscConf.as<JsonObject>()) config[kv.key()] = kv.value();
      /* 
        "uses":{"NetworkOSC":{"config":{"oscIP":"1.2.3.4-restored","oscPort":10029}
        "uses":{"MIDIOSC":{"config":{"voidSysexParam":"null"}
      */
      
    } 

    virtual void monitor(){}

    virtual void* read(){return nullptr;}


    virtual void restore(const JsonObject& config){
      osc->restore(config);
      MixerInterface::restore(config);
    }  
};


class MixingStation: public OSCMixerInterface{
  #define MS_CONFIG_AUTOCONNECT "autoConnect"
  #define MS_CONFIG_AUTOCONNECT_MIXERIP "mixerIP"
  #define MS_CONFIG_AUTOCONNECT_MIXERID "mixerID"
  #define MS_CONFIG_AUTOCONNECT_APIPORT "restAPIport"
  public:
    uint8_t autoConnectMixerID = 1;
    int RESTAPI_port = 8080;
    String mixerIPaddress = "1.1.1.1";
    bool autoConnect = false;    

    MixingStation(OSCInterface* osc):OSCMixerInterface("undef","virtual",osc){
      interfaceName = "MixingStation";
    }

    virtual void derivedConfig(JsonObject& config){      
      OSCMixerInterface::derivedConfig(config); 
      // uses NetworkOSC, so outcome will be: {"mixerName":"undef","model":"virtual","oscIP":"5.6.7.8","oscPort":3000}
      config[MS_CONFIG_AUTOCONNECT_MIXERID] = autoConnectMixerID ;
      config[MS_CONFIG_AUTOCONNECT_APIPORT] = RESTAPI_port;
      config[MS_CONFIG_AUTOCONNECT] = autoConnect;
      config[MS_CONFIG_AUTOCONNECT_MIXERIP] = mixerIPaddress;      
      //{"MixingStation":{"config":{"mixerName":"undef","model":"virtual","oscIP":"5.6.7.8","oscPort":3000,"mixerID":1,"restAPIport":8080,"autoConnect":false,"mixerIP":"1.1.1.1"}}}      
    }          

    virtual void restore(const JsonObject& config){
      autoConnectMixerID = config[MS_CONFIG_AUTOCONNECT_MIXERID];
      RESTAPI_port = config[MS_CONFIG_AUTOCONNECT_APIPORT];
      autoConnect = config[MS_CONFIG_AUTOCONNECT];
      mixerIPaddress = config[MS_CONFIG_AUTOCONNECT_MIXERIP].as<String>();   
      OSCMixerInterface::restore(config);
    }            

    virtual void begin(){}    

    virtual void end(){}         
};

class BehringerXMR: public OSCMixerInterface{
  #define BEHRINGER_CONFIG_DISCOVERY "discovery"
  public:

    bool discovery = false;

    BehringerXMR(OSCInterface* osc):OSCMixerInterface("X32Emul","X32",osc){
      interfaceName = "X+Mseries-"+osc->interfaceName;
    }

    virtual void derivedConfig(JsonObject& config){
      // uses NetworkOSC, so outcome will be: {"mixerName":"undef","model":"virtual","oscIP":"5.6.7.8","oscPort":3000}
      OSCMixerInterface::derivedConfig(config);
      config[BEHRINGER_CONFIG_DISCOVERY] = discovery;      
    }          

    virtual void restore(const JsonObject& config){
      discovery = config[BEHRINGER_CONFIG_DISCOVERY];
      OSCMixerInterface::restore(config);
    }            
};

#endif