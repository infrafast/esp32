#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

#include "Interfaces.h"

#define IF_STATE_LIVE               "live"
#define IF_STATE_DATASYNCING        "Getting data"

#define MIXER_DEFINITION_PATH "/"

class DataSetSynced;

class Synchronizer{
  public:
    bool synced = false;

    uint16_t totalToSync = 0;   // sum of items to be synced
    uint16_t syncedSoFar = 0;
    uint8_t  dsIndex = 0;       // which dataset we are currently synchronizing

    void reset();
    int8_t update();

    void add(DataSetSynced* ds){
       datasets.push_back(ds);
    }

  protected:
    std::vector<DataSetSynced*> datasets;    

};


#define UNSYNC_PATTERN "~"
class DataSetSynced: public DataSet{
  public: 
    String* values = nullptr;
    uint16_t syncCount = 0;
    unsigned long lastSyncTime = 0; 
    //int offset = 0;
    String defaultName="";
    String pattern="";
    uint8_t size = 0;

    DataSetSynced(Address* adress,String name,String defVal,Synchronizer* synchronizer) : DataSet(adress,name,&dataRepo),defaultName(defVal) {
      init(1);
      synchronizer->add(this);    // a SyncDataSet self register himself to the overall synchroniser
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

    void init(uint16_t _size/*, int _offset*/){
      //offset = _offset;
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
          values[i] = concatStrings(defaultName, i);
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

#define ADDRESS_CH_NAME "/chname/%d"
DropDownAddress addressCHname = DropDownAddress(ADDRESS_CH_NAME);

#define ADDRESS_BUS_NAME "/busname/%d"
DropDownAddress addressBUSname = DropDownAddress(ADDRESS_BUS_NAME);

#define ADDRESS_FX_NAME "/fxname/%d"
DropDownAddress addressFXname = DropDownAddress(ADDRESS_FX_NAME);

#define ADDRESS_AUX_NAME "/auxname/%d"
DropDownAddress addressAUXname = DropDownAddress(ADDRESS_AUX_NAME);


void Synchronizer::reset(){
  DEBUG_INFO("Synchronizer:reset()");      
  // resetting all sync counters
  totalToSync = 0;
  syncedSoFar = 0;
  dsIndex = 0;
   for (const auto& dataset : datasets){
    dataset->syncCount = 0;
    totalToSync += dataset->size;
  }  
}
    
int8_t Synchronizer::update(){  
  
  if (dsIndex >= datasets.size() || synced){ // we've synced it all
    return 100; //100%
  }

  DataSetSynced* dataset = datasets[dsIndex]; //sync in serial, next dataset

  if (dataset->syncCount < dataset->getSize()){ // complete ?
    if (dataset->getValuePtr(dataset->syncCount)->indexOf(UNSYNC_PATTERN) != -1){  // if not sync, we sync it.
           
    }   
  }else dsIndex++;

  // computing the overall progress
  int8_t progress = map(syncedSoFar, 0, totalToSync, 0, 100);      
  if (progress >=100) synced = true;
  else synced = false;  

  return progress;      
}

template <typename NetworkBase>
class MixerDevice : public NetworkBase {
public:

    // mixer definition
    MixerIdentity mixerID;
    MixerProtocol mixerProtocol;
    BaseMixerDefinition* mixerDef;  // implemented by derived class

    // properties that are synced
    DataSetSynced* chNamesDS;
    DataSetSynced* busNamesDS;  
    DataSetSynced* auxNamesDS;  
    DataSetSynced* fxNamesDS;

    // this classes does all the work
    Synchronizer synchronizer;

    template<typename... Args>
    MixerDevice(Args&&... args): NetworkBase(std::forward<Args>(args)...){
      mixerID.model = "undef";
      mixerID.name = "offline";
      mixerID.ip = IPAddress(0,0,0,0);

      // SyncDataSet auto register themelves to the overall synchroniser
      chNamesDS = new DataSetSynced(&addressCHname,this->interfaceName,UNSYNC_PATTERN "CH", &synchronizer); 
      busNamesDS = new DataSetSynced(&addressBUSname,this->interfaceName,UNSYNC_PATTERN "BUS", &synchronizer);
      auxNamesDS = new DataSetSynced(&addressAUXname,this->interfaceName,UNSYNC_PATTERN "AUX", &synchronizer);
      fxNamesDS = new DataSetSynced(&addressFXname,this->interfaceName,UNSYNC_PATTERN "FX", &synchronizer);

      setDatasetPattern();    // this defines the templates to recognize incoming frame to match

    }
    
    //// pure virtual utilities ////
    virtual void startMixerDataSync() = 0;
    virtual bool restoreMixerProfile() = 0;
    virtual bool deserializeMixerDefinitionFromFile(const char* filename) = 0;    
    virtual void setDatasetPattern() = 0;
    
    //// pure virtual utilities ////
    virtual void queryCHname(uint8_t index);


    virtual const String& getProviderNameForData(){ return this->interfaceName;}   // we are our own provider do not forget it!

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

      DEBUG_PRINT("\t%s",offlineTooLong?"offline too long⌛":"");

      if (offlineTooLong) synchronizer.reset();

      if (!synchronizer.synced){
        DEBUG_PRINT("\tsynced🔄 required");
        startMixerDataSync();
      }else{
        if (this->state.label != IF_STATE_LIVE) setStateMixerLive();
      }

    }

    void setModel(const char* model){
      DEBUG_INFO("%s:setModel(%s)",this->interfaceName.c_str(),model);
      if (mixerID.model != model){
        mixerID.model = model;                                
        restoreMixerProfile();        // since the model has changed we have to rebuild it    
        initDatasets();
        // as the mixer attributes have changed,
        // ASK APPLICATION MANAGER TO REPUBLISH THE BANKS
        app.webAppToRefreshBanks();
      }else{
        DEBUG_PRINT("\t model didn't change!");
      }
    }

    void initDatasets(){      
      DEBUG_INFO("%s:initDatasets()",this->interfaceName.c_str());
      
      // setting up the data size
      chNamesDS->init(mixerDef->channelCount);
      busNamesDS->init(mixerDef->busCount);
      auxNamesDS->init(mixerDef->auxCount);
      fxNamesDS->init(mixerDef->fxCount);
      
      synchronizer.reset();
    }   

    void newMixer(const MixerIdentity& newMixer){
      DEBUG_INFO("%s:newMixer()🎛",this->interfaceName.c_str());            
      mixerID = newMixer;     
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
      this->state.label = "Offline";
      this->state.details =  "";    // remove the default statement "interface is disabled"
    }

};


#endif