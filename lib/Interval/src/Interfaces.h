#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

#include "interfaces/i_states.h"

#define ADDRESS_CHANGETOPARAMETERNAME "/config/%s"

class DeviceInterface  : public SerializableParameters, public EventDispatcher{
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
  
  #define DEVICE_ON true
  #define DEVICE_OFF false

  public:

    State state;

    Event evt;

    uint8_t stripCount = 0;      // indicate if an interface is not refered by any strip


    String interfaceName; 
  
    int dataAvailable;
    void* data;

    bool exposed = true;

    String group = "undef"; // used by the strip to define which deviceinterface they can connect


    DeviceInterface(const String&, const String&, bool);
    ~DeviceInterface();

    virtual bool begin(){     // called by init()
      DEBUG_PRINT("\n%s::begin() void",interfaceName.c_str());
      return true;
    };        

    virtual bool end(){         // called to shutdown completely
      DEBUG_PRINT("\n%s::end() void",interfaceName.c_str());
      return true;
    };          
    virtual bool stop(){          // called to stop (ex. disconnect the network for network interface) require a begin() to start again
      DEBUG_PRINT("\n%s::stop() void",interfaceName.c_str());
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
      DEBUG_PRINT("\n%s->serializeConfig(%d): %s",interfaceName.c_str(),mainNode,output.c_str());          
      return output;
    }  

    bool init(EventHandler cb){
      DEBUG_PRINT("\n\n----------------------------------  %s->init(...)  ---------------------------------- ",interfaceName.c_str());      
      registerEventHandler(cb);
      load();
      bool success = begin();
      DEBUG_PRINT("\n-----------------------------------------------------------------------------------------------------\n");      
      return success;
    }

    virtual void handleRequestRefreshState(){
      DEBUG_PRINT("\n%s->handleRequestRefreshState() : void",interfaceName.c_str());           
    }

