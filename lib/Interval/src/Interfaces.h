#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

#include "interfaces/i_states.h"


#define ADDRESS_CHANGETOPARAMETERNAME "/config/%s"


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
    bool hasStrip = true;      // if the interface is used by a strip (ex. Midi) or not (ex. Directory Service), by default yes, but some interface are more technical and not relevant for the user to create a strip with them, so we can hide them in the strip creation menu by setting this parameter to false

    String group = "undef"; // used by the strip to define which deviceinterface they can connect

    Event message;


    DeviceInterface(const String&, const String&, bool);
    ~DeviceInterface();

    virtual const char* getDispatcherName() const override{
      return interfaceName.c_str();
    }

    virtual const char* getListenerName()  override{
      return interfaceName.c_str();
    }


    void onEvent(const Event& evt) override {

    }

    virtual bool _begin(){     // called by init()
      DEBUG_INFO("%s::_begin() void",interfaceName.c_str());
      return true;
    };        

    virtual bool begin(){     // called by _begin() and let user define its own begin in derived class. Curently only in user with NetworkDevice
      DEBUG_INFO("DeviceInterface(%s)::begin() => default void",interfaceName.c_str());
      return true;
    }    

    virtual bool end(){       // called to shutdown completely
      DEBUG_INFO("%s::end() void",interfaceName.c_str());
      return true;
    };          

    virtual bool stop(){      // called to stop (ex. disconnect the network for network interface) require a begin() to start again
      DEBUG_INFO("%s::stop() void",interfaceName.c_str());
      return true;
    };           

    virtual const String& getProviderNameForData(){ return group;}

    String serializeState(){
      JsonDocument doc;
      
      doc[interfaceName][INTERFACE_STATE] = state.label;
      doc[interfaceName][INTERFACE_PROGRESS] = state.progress;
      doc[interfaceName][INTERFACE_DETAILS] = state.details;
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

    bool init(/*EventListener* listener*/) {
      DEBUG_INFO("----------------------------------  %s->init(...)  ---------------------------------- ", interfaceName.c_str());
      //registerEventListener(listener);
      load();
      bool success = _begin();
      DEBUG_INFO("-----------------------------------------------------------------------------------------------------\n");
      return success;
    }

    virtual void handleRequestRefreshState(){
      DEBUG_INFO("%s->handleRequestRefreshState() : void",interfaceName.c_str());           
    }

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
      }

      if (state.enabled){      
        return monitor();
      } 
     
      return false;
    }

    bool isSwitchable(){
      return state.switchable;
    }

    virtual void switchDevice(bool _enable){
      state.enabled = _enable;      
      DEBUG_INFO("%s::switchDevice(%s)",interfaceName.c_str(),state.enabled?"true":"false");      
      if (_enable) setStateDisconnected();  // if device is just switched off it can't be yet connected, hence we set its state to disconnected
      else setStateDisabled();
      //DEBUG_PRINT("\nDump after %s switchDevice: %s",interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    void setStateDeactivated(){
      DEBUG_INFO("%s::setStateDeactivated()◎",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DEACTIVATED;
      state.subState =  "";
      state.led = "black";
      state.progress = -1;
    }

    virtual void setStateDisabled(){
      DEBUG_INFO("%s::setStateDisabled()⚫",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DISABLED;
      state.subState =  IF_STATE_DISABLED;
      state.details = *lang.getString("Interface is disabled");         
      state.led = "black";
      state.progress = -1;          
    }

    void setStateNotImplemented(){
      DEBUG_INFO("%s:setStateNotImplemented()◎",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_NOTIMPLEMENTED;
      state.subState =  IF_STATE_DISABLED;
      state.details = *lang.getString("Interface not yet implemented");          
      state.led = "black";
      state.progress = -1;          
    }

    virtual void setStateReady(const char* led = "green", const char* label = lang.getString(IF_STATE_READY)->c_str(), const char* subState = "", const char* details = ""){            
      DEBUG_INFO("%s:setStateReady(%s,%s)🟢",interfaceName.c_str(),label,details);
      state.code = IF_READY;      
      state.label = label;
      state.subState = subState;
      state.details = details;
      state.led = led;
      state.progress = -1;      
    }

    virtual void setStatePending(const char* led = "green medium", const char* details = lang.getString("Scanning network")->c_str()){
      DEBUG_INFO("%s:setStatePending()◐",interfaceName.c_str());      
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.details = details;
      state.subState = "";      
      state.led = led;
      state.progress = -1;
    }

    virtual void setStateDisconnected(const char* details=""){
      DEBUG_INFO("%s:setStateDisconnected()🔴",interfaceName.c_str());      
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISCONNECTED;
      state.subState = "";
      state.details = details;
      state.led = "red fast";
      state.progress = -1;
    }        

    virtual void setStateError(const char* details=""){
      setStateDisconnected(details);
      state.label = IF_STATE_ERROR;
    }

    #define IF_CFG_FILE_PATH "/"

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

    //by default no properties are serialized
    virtual void serializeProperties(JsonArray& properties){}

    virtual bool restoreProperty(const char* property){
      DEBUG_INFO("%s::restoreProperty(%s) not applicable",interfaceName.c_str(),property);
      return false;
    } 
    
    virtual bool monitor() = 0; 

    virtual void handlePropertyChange(){DEBUG_INFO("%s::handlePropertyChange() : void",name.c_str());} // this s called when the interface parameter/config/property is changed, the interface has to decide what to do like restarting

    String getFilename(){
      return  concatStrings(interfaceName,"_if.cfg");
    }       

    virtual const String* getIcon(){
      return &icon;
    }

  protected:
    String icon = ICON_LINK;

};

class NetWorkService{
  public:
    TargetEcho destination = TargetEcho("0.0.0.0");

    // ---- Global inventory of services (NetWorkService*) ----
    static std::vector<NetWorkService*> inventory;
    static int8_t enabledServicesWeightSum;
  
    virtual bool monitorService()=0;
    virtual bool handleConnectService()=0;
    virtual bool handleDisconnectService()=0;

    static void addToInventory(NetWorkService* iface) {
      if (iface == nullptr) return;
      inventory.push_back(iface);

    }

    #define SERVICES_WEIGHT_LIMIT  5
    
    bool serviceLocked = true; // by default locked to avoid monitoring straight after startup

    virtual uint8_t getServiceWeight(){
      return 1; // default
    }


    /* 
      Test de non regression:
        - double click
        - actualisation (sans modification des parametres) et avec differents etats
        - actualisation (avec modification des parametres) et avec differents etats
        - demarrer une interface alors qu'elle doit etre lock
        - demarrer une interface en disable  
    */

    void releaseCapacity(){
      if (hwm.network->media != CABLE_MEDIA) return;    // only applicable for ethernet

      // first we release what we consume
      enabledServicesWeightSum -= getServiceWeight();
      if (enabledServicesWeightSum < 0) enabledServicesWeightSum = 0;

      DEBUG_INFO("\t%s::releaseCapacity() : %i",
                  deviceInterface ? deviceInterface->interfaceName.c_str() : "<?>",
                  getServiceWeight());

      // then we check if any enabled interface that was serviceLocked can be unlocked
      unlockWaitingEnabledServices();
      /*
        Un service locké n’a pas réservé de budget (reserveCapacity() n’incrémente que si ça passe).
        Quand on “unlock”, on appelle enable(), qui rappelle reserveCapacity() :
        si capacité dispo → ça incrémente enabledServicesWeightSum correctement
        sinon → ça reste locké
        Donc le budget reste cohérent sans double comptage.        
      */
    }

    bool reserveCapacity() {
      // Sur WiFi: pas de limitation
      if (hwm.network->media != CABLE_MEDIA) {
        DEBUG_INFO("\tNOT LIMIT (WiFi)");
        return false; // limit = false
      }

      const int w = getServiceWeight();

      // Refuser si l'ajout dépasserait ou atteindrait la limite
      // (choisis > ou >= selon ta règle exacte)
      if (enabledServicesWeightSum + w >= SERVICES_WEIGHT_LIMIT) {
        DEBUG_INFO("\tcapacity %i+%i >= %i => LOCKED",
                    enabledServicesWeightSum, w, SERVICES_WEIGHT_LIMIT);
        return true; // limit = true
      }

      // OK: on réserve
      enabledServicesWeightSum += w;

      DEBUG_INFO("\tRESERVED => %i/%i",
                  enabledServicesWeightSum, SERVICES_WEIGHT_LIMIT);

      return false; // limit = false
    }

    virtual void setStateDisconnected(){ // to be overrided by derived class
      if (!deviceInterface) return;
      DEBUG_INFO("%s::setStateDisconnected()🔴", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.details = "";
      deviceInterface->state.led = "red";
      deviceInterface->state.progress = -1;
    }

    void attachDeviceInterface(DeviceInterface* _devif){
      deviceInterface = _devif;
      addToInventory(this); // <-- single inventory of NetWorkService*
    }

    // ---- NEW: tries to unlock + (re)enable services waiting for capacity ----
    /*
    Objectif : parcourir les interfaces enabled et serviceLocked, et tenter de les activer tant que possible.
    Comme ton enable() :
    appelle reserveCapacity() (qui réserve le budget si possible),
    met serviceLocked et l’état (LOCKED ou tentative de connect),
    …le plus sûr est de réutiliser enable() pour éviter de dupliquer la logique de budget.    
    */
    static void unlockWaitingEnabledServices(){
      // Only meaningful if network is connected; otherwise enable() would just set disconnected
      if (hwm.network->state != CONNECTED) return;

      // Try to enable locked-but-enabled services until we hit the capacity limit
      for (NetWorkService* iface : inventory){
        if (!iface || !iface->deviceInterface) continue;

        if (!iface->deviceInterface->state.enabled) continue;  // only those enabled by user
        if (!iface->serviceLocked) continue;                   // only those currently locked

        if (enabledServicesWeightSum >= SERVICES_WEIGHT_LIMIT) break;

        // Reuse existing logic:
        // - reserveCapacity() will book the budget if available
        // - will set locked state or attempt handleConnectService()
        iface->enable();

        if (enabledServicesWeightSum >= SERVICES_WEIGHT_LIMIT) break;
      }
    }

  protected:
    DeviceInterface* deviceInterface = nullptr;

    void switchNetworkService(bool _enable){      
      deviceInterface->state.enabled = _enable;
      //DEBUG_PRINT("\nDump before switchNetworkService: %s",hwm.network->dumpW5500Sockets().c_str());
      DEBUG_INFO("(%s)::switchNetworkService(%s)",
                  deviceInterface->interfaceName.c_str(),
                  _enable ? "true" : "false");              
      if(_enable) enable();
      else disable();
    }

    void enable(){
      DEBUG_INFO("%s->enable()", deviceInterface->interfaceName.c_str());

      serviceLocked = reserveCapacity();

      if (serviceLocked){
        setStateLocked();
      }else{
        // attempts to enable devices may happen while network is not yet connected
        if (hwm.network->state == CONNECTED){
          if (!handleConnectService()){
            setStateDisconnected();
          }
        }else{
          DEBUG_INFO("%s::Network down, skipped handleConnectService()",
                      deviceInterface->interfaceName.c_str());
          setStateDisconnected();
        }
      }
      //DEBUG_INFO("After %s enable: %s",deviceInterface->interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    void disable(){
      DEBUG_INFO("%s->disable()", deviceInterface->interfaceName.c_str());
      releaseCapacity();
      deviceInterface->setStateDisabled();
      handleDisconnectService();
      //DEBUG_INFO("After %s disable: %s",deviceInterface->interfaceName.c_str(), hwm.network->dumpW5500Sockets().c_str());
    }

    bool monitorNetwork(){

      if (!deviceInterface) return false;  

      if (hwm.network->state == DISCONNECTED){
        deviceInterface->state.progress = hwm.network->networkConnectAttemptProgress;
        deviceInterface->state.subState = hwm.network->SSID;
      }else if (!serviceLocked){
        return monitorService();
      }

      return false;   // no data processed
    }
    
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


    virtual void setStateLocked(const char* led = "orange",const char* details = lang.getString("Reached maximum device capacity. Disable other network device(s) to enable this one.")->c_str()){
      DEBUG_INFO("%s::setStateLocked()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_LOCKED;
      deviceInterface->state.subState = "";
      deviceInterface->state.details = details;
      deviceInterface->state.led = led;
      deviceInterface->state.progress = -1;
    }

    virtual void setSetDirectoryServiceDown(){
      DEBUG_INFO("%s::setStateLocked()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.subState = "";
      deviceInterface->state.details = *lang.getString("Enable Directory to browse devices");
    }

    void setStateConnecting(const char* subState, const char* details){
      DEBUG_INFO("%s: setStateConnecting()",deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;      
      deviceInterface->state.label = IF_STATE_CONNECTING;   
      deviceInterface->state.subState = subState;
      deviceInterface->state.details = details;
      deviceInterface->state.led = "green slow";
      deviceInterface->state.progress = 0; //show progress bar
    }     

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

class InterfaceRepository{

  public:
    std::vector<DeviceInterface*> interfaces;

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

    DeviceInterface* retrieve(String _interfaceName);
}interfaceRepo;


// the interfaces uses its own repository to store parameters to avoid conflict with data sets groups
DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable), SerializableParameters(name,_group,&IFdataRepo, true),EventDispatcher(48){   
  // 48 event handlers = maximum of strip that can inform an interface
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
  message.type = EVENT_INTERFACE_MESSAGE;   // an interface sends message to strip and not to the application manager, so we set a specific type for that
}

DeviceInterface::~DeviceInterface(){
  interfaceRepo.interfaces.erase(std::remove(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), this), interfaceRepo.interfaces.end());
}

DeviceInterface* InterfaceRepository::retrieve(String requestedInterface){
  for (const auto& interface : interfaceRepo.interfaces){ 
    //DEBUG_PRINT("\nInterfaceRepository::retrieve(%s)=>%s",requestedInterface.c_str(),interface->interfaceName.c_str())     ;
    if (interface->interfaceName == requestedInterface) return interface;
  }
  DEBUG_ERROR("\nError: requested interface <%s> is not existing, returning nullptr",requestedInterface.c_str());
  return nullptr;
}

class NetworkDevice: public DeviceInterface, public NetWorkService{

  public:

    int remotePort = 0;
    int localPort = 0;

    NetworkDevice(String name, String group, int lport, int rport, String echoCommand, bool _configurable=false):DeviceInterface(name, group,_configurable){
      
      attachDeviceInterface(this);
      
      destination.echoCommand = echoCommand;

      registerDataSetAddress(&addressPort);          // global
      registerDataSetAddress(&addressIP);           // global       

      remotePort = rport;
      localPort = lport;

      
    }

    virtual bool ping() = 0;

    // network devices listen to network event
    virtual bool _begin() override{     // called by init()
      DEBUG_INFO("NetworkDevice(%s)::_begin() => self registering to network event",interfaceName.c_str());
      hwm.network->registerEventListener(this);   
      return begin();    // derived class call   
    };        

    virtual bool monitor() override{           
      return monitorNetwork();      
    }    

    virtual void switchDevice(bool _enabled) override{
      switchNetworkService(_enabled); // we use our own
    }   

    void calculateLatencyAndJitter(){
      /* we got a echoCommand echo, so we calculate the latency
      we determine latency and jitter based on the ping response time, we use an EWMA to smooth the values and avoid spikes, we also set a max latency of 500ms to avoid crazy values in case of network issues
      Il y a plusieurs façons correctes de moyenner une latence, selon ce que tu veux obtenir. Je te donne les 3 méthodes classiques, de la plus simple à la plus propre pour un système embarqué.
      ✅ Méthode 1 — Moyenne glissante exponentielle (RECOMMANDÉE) très utilisé en réseau (TCP, Wi-Fi, etc.)                
      */
      unsigned long rawLatency = millis() - destination.lastQueryTime;

      // filtre les valeurs incoherentes , par exemple si l'interface est deconnectée
      if (rawLatency < 500){
        // α entre 0.1 (très lisse) et 0.3 (réactif)
        #define LAT_ALPHA 0.2f                                 

        if (rawLatency < 500) {
            destination.latency =
                destination.latency * (1.0f - LAT_ALPHA) +
                rawLatency * LAT_ALPHA;

            destination.latency = (uint16_t)destination.latency;

            float delta = abs((int)rawLatency - (int)destination.latency);

            destination.jitter =
                destination.jitter * 0.75f +
                delta * 0.25f;                      
            }                  

      }
      DEBUG_INFO("%s latency: %i, jitter:%i",interfaceName.c_str(),destination.latency,destination.jitter);    
    }      

    void onEvent(const Event& evt) override {
      // recive the event propagated from Appmanager
      switch(evt.type){
        case EVENT_NETWORK_DISCONNECTED:
          disable();
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

    virtual void handlePropertyChange(){
      DEBUG_INFO("%s::handlePropertyChange()",interfaceName.c_str());
      handleDisconnectService();  // force to reconnect
    }

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
    
    virtual void setStateConnected(){
      DEBUG_INFO("%s:setSetateConnected()🟢",deviceInterface->interfaceName.c_str());
      DeviceInterface::setStateReady("green", lang.getString(IF_STATE_CONNECTED)->c_str(),destination.ip.toString().c_str(), destinationDetails().c_str());
    }

    String destinationDetails(){
      return concatStrings(destination.ip.toString(),":",String(remotePort)).c_str();       
    }

    virtual void setStateDisconnected() override{
      DEBUG_INFO("%s:setStateDisconnected()🔴",deviceInterface->interfaceName.c_str());
      DeviceInterface::setStateDisconnected(concatStrings(*lang.getString("no connection")).c_str());
    }     

};

#endif
