// without callback

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

    Synchronizer(){}   

    bool synced = false;
    bool syncRequestLocker = false;
    unsigned long lockStartTime = 0;    

    uint16_t totalToSync = 0;   // sum of items to be synced
    uint16_t syncedSoFar = 0;
    uint8_t  dsIndex = 0;       // which dataset we are currently synchronizing

    void reset();
    int8_t update(uint16_t latency);

    void add(DataSetSynced* ds){
       datasets.push_back(ds);
    }

    void lockSyncRequest() {
      syncRequestLocker = true;
      lockStartTime = millis();
    }    

    DataSetSynced* getCurrent(){
      if (syncRequestLocker) return nullptr;
      else return datasets[dsIndex];
    }

  protected:
    std::vector<DataSetSynced*> datasets;  

};


#define UNSYNC_PATTERN "~"
#define MAX_SYNC_DS_SIZE 48
class DataSetSynced : public DataSet {
  public:
    String values[MAX_SYNC_DS_SIZE];
    uint16_t syncStep = 0;
    String defaultName = "";
    uint16_t size = 0;

    DataSetSynced(Address* adress, String name, String defVal, Synchronizer* synchronizer)
      : DataSet(adress, name, &dataRepo), defaultName(defVal) {
      init(1);                  // one data by default
      synchronizer->add(this);  // self registration
    }

    virtual uint16_t getSize() override {
      return size;
    }

    virtual const String* getValuePtr(uint16_t i) override {
      if (i < size) return &values[i];
      return &errorDatasetOOR;
    }

    void init(uint16_t newSize) {
      size = (newSize > MAX_SYNC_DS_SIZE) ? MAX_SYNC_DS_SIZE : newSize;
      clearValues();
    }

    void clearValues() {
      for (uint16_t i = 0; i < size; ++i) {
        values[i] = concatStrings(UNSYNC_PATTERN, defaultName, i + 1);
      }
      syncStep = 0;
    }

    void set(uint16_t index, const String& value) {
      if (index >= size) return;

      String newValue = (value == "") ? concatStrings(defaultName, index + 1) : value;

      if (values[index] == newValue) return;

      values[index] = newValue;

      char addrBuf[64];
      snprintf(addrBuf, sizeof(addrBuf), address->pattern.c_str(), index + 1); // addresses start at 1
      
      //{"dataSet":{"updateDataSets":[{"XMSeries":{"/ch/name/3":"toto"}}]}}
      String frame = concatStrings(
        "{\"updateDataSets\":[{\"",
        name,
        "\":{\"",
        String(addrBuf),
        "\":\"",
        values[index],
        "\"}}]}"
      );

      app.broadCast(frame.c_str());
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
};

struct BaseMixerDefinition{
  uint8_t channelCount = 18;
  uint8_t busCount = 4 ;
  uint8_t auxCount = 1;
  uint8_t fxCount = 4; 
  uint8_t mtxCount = 8;
  uint8_t mainCount = 2;
  uint8_t DCAcount = 6;
  uint8_t fxSndCount = 2;
  uint8_t groupCount = 2;
  uint8_t talkbackCount = 1;
};

#define ADDRESS_CH_NAME "/chname/%d"
DropDownAddress addressCHname = DropDownAddress(ADDRESS_CH_NAME);

#define ADDRESS_BUS_NAME "/busname/%d"
DropDownAddress addressBUSname = DropDownAddress(ADDRESS_BUS_NAME);

#define ADDRESS_FX_NAME "/fxname/%d"
DropDownAddress addressFXname = DropDownAddress(ADDRESS_FX_NAME);

#define ADDRESS_AUX_NAME "/auxname/%d"
DropDownAddress addressAUXname = DropDownAddress(ADDRESS_AUX_NAME);


class MixerBase {
public:
    struct Binding {
      DeviceInterface* device = nullptr;
      MixerBase* mixer = nullptr;
    };

    // properties that are synced
    DataSetSynced* chNamesDS;
    DataSetSynced* busNamesDS;  
    DataSetSynced* auxNamesDS;  
    DataSetSynced* fxNamesDS;

    virtual ~MixerBase() {}

