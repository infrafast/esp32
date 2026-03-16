#ifndef _MSTATION_h
#define _MSTATION_h

#include "interfaces/i_socket.h"
#include "mixer.h"

#define MS_APP_STATE "/app/state"
#define MS_MIXER_IDENTITY "/app/mixers/current"                   // mixer name, model, address IP
#define MS_MIXER_DEFINITION "/console/information"                // mixer attributes like  number of channel...
#define MS_SUBSCRIBE "/console/data/subscribe"
#define MS_MIXERS_AVAILABLE "/app/mixers/available"
#define MS_MIXER_GET_ANY_DATA "/console/data/get/*"

#define MS_MIXER_PROFILE_FILENAME "/mixerDef.ms"

/* data types
  0: Channel
  1: Aux
  2:  => not behringer / not used
  3: FX
  4: Bus
  5: => not behringer / not used
  6: Main L/R
  7: Mtx (Matrix)
  8: DCA
*/

struct MSmixerDefinition : public BaseMixerDefinition{
  uint8_t channelOffset = 0;
  uint8_t busOffset = 0 ;
  uint8_t auxOffset = 0;
  uint8_t fxOffset = 0; 
};

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation:  public MixerDevice<NetworkSocket>{
  public:
  
    MSmixerDefinition mixerDef;

    MixingStation():MixerDevice<NetworkSocket>(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,MS_MIXER_IDENTITY,8887,8088,true){
      //destination.host = "192.168.99.99";
      mixerID.name = "MSStation";
      
      // protocol is hard-coded
      mixerProtocol.anyChannelName = "ch.*.cfg.name";

    }

    virtual bool ping() override{
      hwm.network->clientSocketSend(get(destination.echoCommand.c_str()));  // forge a MS get json query with echoCommand as the path
      return true;
    }

    virtual void onSocketConnected() override{
      hwm.network->clientSocketSend(get(MS_APP_STATE));  // forge a MS get json query with echoCommand as the path
      //hwm.network->clientSocketSend(post("/console/data/subscribe",bodyPathFormat("ch.*.mix.lvl","val")));  // forge a MS get json query with echoCommand as the path
      //requestMixerInfo();
    }
 
    virtual bool restoreMixerProfile(){
      DEBUG_INFO("%s::restoreMixerProfile() => only deserializing definition, protocol is unique and hard wired",interfaceName.c_str());       
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return deserializeMixerDefinitionFromFile(MS_MIXER_PROFILE_FILENAME);
    }    

    virtual bool begin() override{
      return restoreMixerProfile();              // setting up the last connected mixer
    }
    
    bool saveMixerDefinition(){
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return saveStruct(MS_MIXER_PROFILE_FILENAME, mixerDef);
    }    

    virtual bool deserializeMixerDefinitionFromFile(const char* filename) override{
      return loadStruct(filename, mixerDef);
    }    
 
  protected: 
    ////////////////////////////////////////
    // JSON and ROUTE HANDLING DEFINITION
    ///////////////////////////////////////
    static constexpr size_t RX_MAX = kBufferMaxSize + 1;
    char rxBuf[RX_MAX];

    JsonDocument docHeader;
    JsonDocument docFull;

    enum class RouteMode : uint8_t { JsonBody, RxMessage };

    using JsonHandler = void (MixingStation::*)(const char* path,JsonVariant body);
    using RxHandler   = void (MixingStation::*)(char* rx, size_t len);

    struct Route {
      const char* path;
      RouteMode mode;
      union { JsonHandler jh; RxHandler rh; } fn;
      bool propagate;
    };

    static const Route routes[];

    #define ROUTES_COUNT 3

    bool routeMatches(const char* routePath, const char* testedPath) const {
      if (!routePath || !testedPath) return false;

      size_t routeLen = strlen(routePath);

      // support wildcard final: "/console/data/get/*"
      if (routeLen > 0 && routePath[routeLen - 1] == '*') {
        size_t prefixLen = routeLen - 1; // ignore '*'
        return strncmp(testedPath, routePath, prefixLen) == 0;
      }

      return strcmp(routePath, testedPath) == 0;
    }

    const Route* findRoute(const char* path) const {
      for (size_t i = 0; i < ROUTES_COUNT; ++i) {
        if (routeMatches(routes[i].path, path)) {
          return &routes[i];
        }
      }
      return nullptr;
    }
  
    ////////////////////////////////////////
    // Main processor : JSON PATH ROUTER
    ///////////////////////////////////////    
    virtual void route(const char *message,uint16_t length) override{

      //DEBUG_INFO("%s::process(%s,%i)",interfaceName.c_str(),message,length);
      destination.lastQueryTime = millis(); 
    
      if (length + 1 > RX_MAX){
        DEBUG_ERROR("%s::process(...,%i) exceed buffer size:%i",interfaceName.c_str(),length,RX_MAX);
      } 

      JsonDocument filter;
      filter["path"]  = true;    
      filter["error"] = true;   

      docHeader.clear();
      DeserializationError e = deserializeJson(
        docHeader,
        message,
        length,
        DeserializationOption::Filter(filter)
      );
      if (e){
        DEBUG_ERROR("JSON error: %s", e.c_str());        
        return;
      }

      if (!docHeader["error"].isNull()) {
        const char* error = docHeader["error"];
        DEBUG_ERROR("error:%s",error);        
        return;
      }

      const char* path = docHeader["path"] | "";
      if (!path[0]){
        DEBUG_WARN("\tNo Path");
        return;
      }else{
        DEBUG_INFO("%s::path ✅ %s",interfaceName.c_str(),path);
      }

      const Route* r = findRoute(path);
      if (!r) return;

      memcpy(rxBuf, message, length);
      rxBuf[length] = '\0';

      if (r->mode == RouteMode::RxMessage) {
        (this->*(r->fn.rh))(rxBuf, (size_t)length);
      }else{
        docFull.clear();
        if (deserializeJson(docFull, rxBuf, length)) return;
        JsonVariant body = docFull["body"];
        (this->*(r->fn.jh))(path,body);
      }

      if (r->propagate) fireEvent(evtSock);                 // filter event propagation based on their flag
    }  

    // ------------------ Path Handlers ------------------

    void processState(const char* path, JsonVariant body) {
      #define MS_IDLE         "idle"
      #define MS_CONNECTING   "connecting"
      #define MS_CONNECTED    "connected"
      #define MS_RECONNECTING "reconnecting"
      #define MS_SYNCING      "synchronizing"

      const char* msg      = body["msg"]      | "";
      int progress         = body["progress"] | 0;
      const char* state    = body["state"]    | "";
      const char* topState = body["topState"] | "";
      DEBUG_INFO("%s::processState: msg:%s,progress:%i,state:%s,topState:%s",interfaceName.c_str(),msg,progress,state,topState);

      if (strcmp(topState, MS_IDLE) == 0) {
        if (deviceInterface->state.label = MS_IDLE){
          setStatePending();
          deviceInterface->state.label = MS_IDLE;               // do not invert details/label as label=IDLE forces state refresh for the GUI compared to label=PENDING which is also set by device scan
          deviceInterface->state.details = IF_STATE_PENDING;
        }
      }
      else if (strcmp(topState, MS_CONNECTING) == 0) {
        if (strcmp(state, MS_SYNCING) == 0){
          if (deviceInterface->state.subState != IF_STATE_SYNCING){
            setStateSyncing(MS_SYNCING);            
          }
        }else{
          if (deviceInterface->state.subState != String(state)){
            setStateConnecting(state,msg);
          }
        }
        deviceInterface->state.progress = progress;         
      }
      else if (strcmp(topState, MS_CONNECTED) == 0) {  
        if (!dataSync) getMixerDefinition();
      }
      else if (strcmp(topState, MS_RECONNECTING) == 0) {
        setStatePending("green medium",concatStrings(state).c_str());  
        deviceInterface->state.label = topState;
        deviceInterface->state.subState = state;      
      }
      else{
        // we do not alter main state code, only report what we received
        setStatePending("green medium",concatStrings("UNHANDLED MS STATE:",state).c_str());
        deviceInterface->state.label = topState;
        deviceInterface->state.subState = state;        
      }

    }

    void getMixerDefinition(){      
      hwm.network->clientSocketSend(get(MS_MIXER_DEFINITION)); // retrieve name, etc...            
    }

    void processMixerDefinition(char* rx, size_t len) {
      JsonDocument filter;
      filter["body"]["type"]     = true;
      filter["body"]["count"]     = true;
      filter["body"]["offset"]  = true;

      JsonDocument d;

      DeserializationError e;
      e = deserializeJson(d, rx, len, DeserializationOption::Filter(filter));
      if (e){
        DEBUG_ERROR("JSON error: %s", e.c_str());        
        return;
      }

      //feed the mixerDef
      JsonArray channelTypes = d["channelTypes"].as<JsonArray>();
      if (channelTypes.isNull()) {
        DEBUG_ERROR("JSON error: channelTypes missing");
        return;
      }

      // Valeurs par défaut si certaines sections n'existent pas
      mixerDef.channelCount = 0;
      mixerDef.busCount     = 0;
      mixerDef.auxCount     = 0;
      mixerDef.fxCount      = 0;
      mixerDef.channelOffset = 0;
      mixerDef.busOffset     = 0;
      mixerDef.auxOffset    = 0;
      mixerDef.fxOffset      = 0;

      for (JsonObject ct : channelTypes) {
        uint8_t type  = ct["type"]  | 255;
        uint8_t count = ct["count"] | 0;
        uint8_t offset = ct["offset"] | 0;

        switch (type) {
          case 0: // Channel / Input
            mixerDef.channelCount = count;
            mixerDef.channelOffset = offset;
            break;

          case 1: // Aux
            mixerDef.auxCount = count;
            mixerDef.auxOffset = offset;
            break;

          case 3: // FX Return
            mixerDef.fxCount = count;
            mixerDef.fxOffset = offset;
            break;

          case 4: // Bus
            mixerDef.busCount = count;
            mixerDef.busOffset = offset;
            break;

          // Types présents mais non utilisés ici
          case 2: // not used
          case 5: // not used
          case 6: // Main L/R
          case 7: // Matrix
          case 8: // DCA
          default:
            break;
        }
      }

      // now serialize this onfo into the MS_MIXER_PROFILE_FILENAME in binary form
      saveMixerDefinition();

      // now get the mixer identity
      hwm.network->clientSocketSend(get(MS_MIXER_IDENTITY)); // retrieve name, etc...            

    }


    void processMixerIdentity(const char* path, JsonVariant body) {
      // got the connected mixer informartion
      #define OFFLINE "offline"
      IPAddress ip;
      ip.fromString(body["ipAddress"] | "");
      String name   = body["name"]    | OFFLINE;
      String model  = body["currentModel"] | "";

      if (name != OFFLINE){
        
        if (isNewMixer(name,model,ip)){
          // nothing special to be done by mixing station upon connexion of a new mixer
        }
        
      }else{

        hwm.network->clientSocketSend(get(MS_APP_STATE));   // force getting the state as a backup in case mixerQuery didn't pass thru

      }

    }

    virtual void startMixerDataSync() override{
      //dataSync = true;
      hwm.network->clientSocketSend(post(MS_SUBSCRIBE,bodyPathFormat(mixerProtocol.anyChannelName.c_str(),"val")));  // forge a MS get json query with echoCommand as the path
      //setStateMixerLive();
      setStatePendingDataSync();
    }

    void processMixersAvailable(char* rx, size_t len) {
      JsonDocument filter;
      filter["body"]["consoles"][0]["consoleId"]     = true;
      filter["body"]["consoles"][0]["models"][0]     = true;
      filter["body"]["consoles"][0]["manufacturer"]  = true;
      filter["body"]["consoles"][0]["name"]          = true;

      JsonDocument d;

      DeserializationError e;
      e = deserializeJson(d, rx, len, DeserializationOption::Filter(filter));
      if (e){
        DEBUG_ERROR("JSON error: %s", e.c_str());        
        return;
      }

      JsonArray consoles = d["body"]["consoles"].as<JsonArray>();
      for (JsonObject c : consoles) {
        int id = c["consoleId"] | -1;
        const char* name = c["name"] | "";
        const char* mfg  = c["manufacturer"] | "";
        JsonArray models = c["models"].as<JsonArray>();        

        for (const char* m : models) {
          DEBUG_INFO("model=%s name=%s manufacturer=%s id=%d", m, name, mfg, id);
        }
      }
    }

    void processDataGet(const char* path, JsonVariant body) {
      // traitement de /console/data/get/*
      float value          =  body["value"]   | 0.0;
      DEBUG_INFO("%s::processDataGet: %s = %.2f",interfaceName.c_str(),path,value);
    }

    //////////////////////////////////////////////////////////////
    ///                      DEVICE STATES                    ///
    /////////////////////////////////////////////////////////////

    void setStateDisconnected() override{
      DeviceInterface::setStateDisconnected(lang.getString("MixingStation unreachable")->c_str());      
    }

    //////////////////////////////////////////////
    // HELPER TO BUILD AND RETURN A SOCKET REQUEST
    //////////////////////////////////////////////  
    const char* build(const char* path, const char* method, const char* body) {
      static char buffer[150];

      if (body == nullptr){
        snprintf(buffer, sizeof(buffer),
          "{"
          "\"path\":\"%s\","
          "\"method\":\"%s\","
          "\"body\":null"
          "}",
          path, method);

      }else{
        snprintf(buffer, sizeof(buffer),
          "{"
          "\"path\":\"%s\","
          "\"method\":\"%s\","
          "\"body\":%s"
          "}",
          path, method,body);

      }
      return buffer;
    }

    const char* get(const char* path, const char* body=nullptr) {
      return build(path, "GET",body);
    }

    const char* post(const char* path,const char* body=nullptr) {
      return build(path, "POST",body);
    }    

    const char* bodyPathFormat(const char* path, const char* format) {
        static char buffer[160];

        snprintf(buffer, sizeof(buffer),
                "{"
                "\"path\":\"%s\","
                "\"format\":\"%s\""
                "}",
                path, format);

        return buffer;
    }    
  
};

const MixingStation::Route MixingStation::routes[] = {
  { MS_APP_STATE,            RouteMode::JsonBody,   { .jh = &MixingStation::processState },           false },
  { MS_MIXER_IDENTITY,       RouteMode::JsonBody,   { .jh = &MixingStation::processMixerIdentity },   false },  
  { MS_MIXER_DEFINITION,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixerDefinition }, false },  
  { MS_MIXERS_AVAILABLE,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixersAvailable }, false },
  { MS_MIXER_GET_ANY_DATA,   RouteMode::JsonBody,   { .jh = &MixingStation::processDataGet },         true },
};

#endif