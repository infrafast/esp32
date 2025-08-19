#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

class DeviceInterface {
  #define INTERFACE_STATE       "state"
  #define INTERFACE_PROGRESS    "progress"
  #define INTERFACE_EXPOSED     "exposed"
  #define INTERFACE_CONFIGZ     "config"
  #define INTERFACE_OFFLINE     "offline"
  #define INTERFACE_GROUP       "group"
  #define INTERFACE_USENETWORK  "network"  
  #define INTERFACE_PROVIDER    "provider"
  
  public:

  State state;

  String previousStateLabel = state.label;
  uint8_t previousStateCode = 0;

  String interfaceName; 

  bool exposed = true;
  bool network = false;  // by default an interface do not use betwork
  String group = "undef"; // used by the strip to define which deviceinterface they can connect

  String icon = ICON_LINK;

  virtual String getProviderName() = 0;

  DeviceInterface(const String&, const String&, bool);
  ~DeviceInterface();

  String serializeState(){
    JsonDocument doc;
    
    doc[interfaceName][INTERFACE_STATE] = state.label;
    doc[interfaceName][INTERFACE_PROGRESS] = state.progress;

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

    if (mainNode == true){
      // that's the attribute we do not serialize in case of partial serialization
      config[INTERFACE_EXPOSED] = exposed;
      config[INTERFACE_USENETWORK] = network;
      config[INTERFACE_OFFLINE] = state.offline;
      config[INTERFACE_GROUP] = group;
      config[INTERFACE_PROVIDER] = getProviderName();      
    }
    derivedConfig(config);

    doc.shrinkToFit();
    String output;
    serializeJson(doc, output);    
    //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
    //DEBUG_PRINT("\nserializeConfig: %s",output.c_str());          
    return output;
  }  

  bool restoreFromString(const char* config){    
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

    restore(configJson);    

    doc.shrinkToFit();

    #ifdef DEBUG
      String output;
      serializeJson(doc, output);    
      //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
      DEBUG_PRINT("\nrestoreFromString: %s",output.c_str());          
    #endif
    return true;      
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

  bool update(){      // return a boolean indicatin if the interface has changed its state
    state.stateChanged = false;
    // reset the flag so strips will not be updated and re-processed already consummed data
    dataAvailable = false;
    // in case interface is offline, then easy: we do not waste time
    // report if state has changed
    if (state.label != previousStateLabel){
      state.stateChanged  = true;
      previousStateLabel = state.label;
    }
    if (state.code != previousStateCode){
      state.stateChanged  = true;
      previousStateCode = state.code;
    }    
    if (state.offline != true){      
      monitor();    //do specific interface internal workflow
    }
    
      return state.stateChanged;
  }

  bool available(){ // called by strip 
    return dataAvailable; 
  }

  void goOffline(){
    state.offline = true;
  }

  void goOnline(){
    state.offline = false;
  }

  virtual void restore(const JsonObject& config) = 0;  
  virtual void derivedConfig(JsonObject& config) = 0;  
  virtual void monitor() = 0; // => manage internal state and set the dataAvailable flag using internal functions to indicate data are available to be read with read() method
  virtual void* read() = 0; // called by strip 
  virtual void begin() = 0;
  virtual void end() = 0;

  protected:

    bool dataAvailable;


};

class NetworkInterface{
  public:
    DeviceInterface* interface;
    // this class override default one to overall encapsulate state management
    NetworkInterface(DeviceInterface* _if): interface(_if){
      interface->network = true;
    }

    void computeNetworkState(){     
      // detect if network is down
      if (hwm.network->refreshed){
        if (hwm.network->state == DISCONNECTED){
          interface->state.code = IF_STATE_NOT_READY;
          interface->state.label = "disconnected";
          interface->state.subLabel = hwm.network->SSID;
          interface->state.userInstructions = "Attempt";
        }
      }
    }

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
};

InterfaceRepository interfaceRepo;


DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable){
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
}

