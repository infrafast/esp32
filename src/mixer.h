#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

#include "Interfaces.h"

#define MIXER_DEFINITION_PATH "/"

struct MixerIdentity {
    String name;
    String model;
    IPAddress ip;

    bool equals(const MixerIdentity& other) const {
        return name == other.name &&
               model == other.model &&
               ip == other.ip;
    }

    /*void clear() {
        name = "";
        model = "";
        ip = IPAddress(0,0,0,0);
    }

    bool isValid() const {
        return ip != IPAddress(0,0,0,0);
    }*/
};

struct MixerDefinition{
  uint8_t channelCount = 18;
  uint8_t busCount = 4 ;
  uint8_t auxCount = 1;
  uint8_t fxCount = 4; 
};

struct MixerProtocol{
  String anyChannelName = "ch.*.cfg.name";
};



template <typename NetworkBase>
class MixerDevice : public NetworkBase {
public:

    // mixer definition
    MixerIdentity mixerID;
    MixerDefinition mixerDef;
    MixerProtocol mixerProtocol;

    bool dataSync = false;

    unsigned long lastTimeMixerWasSet = 0;     
    //bool dataSync = false;

    template<typename... Args>
    MixerDevice(Args&&... args)
        : NetworkBase(std::forward<Args>(args)...)
    {
      mixerID.model = "undef";
      mixerID.name = "offline";
      mixerID.ip = IPAddress(0,0,0,0);
    }

    virtual void  startMixerDataSync() = 0;

    bool isNewMixer(String& name, String&model, IPAddress& ip) {
      DEBUG_INFO("%s:isNewMixer(%s,%s,%s)",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newMixer { name, model, ip };

      bool mixerChanged = !mixerID.equals(newMixer);
      
      if (mixerChanged){
        DEBUG_PRINT("\t%s",mixerChanged?"yes✅":"no");
        setMixer(newMixer);
      }           
      
      bool offlineTooLong = (this->state.label == IF_STATE_PENDING) &&
                            this->pendingSinceMs != 0 &&
                            (millis() - this->pendingSinceMs >= 15000);

      DEBUG_PRINT("\t%s",offlineTooLong?"offline too long⌛":""); //,dataSync?"":"not synced🔄");

      bool needSync = mixerChanged || offlineTooLong; //|| !dataSync;

      if (needSync) {
        if (this->state.label != IF_STATE_DATASYNCING) startMixerDataSync();            
      }else{
        if (this->state.label != IF_STATE_LIVE) setStateMixerLive();
      }

      return needSync;

    }

    // used to retrieve the 
    virtual bool restoreMixerProfile() = 0;

    virtual bool deserializeMixerDefinitionFromFile(const char* filename) = 0;

    void setModel(const char* model){
      DEBUG_INFO("%s:setModel(%s)",this->interfaceName.c_str(),model);
      if (mixerID.model != model){
        mixerID.model = model;                                
        restoreMixerProfile();        // since the model has changed we have to rebuild it    
        // PROBABLY HERE WE SHOULD BRODCAST interfaceConfig 
        // BUT ALSO SEND AN EVENT TO ASK APPLICATION MANAGER TO REPUBLISH THE BANKS
      }else{
        DEBUG_PRINT("\t model didn't change!");
      }
    }

    void setMixer(const MixerIdentity& newMixer){
      mixerID = newMixer;     
      //dataSync = false;
      DEBUG_INFO("%s:setMixer(%s,%s,%s)🎛, data sync cleared",this->interfaceName.c_str(),mixerID.name.c_str(),mixerID.model.c_str(),mixerID.ip.toString().c_str());            
      setModel(mixerID.model.c_str());
    }

    void setStatePendingDataSync(){
      DEBUG_INFO("%s:setStatePendingDataSync()🕒",this->interfaceName.c_str());
      this->state.code = IF_READY;
      this->state.label = IF_STATE_SYNCING;
      this->state.details = "data sync";
      this->state.led = "green fast";      
    }

    void setStateMixerLive(){
      this->setStateConnected();
      DEBUG_INFO("%s:setStateMixerLive()🟢",this->interfaceName.c_str());
      this->state.label = IF_STATE_LIVE;
      this->state.details = concatStrings(mixerID.name," ",mixerID.model," ",mixerID.ip.toString(),":",this->remotePort);
    }    


};


#endif