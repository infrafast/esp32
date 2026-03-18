#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

#include "Interfaces.h"

#define IF_STATE_LIVE               "live"
#define IF_STATE_DATASYNCING        "Getting data"

#define MIXER_DEFINITION_PATH "/"

#define UNSYNC_PATTERN "~"
class DataSetSynced: public DataSet{
  public: 
    String* values = nullptr;
    uint16_t syncCount = 0;
    unsigned long lastSyncTime = 0; 
    //int offset = 0;
    String defaultName="";
    uint8_t size;

    DataSetSynced(Address* adress,String name,uint16_t _size,String defVal/*, int _offset*/) : DataSet(adress,name,&dataRepo){
      init(_size, defVal/*, _offset*/);
    }

    virtual uint16_t getSize() override{
      return size;
    };

    void set(uint16_t index, String value){
      if (index>size) return;
      
      if (values[index] != value){
        values[index] = value;
      }
       
    }

    void init(uint16_t _size,String defVal/*, int _offset*/){
      //offset = _offset;
      defaultName = defVal;
      //DEBUG_PRINT("\nDataSetSynced init(%d,%s,%d)",size,defVal.c_str(),offset);

      if (size != _size){
        size = _size;
        // Supprimer le tableau existant s'il y en a un
        if (values != nullptr) delete[] values; // Libérer la mémoire précédemment allouée
        // Créer un nouveau tableau dynamique avec la taille passée
        values = new String[size];
        // Initialiser les valeurs (par exemple, 0)
      }
     
      for (int i = 0; i < size; ++i) {
          values[i] = concatStrings(defVal, i);
      }
    }


    virtual const String* getValuePtr(uint16_t i) override{    
      if (i < size) return &values[i];
      else return &defaultName;
    }      

};



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

#define ADDRESS_CH_NAME "/ch/name/%s"
DropDownAddress addressCHname = DropDownAddress(ADDRESS_CH_NAME);


template <typename NetworkBase>
class MixerDevice : public NetworkBase {
public:

    // mixer definition
    MixerIdentity mixerID;
    MixerProtocol mixerProtocol;
    BaseMixerDefinition* mixerDef;  // implemented by derived class


    DataSetSynced* chNames;
    DataSetSynced* busNames;  
    DataSetSynced* auxNames;  
    DataSetSynced* fxRetNames;
    DataSetSynced* mainNames;

    std::vector<DataSetSynced*> dsNames;

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
        buildDatasets();

        // as the mixer attributes have changed,
        // ASK APPLICATION MANAGER TO REPUBLISH THE BANKS
        app.webAppToRefreshBanks();
      }else{
        DEBUG_PRINT("\t model didn't change!");
      }
    }

    void buildDatasets(){
      chNames = new DataSetSynced(&addressCHname,this->interfaceName,mixerDef->channelCount,UNSYNC_PATTERN "CH");

      /*busNames = new DataSetSynced(MS_BUS_SHORTNAME,ADDRESS_BUSNAME,INPUTSELECT,busSize,UNSYNC_PATTERN "BUS",1);
      auxNames = new DataSetSynced(MS_AUX_SHORTNAME,ADDRESS_AUXNAME,INPUTSELECT,auxSize,UNSYNC_PATTERN "AUX",1);
      fxRetNames = new DataSetSynced(MS_FXRET_SHORTNAME,ADDRESS_FXRETNAME,INPUTSELECT,fxRetSize,UNSYNC_PATTERN "FX",1);
      mainNames = new DataSetSynced(MS_MAIN_SHORTNAME,ADDRESS_MAINNAME, INPUTSELECT,mainSize,UNSYNC_PATTERN "MAIN",1);      */
    }

    void newMixer(const MixerIdentity& newMixer){
      DEBUG_INFO("%s:newMixer()🎛",this->interfaceName.c_str());            
      mixerID = newMixer;     
      dataSync = false;       // mixer is new so sync it is a must
      setModel(mixerID.model.c_str());
      this->state.broadcastConfig = true;   // inform web app about the chnage of mixer
    }

    void setStatePendingDataSync(){
      NetworkBase::setStateConnected();
      DEBUG_INFO("%s:setStatePendingDataSync()🕒",this->interfaceName.c_str());      
      this->state.label = IF_STATE_DATASYNCING;
      setMixerDetails();
      this->state.led = "green fast";      
    }

    void setMixerDetails(){
      this->state.details = concatStrings(mixerID.name," ",mixerID.model," ",mixerID.ip.toString(),":",this->remotePort);
    }

    void setStateMixerLive(){
      this->setStateConnected();
      DEBUG_INFO("%s:setStateMixerLive()🟢",this->interfaceName.c_str());
      this->state.label = IF_STATE_LIVE;      
      setMixerDetails();
    }    

    virtual void setStateDisabled() override{
      NetworkBase::setStateDisabled();
      this->state.details =  "Interface is offline";
    }

};


#endif