    //// pure virtual utilities ////
    //virtual void startMixerDataSync() = 0;
    virtual bool restoreMixerProfile() = 0;
    virtual bool deserializeMixerDefinitionFromFile(const char* filename) = 0;    

    //// pure virtual mixer methods called by the strips ////
    virtual void dropDSnameRequest(DataSetSynced*, uint8_t) = 0;    
    virtual void dropCHMuteStateRequest(uint8_t index) = 0;
    virtual void dropBUSMuteStateRequest(uint8_t index) = 0;
    virtual void testCall()=0;

    static MixerBase* fromDeviceInterface(DeviceInterface* interface){
      if (interface == nullptr) return nullptr;
      for (const Binding& binding : getBindings()){
        if (binding.device == interface) return binding.mixer;
      }
      return nullptr;
    }

  protected:
    static void bindDeviceInterface(DeviceInterface* interface, MixerBase* mixer){
      if (interface == nullptr || mixer == nullptr) return;

      for (Binding& binding : getBindings()){
        if (binding.device == interface){
          binding.mixer = mixer;
          return;
        }
      }

      Binding b;
      b.device = interface;
      b.mixer = mixer;
      getBindings().push_back(b);
    }

  private:
    static std::vector<Binding>& getBindings(){
      static std::vector<Binding> bindings;
      return bindings;
    }
};


template <typename NetworkBase>
class MixerDevice : public NetworkBase, public MixerBase {
public:

    // mixer definition
    MixerIdentity mixerID;
    BaseMixerDefinition* mixerDef;  // implemented by derived class

    // this classes does all the work
    Synchronizer synchronizer;

    template<typename... Args>
    MixerDevice(Args&&... args): NetworkBase(std::forward<Args>(args)...){
      bindDeviceInterface(static_cast<DeviceInterface*>(this), static_cast<MixerBase*>(this));

      mixerID.model = "undef";
      mixerID.name = "offline";
      mixerID.ip = IPAddress(0,0,0,0);

      // SyncDataSet auto register themelves to the overall synchroniser
      chNamesDS = new DataSetSynced(&addressCHname,this->interfaceName,"CH", &synchronizer); 
      busNamesDS = new DataSetSynced(&addressBUSname,this->interfaceName,"BUS", &synchronizer);
      auxNamesDS = new DataSetSynced(&addressAUXname,this->interfaceName,"AUX", &synchronizer);
      fxNamesDS = new DataSetSynced(&addressFXname,this->interfaceName, "FX", &synchronizer);

    }
    
    virtual const String& getProviderNameForData(){ return this->interfaceName;}   // we are our own provider do not forget it!


    virtual void postSync() = 0;

    virtual void monitor() override{           
      NetworkBase::monitorNetwork();
      
      /////////// GESTION DE LA PROGRESSION DE SYNCHRO
      if (this->state.label == IF_STATE_DATASYNCING) {
        // call synchronizer update to return global progress and sets all sync index        
        this->state.progress = synchronizer.update(this->destination.latency);    // wait latency between two request to avoid doub,ling them
        if (synchronizer.synced){
          // post sync action (like subscription)
          postSync();
          // all done, we go back to full connected mode          
          this->setStateLive();          
        }else{          
          DataSetSynced* dataset = synchronizer.getCurrent();   // return null if syncRequest is still locked
          if (dataset){
            dropDSnameRequest(dataset, dataset->syncStep);
            synchronizer.lockSyncRequest();
          }            
        }
      }
    }  


