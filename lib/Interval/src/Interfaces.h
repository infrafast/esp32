#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

#include "interfaces/i_states.h"

#define ADDRESS_CHANGETOPARAMETERNAME "/config/%s"

class DeviceInterface  : public SerializableDataset{
  #define INTERFACE_STATE       "state"
  #define INTERFACE_PROGRESS    "progress"
  #define INTERFACE_EXPOSED     "exposed"
  #define INTERFACE_OFFLINE     "offline"
  #define INTERFACE_GROUP       "group"
  #define INTERFACE_USENETWORK  "network"  
  #define INTERFACE_PROVIDER    "provider"
  #define INTERFACE_ICON        "icon"
  #define INTERFACE_DETAILS     "details"
  #define INTERFACE_LED         "led"
  
  public:

    State state;

    String previousStateLabel;
    String previousLed = "";
    uint8_t previousStateCode = 0;
    bool previousOffline = false;
    float previousProgress = -1;

    String interfaceName; 

    bool configChanged = false;
    
    int dataAvailable;
    void* data;

    bool exposed = true;

    String group = "undef"; // used by the strip to define which deviceinterface they can connect


    DeviceInterface(const String&, const String&, bool);
    ~DeviceInterface();

        /////////////////////////////////////

    virtual const String& getDataProviderNameForThisInterface(){ return group;}

    String serializeState(){
      JsonDocument doc;
      
      doc[interfaceName][INTERFACE_STATE] = state.label;
      doc[interfaceName][INTERFACE_PROGRESS] = state.progress;
      doc[interfaceName][INTERFACE_DETAILS] = state.details;
      doc[interfaceName][INTERFACE_OFFLINE] = state.offline;
      
      if(state.offline == true){
        //we force the GUI to be black
        doc[interfaceName][INTERFACE_LED] = "black";
      }else{
        //we adopt the state gui
        doc[interfaceName][INTERFACE_LED] = state.led;  
      }
      
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

      config[INTERFACE_OFFLINE] = state.offline;      // common attribute

      if (mainNode == true){
        // that's the attribute we do not serialize in case of partial serialization
        config[INTERFACE_EXPOSED] = exposed;
        config[INTERFACE_GROUP] = group;
        config[INTERFACE_PROVIDER] = getDataProviderNameForThisInterface();      
        config[INTERFACE_ICON] = icon;
      }

      JsonArray propertiesDoc = config[DATASET_PROPERTIES].to<JsonArray>();
      serializeProperties(propertiesDoc);


      doc.shrinkToFit();
      String output;
      serializeJson(doc, output);    
      //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
      //DEBUG_PRINT("\nserializeConfig: %s",output.c_str());          
      return output;
    }  

    bool restoreFromString(const char* config){    
      bool changed = false;
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, config);
      if (error) {
        DEBUG_PRINT("\n%s restoreFromString %s",interfaceName.c_str(),error.c_str());
        return false;
      }
      JsonObject configJson = doc.as<JsonObject>();

      //by default a used interface (which means it had a partial serialization) 
      // is not exposed and is online, this can be overwritten by the deserialization 
      // of a full interface
      //offline = false;
      //exposed = false;

      changed = restoreBase(configJson);    

      doc.shrinkToFit();

      #ifdef DEBUG
        String output;
        serializeJson(doc, output);    
        //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
        DEBUG_PRINT("\nrestoreFromString: %s",output.c_str());          
      #endif
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

    virtual bool update(){      // return a boolean indicatin if the interface has changed its state
      // reset change state
      state.stateChanged = false;
      state.progressChanged = false;

      // reset the flag so strips will not be updated and re-processed already consummed data
      dataAvailable = 0;

      // report if state has changed
      if (state.label != previousStateLabel){
        state.stateChanged  = true;
        previousStateLabel = state.label;
      }
      if (state.code != previousStateCode){
        state.stateChanged  = true;
        previousStateCode = state.code;
      }    
      if (state.offline != previousOffline){
        state.stateChanged  = true;
        previousOffline = state.offline;
      }    
      if (state.progress != previousProgress){
        // a progress change is captured but is not a state change
        // indeed, a state change would trigger redraw which we do not want
        state.progressChanged = true;        
        previousProgress = state.progress;        
      }
      if (state.led != previousLed){
        state.stateChanged  = true;
        previousLed = state.led;
      }

      //do specific interface internal workflow
      if (state.offline != true){      
        monitor();
      }
      
      return state.stateChanged;
    }

    int available(){ // called by strip 
      return dataAvailable; 
    }

    virtual void goOffline(bool _offline){
      state.offline = _offline;
    }

    virtual bool restoreBase(const JsonObject& config){   // == deserialize = from string to variable
      
      configChanged = false;

      // this restore the base parameters common to all base class interface
      if (config.containsKey(INTERFACE_OFFLINE)){
        bool offline = config[INTERFACE_OFFLINE];
        if (offline != state.offline) configChanged = true;
        state.offline = offline;
        DEBUG_PRINT("\n%s offline=>%s",interfaceName.c_str(),state.offline?"true":"false");        
      }

      // and then we call the specific part
      JsonArray properties = config[KEY_PROPERTIES];
      for (uint8_t i = 0; i< properties.size(); i++){
        if (restoreProperty(properties[i])){
          configChanged |= true;
          DEBUG_PRINT("\n%s property %s changed",properties[i]);
        }

      }            

      if (configChanged) handlePropertyChange();
      return configChanged;
    }  

