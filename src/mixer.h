#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

#include "Interfaces.h"

#define IF_STATE_LIVE               "live"
#define IF_STATE_DATASYNCING        "syncing"

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

struct BaseMixerDefinition{
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
    MixerProtocol mixerProtocol;
    //BaseMixerDefinition mixerDef  : implemented by derived class


    bool dataSync = false;

    unsigned long lastTimeMixerWasSet = 0;     

    template<typename... Args>
    MixerDevice(Args&&... args)
        : NetworkBase(std::forward<Args>(args)...)
    {
      mixerID.model = "undef";
      mixerID.name = "offline";
      mixerID.ip = IPAddress(0,0,0,0);
    }

    virtual void  startMixerDataSync() = 0;

    void setMixer(String& name, String&model, IPAddress& ip) {
      DEBUG_INFO("%s:setMixer(%s,%s,%s)",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newID { name, model, ip };

      bool mixerChanged = !mixerID.equals(newID);
      
      if (mixerChanged){
        DEBUG_PRINT("\t%s",mixerChanged?"yes✅":"no");
        newMixer(newID);
      }           

      bool offlineTooLong = (this->state.label == IF_STATE_PENDING) &&
                            this->pendingSinceMs != 0 &&
                            (millis() - this->pendingSinceMs >= 15000);

      if (offlineTooLong) dataSync = false;

      DEBUG_PRINT("\t%s%s",offlineTooLong?"offline too long⌛":"",dataSync?"":"not synced🔄");

      if (!dataSync){
        if (this->state.label != IF_STATE_DATASYNCING) startMixerDataSync();
      }else{
        if (this->state.label != IF_STATE_LIVE) setStateMixerLive();
      }

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

    void newMixer(const MixerIdentity& newMixer){
      DEBUG_INFO("%s:newMixer()🎛",this->interfaceName.c_str());            
      mixerID = newMixer;     
      dataSync = false;       // mixer is new so sync it is a must
      setModel(mixerID.model.c_str());
      this->state.broadcastConfig = true;   // inform web app about the chnage of mixer
    }

    void setStatePendingDataSync(){
      DEBUG_INFO("%s:setStatePendingDataSync()🕒",this->interfaceName.c_str());
      this->state.code = IF_READY;
      this->state.label = IF_STATE_DATASYNCING;
      this->state.details = "Getting data";
      this->state.led = "green fast";      
    }

    void setStateMixerLive(){
      this->setStateConnected();
      DEBUG_INFO("%s:setStateMixerLive()🟢",this->interfaceName.c_str());
      this->state.label = IF_STATE_LIVE;
      this->state.details = concatStrings(mixerID.name," ",mixerID.model," ",mixerID.ip.toString(),":",this->remotePort);
    }    

    virtual void setStateDisabled() override{
      NetworkBase::setStateDisabled();
      this->state.details =  "Interface is offline";
    }

};


#endif