    void setMixer(String& name, String&model, IPAddress& ip) {
      DEBUG_INFO("%s:setMixer(%s,%s,%s)",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newID { name, model, ip };

      bool mixerChanged = !mixerID.equals(newID);
      
      if (mixerChanged){
        DEBUG_PRINT("\t%s",mixerChanged?"yes✅":"no");
        newMixer(newID);
      }           
      
      
      bool offlineTooLong = false; 
      if (this->offlineStart > 0 ){
        unsigned long  elpasedOffline = millis() - this->offlineStart;
        offlineTooLong = (elpasedOffline > 10000);
        DEBUG_PRINT("\t%s 🕒 elpasedOffline:%i",offlineTooLong?"offline too long⌛":"",elpasedOffline);        
      }

      if (offlineTooLong || mixerChanged) synchronizer.reset();

      if (!synchronizer.synced){
        setStatePendingDataSync();
      }else{
        if (this->state.label != IF_STATE_LIVE) setStateLive();
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
        app.triggerRedraw();            // tells strip to redraw themselves as labels may have change
        this->state.broadcastConfig = true;   // inform web app about the chnage of model
      }else{
        DEBUG_PRINT("\t model didn't change!");
      }
    }

    void initDatasets(){      
      DEBUG_INFO("%s:initDatasets()",this->interfaceName.c_str());
      
      // setting up the data size and flag their value as not synchronized using the defaultvalue 
      chNamesDS->init(mixerDef->channelCount);
      busNamesDS->init(mixerDef->busCount);
      auxNamesDS->init(mixerDef->auxCount);
      fxNamesDS->init(mixerDef->fxCount);
      
      // clear all counters
      synchronizer.reset();
    }   

    void newMixer(const MixerIdentity& newMixer){
      DEBUG_INFO("%s:newMixer()🎛",this->interfaceName.c_str());            
      setModel(newMixer.model.c_str());
      mixerID = newMixer;     
      this->state.broadcastConfig = true;   // inform web app about the chnage of mixer
    }

    void setStatePendingDataSync(){
      NetworkBase::setStateConnected();
      DEBUG_INFO("%s:setStatePendingDataSync()🔄",this->interfaceName.c_str());      
      this->state.label = IF_STATE_DATASYNCING;
      setMixerDetails();
      this->state.led = "green fast";      
    }

    void setMixerDetails(){
      String mixerIP_Port;

      if (mixerID.ip == IPAddress(0, 0, 0, 0)) mixerIP_Port = "virtual offline";     // if mixer IP = 0.0.0.0 it means we are connected to a virtual offline mixer (ex. mixing station)
      else mixerIP_Port = mixerID.ip.toString(),":",this->remotePort;

      this->state.details = concatStrings(mixerID.name," ",mixerID.model," ",mixerIP_Port);
    }

    void setStateLive(){
      this->setStateConnected();
      DEBUG_INFO("%s:setStateLive()🟢",this->interfaceName.c_str());
      this->state.label = IF_STATE_LIVE;      
      setMixerDetails();
    }    

    virtual void setStateDisabled() override{
      NetworkBase::setStateDisabled();
      this->state.label = "Offline";
      this->state.details =  "";    //mixerID.model;    // remove the default statement "interface is disabled"
    }

};

void Synchronizer::reset(){
  DEBUG_INFO("Synchronizer:reset()");      
  // resetting all sync counters
  totalToSync = 0;
  syncedSoFar = 0;
  synced = false;
  dsIndex = 0;  
  syncRequestLocker = false; // allow to request a dataset
  lockStartTime = 0;
   for (const auto& dataset : datasets){
    dataset->clearValues();
    totalToSync += dataset->size;
  }  
}
    

int8_t Synchronizer::update(uint16_t latency){  
  
  //should we unlock (after 3 loop)?
  if (syncRequestLocker && (millis() - lockStartTime >= latency)) {
    syncRequestLocker = false; // autorise relance d'une requête
  }

  DataSetSynced* dataset = datasets[dsIndex]; //sync in serial, next dataset  
  if (dataset->syncStep < dataset->getSize()){ // complete ?
    //DEBUG_INFO("Synchronizer::update() %i %s",dataset->syncStep, dataset->getValuePtr(dataset->syncStep));
    while (dataset->getValuePtr(dataset->syncStep)->indexOf(UNSYNC_PATTERN) == -1 && dataset->syncStep < dataset->getSize()){
      dataset->syncStep++;  // check the next item
      syncedSoFar++;
    }
  }else dsIndex++;

  // computing the overall progress
  int8_t progress = map(syncedSoFar, 0, totalToSync, 0, 100);      
  if (progress >=100){
    synced = true;
    progress = 100;
  } else synced = false;  

  return progress;      
}


#endif
