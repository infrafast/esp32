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

    //Event evt;  // in EventDispatcher

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

    virtual InterfaceQuality quality(){
      return GOOD; 
    }      

    virtual const String& getProviderNameForData(){ return group;}

    String serializeState(){
      JsonDocument doc;
      
      doc[interfaceName][INTERFACE_STATE] = state.label;
      doc[interfaceName][INTERFACE_PROGRESS] = state.progress;
      doc[interfaceName][INTERFACE_DETAILS] = state.details;
      doc[interfaceName][INTERFACE_ENABLED] = state.enabled;
      doc[interfaceName][INTERFACE_ACTIVABLE] = state.activable;
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

    bool init(EventCallback cb){
      DEBUG_PRINT("\n\n----------------------------------  %s->init(...)  ---------------------------------- ",interfaceName.c_str());      
      registerCallback(cb);
      load();
      bool success = begin();
      DEBUG_PRINT("\n-----------------------------------------------------------------------------------------------------\n");      
      return success;
    }

    virtual void userRequestedRefreshInterface(){
      DEBUG_PRINT("\n%s->userRequestedRefreshInterface() : void",interfaceName.c_str());           
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
        if (isActivable()){
          // check if it is allowed to enable this interface
          // for instance the interface belongs to  a pool of resources, it may check if there is enough (ex. memory, network, ....) and return a no go
          // the most obvious case is network interface in Ethernet that can not instanciante more than 8 sockes, hence we are limited to 3 possible interface
          enable(enabled);
        }else{
          enable(false);
        }
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
      if (state.activable != state.previousActivable){
        state.changed  = true;
        state.previousActivable = state.activable;
      }      

      if (state.changed) state.broadcastState = true; // inform application manager that the state has changed. See why we managed both separately => state.progress

      //call the specific interface internal workflow
      if (state.enabled){      
        monitor();
      }

      isActivable();      // check if interface can be activated or not , independantly from its state, an interface can be activated or disativated because change state of others      
   
    }

    virtual bool isActivable(){
      return state.activable;
    }

    /*virtual void switchDevice(bool _enabled){
      enable(_enabled);
      DEBUG_PRINT("\n%s switchDevice=>%s",interfaceName.c_str(),state.enabled?"true":"false");      
    }*/

    virtual void enable(bool _enable){
      state.enabled = _enable;      
      DEBUG_PRINT("\n%s::enable(%s)",interfaceName.c_str(),state.enabled?"true":"false");      
      if (_enable) setStateDisconnected();
      else setStateDisabled();
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

    virtual void setStateConnected(String led = "green", String label = *lang.getString(IF_STATE_CONNECTED)){            
      DEBUG_PRINT("\n%s::setStateConnected()",interfaceName.c_str());
      state.code = IF_READY;      
      state.label = label;
      state.subState = "";      
      state.details = "";
      state.led = led;
      state.progress = -1;      
    }

    virtual void setStatePending(String led = "green medium", String details = *lang.getString("Trying to connect")){
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

class NetWorkInterface{
  public:
    PingEcho* destination;
    
    virtual void* read(){
      return nullptr;
    }
    virtual void connectedNetworkMonitor(){}
    virtual bool connectDeviceToNetwork(){return false;}
    virtual bool disconnectDeviceFromNetwork(){return false;}

   
    virtual void setStateDisconnected(){    // to be overrided by derived class
      if (!deviceInterface) return;
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.details = concatStrings(*lang.getString("disconnected"));
      deviceInterface->state.led = "red";
      deviceInterface->state.progress = -1;
    }  

    void attachDeviceInterface(DeviceInterface* _devif){
      deviceInterface = _devif;
    }

  protected:
    DeviceInterface* deviceInterface = nullptr;

    void setNetworkStateDown(){
      if (!deviceInterface) return;
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;           
      deviceInterface->state.led = "red medium";
      deviceInterface->state.details = *lang.getString("setNetworkStateDown");                // connect Eth cable or attempt to connect a previous SSID
    }
    
    void monitorNetworkInterface(){         
    
      if (!deviceInterface) return;
     
      if (hwm.network->refreshed){
        if (hwm.network->state == CONNECTED){
          if (deviceInterface->state.enabled){
            DEBUG_PRINT("\n%s->monitor connectDeviceToNetwork()",deviceInterface->interfaceName.c_str());
            if (!connectDeviceToNetwork()) setStateDisconnected();            
            return;
          }else{
            DEBUG_PRINT("\n%s->monitor<DISABLED>() ignored",deviceInterface->interfaceName.c_str());
            return;
          }
        }else{
          setNetworkStateDown();    
          DEBUG_PRINT("\n%s->disconnectDeviceFromNetwork()",deviceInterface->interfaceName.c_str());
          disconnectDeviceFromNetwork();
          return;
        }
      }
      // we only monitor networkdevices when network is on
      if (hwm.network->state == CONNECTED) connectedNetworkMonitor();
      else{
        // otherwise we report on progress and the network we try to connect         
        deviceInterface->state.progress = hwm.network->networkConnectAttemptProgress;
        deviceInterface->state.subState = hwm.network->SSID;
      }
    }

    /*virtual const String* getIcon(){
      if (hwm.network->state == CONNECTED) return &icon;
      else return &GUI::iconMixEthWiFi;
    } */   
};

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

#define MACRO_NUMBER_MAX      10
#define ADDRESS_TEXT    "/text/%s"
#define ADDRESS_COMMAND    "/command/%s"
#define ADDRESS_STIPID      "/strip/id/%s"
#define ADDRESS_ACTION        "/action/%s"
  #define ADDRESS_ACTION_SIZE 5
#define ADDRESS_MACRONAME     "/macro/name/%s"
#define ADDRESS_PROVIDER      "/provider/"

#define SYSTEM_INTERFACE "system"
class SystemInterface: public DeviceInterface, public DataProvider{
  #define GROUP_INTERFACE_SYSTEM "utils"
  public:    

    SerializableParameters systemDS = SerializableParameters(SYSTEM_INTERFACE, GROUP_INTERFACE_SYSTEM, &dataRepo);   // one instance
        
    DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_SYSTEM,_configurable),DataProvider(getProviderNameForData()){
      state.code = CONNECTED;     
      state.enabled = true;       
    }

    virtual bool restore(const JsonObject& config){return false;}

    virtual void derivedConfig(JsonObject& config){};    
    virtual void monitor(){}
    
    virtual void* read(){return nullptr;}
    
    virtual bool begin(){

      systemDS.registerDataSetAddress(dsText.address);      // register all ds             
      jsonInputRepo.add(&systemDS);     // registered once

      //dataProviderName = getProviderNameForData();      

      return true;
    }
    
    virtual bool end(){
      DEBUG_PRINT("\n%s::end() void",interfaceName.c_str());
      return true;
    }     

    String getFilename(){
      return "system_if.cfg";
    }

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_TEXT) return 1;
      else{
        // IS IT AN INTERFACE ?
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX            
          uint16_t count = 0;
          for (const auto& interface : interfaceRepo.interfaces){
            // check if we request provider listing
            if (interface->group == group) count++;
          }
          //DEBUG_PRINT("\nDataSet getSize(%s) has %d items",addrr.c_str(),count);
          return count;
        }

      }

      
      DEBUG_PRINT("\n<%s>DataSet getSize(%s) is not implemented",dataProviderName.c_str(),addrr.c_str());
      return 0;

    }    
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_TEXT){
        textValue = concatStrings("hello", index);
        return &textValue;
      }else{
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX    
          int16_t count = -1;        
          for (const auto& interface : interfaceRepo.interfaces){
            if (interface->group == group) count++;
            if (count == index){
              String* s = &interface->interfaceName;
              //DEBUG_PRINT("\nDataSet getValuePtr(%s,%d) = %s",addrr.c_str(),index,s->c_str());
              return s;
            } 
          }
        }      
      }     

      DEBUG_PRINT("\n<%s>DataSet getValuePtr(%s,%d) is not implemented",dataProviderName.c_str(),addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";    
        
    
}systemInterface(false);

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

// a NetworkOSCInterface is an interface that connect and IP 
class NetworkOSCInterface: public NetWorkInterface{
  public:
};


#endif
