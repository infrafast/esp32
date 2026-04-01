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
    String names[MAX_SYNC_DS_SIZE];

    uint16_t syncStep = 0;
    String defaultName = "";
    uint16_t size = 0;
    uint16_t maxSize = 0;

    DataSetSynced(Address* adress, String name, String defVal, uint8_t _maxSize, Synchronizer* synchronizer)
      : DataSet(adress, name, &dataRepo), defaultName(defVal), maxSize(_maxSize) {
      init(maxSize);                  // one data by default
      synchronizer->add(this);  // self registration
    }

    virtual uint16_t getSize() override {
      return size;
    }

    virtual const String* getValuePtr(uint16_t i) override {
      if (i < size) return &names[i];
      return &errorDatasetOOR;
    }

    void init(uint16_t newSize) {
      size = (newSize > MAX_SYNC_DS_SIZE) ? MAX_SYNC_DS_SIZE : newSize;
      clearValues();
    }

    void clearValues() {
      for (uint16_t i = 0; i < size; ++i) {
        names[i] = concatStrings(UNSYNC_PATTERN, defaultName, i + 1);
      }
      syncStep = 0;
    }

    void set(uint16_t index, const String& value) {
      if (index >= size) return;

      String newValue = (value == "") ? concatStrings(defaultName, index + 1) : value;

      if (names[index] == newValue) return;

      names[index] = newValue;

      char addrBuf[64];
      snprintf(addrBuf, sizeof(addrBuf), address->pattern.c_str(), index + 1); // addresses start at 1
      
      //{"dataSet":{"updateDataSets":[{"XMSeries":{"/ch/name/3":"toto"}}]}}
      String frame = concatStrings(
        "{\"updateDataSets\":[{\"",
        name,
        "\":{\"",
        String(addrBuf),
        "\":\"",
        names[index],
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

#define ADDRESS_MAIN_NAME "/mainname/%d"
DropDownAddress addressMAINname = DropDownAddress(ADDRESS_MAIN_NAME);

#define ADDRESS_DCA_NAME "/dcaname/%d"
DropDownAddress addressDCAname = DropDownAddress(ADDRESS_DCA_NAME);  

enum{
  STRIP_CH_BUS_MUTE,
  STRIP_CH_MAIN_MUTE,
  STRIP_CH_BUS_MIX_LEVEL
};



class MixerBase {
public:
    struct Binding {
      DeviceInterface* device = nullptr;
      MixerBase* mixer = nullptr;
    };

    // properties that are synced
    DataSetSynced* inputNamesDS;
    DataSetSynced* mixbusNamesDS;  
    DataSetSynced* auxNamesDS;  
    DataSetSynced* fxNamesDS;
    DataSetSynced* mainNamesDS;
    DataSetSynced* dcaNamesDS;

    virtual ~MixerBase() {}

    // event system to decouple mixer implementation from strip management
    JsonDocument payload;                     // data from the mixer

    //// pure virtual utilities ////
    //virtual void startMixerDataSync() = 0;
    virtual bool restoreMixerProfile() = 0;
    virtual bool deserializeMixerDefinitionFromFile(const char* filename) = 0;    

    //// pure virtual mixer methods called by the strips ////
    // GETTERS
    virtual void requestDSname(DataSetSynced*, uint8_t) = 0;    
    virtual void requestCHBUSmute(uint8_t chIndex, uint8_t busIndex) = 0;
    virtual void requestBUSmute(uint8_t busIndex) = 0;
    virtual void requestCHBUSlevel(uint8_t chIndex, uint8_t busIndex) = 0;
    // SETTERS
    virtual void setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level) = 0;
    virtual void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, bool mute) = 0;

    ////////////////////////////////////////////////////////////////

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

    WebApplicationService* webAppService;

    template<typename... Args>
    MixerDevice(WebApplicationService* webApp, Args&&... args)      // a MixerDevice need a webAppService to listen to event EVENT_WEBAPP_STARTED to start the synchronization process, so we inject it in the constructor and store a pointer to it
        : NetworkBase(std::forward<Args>(args)...),
          webAppService(webApp) {
        bindDeviceInterface(static_cast<DeviceInterface*>(this),
                            static_cast<MixerBase*>(this));

      mixerID.model = "not synced";
      mixerID.name = "offline";
      mixerID.ip = IPAddress(0,0,0,0);

      // SyncDataSet auto register themelves to the overall synchroniser
      #define MAX_INPUT 48
      #define MAX_BUSES 32
      #define MAX_AUX 16
      #define MAX_FX 16      
      #define MAX_MAIN 4
      #define MAX_DCA 8
      inputNamesDS = new DataSetSynced(&addressCHname,this->interfaceName,"CH", MAX_INPUT, &synchronizer); 
      mixbusNamesDS = new DataSetSynced(&addressBUSname,this->interfaceName,"BUS", MAX_BUSES, &synchronizer);
      auxNamesDS = new DataSetSynced(&addressAUXname,this->interfaceName,"AUX", MAX_AUX, &synchronizer);
      fxNamesDS = new DataSetSynced(&addressFXname,this->interfaceName, "FX", MAX_FX, &synchronizer);
      mainNamesDS = new DataSetSynced(&addressMAINname,this->interfaceName, "MAIN", MAX_MAIN, &synchronizer);
      dcaNamesDS = new DataSetSynced(&addressDCAname,this->interfaceName, "DCA", MAX_DCA, &synchronizer);

      this->message.attachData(&payload); // this interface is sending json payload directly to the strip so we attach the payload to the message event, this way we can reuse the same event for all kind of messages and just check the content of the payload to know what to do with it in the strip (for example if it is a mix level update or a main mute update)

    }
    
    virtual const String& getProviderNameForData(){ return this->interfaceName;}   // we are our own provider do not forget it!

    ////////////////////////////////////////////////////// the setters always echo the command so other strips update themseleves
    virtual void _setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level) = 0;
    virtual void _setCHBUSmute(uint8_t chIndex, uint8_t busIndex, bool mute) = 0;
    //////////////////////////////////////////////////////////////////////////////////////
   
    //
    bool begin() override {
      bool networkOk = NetworkBase::begin();
      DEBUG_INFO("%s::_begin() => self registering to webAppService event",this->deviceInterface->interfaceName.c_str());      
      webAppService->registerEventListener(this);   // we listen to web app event to start the synchronization when it is ready
      return networkOk;
    }

    void setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level){
      _setCHBUSlevel(chIndex, busIndex, level);       
      fireChBusMixLevel(chIndex, busIndex, level);      // echo command to strips so they can update themselves, we have to do it here because the level is needed to update the strip but it is not returned by the mixer as it is set (we could of course ask the mixer to return the new level after setting it but it would generate a lot of network traffic and we already have the right level at hand so let's just use it)
    };

    void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, bool mute){
      _setCHBUSmute(chIndex, busIndex, mute);
      fireChBusMute(chIndex, busIndex, mute);
    };

    ////////////////////////////////// EVENT TRANSMISSION TO STRIPS SO THEY CAN UPDATE THEMSELVES ///////////////////
    void fireChBusMixLevel(int ch, int bus, float level) {
      payload[0] = STRIP_CH_BUS_MIX_LEVEL;
      payload[1] = ch;
      payload[2] = bus;
      payload[3] = level;
      this->fireEvent(this->message);
    }  

    void fireChMainMute(int ch, int muteState) {
      payload[0] = STRIP_CH_MAIN_MUTE;
      payload[1] = ch;
      payload[2] = muteState;
      this->fireEvent(this->message);
    }

    void fireChBusMute(int ch, int bus, int muteState) {
      payload[0] = STRIP_CH_BUS_MUTE;
      payload[1] = ch;
      payload[2] = bus;
      payload[3] = muteState;
      this->fireEvent(this->message);
    }

    //////////////////////////////////

    void onEvent(const Event& event) override {
      NetworkBase::onEvent(event);   // call parent to handle network events

      if (event.type == EVENT_WEBAPP_STARTED) {
        // when web app has started we launch the data synchronization
        if (!synchronizer.synced) {
          setStatePendingDataSync();
        }
      }
    }
      
    /////////////////////////////////

    virtual void postSync() = 0;

    virtual bool monitor() override{                 
      
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
            requestDSname(dataset, dataset->syncStep);
            synchronizer.lockSyncRequest();
          }            
        }
      }

      return NetworkBase::monitorNetwork();

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

      /*if (!synchronizer.synced){
        setStatePendingDataSync();
      }else*/
        if (this->state.label != IF_STATE_LIVE) setStateLive();
    

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
      inputNamesDS->init(mixerDef->channelCount);
      mixbusNamesDS->init(mixerDef->busCount);
      auxNamesDS->init(mixerDef->auxCount);
      fxNamesDS->init(mixerDef->fxCount);
      mainNamesDS->init(mixerDef->mainCount);
      dcaNamesDS->init(mixerDef->DCAcount); 
      
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
  
  if (latency<20) latency = 20;   // minimum latency of 20ms to avoid too much request / flood
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