DeviceInterface::~DeviceInterface(){
  interfaceRepo.interfaces.erase(std::remove(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), this), interfaceRepo.interfaces.end());
}

#define MACRO_NUMBER_MAX      10
#define ADDRESS_SIMPLETEXT    "/text/%s"
#define ADDRESS_STRIPIDS      "/strip/id/%s"
#define ADDRESS_ACTION        "/action/%s"
  #define ADDRESS_ACTION_SIZE 5
#define ADDRESS_MACRONAME     "/macro/name/%s"
#define ADDRESS_PROVIDER      "/provider/"

#define SYSTEM_INTERFACE "system"
class SystemInterface: public DeviceInterface, public DataProvider{
  #define GROUP_INTERFACE_SYSTEM "utils"
  public:    

    DynamicDataSet simpleText = DynamicDataSet({ ADDRESS_SIMPLETEXT, INPUTTEXT },this);
    DynamicDataSet stripIDs = DynamicDataSet({ADDRESS_STRIPIDS,INPUTSELECT},this);
    DynamicDataSet dropDownActions = DynamicDataSet({ADDRESS_ACTION,INPUTSELECT},this);        
    DynamicDataSet macroNames = DynamicDataSet({ADDRESS_MACRONAME,INPUTSELECT},this); 


    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_SYSTEM,_configurable){
      state.code = CONNECTED;
    }

    //SystemInterface uses himself as data provider
    virtual String getProviderName(){
      return interfaceName;
    }

    virtual String getDataSetName(){
      return interfaceName;
    }

    virtual void restore(const JsonObject& config){};  
    virtual void derivedConfig(JsonObject& config){};    
    virtual void monitor(){}
    virtual void* read(){return nullptr;}
    virtual void begin(){}
    virtual void end(){}     

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_SIMPLETEXT) return 1;
      
      else if (addrr == ADDRESS_STRIPIDS) return 0;
      else if (addrr == ADDRESS_ACTION) return ADDRESS_ACTION_SIZE;
      else if (addrr == ADDRESS_MACRONAME) return MACRO_NUMBER_MAX;
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

      DEBUG_PRINT("\nDataSet getSize(%s) is not implemented",addrr.c_str());
      return 0;

    }
    

    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_SIMPLETEXT){
        textValue = "hello"+String(index);
        return &textValue;
      }
      else if (addrr == ADDRESS_STRIPIDS){
        return &getStripID;         //javascript local function callback
      }else if (addrr == ADDRESS_ACTION){
        return &dropDownActionsValues[index];   
      }else if (addrr == ADDRESS_MACRONAME){
        return &macroNamesValues[index];      
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
      
      DEBUG_PRINT("\nDataSet getValuePtr(%s,%d) is not implemented",addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";
    
    // javascript function that generates the data locally
    String getStripID = "getStripID";
        
    String dropDownActionsValues[ADDRESS_ACTION_SIZE] = {
      "play",
      "pause",
      "stop",
      "FF",
      "RW"      
    };   
    
    String macroNamesValues[MACRO_NUMBER_MAX] = {
      "macro1",
      "macro2",
      "macro3",      
      "macro4",
      "macro5",
      "macro6",
      "macro7",
      "macro8",
      "macro9",
      "macro10"
    };            
  
}systemInterface(false);

DeviceInterface* InterfaceRepository::retrieve(String requestedInterface){
  for (const auto& interface : interfaceRepo.interfaces){ 
    //DEBUG_PRINT("\nInterfaceRepository::retrieve(%s)=>%s",requestedInterface.c_str(),interface->interfaceName.c_str())     ;
    if (interface->interfaceName == requestedInterface) return interface;
  }
  DEBUG_PRINT("\nError: requested interface <%s> is not existing, returning <%s>",requestedInterface.c_str(),systemInterface.interfaceName.c_str() );
  return &systemInterface;
}


#endif