    #define IF_CFG_FILE_PATH "/"

    bool restoreFromFile() {

      // Ouvrir le fichier
      File file = LittleFS.open(IF_CFG_FILE_PATH+getFilename(), "r");
      if (!file) {
        DEBUG_PRINT("\t%s::restoreFromFile : impossible d'ouvrir %s\t",interfaceName.c_str(), getFilename().c_str());
        return false;
      }

      // Lire tout le contenu dans une String
      String content = file.readString();
      file.close();

      if (content.length() == 0) {
        DEBUG_PRINT("\t%s::restoreFromFile %s vide\t",interfaceName.c_str(), getFilename().c_str());
        return false;
      }

      // Appeler la fonction de restauration
      return restoreFromString(content.c_str());
    }

    bool save(){
      File file = LittleFS.open(IF_CFG_FILE_PATH+getFilename(), "w");
      if (!file) {
        DEBUG_PRINT("\t%s::save : impossible d'ouvrir %s en écriture\t", interfaceName.c_str(),getFilename().c_str());
        return false;
      }

      String content = serializeConfig();

      size_t written = file.print(content);
      file.close();

      if (written != content.length()) {
        DEBUG_PRINT("\t%s::save : seulement %d/%d octets écrits dans %s\t", interfaceName.c_str(),written, content.length(), getFilename().c_str());
        return false;
      }    

      return true;

    }

    void* read(){
      return data;
    }

    virtual bool derivedBegin() = 0;

    virtual bool begin(){
      DEBUG_PRINT("\n%s->begin()...",interfaceName.c_str());
      bool success = end();
      success &= restoreFromFile();
      success &= derivedBegin();
      DEBUG_PRINT("\t%s",success?"ok":"failed or is not implemented");    
      return success;
    }

    //by default no properties are serialized
    virtual void serializeProperties(JsonArray& properties){}
    virtual bool restoreProperty(const char* property) = 0;         // == deserialize properties = string to variable
    
    // monitor manage internal state and set the dataAvailable flag using internal functions to indicate data are available to be read with read() method
    // do not need to care about offline state as it is managed by update method
    virtual void monitor() = 0; 

    virtual void handlePropertyChange(){DEBUG_PRINT("\n%s::handlePropertyChange() : void");} // this s called when the interface parameter/config/property is changed, the interface has to decide what to do like restarting
    virtual bool end() = 0;     
    virtual String getFilename() = 0;

    virtual const String* getIcon(){
      return &icon;
    }

  protected:
    String icon = ICON_LINK;

};

class NetworkDevice : public DeviceInterface{

  public:
    
    NetworkDevice(const String& name, const String& _group,bool _configurable = true):DeviceInterface(name, _group, _configurable){
      networkDown();
    }

    virtual void derivedMonitor() = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;    
    
    void networkDown(){
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISCONNECTED;           
      state.led = "red medium";
      state.details = *lang.getString("networkDown");                // connect Eth cable or attempt to connect a previous SSID
    }

    void networkUp(){
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISCONNECTED;
      state.details = *lang.getString("networkUp");      
      state.led = "red";
      state.progress = -1;
    }    
    

    virtual void monitor(){         
      if (hwm.network->refreshed){
        bool success = false;
        if (hwm.network->state == CONNECTED){
          if (state.offline){
            DEBUG_PRINT("\n%s->connect<OFFLINE>() ignored",interfaceName.c_str());
            return;
          }else{
            networkUp();
            success = connect();
            DEBUG_PRINT("\n%s->connect<ONLINE>()...%s",interfaceName.c_str(),success?"ok":"failed or is not implemented");                    
          }
        }else{
          networkDown();    
          success = disconnect();
          DEBUG_PRINT("\n%s->disconnect()...%s",interfaceName.c_str(),success?"ok":"failed or is not implemented");
        }
      }
      // we only monitor networkdevices when network is on
      if (hwm.network->state == CONNECTED) derivedMonitor();
      // otherwise we report on progress and the network we try to connect 
      else{
        state.progress = hwm.network->networkConnectAttemptProgress;
        state.subState = hwm.network->SSID;
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

    SerializableDataset systemDS = SerializableDataset(SYSTEM_INTERFACE, GROUP_INTERFACE_SYSTEM);   // one instance
        
    DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_SYSTEM,_configurable),DataProvider(getDataProviderNameForThisInterface()){
      state.code = CONNECTED;            
    }

    virtual bool restore(const JsonObject& config){return false;}
    virtual bool restoreProperty(const char* property){return false;}

    virtual void derivedConfig(JsonObject& config){};    
    virtual void monitor(){}
    
    virtual void* read(){return nullptr;}
    
    virtual bool derivedBegin(){

      systemDS.registerDataSetAddress(dsText.address);      // register all ds             
      jsonInputRepo.add(&systemDS);     // registered once

      //dataProviderName = getDataProviderNameForThisInterface();      

      return true;
    }
    
    virtual bool end(){
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

DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable), SerializableDataset(name,_group){
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
  previousStateLabel = state.label;  
  previousOffline = state.offline;
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