/**
 * @file Interfaces.h
 * @brief Device lifecycle, configuration and network-service contracts.
 */

#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

#include "interfaces/i_states.h"


#define ADDRESS_CHANGETOPARAMETERNAME "/config/%s"


/**
 * @brief Common contract for device configuration, events and lifecycle.
 *
 * Configuration and runtime status are separate JSON views. init() loads settings
 * before startup; update() detects state changes and polls enabled interfaces.
 * Instances register by name so saved strips can resolve their interface binding.
 *
 * State-transition rule:
 * - workflow methods (enable/disable, connect/disconnect, discovery, and event
 *   handlers) decide when a business transition occurs;
 * - setStateXXX() methods only project the already-decided result into the
 *   framework state consumed by the GUI, serializers, QoS logic, and message
 *   broadcasts.
 *
 * A setStateXXX() method must not start or stop a service, reserve capacity,
 * change network ownership, or otherwise create a business transition. Keep
 * those side effects in the owning workflow method, then call the appropriate
 * setStateXXX() method to describe the resulting state. This separation keeps
 * state reporting deterministic and prevents GUI updates from triggering
 * hidden lifecycle changes.
 */
class DeviceInterface  : public SerializableParameters, public EventDispatcher, public EventListener{
  #define INTERFACE_STATE       "state"
  #define INTERFACE_PROGRESS    "progress"
  #define INTERFACE_EXPOSED     "exposed"
  #define INTERFACE_ENABLED     "enabled"
  #define INTERFACE_ACTIVABLE   "activable"
  #define INTERFACE_GROUP       "group"
  #define INTERFACE_USENETWORK  "network"  
  #define INTERFACE_PROVIDER    "provider"
  #define INTERFACE_ICON        "icon"
  #define INTERFACE_DETAILS     "details"
  #define INTERFACE_HINT        "hint"
  #define INTERFACE_LED         "led"
  #define INTERFACE_CONFIG      "deviceConfig"
  #define INTERFACE_USEDBYSTRIP "hasStrip"          // linked to the property hasStrip and indicate if it should show up in the menu to create a strip for this interface
  
  #define DEVICE_ON true
  #define DEVICE_OFF false

  public:

    State state;

    //int8_t stripCount = 0;      // indicate if an interface is not refered by any strip
    unsigned long offlineStart = 0;

    String interfaceName; 
  
    //void* data;                 // will be replaced by EVENT QUEUE : MUST BE AFFECTED IN CONSTRUCTOR OF DERIVED CLASS AND THEN CASTED IN THE UPDATE() MEDTHOD OF STRIP (WHICH IS CALLED WHEN AVAILABLE RETURN > 0 in STRIP PROCESS())    

    bool exposed = true;          // 
    bool hasStrip = true;      ///< if the interface is used by a strip (ex. Midi) or not (ex. Directory Service), by default yes, but some interface are more technical and not relevant for the user to create a strip with them, so we can hide them in the strip creation menu by setting this parameter to false

    String group = "undef"; ///< used by the strip to define which deviceinterface they can connect

    Event message;

    bool dataProcessed;         ///< generic flag used to tells if we had some data in or out of the interface

    /** @brief Initialize DeviceInterface with the supplied configuration and bindings. */
    DeviceInterface(const String&, const String&, bool);
    /** @brief Release resources held by this interface. */
    ~DeviceInterface();

    /** @brief Return the name used to identify this event source in logs. */
    virtual const char* getDispatcherName() const override{
      return interfaceName.c_str();
    }

    /** @brief Return the name used to identify this listener in logs. */
    virtual const char* getListenerName()  override{
      return interfaceName.c_str();
    }


    /** @brief Placeholder hook; no active processing in this implementation. */
    void onEvent(const Event& evt) override {

    }

    /** @brief Run the initialization-specific interface startup path. */
    virtual bool _begin(){     // called by init()
      return begin();
      //DEBUG_INFO("%s::_begin() void",interfaceName.c_str());
      return true;
    };        

    /** @brief Start this interface using its current configuration. */
    virtual bool begin() = 0;       // called to start the interface, require a end() to stop it
    /** @brief End the interface session and release its active connection. */
    virtual bool end() = 0;       // called to stop the interface, require a begin() to start again

    /** @brief Stop the active operation or connection. */
    virtual bool stop(){      // called to stop (ex. disconnect the network for network interface) require a begin() to start again
      DEBUG_INFO("%s::stop() void",interfaceName.c_str());
      return true;
    };           

    /** @brief Return the provider key used to resolve configuration choices. */
    virtual const String& getProviderNameForData(){ return group;}

    /** @brief Return the web-catalogue key for this interface tooltip. */
    virtual String getDefaultHint() const {
      return "hint.interface";
    }

