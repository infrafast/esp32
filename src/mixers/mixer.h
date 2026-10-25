/**
 * @file mixer.h
 * @brief Common mixer operations, identity and name synchronization.
 */

// without callback

#ifndef MixerInterface_h
#define MixerInterface_h

#define GROUP_INTERFACE_MIXER "Mixer"   

#include "Interfaces.h"
#include "mixers/values.h"

#define IF_STATE_LIVE               "live"
#define IF_STATE_DATASYNCING        "Getting data"

#define MIXER_DEFINITION_PATH "/"


class DataSetSynced;

/**
 * @brief Walks mixer name datasets and tracks metadata synchronization progress.
 */
class Synchronizer{
  public:

    /**
     * @brief Initialize Synchronizer and its configured member bindings.
     */
    Synchronizer(){}   

    bool synced = false;
    bool syncRequestLocker = false;
    unsigned long lockStartTime = 0;    

    uint16_t totalToSync = 0;   ///< sum of items to be synced
    uint16_t syncedSoFar = 0;
    uint8_t  dsIndex = 0;       ///< which dataset we are currently synchronizing

    /**
     * @brief Restart synchronization of every registered name dataset.
     *
     * Restore placeholder names, clear the request lock and recount items to load.
     */
    void reset();
    /**
     * @brief Advance metadata synchronization and report percentage completion.
     *
     * Only one dataset request is outstanding while locked; unlock after the reply
     * or the one-second retry timeout. Skip names whose unsynchronized marker is
     * gone, advance the dataset cursor, and combine progress across all datasets.
     * @return Progress from 0 to 100; no datasets means synchronization is complete.
     */
    int8_t update(/*uint16_t latency*/);

    /** @brief Append an entry to this collection. */
    void add(DataSetSynced* ds){
       datasets.push_back(ds);
    }

    /** @brief Pause further metadata requests until the reply or retry timeout. */
    void lockSyncRequest() {
      syncRequestLocker = true;
      lockStartTime = millis();
    }    

    /** @brief Return the dataset awaiting synchronization, unless requests are locked. */
    DataSetSynced* getCurrent(){
      if (syncRequestLocker) return nullptr;    // we are locked so we do not provide the data set
      if (dsIndex >= datasets.size()) return nullptr;
      return datasets[dsIndex];
    }

  protected:
    std::vector<DataSetSynced*> datasets;  

};


#define UNSYNC_PATTERN "~"
#define MAX_SYNC_DS_SIZE 48
/**
 * @brief Stores mixer names and exposes updates to configuration consumers.
 *
 * Stores at most 48 names. A leading ~ marks a placeholder awaiting synchronization;
 * set() replaces it with a received/default name and publishes changed labels.
 */
class DataSetSynced : public DataSet {
  public:
    String names[MAX_SYNC_DS_SIZE];

    uint16_t syncStep = 0;
    String defaultName = "";
    uint16_t size = 0;
    uint16_t maxSize = 0;

    /** @brief Initialize DataSetSynced with the supplied configuration and bindings. */
    DataSetSynced(Address* adress, String name, String defVal, uint8_t _maxSize, Synchronizer* synchronizer)
      : DataSet(adress, name, &dataRepo), defaultName(defVal), maxSize(_maxSize) {
      init(maxSize);                  // one data by default
      synchronizer->add(this);  // self registration
    }

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize() override {
      return size;
    }

    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t i) override {
      if (i < size) return &names[i];
      return &errorDatasetOOR;
    }

    /**
     * @brief Set the dataset size, capped at 48 entries, and restore placeholder names.
     */
    void init(uint16_t newSize) {
      size = (newSize > MAX_SYNC_DS_SIZE) ? MAX_SYNC_DS_SIZE : newSize;
      clearValues();
    }

    /** @brief Restore unsynchronized placeholder names and reset the dataset cursor. */
    void clearValues() {
      for (uint16_t i = 0; i < size; ++i) {
        names[i] = concatStrings(UNSYNC_PATTERN, defaultName, i + 1);
      }
      syncStep = 0;
    }

    /**
     * @brief Store a synchronized name and notify the browser only when it changes.
     *
     * Indices are zero-based internally. Empty names receive the default label;
     * outgoing address patterns use index+1 for the one-based device/UI address.
     */
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

/**
 * @brief Detected mixer name, model and network address.
 */
struct MixerIdentity {
    String name;
    String model;
    IPAddress ip;

