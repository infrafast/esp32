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

    /*void clear() {
        name = "";
        model = "";
        ip = IPAddress(0,0,0,0);
    }

    bool isValid() const {
        return ip != IPAddress(0,0,0,0);
    }*/
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
      currentMixer.model = "undef";
      currentMixer.name = "offline";
      currentMixer.ip = IPAddress(0,0,0,0);
    }

    virtual void  startMixerDataSync() = 0;

    /*virtual bool begin(){     // called by init()
      NetworkBase::begin();
      DEBUG_INFO("%s:begin() setting up mixer");
      setModel(currentMixer.model);
    }*/

    bool isNewMixer(String& name, String&model, IPAddress& ip) {
      DEBUG_INFO("%s:isNewMixer(%s,%s,%s)",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newMixer { name, model, ip };

      bool mixerChanged = !currentMixer.equals(newMixer);
      
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

    void setModel(const char* model){
      DEBUG_INFO("%s:setModel(%s)",this->interfaceName.c_str(),model);                  
      if (currentMixer.model != model){
        // perform json file load to set mixer attributes like number of channels, etc....
        // clear all datasets
        // ...            
        DEBUG_PRINT("\tMODEL HAS CHANGED: CLEARING MIXER DATASET AND SETTTING UP ATTRIBUTES");                  
      }else{
        DEBUG_PRINT("\tmodel didn't change");                  
      }
    }

    void setMixer(const MixerIdentity& newMixer){
      currentMixer = newMixer;     
      //dataSync = false;
      DEBUG_INFO("%s:setMixer(%s,%s,%s)🎛, data sync cleared",this->interfaceName.c_str(),currentMixer.name.c_str(),currentMixer.model.c_str(),currentMixer.ip.toString().c_str());            
      setModel(currentMixer.model.c_str());
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