    /** @brief Export current status, progress and user-visible state information as JSON. */
    String serializeState(){
      JsonDocument doc;
      
      doc[interfaceName][INTERFACE_STATE] = state.label;
      doc[interfaceName][INTERFACE_PROGRESS] = state.progress;
      doc[interfaceName][INTERFACE_DETAILS] = state.details;
      doc[interfaceName][INTERFACE_HINT] = state.hint;
      doc[interfaceName][INTERFACE_ENABLED] = state.enabled;
      doc[interfaceName][INTERFACE_ACTIVABLE] = state.switchable;
      doc[interfaceName][INTERFACE_LED] = state.led;          
     
      doc.shrinkToFit();
      String output;
      serializeJson(doc, output);    
      //{"MixerOffline":{"state":"disconnected","progress":0}}
      //DEBUG_PRINT("\n<%s> serializeState: %s",interfaceName.c_str(),output.c_str());          
      return output;
    }  

    /** @brief Export enabled state and interface configuration properties as JSON. */
    String serializeConfig(bool mainNode = true){
      // by default we serialize the full content (main node)
      // but if an interface uses another one, it will only partially serialize 
      // means we don't include somes attributes
      JsonDocument doc;
      //JsonObject config = doc[interfaceName].createNestedObject(/*INTERFACE_CONFIG*/);
      JsonObject config;

      if (exposed) 
        config = doc.createNestedObject(interfaceName);
      else config = doc.to<JsonObject>();

      config[INTERFACE_ENABLED] = state.enabled;      // common attribute

      if (mainNode == true){
        // that's the attribute we do not serialize in case of partial serialization
        config[INTERFACE_EXPOSED] = exposed;
        config[INTERFACE_USEDBYSTRIP] = hasStrip;
        config[INTERFACE_GROUP] = group;
        config[INTERFACE_PROVIDER] = getProviderNameForData();      
        config[INTERFACE_ICON] = icon;
      }

      JsonArray propertiesDoc = config[DATASET_PROPERTIES].to<JsonArray>();
      serializeProperties(propertiesDoc);

      doc.shrinkToFit();
      String output;
      serializeJson(doc, output);    
      //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
      DEBUG_INFO("%s->serializeConfig(%d): %s",interfaceName.c_str(),mainNode,output.c_str());          
      return output;
    }  

    /** @brief Prepare this object for use with its current configuration. */
    bool init(/*EventListener* listener*/) {
      determineQoS();
      DEBUG_INFO("----------------------------------  %s->init(...)  ---------------------------------- ", interfaceName.c_str());
      //registerEventListener(listener);
      load();
      // A derived interface may keep a runtime hint from load() (Directory
      // does this). Otherwise initialize the localized default once here.
      if (state.hint.length() == 0) state.hint = getDefaultHint();
      bool success = _begin();
      DEBUG_INFO("-----------------------------------------------------------------------------------------------------\n");
      return success;
    }

    /** @brief Refresh the configuration choices or status exposed to consumers. */
    virtual void handleRequestRefreshState(){
      DEBUG_INFO("%s->handleRequestRefreshState() : void",interfaceName.c_str());           
    }