    /** @brief Compare the mixer identity fields. */
    bool equals(const MixerIdentity& other) const {
        return name == other.name &&
               model == other.model &&
               ip == other.ip;
    }
};

/**
 * @brief Common mixer capabilities and supported element counts.
 */
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
#define ADDRESS_BUS_NAME "/busname/%d"
#define ADDRESS_FX_NAME "/fxname/%d"
#define ADDRESS_AUX_NAME "/auxname/%d"
#define ADDRESS_MAIN_NAME "/mainname/%d"
#define ADDRESS_DCA_NAME "/dcaname/%d"

/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressCHname(ADDRESS_CH_NAME);
/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressBUSname(ADDRESS_BUS_NAME);
/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressFXname(ADDRESS_FX_NAME);
/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressAUXname(ADDRESS_AUX_NAME);
/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressMAINname(ADDRESS_MAIN_NAME);
/** @brief Describe the mixer-name dropdown address used by strips and datasets. */
inline DropDownAddress addressDCAname(ADDRESS_DCA_NAME);


/**
 * @brief strip action / event
 */
enum{
  STRIP_REFRESH_VISIBLE
};


/**
 * @brief Transport-independent mixer operations and shared values used by strips.
 */
class MixerBase {
public:
    /**
     * @brief Association between a device interface and its mixer contract.
     */
    struct Binding {
      DeviceInterface* device = nullptr;
      MixerBase* mixer = nullptr;
    };

    /**
     * @brief values shared between strips and mixers
     */
    TwoIndexValueList allInput2MixbusValues;
    OneIndexValueList allBusMixValues;
    OneIndexValueList allChMainValues;  

    /**
     * @brief properties that are synced
     */
    DataSetSynced* inputNamesDS = nullptr;
    DataSetSynced* mixbusNamesDS = nullptr;
    DataSetSynced* auxNamesDS = nullptr;
    DataSetSynced* fxNamesDS = nullptr;
    DataSetSynced* mainNamesDS = nullptr;
    DataSetSynced* dcaNamesDS = nullptr;

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual ~MixerBase() {}

    /**
     * @brief event system to decouple mixer implementation from strip management
     */
    JsonDocument payload;                     ///< data from the mixer

    /**
     * @brief define if the mixer sends an echo after it receives a command, by default, it is the case
     */
    bool echoing = true;

    /// pure virtual utilities ////
    virtual bool restoreMixerProfile() = 0;
    /** @brief Load mixer capabilities from the saved model definition. */
    virtual bool deserializeMixerDefinitionFromFile(const char* filename) = 0;    

    /** @brief Request one name using a zero-based synchronized-dataset index.
     *
     * / pure virtual mixer methods called by the strips ////
     * GETTERS
     */
    virtual void requestDSname(DataSetSynced*, uint8_t) = 0;          // used by datasync
    
    /**
     * @brief the following two one is the exception
     */
    void requestCHname(uint8_t chIndex){
      requestDSname(inputNamesDS, chIndex-1);  // only -1 for requestDSname which is generic and expect index starting at 0
    }

    /** @brief Request the name of a one-based bus id. */
    void requestBUSname(uint8_t busIndex){
      requestDSname(mixbusNamesDS, busIndex-1);  // only -1 for requestDSname which is generic and expect index starting at 0
    };


    /** @brief Request the mute state of an input send to a bus. */
    virtual void requestCHBUSmute(uint8_t chIndex, uint8_t busIndex) = 0;
    /** @brief Request the mute state of a bus. */
    virtual void requestBUSmute(uint8_t busIndex) = 0;
    /** @brief Request the level of an input send to a bus. */
    virtual void requestCHBUSlevel(uint8_t chIndex, uint8_t busIndex) = 0;
    // SETTERS
    /** @brief Set the level of an input send to a bus using the backend value scale. */
    virtual void setCHBUSlevel(int chIndex, int busIndex, float level) = 0;
    /** @brief Set the mute state of an input send to a bus. */
    virtual void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, int mute) = 0;

    ///////////////////////////////////////////////////////////////

    /** @brief Find the common mixer API bound to an interface, or nullptr if absent. */
    static MixerBase* fromDeviceInterface(DeviceInterface* interface){
      if (interface == nullptr) return nullptr;
      for (const Binding& binding : getBindings()){
        if (binding.device == interface) return binding.mixer;
      }
      return nullptr;
    }

  protected:
    /** @brief Register the association between the transport interface and mixer API. */
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
    /** @brief Return the shared registry linking device interfaces to mixer contracts. */
    static std::vector<Binding>& getBindings(){
      static std::vector<Binding> bindings;
      return bindings;
    }
};