    bool restore(const char* config){    
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, config);
      if (error) {
        DEBUG_PRINT("\n%s restore %s",interfaceName.c_str(),error.c_str());
        return false;
      }
      JsonObject configJson = doc.as<JsonObject>();
      return restore(configJson, false);       // true means => trigger an interface property change check that may lead to disconnecting/reconnecting , which is not the case when we start the interface
    }

    virtual bool restore(const JsonObject& config, bool triggerPropertyChange = true){   // == trigger or not the handlePropertyChange at the end of the restore
      String cfgString;
      serializeJson(config, cfgString);     
      DEBUG_PRINT("\n%s->restore(%s,%s)",interfaceName.c_str(),cfgString.c_str(),triggerPropertyChange?"triggerPropertyChange yes":"triggerPropertyChange no");           
      
      bool changed = false;
      //DEBUG_PRINT("\n%s restore(JsonObject<...>)",interfaceName.c_str());
      // this restore the base parameters common to all base class interface
      if (config.containsKey(INTERFACE_ENABLED)){
        bool enabled = config[INTERFACE_ENABLED];
        switchDevice(enabled);
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

    int available(){ // called by strip 
      return dataAvailable; 
    }


    virtual void update(){      // return a boolean indicatin if the interface has changed its state
      // reset change state
      state.changed = false;

      // reset the flag so strips will not be updated and re-processed already consummed data
      dataAvailable = 0;

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

      if (state.changed) state.broadcastState = true; // inform application manager that the state has changed. See why we managed both separately => state.progress

      if (state.enabled){      
        monitor();
      }      
    }

    virtual bool isSwitchable(){
      return state.switchable;
    }

    /*virtual void switchDevice(bool _enabled){
      enable(_enabled);
      DEBUG_PRINT("\n%s switchDevice=>%s",interfaceName.c_str(),state.enabled?"true":"false");      
    }*/

    virtual void switchDevice(bool _enable){
      state.enabled = _enable;      
      DEBUG_PRINT("\n%s::switchDevice(%s)",interfaceName.c_str(),state.enabled?"true":"false");      
      if (_enable) setStateDisconnected();  // if device is just switched off it can't be yet connected, hence we set its state to disconnected
      else setStateDisabled();
    }

    void setStateDeactivated(){
      DEBUG_PRINT("\n%s::setStateDeactivated()",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DEACTIVATED;
      state.subState =  "";
      state.led = "black";
      state.progress = -1;
    }

    void setStateDisabled(){
      DEBUG_PRINT("\n%s::setStateDisabled()",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_DISABLED;
      state.subState =  IF_STATE_DISABLED;
      state.details = *lang.getString("Interface is disabled");         
      state.led = "black";
      state.progress = -1;          
    }

    void setStateNotImplemented(){
      DEBUG_PRINT("\n%s:setStateNotImplemented",interfaceName.c_str());
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_NOTIMPLEMENTED;
      state.subState =  IF_STATE_DISABLED;
      state.details = *lang.getString("Interface not yet implemented");          
      state.led = "black";
      state.progress = -1;          
    }

    virtual void setStateReady(String led = "green", String label = *lang.getString(IF_STATE_READY), String subState = "", String details = ""){            
      DEBUG_PRINT("\n%s::setStateReady()",interfaceName.c_str());
      state.code = IF_READY;      
      state.label = label;
      state.subState = subState;
      state.details = details;
      state.led = led;
      state.progress = -1;      
    }

    virtual void setStatePending(String led = "green medium", String details = *lang.getString("Scanning devices")){
      DEBUG_PRINT("\n%s::setStatePending()",interfaceName.c_str());      
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.details = details;
      state.subState = "";      
      state.led = led;
      state.progress = -1;
    }

    virtual void setStateDisconnected(){
      DEBUG_PRINT("\n%s::setStateDisconnected()",interfaceName.c_str());      
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISCONNECTED;
      state.subState = "";
      state.details = "";
      state.led = "red fast";
      state.progress = -1;
    }        

    #define IF_CFG_FILE_PATH "/"

    void load() { // some interfaces may override this to be empty as they do not load or save parameters
      String fname = concatStrings(IF_CFG_FILE_PATH,getFilename());
      DEBUG_PRINT("\nload(%s)",fname.c_str());
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
          DEBUG_PRINT("\t::empty file, use default"); 
          content = defaultConfig;
        }        
      }else{
        DEBUG_PRINT("\t::file open failed, use default");
        content = defaultConfig;
      }
      restore(content.c_str());      
    }

    bool save(){  // some interfaces may override this to be empty as they do not load or save parameters
      File file = LittleFS.open(IF_CFG_FILE_PATH+getFilename(), "w");
      if (!file) {
        DEBUG_PRINT("\t%s::save : impossible d'ouvrir %s en écriture\t", interfaceName.c_str(),getFilename().c_str());
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
        DEBUG_PRINT("\n%s::saved : %s", interfaceName.c_str(),content.c_str());  
      }else{
        DEBUG_PRINT("\n%s::save : ony %d/%d bytes in %s\t", interfaceName.c_str(),written, content.length(), getFilename().c_str());
        return false;

      }

      return true;

    }

    //by default no properties are serialized
    virtual void serializeProperties(JsonArray& properties){}

    virtual bool restoreProperty(const char* property){
      DEBUG_PRINT("\n%s::restoreProperty(%s) not applicable",interfaceName.c_str(),property);
      return false;
    } 
    
    // monitor manage internal state and set the dataAvailable flag using internal functions to indicate data are available to be read with read() method
    // do not need to care about offline state as it is managed by update method
    virtual void monitor() = 0; 

    virtual void handlePropertyChange(){DEBUG_PRINT("\n%s::handlePropertyChange() : void",name.c_str());} // this s called when the interface parameter/config/property is changed, the interface has to decide what to do like restarting

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
    PingEcho* destination;

    // ---- Global inventory of services (NetWorkService*) ----
    static std::vector<NetWorkService*> inventory;
    static int8_t enabledServicesWeightSum;

    NetWorkService(){
      hwm.network->registerEventHandler([this](Event* e) {
        networkEventHandler(e);   // équivaut à this->networkEventHandler(e)
      });
    }

    virtual void* read(){
      return nullptr;
    }
    virtual void monitorService()=0;
    virtual bool handleConnectService()=0;
    virtual bool handleDisconnectService()=0;



    static void addToInventory(NetWorkService* iface) {
      if (iface == nullptr) return;
      inventory.push_back(iface);

    }

    #define SERVICES_WEIGHT_LIMIT  2

    bool serviceLocked = true; // by default locked to avoid monitoring straight after startup

    virtual uint8_t getServiceWeight(){
      return 1; // default
    }

    void networkEventHandler(Event* event){
      switch(event->type){
        case EventDispatcher::EVENT_NETWORK_DISCONNECTED:
          disable();
          break;
        case EventDispatcher::EVENT_NETWORK_CONNECTED:
          if (hwm.network->state == CONNECTED){
            if (deviceInterface->state.enabled){
              enable();
            }else{
              DEBUG_PRINT("\n%s->monitor<DISABLED>() ignored", deviceInterface->interfaceName.c_str());            
            }
          }
        break;
      }
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

      DEBUG_PRINT("\t%s::releaseCapacity() : %i",
                  deviceInterface ? deviceInterface->interfaceName.c_str() : "<?>",
                  getServiceWeight());

      // then we check if any enabled interface that was serviceLocked can be unlocked
      unlockWaitingEnabledServices();
      /*
        Un service locké n’a pas réservé de budget (reachedCapacity() n’incrémente que si ça passe).
        Quand on “unlock”, on appelle enable(), qui rappelle reachedCapacity() :
        si capacité dispo → ça incrémente enabledServicesWeightSum correctement
        sinon → ça reste locké
        Donc le budget reste cohérent sans double comptage.        
      */
    }

    bool reachedCapacity(){
      bool limit = false;

      if (hwm.network->media != CABLE_MEDIA) return limit;

      if (enabledServicesWeightSum >= SERVICES_WEIGHT_LIMIT) limit = true;  // already reached the limit
      else enabledServicesWeightSum += getServiceWeight();                  // book the correct amount

      return limit;
    }

    virtual void setStateDisconnected(){ // to be overrided by derived class
      if (!deviceInterface) return;
      DEBUG_PRINT("\n%s::setStateDisconnected()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.details = concatStrings(*lang.getString("disconnected"));
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
    appelle reachedCapacity() (qui réserve le budget si possible),
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
        // - reachedCapacity() will book the budget if available
        // - will set locked state or attempt handleConnectService()
        iface->enable();

        if (enabledServicesWeightSum >= SERVICES_WEIGHT_LIMIT) break;
      }
    }

  protected:
    DeviceInterface* deviceInterface = nullptr;

    void setNetworkStateDown(){
      if (!deviceInterface) return;
      DEBUG_PRINT("\n%s::setNetworkStateDown()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.led = "red medium";
      deviceInterface->state.details = *lang.getString("setNetworkStateDown");
    }

    void switchDevice(bool _enable){
      deviceInterface->state.enabled = _enable;
      DEBUG_PRINT("\nNetworkInterface(%s)::switchDevice(%s)",
                  deviceInterface->interfaceName.c_str(),
                  _enable ? "true" : "false");
      if(_enable) enable();
      else disable();
    }

    void enable(){
      DEBUG_PRINT("\n%s->enable()", deviceInterface->interfaceName.c_str());

      serviceLocked = reachedCapacity();

      DEBUG_PRINT("\tcounts %i/%i => %s",
                  enabledServicesWeightSum,
                  SERVICES_WEIGHT_LIMIT,
                  serviceLocked ? "LOCKED" : "NOT LOCKED");

      if (serviceLocked){
        setStateLocked();
      }else{
        // attempts to enable devices may happen while network is not yet connected
        if (hwm.network->state == CONNECTED){
          if (!handleConnectService()){
            setStateDisconnected();
          }
        }else{
          DEBUG_PRINT("\n%s::Network down, skipped handleConnectService()",
                      deviceInterface->interfaceName.c_str());
          setStateDisconnected();
        }
      }
    }

    void disable(){
      DEBUG_PRINT("\n%s->disable()", deviceInterface->interfaceName.c_str());
      releaseCapacity();
      deviceInterface->setStateDisabled();
      handleDisconnectService();
    }

    void monitorNetwork(){

      if (!deviceInterface) return;

      if (hwm.network->state == DISCONNECTED){
        deviceInterface->state.progress = hwm.network->networkConnectAttemptProgress;
        deviceInterface->state.subState = hwm.network->SSID;
      }else if (!serviceLocked){
        monitorService();
      }
    }

    virtual void setStateLocked(String led = "orange",String details = *lang.getString("Network busy")){
      DEBUG_PRINT("\n%s::setStateLocked()", deviceInterface->interfaceName.c_str());
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_LOCKED;
      deviceInterface->state.subState = "";
      deviceInterface->state.details = details;
      deviceInterface->state.led = led;
      deviceInterface->state.progress = -1;
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


DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable), SerializableParameters(name,_group,&IFdataRepo, true){   // the interfaces uses its own repository to store parameters to avoid conflict with data sets groups
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
}

DeviceInterface::~DeviceInterface(){
  interfaceRepo.interfaces.erase(std::remove(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), this), interfaceRepo.interfaces.end());
}

DeviceInterface* InterfaceRepository::retrieve(String requestedInterface){
  for (const auto& interface : interfaceRepo.interfaces){ 
    //DEBUG_PRINT("\nInterfaceRepository::retrieve(%s)=>%s",requestedInterface.c_str(),interface->interfaceName.c_str())     ;
    if (interface->interfaceName == requestedInterface) return interface;
  }
  DEBUG_PRINT("\nError: requested interface <%s> is not existing, returning nullptr",requestedInterface.c_str());
  return nullptr;
}

#endif