    /**
     * @brief Restore enabled state and delegate property strings to the interface.
     *
     * The JSON-text overload loads startup settings without reconnecting. The object
     * overload can call handlePropertyChange() after actual property changes.
     * @return Whether tracked properties changed; false can also mean no change.
     */
    bool restore(const char* config){    
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, config);
      if (error) {
        DEBUG_INFO("%s restore %s",interfaceName.c_str(),error.c_str());
        return false;
      }
      JsonObject configJson = doc.as<JsonObject>();
      return restore(configJson, false);       // true means => trigger an interface property change check that may lead to disconnecting/reconnecting , which is not the case when we start the interface
    }

    /**
     * @brief Restore enabled state and delegate property strings to the interface.
     *
     * The JSON-text overload loads startup settings without reconnecting. The object
     * overload can call handlePropertyChange() after actual property changes.
     * @return Whether tracked properties changed; false can also mean no change.
     */
    bool restore(const JsonObject& config, bool triggerPropertyChange = true){   // == trigger or not the handlePropertyChange at the end of the restore
      String cfgString;
      serializeJson(config, cfgString);     
      DEBUG_INFO("%s->restore(%s,%s)",interfaceName.c_str(),cfgString.c_str(),triggerPropertyChange?"triggerPropertyChange yes":"triggerPropertyChange no");           
      
      bool changed = false;
      //DEBUG_PRINT("\n%s restore(JsonObject<...>)",interfaceName.c_str());
      // this restore the base parameters common to all base class interface
      if (config.containsKey(INTERFACE_ENABLED)){
        bool enabled = config[INTERFACE_ENABLED];
        state.enabled = enabled;  //switchDevice(enabled); // BE CAREFULL!
        DEBUG_INFO("%s : %s",interfaceName.c_str(),enabled?"✅ENABLED":"🚫DISABLED");
      }
      
      // and then we call the restoreProperty of the interface which will check if the properties is for her
      JsonArray properties = config[KEY_PROPERTIES];
      for (uint8_t i = 0; i< properties.size(); i++) changed |= restoreProperty(properties[i]);
     
      // properties triggering a callback
      // like interface who would need to reconnect because the IP address has changed 
      if (changed && triggerPropertyChange){
        handlePropertyChange();
      }
      return changed;
    }      


    /** @brief Export this object using its JSON representation. */
    virtual void serialize(JsonDocument& interfaceDoc){
      //start with the config
      deserializeJson(interfaceDoc, serializeConfig());      
      JsonDocument interfaceState;
      // use another document to grab the state
      deserializeJson(interfaceState, serializeState());
      // fuzion the two, we have to do this on object as otherwise, key content would be overwritteb
      // on utilise le nom de l'interface (qui est la clef commune aux deux documents)
      //const char* interfaceName = interfaceDoc.as<JsonObject>().begin()->key().c_str();
      JsonObject state = interfaceState[interfaceName];
      JsonObject fullDoc = interfaceDoc[interfaceName];
      for (JsonPair kv : state) {
        fullDoc[kv.key()] = kv.value();  // Ajoute ou écrase les clés dans name1
      }

      // in case the "top level" interface is not exposed we have to put back the interface name as the key
      // as it was filtered in the serializeConfig
      if (!exposed){

        // Sauvegarder l'ancien contenu dans un objet temporaire
        JsonObject oldContent = interfaceDoc.as<JsonObject>();
        JsonDocument temp;
        temp.set(oldContent);

        // Vider le document original
        interfaceDoc.clear();

        // Créer une nouvelle clé contenant l'ancien contenu
        interfaceDoc[interfaceName].set(temp);

      }

    }

    /**
     * @brief Detect meaningful state changes, then run interface monitoring.
     *
     * Compare labels, state codes, enabled state, LEDs and switchability with their
     * previous values. Changes set state.changed and request browser notification.
     * Progress-only changes request notification without marking a layout change,
     * avoiding a screen rebuild for every progress increment.
     */
    virtual bool update(){      // return a boolean indicatin if the interface has processed some data
      state.changed = false;

      // report if state has changed
      if (state.label != state.previousStateLabel){
        state.changed = true;
        state.previousStateLabel = state.label;
      }
      if (state.code != state.previousStateCode){
        state.changed  = true;
        state.previousStateCode = state.code;
      }    
      if (state.hint != state.previousHint){
        state.changed = true;
        state.previousHint = state.hint;
      }
      if (state.enabled != state.previousEnabled){
        state.changed   = true;
        state.previousEnabled = state.enabled;
      }    
      if (state.progress != state.previousProgress){
        // a progress change is captured so applicatoin manager can broadcast it to the webclient but is not a state change
        // indeed, a state change would trigger redraw which we do not want
        state.previousProgress = state.progress;   
        state.broadcastState = true;     
      }
      if (state.led != state.previousLed){
        state.changed  = true;
        state.previousLed = state.led;
      }
      if (state.switchable != state.previousSwitchable){
        state.changed  = true;
        state.previousSwitchable = state.switchable;
      }      

      if (state.changed){
        state.broadcastState = true; // inform application manager that the state has changed. See why we managed both separately => state.progress

        if (state.code == IF_READY){      // we just went online 
          offlineStart = 0;               // so we reset timer
          //DEBUG_INFO("%s 🔌 ✔️ ------- Online, offlineStart = 0 ---------- ",interfaceName.c_str());           
        }else{
          if (offlineStart == 0){          
            offlineStart = millis();     // we went offline, so we start counting
            //DEBUG_INFO("%s 🔌 ❌------- Offline, offlineStart=%i ---------- ",interfaceName.c_str(),offlineStart);           
          }
        }

        this->determineQoS();            // recalculate the color
        
      }

      if (state.enabled){      
        return monitor();
      } 
     
      return false;
    }

    /** @brief Report whether the interface can be enabled or disabled by the user. */
    bool isSwitchable(){
      return state.switchable;
    }

    /** @brief Apply the requested enabled state through the interface lifecycle. */
    virtual void switchDevice(bool _enable){
      state.enabled = _enable;      
      DEBUG_INFO("%s::switchDevice(%s)",interfaceName.c_str(),state.enabled?"true":"false");      
      if (_enable) setStateDisconnected();  // if device is just switched off it can't be yet connected, hence we set its state to disconnected
      else setStateDisabled();
      //DEBUG_PRINT("\nDump after %s switchDevice: %s",interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    /** @brief Set the interface status to deactivated. */
    void setStateDeactivated(){
      DEBUG_INFO("%s::setStateDeactivated()◎",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DEACTIVATED;
      state.subState =  "";
      state.led = "black";
      state.progress = -1;
    }

    /** @brief Set the interface status to disabled. */
    virtual void setStateDisabled(){
      DEBUG_INFO("%s::setStateDisabled()⚫",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DISABLED;
      state.subState =  IF_STATE_DISABLED;
      state.details = "Interface is disabled";
      state.led = "black";
      state.progress = -1;          
    }

    /** @brief Set the interface status to not implemented. */
    void setStateNotImplemented(){
      DEBUG_INFO("%s:setStateNotImplemented()◎",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_NOTIMPLEMENTED;
      state.subState =  IF_STATE_DISABLED;
      state.details = "Interface not yet implemented";
      state.led = "black";
      state.progress = -1;          
    }

    /** @brief Set the interface status to ready. */
    virtual void setStateReady(const char* led = "green", const char* label = lang.getString(IF_STATE_READY)->c_str(), const char* subState = "", const char* details = ""){            
      DEBUG_INFO("%s:setStateReady(%s,%s)🟢",interfaceName.c_str(),label,details);
      state.code = IF_READY;      
      state.label = label;
      state.subState = subState;
      state.details = details;
      state.led = led;
      state.progress = -1;      
    }

    /** @brief Set the interface status to pending. */
    virtual void setStatePending(const char* led = "green medium", const char* details = "Scanning network"){
      DEBUG_INFO("%s:setStatePending(%s)◐",interfaceName.c_str(),details);      
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.details = details;
      state.subState = "";      
      state.led = led;
      state.progress = -1;
    }

    /** @brief Set the interface status to disconnected. */
    virtual void setStateDisconnected(const char* details=""){
      DEBUG_INFO("%s:setStateDisconnected(%s)🔴",interfaceName.c_str(),details);      
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISCONNECTED;
      state.subState = "";
      state.details = details;
      state.led = "red fast";
      state.progress = -1;
    }        

    /** @brief Set the interface status to error. */
    virtual void setStateError(const char* details=""){
      setStateDisconnected(details);
      state.label = IF_STATE_ERROR;
    }

    #define IF_CFG_FILE_PATH "/"

    /** @brief Load the saved configuration from its backing file. */
    virtual void load() { // some interfaces may override this to be empty as they do not load or save parameters
      String fname = concatStrings(IF_CFG_FILE_PATH,getFilename());
      DEBUG_INFO("load(%s)",fname.c_str());
      // Ouvrir le fichier
      File file = LittleFS.open(fname, "r");
      String content="";
      char* defaultConfig = "{\"enabled\":false}";
      if (file) {
        // Lire tout le contenu dans une String
        content = file.readString();
        file.close();
        if (content.length() == 0) {
          // Appeler la fonction de restauration
          DEBUG_INFO("\t::empty file, use default"); 
          content = defaultConfig;
        }        
      }else{
        //DEBUG_PRINT("\t::file open failed, use default");
        content = defaultConfig;
      }
      restore(content.c_str());      
    }

    /** @brief Write the current configuration to its backing file. */
    virtual bool save(){  // some interfaces may override this to be empty as they do not load or save parameters
      File file = LittleFS.open(IF_CFG_FILE_PATH+getFilename(), "w");
      if (!file) {
        DEBUG_ERROR("\t%s::save : impossible d'ouvrir %s en écriture\t", interfaceName.c_str(),getFilename().c_str());
        return false;
      }

      String content = serializeConfig(false); // only serialize properties with interface name, return {"sysOSC":{"properties":["/oscport/5000","/ip/1.1.1.2"]}}}

      JsonDocument doc;
      deserializeJson(doc, content);

      // Récupérer le noeud sysOSC afin de sauvegarder son contenu dans le nom de l'interface
      JsonObject sysOSC = doc[interfaceName];
      serializeJson(sysOSC, content);      

      size_t written = file.print(content);
      file.close();

      if (written == content.length()) {
        DEBUG_INFO("%s::saved : %s", interfaceName.c_str(),content.c_str());  
      }else{
        DEBUG_INFO("%s::save : ony %d/%d bytes in %s\t", interfaceName.c_str(),written, content.length(), getFilename().c_str());
        return false;

      }

      return true;

    }

    /**
     * @brief by default no properties are serialized
     */
    virtual void serializeProperties(JsonArray& properties){}

    /** @brief Restore one property string and report whether its value changed. */
    virtual bool restoreProperty(const char* property){
      DEBUG_INFO("%s::restoreProperty(%s) not applicable",interfaceName.c_str(),property);
      return false;
    } 
    
    /** @brief Poll the interface connection and process pending data. */
    virtual bool monitor() = 0; 

    /** @brief Update the quality-of-service indicator used by the display. */
    virtual void determineQoS(){
      if (state.code == IF_READY) state.QoSlevel = QoS::GOOD;
      else state.QoSlevel = QoS::UNDETERMINED;
    };

    /** @brief React to changed connection settings, including reconnection when required. */
    virtual void handlePropertyChange(){DEBUG_INFO("%s::handlePropertyChange() : void",name.c_str());} // this s called when the interface parameter/config/property is changed, the interface has to decide what to do like restarting

    /** @brief Build the filename used to persist this interface configuration. */
    String getFilename(){
      return  concatStrings(interfaceName,"_if.cfg");
    }       

    /** @brief Return the borrowed icon text representing this interface. */
    virtual const String* getIcon(){
      return &icon; //hwm.network->networkConnectivityIcon
    }

    /** @brief Return the renderer associated with this interface icon. */
    virtual OpenFontRender* getIconRenderer(){ return &gui.iconOfr; }

  protected:
    String icon = ICON_LINK;

};

/**
 * @brief Coordinates network-service startup and shared connection budgets.
 *
 * On Ethernet, enabled services share a weighted budget of four. Services waiting
 * for capacity remain enabled but locked; releasing capacity retries them through
 * the same enable path. Wi-Fi bypasses this budget. Inventory entries are borrowed.
 */
class NetWorkService{
  public:
    TargetEcho destination = TargetEcho("0.0.0.0");

    /**
     * @brief ---- Global inventory of services (NetWorkService*) ----
     */
    static std::vector<NetWorkService*> inventory;
    static int8_t enabledServicesWeightSum;
  
    /** @brief Perform one service-specific connection and data-processing pass. */
    virtual bool monitorService()=0;
    /** @brief Start this service on the available network connection. */
    virtual bool handleConnectService()=0;
    /** @brief Close this service and release its transport resources. */
    virtual bool handleDisconnectService()=0;

    /** @brief Register a borrowed network-service pointer in the shared inventory. */
    static void addToInventory(NetWorkService* iface) {
      if (iface == nullptr) return;
      inventory.push_back(iface);

    }

    #define SERVICES_WEIGHT_LIMIT  4
    
    /** @brief Return the Ethernet connection-budget weight used by this service. */
    virtual uint8_t getServiceWeight(){
      return 1; // default
    }

    /** @brief Report whether the service is waiting for network capacity. */
    bool isServiceLocked() const {
      if (!deviceInterface) return true;
      return deviceInterface->state.label == IF_STATE_LOCKED;
    }


    /**
     * @brief Test de non regression:
     *   - double click
     *   - actualisation (sans modification des parametres) et avec differents etats
     *   - actualisation (avec modification des parametres) et avec differents etats
     *   - demarrer une interface alors qu'elle doit etre lock
     *   - demarrer une interface en disable
     */

    void releaseCapacity(){
      if (!capacityReserved){
        DEBUG_INFO("\t%s::releaseCapacity() skipped (not reserved)",
                    deviceInterface ? deviceInterface->interfaceName.c_str() : "<?>");
        return;
      }

      // first we release what we consume
      enabledServicesWeightSum -= getServiceWeight();
      if (enabledServicesWeightSum < 0) enabledServicesWeightSum = 0;
      capacityReserved = false;

      DEBUG_INFO("\t💪%s::releaseCapacity() : %i, consummed:%i/%i",
                  deviceInterface ? deviceInterface->interfaceName.c_str() : "<?>",
                  getServiceWeight(),enabledServicesWeightSum,SERVICES_WEIGHT_LIMIT);

      // then we check if any enabled interface currently in LOCKED state can be unlocked
      unlockWaitingEnabledServices();
      /*
        Un service locké n’a pas réservé de budget (reserveCapacity() n’incrémente que si ça passe).
        Quand on “unlock”, on appelle enable(), qui rappelle reserveCapacity() :
        si capacité dispo → ça incrémente enabledServicesWeightSum correctement
        sinon → ça reste locké
        Donc le budget reste cohérent sans double comptage.        
      */
    }

    /**
     * @brief Reserve shared Ethernet service budget without double counting.
     *
     * Wi-Fi bypasses this budget. On Ethernet, reject only when the added weight
     * would exceed SERVICES_WEIGHT_LIMIT. Repeated reservations do not add weight.
     * @return True when capacity is insufficient; false when allowed/already reserved.
     */
    bool reserveCapacity() {
      if (capacityReserved){
        DEBUG_INFO("\t💪%s::reserveCapacity() skipped (already reserved)",
                    deviceInterface ? deviceInterface->interfaceName.c_str() : "<?>");
        return false; // limit = false
      }

      // Sur WiFi: pas de limitation
      if (hwm.network->media != CABLE_MEDIA) {
        DEBUG_INFO("\tNOT LIMIT (WiFi)");
        return false; // limit = false
      }

      const int w = getServiceWeight();

      // Refuser si l'ajout dépasserait ou atteindrait la limite
      // (choisis > ou >= selon ta règle exacte)
      if (enabledServicesWeightSum + w > SERVICES_WEIGHT_LIMIT) {
        DEBUG_INFO("\t💪capacity %i+%i > %i => LOCKED",
                    enabledServicesWeightSum, w, SERVICES_WEIGHT_LIMIT);
        return true; // limit = true
      }

      // OK: on réserve
      enabledServicesWeightSum += w;
      capacityReserved = true;

      DEBUG_INFO("\t💪RESERVED => %i/%i",
                  enabledServicesWeightSum, SERVICES_WEIGHT_LIMIT);

      return false; // limit = false
    }

    /** @brief Set the interface status to disconnected. */
    virtual void setStateDisconnected(){ // to be overrided by derived class
      if (!deviceInterface) return;
      DEBUG_INFO("%s::setStateDisconnected()🔴", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.details = "";
      deviceInterface->state.led = "red";
      deviceInterface->state.progress = -1;
    }

    /** @brief Associate this network service with its device interface. */
    void attachDeviceInterface(DeviceInterface* _devif){
      deviceInterface = _devif;
      addToInventory(this); // <-- single inventory of NetWorkService*
    }

    // ---- NEW: tries to unlock + (re)enable services waiting for capacity ----
    /*
    Objectif : parcourir les interfaces enabled et verrouillées (state=LOCKED), et tenter de les activer tant que possible.
    Comme ton enable() :
    appelle reserveCapacity() (qui réserve le budget si possible),
    met l’état (LOCKED ou tentative de connect),
    …le plus sûr est de réutiliser enable() pour éviter de dupliquer la logique de budget.    
    */
    /** @brief Retry enabled services waiting for Ethernet budget through the normal enable path. */
    static void unlockWaitingEnabledServices(){
      // Only meaningful if network is connected; otherwise enable() would just set disconnected
      if (hwm.network->state != CONNECTED) return;

      // Try to enable locked-but-enabled services until we hit the capacity limit
      for (NetWorkService* iface : inventory){
        if (!iface || !iface->deviceInterface) continue;

        if (!iface->deviceInterface->state.enabled) continue;  // only those enabled by user
        if (!iface->isServiceLocked()) continue;               // only those currently locked

        if (enabledServicesWeightSum > SERVICES_WEIGHT_LIMIT) break;

        // Reuse existing logic:
        // - reserveCapacity() will book the budget if available
        // - will set locked state or attempt handleConnectService()
        iface->enable();

        if (enabledServicesWeightSum > SERVICES_WEIGHT_LIMIT) break;
      }
    }

  protected:
    DeviceInterface* deviceInterface = nullptr;
    bool capacityReserved = false;

    /** @brief Enable or disable the service and reset its reachability timestamps. */
    void switchNetworkService(bool _enable){      
      //DEBUG_PRINT("\nDump before switchNetworkService: %s",hwm.network->dumpW5500Sockets().c_str());
      DEBUG_INFO("(%s)::switchNetworkService(%s)",
                  deviceInterface->interfaceName.c_str(),
                  _enable ? "true" : "false");              
      if(_enable) enable();
      else disable();
      destination.lastRxTime = 0; // immediate state query
      destination.lastPingTx = 0; // reset ping timestamp
      destination.lastPingRx = 0; // reset ping timestamp
    }

    /** @brief Enable the service, respecting the shared network capacity and connection state. */
    void enable(){
      DEBUG_INFO("%s->enable()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.enabled = true;
      if (reserveCapacity()){
        setStateLocked();
      }else{
        deviceInterface->state.enabled = true;
        // attempts to enable devices may happen while network is not yet connected
        if (hwm.network->state == CONNECTED){
          if (!handleConnectService()){
            setStateDisconnected();
          }else{
            deviceInterface->setStatePending();
          }
        }else{
          DEBUG_INFO("%s::Network down, skipped handleConnectService()",
                      deviceInterface->interfaceName.c_str());
          setStateDisconnected();
        }
      }
      //DEBUG_INFO("After %s enable: %s",deviceInterface->interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    /** @brief Disable the service, release its capacity and disconnect its transport. */
    void disable(){
      DEBUG_INFO("%s->disable()", deviceInterface->interfaceName.c_str());      
      deviceInterface->state.enabled = false;
      releaseCapacity();   // we only release if the device was enabled otherwise we may fool the count
      deviceInterface->setStateDisabled();
      handleDisconnectService();
      //DEBUG_INFO("After %s disable: %s",deviceInterface->interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    /** @brief Poll the service only when networking is available and capacity is not locked. */
    bool monitorNetwork(){

      if (!deviceInterface) return false;  

      if (hwm.network->state == DISCONNECTED){
        deviceInterface->state.progress = hwm.network->networkConnectAttemptProgress;
        deviceInterface->state.subState = hwm.network->SSID;
      }else if (!isServiceLocked()){
        return monitorService();
      }

      return false;   // no data processed
    }
    
    /** @brief Show that the underlying network connection is unavailable. */
    void setNetworkStateDown(){
      if (!deviceInterface) return;
      DEBUG_INFO("%s::setNetworkStateDown()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.led = "red medium";
      deviceInterface->state.details = *lang.getString("setNetworkStateDown");
      deviceInterface->state.progress = 0; // show progress bar
      // progress is altered by the network
    }


    /** @brief Set the interface status to waiting for capacity. */
    virtual void setStateLocked(const char* led = "orange",const char* details = "Reached maximum capacity. Disable other network device(s) to enable this one."){
      DEBUG_INFO("%s::setStateLocked()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_LOCKED;
      deviceInterface->state.subState = "";
      deviceInterface->state.details = details;
      deviceInterface->state.led = led;
      deviceInterface->state.progress = -1;
    }

    /** @brief Show that endpoint discovery must be enabled to browse devices. */
    virtual void setSetDirectoryServiceDown(){
      DEBUG_INFO("%s::setStateLocked()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.subState = "";
      deviceInterface->state.details = "Enable Directory to browse devices";
    }

    /** @brief Set the interface status to connecting. */
    void setStateConnecting(const char* subState, const char* details){
      DEBUG_INFO("%s: setStateConnecting()",deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;      
      deviceInterface->state.label = IF_STATE_CONNECTING;   
      deviceInterface->state.subState = subState;
      deviceInterface->state.details = details;
      deviceInterface->state.led = "green slow";
      deviceInterface->state.progress = 0; //show progress bar
    }     

    /** @brief Set the interface status to synchronizing. */
    void setStateSyncing(const char* details){
      setStateConnecting(IF_STATE_SYNCING,details);
      DEBUG_INFO("%s: setStateSyncing()",deviceInterface->interfaceName.c_str());
      deviceInterface->state.label = IF_STATE_SYNCING;
      deviceInterface->state.led = "green fast";
      deviceInterface->state.progress = 0; //show progress bar
    }     

};
std::vector<NetWorkService*>NetWorkService::inventory;
int8_t NetWorkService::enabledServicesWeightSum = 0;

/**
 * @brief Non-owning registry used to locate device interfaces by name.
 */
class InterfaceRepository{

  public:
    std::vector<DeviceInterface*> interfaces;

    /** @brief Export this object using its JSON representation. */
    void serialize(JsonArray* interfacesArray){
      for (DeviceInterface* interface : interfaces) {
        // serializing is used to save the interfaces config in the config file or to send them to the web app to be modified
        // hence, we only do that for interface which are "persistant"
        //if (interface->exposed == true){
          JsonDocument interfaceDoc;
          interface->serialize(interfaceDoc);
          interfaceDoc.shrinkToFit();
          interfacesArray->add(interfaceDoc);
        //}
      }     
    }  

    /** @brief Find the requested registered object by name. */
    DeviceInterface* retrieve(String _interfaceName);
}interfaceRepo;


/**
 * @brief the interfaces uses its own repository to store parameters to avoid conflict with data sets groups
 */
DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable), SerializableParameters(name,_group,&IFdataRepo, true),EventDispatcher(48){   
  // 48 event handlers = maximum of strip that can inform an interface
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
  message.type = EVENT_INTERFACE_MESSAGE;   // an interface sends message to strip and not to the application manager, so we set a specific type for that
}

/** @brief Release resources held by this interface. */
DeviceInterface::~DeviceInterface(){
  interfaceRepo.interfaces.erase(std::remove(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), this), interfaceRepo.interfaces.end());
}

/** @brief Find the requested registered object by name. */
DeviceInterface* InterfaceRepository::retrieve(String requestedInterface){
  for (const auto& interface : interfaceRepo.interfaces){ 
    //DEBUG_PRINT("\nInterfaceRepository::retrieve(%s)=>%s",requestedInterface.c_str(),interface->interfaceName.c_str())     ;
    if (interface->interfaceName == requestedInterface) return interface;
  }
  DEBUG_ERROR("\nError: requested interface <%s> is not existing, returning nullptr",requestedInterface.c_str());
  return nullptr;
}

/**
 * @brief Device interface with connection monitoring and network-service management.
 *
 * Adds peer reachability and connection timing to DeviceInterface. Protocol-specific
 * subclasses supply requests/replies and service connect/disconnect operations.
 * Keep state transitions in the shared contract so UI and browser status agree.
 */
class NetworkDevice: public DeviceInterface, public NetWorkService{

  public:

    int remotePort = 0;
    int localPort = 0;

    /** @brief Initialize NetworkDevice with the supplied configuration and bindings. */
    NetworkDevice(String name, String group, int lport, int rport, String echoCommand, bool _configurable=false):DeviceInterface(name, group,_configurable){
      
      attachDeviceInterface(this);
      
      destination.echoCommand = echoCommand;

      registerDataSetAddress(&addressPort);          // global
      registerDataSetAddress(&addressIP);           // global       

      remotePort = rport;
      localPort = lport;     
    }

    /** @brief Request a presence check for the remote device. */
    virtual bool ping() = 0;    
    /** @brief Record the remote reply used for reachability monitoring. */
    virtual void echo(void* data) = 0;

    /**
     * @brief this function is the internal TOP method called when you receive an answered from the device ping, showing it is live
     * it calculates the latency and calls the  method that actually implements the mecasim that processes the data from the derived class, like device dientification, etc...
     */
    virtual void pulseBeat(void* data){
      destination.lastPingRx = millis();   // we set the reception timestamp as soon as we receive the ping response, so we can calculate the latency and jitter based on the difference between the ping transmission timestamp and the reception timestamp  
      calculateLatencyAndJitter();
      echo(data);   //handover to derived class
    }

    /**
     * @brief this function is the internal TOP method called when we want to query network to see if device is live
     * it sets the timeout and calls the  method that actually implements the ping mecasim from the derived class
     */
    virtual bool pulseCheck(){
      DEBUG_INFO("NetworkDevice(%s)::pulseCheck() (%lu)",interfaceName.c_str(), millis());
      dataProcessed = true;       // the ping throws some data out
      return ping();
    }

    /**
     * @brief network devices listen to network event
     */
    virtual bool _begin() override{     // called by init()
      DEBUG_INFO("NetworkDevice(%s)::_begin() => self registering to network event",interfaceName.c_str());
      hwm.network->registerEventListener(this);   
      return begin();    // derived class call   
    };        
   
    /** @brief Poll the interface connection and process pending data. */
    virtual bool monitor() override{           
      return monitorNetwork();      
    }    

    /** @brief Apply the requested enabled state through the interface lifecycle. */
    virtual void switchDevice(bool _enabled) override{
      switchNetworkService(_enabled); // we use our own
    }   

    /** @brief Update timing measurements from recent request/reply timestamps. */
    void calculateLatencyAndJitter(){
      /* 
        we got an echo, so we calculate the latency
        we determine latency and jitter based on the ping response time, we use an EWMA to smooth the values and avoid spikes, we also set a max latency of 500ms to avoid crazy values in case of network issues                    
      */

      unsigned long rawLatency = destination.lastPingRx - destination.lastPingTx; // tx is set when we send the ping, rx is set when we receive the echo, so the difference is the round trip time (RTT) which is a common way to measure latency in network communication

      const unsigned long MAX_VALID_LATENCY = 500;
      const float LAT_ALPHA = 0.20f;
      const float JIT_ALPHA = 0.25f;

      if (rawLatency == 0 || rawLatency > MAX_VALID_LATENCY) {
        DEBUG_WARN("rawLatency=%i",rawLatency);
        return;
      }

      const float previousLatency = destination.latency;

      if (destination.latency == 0) {     
        destination.latency = (uint16_t)rawLatency; // first value, just store it
        destination.jitter  = 0;                    // and no jitter
      } else {
        float filteredLatency =                       //✅ Méthode 1 — Moyenne glissante exponentielle (RECOMMANDÉE) très utilisé en réseau (TCP, Wi-Fi, etc.)  
            previousLatency * (1.0f - LAT_ALPHA) +
            rawLatency * LAT_ALPHA;

        float delta = fabsf((float)rawLatency - previousLatency);

        float filteredJitter =
            destination.jitter * (1.0f - JIT_ALPHA) +
            delta * JIT_ALPHA;

        destination.latency = (uint16_t)filteredLatency;
        destination.jitter  = (uint16_t)filteredJitter;
      }

      if (destination.latency > MAX_VALID_LATENCY) {
        destination.latency = MAX_VALID_LATENCY;
      }

      if (destination.jitter > MAX_VALID_LATENCY) {
        destination.jitter = MAX_VALID_LATENCY;
      }

      DEBUG_INFO("📉 %s latency:%u jitter:%u lastPingRx:(%lu)-lastPingTx:(%lu)=raw:(%lu)",
                interfaceName.c_str(),
                destination.latency,
                destination.jitter,
                destination.lastPingRx,
                destination.lastPingTx,
                rawLatency);
    } 

    /** @brief Handle relevant incoming events for this component. */
    void onEvent(const Event& evt) override {
      // recive the event propagated from Appmanager
      switch(evt.type){
        case EVENT_NETWORK_DISCONNECTED:
          handleDisconnectService();
          break;
        case EVENT_NETWORK_CONNECTED:
          if (hwm.network->state == CONNECTED){
            if (deviceInterface->state.enabled){
              enable();
            }else{
              deviceInterface->setStateDisabled();
            }
          }
        break;
      }
    }    

    /** @brief React to changed connection settings, including reconnection when required. */
    virtual void handlePropertyChange(){
      DEBUG_INFO("%s::handlePropertyChange()",interfaceName.c_str());
      handleDisconnectService();  // force to reconnect
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(JsonArray& properties) override{
      buf = (char*)malloc((strlen(ADDRESS_PORT)+6)*sizeof(char*)+1);
      sprintf(buf,ADDRESS_PORT,remotePort);
      properties.add(String(buf)); 
      free(buf);

      buf = (char*)malloc((strlen(ADDRESS_IP)+destination.ip.toString().length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,destination.ip.toString().c_str());
      properties.add(String(buf)); 
      free(buf);                
    }

    /** @brief Restore one property string and report whether its value changed. */
    virtual bool restoreProperty(const char* property) override{
      bool changed = false;
      uint16_t _port;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];      
      if (sscanf(property, ADDRESS_PORT, &_port) == 1) {
        if (_port != remotePort)   changed = true;
        remotePort = _port;
        if (remotePort>65535 || remotePort<0) remotePort = 0;
      }else if (sscanf(property, ADDRESS_IP, &inputStr)) {
        if (destination.host != inputStr)  changed = true;
        destination.host = inputStr;
        IPAddress ip;
        if (ip.fromString(inputStr)){
          destination.ip = ip;                
        }else{
          DEBUG_ERROR("\ninvalid IP address: %s",inputStr);
        }
      }
      return changed;
    }       
    
    /** @brief Set the interface status to connected. */
    virtual void setStateConnected(){
      DEBUG_INFO("%s:setSetateConnected()🟢",deviceInterface->interfaceName.c_str());
      DeviceInterface::setStateReady("green", IF_STATE_CONNECTED, destination.ip.toString().c_str(), destinationDetails().c_str());
    }

    /** @brief Build the text describing the selected network destination. */
    String destinationDetails(){
      return concatStrings(destination.ip.toString(),":",String(remotePort)).c_str();       
    }

    /** @brief Set the interface status to disconnected. */
    virtual void setStateDisconnected() override{
      DEBUG_INFO("%s:setStateDisconnected()🔴",deviceInterface->interfaceName.c_str());
      DeviceInterface::setStateDisconnected("no connection");
    }     

};

#endif