/**
 * @brief Combines the mixer contract with a concrete network transport.
 *
 * @tparam NetworkBase Transport implementing the NetworkDevice contract.
 * Combines backend-independent mixer operations with OSC or WebSocket transport.
 * Name datasets are synchronized before the backend enters its live state.
 */
template <typename NetworkBase>
class MixerDevice : public NetworkBase, public MixerBase {
public:

    // mixer definition
    MixerIdentity mixerID;
    BaseMixerDefinition* mixerDef;  ///< implemented by derived class

    /**
     * @brief this classes does all the work
     */
    Synchronizer synchronizer;

    WebApplicationService* webAppService;

    // used to handle path templates and values
    #define PATH_MAX_SIZE 64
    char pathBuffer[PATH_MAX_SIZE];        
    unsigned long lastVisibleStripRefreshAt = 0;
      
    
    /** @brief Initialize MixerDevice with the supplied configuration and bindings. */
    template<typename... Args>
    MixerDevice(Args&&... args)      
        : NetworkBase(std::forward<Args>(args)...){
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
    
    /** @brief Return the provider key used to resolve configuration choices. */
    virtual const String& getProviderNameForData(){ return this->interfaceName;}   // we are our own provider do not forget it!

    ///////////////////////////////////////////////////// the setters always echo the command so other strips update themseleves using events
    /*virtual void setCHBUSlevel(int chIndex, int busIndex, float level) = 0;
    virtual void _setCHBUSmute(uint8_t chIndex, uint8_t busIndex, int mute) = 0;*/
    /////////////////////////////////////////////////////////////////////////////////////
   
    /** @brief Start backend-specific mixer identification and setup. */
    virtual void beginMixer() = 0;   // 

    //
    /** @brief Start this interface using its current configuration. */
    bool begin() override {
      // NOTE:
      // In this template hierarchy, NetworkBase can be an abstract transport
      // class (e.g. NetworkSocket) that does not implement begin().
      // Calling NetworkBase::begin() with qualified lookup forces that symbol
      // and can trigger link errors.
      //
      // Network bootstrap is already driven by NetworkDevice::_begin(), which
      // dispatches to the most-derived begin() implementation.
      DEBUG_INFO("%s::_begin() => self registering to webAppService event",this->deviceInterface->interfaceName.c_str());      
            
      beginMixer();  // call the mixer specific begin method to setup model and dataset pattern, we do it here in the generic begin method because it is the one called by the NetworkDevice::_begin() and this way we are sure it is called at the right moment in the connection process (for example if we do it in the constructor it may be too early and if we do it in onEvent when receiving EVENT_WEBAPP_STARTED it may be too late if the web app is already started by then)
      return true;
    }

    /*void setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level){
      _setCHBUSlevel(chIndex, busIndex, level);       
      fireChBusMixLevel(chIndex, busIndex, level);      // echo command to strips so they can update themselves, we have to do it here because the level is needed to update the strip but it is not returned by the mixer as it is set (we could of course ask the mixer to return the new level after setting it but it would generate a lot of network traffic and we already have the right level at hand so let's just use it)
    };

    void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, bool mute){
      _setCHBUSmute(chIndex, busIndex, mute);
      fireChBusMute(chIndex, busIndex, mute);
    };

    ////////////////////////////////// EVENT TRANSMISSION TO STRIPS SO THEY CAN UPDATE THEMSELVES ///////////////////
    / *void fireChBusMixLevel(int ch, int bus, float level) {
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
    }*/

    ////////////////////////////////////////////////////////////////////////////
    //////////////////////////// incoming data handlers ////////////////////////
    // called by the mixers when data arrive and generically modify the values //
    ////////////////////////////////////////////////////////////////////////////
    /** @brief Apply a received input-to-bus level to the shared value for that key pair. */
    void handleChBusMixLevel(int ch, int bus, float level) {
      TwoIndexValue* chBus = allInput2MixbusValues.get(ch,bus);       // retrieve the value based on the ch+bus key tupple
      if (chBus != nullptr) chBus->value = level;
      DEBUG_INFO("%s->handleChBusMixLevel(%i,%i,%.2f)",this->interfaceName.c_str(),ch,bus,level);      
    }       

    /** @brief Apply a received channel-to-bus mute value to shared state. */
    void handleChBusMute(int ch, int bus, int on){
      TwoIndexValue* chBus = allInput2MixbusValues.get(ch,bus);
      if (chBus != nullptr) chBus->toggle = on;
      DEBUG_INFO("%s->handleChBusMute(%i,%i,%i)",this->interfaceName.c_str(),ch,bus,on);      
    }

    /** @brief Apply a received bus mute value to shared state. */
    void  handleBusMute(int bus, int on){
      OneIndexValue* chBus = allBusMixValues.get(bus);
      if (chBus != nullptr) chBus->toggle = on;
      DEBUG_INFO("%s->handleBusMute(%i,%i)",this->interfaceName.c_str(),bus,on);      
    }

    /** @brief Apply a received channel-to-main mute value to shared state. */
    void  handleChMainMute(int ch, int on){
      OneIndexValue* chMain = allChMainValues.get(ch);
      if (chMain != nullptr) chMain->toggle = on;
      DEBUG_INFO("%s->handleMainMute(%i,%i)",this->interfaceName.c_str(),ch,on);           
    }


    //////////// helpers to build command path from id on template /////////////
    //construit la chaîne en remplaçant les placeholders %d et %s
    /** @brief Format a protocol path containing one numeric index. */
    void buildPath1Int(char* destination, size_t bufferSize, int index, const char* pattern) {
        snprintf(destination, bufferSize, pattern, index);
    }   
    
    /** @brief Format a protocol path containing two numeric indices. */
    void buildPath2Int(char* destination, size_t bufferSize, int index1, int index2, const char* pattern) {
        snprintf(destination, bufferSize, pattern, index1, index2);
    }      


    /////////////////////////////////
    /** @brief Ask visible strips to refresh values through the common mixer event. */
    void fireVisibleStripRefreshEvent() {
      payload[0] = STRIP_REFRESH_VISIBLE;
      this->fireEvent(this->message);
    }

      
    ////////////////////////////////

    /** @brief Perform backend subscriptions/setup after name synchronization completes. */
    virtual void postSync() = 0;

    /**
     * @brief Coordinate transport polling, mixer-name synchronization and strip refresh.
     *
     * When synchronization is active, request the current missing name and lock
     * further requests until a response or timeout. Once complete, enter the live
     * state and run backend postSync(). Periodic events ask visible strips for values.
     */
    virtual bool monitor() override{                 
      
      ////////// GESTION DE LA PROGRESSION DE SYNCHRO
      if (this->state.label == IF_STATE_DATASYNCING) {
        // call synchronizer update to return global progress and sets all sync index        
        this->state.progress = synchronizer.update(/*this->destination.latency*/);    // wait latency between two request to avoid doubling them
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

      if (this->state.code == IF_READY) {  // we have a bad QoS, so we force a data sync at VISIBLE_STRIP_REFRESH_RATE to catch up possible data loss and stay sync
        unsigned long now = millis();
        uint16_t visibleStripRefreshRate = 60000; // by default we refresh every minute

        if (this->state.QoSlevel < QoS::GOOD) visibleStripRefreshRate = 10000; // if QoS is bad, we reduce to every 10s, this value should always be above the ping delay

        if ((now - lastVisibleStripRefreshAt) >= visibleStripRefreshRate) {
          lastVisibleStripRefreshAt = now;
          fireVisibleStripRefreshEvent();
        }       
      }

      return NetworkBase::monitorNetwork();

    }  


    /** @brief Apply detected mixer identity and refresh model-dependent information. */
    void setMixer(String& name, String&model, IPAddress& ip) {
      //DEBUG_INFO("%s:setMixer(%s,%s,%s)",this->interfaceName.c_str(),name.c_str(),model.c_str(),ip.toString().c_str());

      MixerIdentity newID { name, model, ip };

      this->determineQoS();       // mixer replied so we set the QoS based on latency and jitter 

      bool mixerChanged = !mixerID.equals(newID);
      
      if (mixerChanged){
        //DEBUG_PRINT("\t%s",mixerChanged?"yes✅":"no");
        newMixer(newID);
      }           
      
      
      bool offlineTooLong = false; 
      if (this->offlineStart > 0 ){
        unsigned long  elpasedOffline = millis() - this->offlineStart;
        offlineTooLong = (elpasedOffline > 10000);
        //DEBUG_PRINT("\t%s 🕒 elpasedOffline:%i",offlineTooLong?"offline too long⌛":"",elpasedOffline);        
      }

      if (offlineTooLong || mixerChanged) synchronizer.reset();

      if (!synchronizer.synced && app.webAppConnected){
        setStatePendingDataSync();
      }else
        if (this->state.label != IF_STATE_LIVE) setStateLive();
    

    }

    /** @brief Select model capabilities for the detected mixer. */
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

    /** @brief Size and initialize name datasets from the active mixer definition. */
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

    /** @brief Apply a newly detected mixer identity. */
    void newMixer(const MixerIdentity& newMixer){
      DEBUG_INFO("%s:newMixer()🎛",this->interfaceName.c_str());            
      setModel(newMixer.model.c_str());
      mixerID = newMixer;     
      this->state.broadcastConfig = true;   // inform web app about the chnage of mixer
    }

    /** @brief Set the interface status to waiting for mixer metadata. */
    void setStatePendingDataSync(){
      NetworkBase::setStateConnected();
      DEBUG_INFO("%s:setStatePendingDataSync()🔄",this->interfaceName.c_str());      
      this->state.label = IF_STATE_DATASYNCING;
      setMixerDetails();
      this->state.led = "green fast";      
    }

    /** @brief Build the user-visible mixer description. */
    void setMixerDetails(){
      String mixerIP_Port;

      if (mixerID.ip == IPAddress(0, 0, 0, 0)) mixerIP_Port = "virtual offline";     // if mixer IP = 0.0.0.0 it means we are connected to a virtual offline mixer (ex. mixing station)
      else mixerIP_Port = concatStrings(mixerID.ip.toString(),":",this->remotePort);

      this->state.details = concatStrings(mixerID.name," ",mixerID.model," ",mixerIP_Port);
    }

    /** @brief Set the interface status to live. */
    void setStateLive(){
      this->setStateConnected();
      DEBUG_INFO("%s:setStateLive()🟢",this->interfaceName.c_str());
      this->state.label = IF_STATE_LIVE;      
      setMixerDetails();      
    }    

    /** @brief Set the interface status to disabled. */
    virtual void setStateDisabled() override{
      NetworkBase::setStateDisabled();
      this->state.label = "Offline";
      this->state.details =  "";    //mixerID.model;    // remove the default statement "interface is disabled"
    }



};

/**
 * @brief Restart synchronization of every registered name dataset.
 *
 * Restore placeholder names, clear the request lock and recount items to load.
 */
inline void Synchronizer::reset(){
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
    

/**
 * @brief Advance metadata synchronization and report percentage completion.
 *
 * Only one dataset request is outstanding while locked; unlock after the reply
 * or the one-second retry timeout. Skip names whose unsynchronized marker is
 * gone, advance the dataset cursor, and combine progress across all datasets.
 * @return Progress from 0 to 100; no datasets means synchronization is complete.
 */
inline int8_t Synchronizer::update(/*uint16_t latency*/){  
  #define SYNC_LOCK_TIMEOUT 1000      // une seconde laisse le temps au data de se flusher et se resynchro
  //if (latency<20) latency = 20;   // minimum latency of 20ms to avoid too much request / flood
  if (datasets.empty()) {
    synced = true;
    return 100;
  }

  if (syncRequestLocker && (millis() - lockStartTime >= /*latency*/SYNC_LOCK_TIMEOUT)) {
    syncRequestLocker = false; // autorise relance d'une requête
  }

  if (dsIndex >= datasets.size()) {
    synced = true;
    return 100;
  }

  DataSetSynced* dataset = datasets[dsIndex]; //sync in serial, next dataset  
  if (dataset->syncStep < dataset->getSize()){ // complete ?
    //DEBUG_INFO("Synchronizer::update() %i %s",dataset->syncStep, dataset->getValuePtr(dataset->syncStep));
    while (dataset->syncStep < dataset->getSize() // on  a pas atteind la fin du dataset 
      && dataset->getValuePtr(dataset->syncStep)->indexOf(UNSYNC_PATTERN) == -1){ // et le data courant a été synchronisé car on n'y trouve plus le tag de snychro dans sa valeur
        dataset->syncStep++;  // donc on check the next item
        syncRequestLocker = false; // et autorise relance d'une requête
        syncedSoFar++; // et on incremente le progres 
    }
  }else dsIndex++;

  // computing the overall progress
  int8_t progress = (totalToSync == 0) ? 100 : (syncedSoFar * 100) / totalToSync;      
  if (progress >=100){
    synced = true;
    progress = 100;
  } else synced = false;  

  return progress;      
}


#endif
