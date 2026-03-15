#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   


#include "Interfaces.h"

struct MixerIdentity {
    String name;
    String model;
    IPAddress ip;

    bool equals(const MixerIdentity& other) const {
        return name == other.name &&
               model == other.model &&
               ip == other.ip;
    }

    void clear() {
        name = "";
        model = "";
        ip = IPAddress(0,0,0,0);
    }

    bool isValid() const {
        return ip != IPAddress(0,0,0,0);
    }
};

template <typename NetworkBase>
class MixerDevice : public NetworkBase {
public:

    MixerIdentity currentMixer;

    bool dataSync = false;

    unsigned long lastTimeMixerWasSet = 0;     
    //bool dataSync = false;

    template<typename... Args>
    MixerDevice(Args&&... args)
        : NetworkBase(std::forward<Args>(args)...)
    {

    }

    virtual void  startMixerDataSync() = 0;

    bool syncMixer(String& name, String&model, IPAddress& ip) {
      DEBUG_INFO("%s:syncMixer(%s,%s,%s) 🎛",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newMixer { name, model, ip };

      bool mixerChanged = !currentMixer.equals(newMixer);
      bool offlineTooLong = (this->state.label == IF_STATE_PENDING) &&
                            this->pendingSinceMs != 0 &&
                            (millis() - this->pendingSinceMs >= 20000);

      currentMixer = newMixer;

      bool needSync = mixerChanged || offlineTooLong || !dataSync;

      if (needSync) {
        DEBUG_PRINT("\t%s %s %s",mixerChanged?"mixer changed✅":"",offlineTooLong?"offline too long⌛":"",dataSync?"":"not synced🔄");
        if (this->state.label != IF_STATE_DATASYNCING) startMixerDataSync();            
      }else{
        if (this->state.label != IF_STATE_LIVE) setStateMixerLive();
      }

      return needSync;

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
      this->state.details = concatStrings(currentMixer.name," ",currentMixer.model," ",currentMixer.ip.toString(),":",this->remotePort);
    }

};


#endif