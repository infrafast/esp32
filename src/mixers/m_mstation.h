#ifndef _MSTATION_h
#define _MSTATION_h

#include "interfaces/i_socket.h"
#include "mixer.h"

#define MS_APP_STATE "/app/state"

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation: public NetworkSocket, public MixerInterface{
  public:
  
    MixingStation():NetworkSocket(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,MS_APP_STATE,8887,8088,true){
      //destination.host = "192.168.99.99";
    }

    virtual void process() override{

    }
 
    virtual bool ping() override{
      hwm.network->clientSocketSend(get(destination.echoCommand.c_str()));  // forge a MS get json query with echoCommand as the path
      return true;
    }
 
  protected:
    //////////////////////////////////////////////
    // HELPER TO BUILD AND RETURN A SOCKET REQUEST
    //  {
    //    "path": <path>,
    //    "method": <GET/POST>,
    //    "body": null
    //  }
    //////////////////////////////////////////////  
    const char* build(const char* path, const char* method) {
        static char buffer[150];

        snprintf(buffer, sizeof(buffer),
                 "{"
                 "\"path\":\"%s\","
                 "\"method\":\"%s\","
                 "\"body\":null"
                 "}",
                 path, method);

        return buffer;
    }

    const char* get(const char* path) {
        return build(path, "GET");
    }

    const char* post(const char* path) {
        return build(path, "POST");
    }

  
    ////////////////////////////////////////
    // JSON and ROUTE HANDLING DEFINITION
    ///////////////////////////////////////
    // RX_MAX aligné sur le buffer max de la lib (+1 pour terminator) utilisé pour le parsing du json
    static constexpr size_t RX_MAX = kBufferMaxSize + 1;
    char rxBuf[RX_MAX];

    // doc "header" (petit) + doc "full" (réutilisé)
    JsonDocument docHeader;
    JsonDocument docFull;

    enum class RouteMode : uint8_t { JsonBody, RxMessage };

    using JsonHandler = void (MixingStation::*)(JsonVariant body);      // pour les GROS objet json qui doivent être filtrés
    using RxHandler   = void (MixingStation::*)(char* rx, size_t len);  // pour les petits objet json du style /app/state

    struct Route {
      const char* path;
      RouteMode mode;
      union { JsonHandler jh; RxHandler rh; } fn;
    };

    static const Route routes[];

    #define ROUTES_COUNT 2
    const Route* findRoute(const char* path) const {
      for (size_t i = 0; i < ROUTES_COUNT; ++i)
        if (strcmp(path, routes[i].path) == 0) return &routes[i];
      return nullptr;
    }
  
    ////////////////////////////////////////
    // JSON PATH ROUTER
    ///////////////////////////////////////    
    void onTXTmessage(const char *message, uint16_t length) {
      // we received a message so we reset the lastQuery timer
      destination.lastQueryTime = millis(); 
    
      if (length + 1 > RX_MAX){
        DEBUG_INFO("%s::onTXTmessage(...,%i) exceed buffer size:%i",interfaceName.c_str(),length,RX_MAX);
      } 

      // -------- Pass 1 : parse HEADER ONLY (sans memcpy) --------
      /* at that time we are only interested in path or error
        {
          "path": "/app/state",
          "method": "GET",
          "body": {},
          "error": null
        }           
      */      
      // Filtre minimal : path + error
      JsonDocument filter;
      filter["path"]  = true;    
      filter["error"] = true;   
      // most of the message contains a small payload that's why we do it in two pass.
      docHeader.clear();
      DeserializationError e = deserializeJson(
        docHeader,
        message,
        length,
        DeserializationOption::Filter(filter)
      );
      if (e){
        DEBUG_ERROR("%s:onTXTmessage(): JSON error: %s", e.c_str());        
        return;
      }

      const char* error = docHeader["error"] | nullptr;
      if (error && *error) {
        // processError(...) si tu veux
        DEBUG_ERROR("%s::onTXTmessage() error:%s",interfaceName.c_str(),error);        
        return;
      }

      const char* path = docHeader["path"] | "";
      if (!path[0]) return;

      const Route* r = findRoute(path);
      if (!r) return;

      // -------- Pass 2 : parse Full doc. and focus on body by avoid parsing by copy: we do the copy ourself
      // -------- Phase "on s'engage" : on copie UNE FOIS si nécessaire --------
      memcpy(rxBuf, message, length);
      rxBuf[length] = '\0';

      if (r->mode == RouteMode::RxMessage) {
        // Le handler décidera s'il reparses avec filtre / zero-copy etc.
        (this->*(r->fn.rh))(rxBuf, (size_t)length);
        return;
      }

      // RouteMode::JsonBody : on veut body en JsonVariant
      // Parse complet en ZERO-COPY (buffer modifiable)
      docFull.clear();
      if (deserializeJson(docFull, rxBuf, length)) return;

      JsonVariant body = docFull["body"];
      (this->*(r->fn.jh))(body);
    }  

    // ------------------ Path Handlers ------------------

    void processState(JsonVariant body) {
      // see https://mixingstation.app/ms-docs/use-cases/app-states/
      #define MS_IDLE         "IDLE"
      #define MS_CONNECTING   "CONNECTING"
      #define MS_CONNECTED    "CONNECTED"
      #define MS_RECONNECTING "RECONNECTING"
      #define MS_SYNCING      "SYNCHRONIZING"

      const char* msg      = body["msg"]      | "";
      int progress         = body["progress"] | 0;
      const char* state    = body["state"]    | "";
      const char* topState = body["topState"] | "";
      DEBUG_INFO("%s::processState: msg:%s,progress:%i,state:%s,topState:%s",interfaceName.c_str(),msg,progress,state,topState);

      if (strcmp(topState, MS_IDLE) == 0) {
        // état IDLE
        setStatePending("green medium", *lang.getString("Idle"));
      }
      else if (strcmp(topState, MS_CONNECTING) == 0) {
        // état CONNECTING
        if (strcmp(state, MS_SYNCING) == 0){         // check substate
          if (deviceInterface->state.subState != IF_STATE_SYNCING){
            setStateSyncing("Syncing software");
          }
        }else{
          if (deviceInterface->state.subState != String(state)){
            setStateConnecting(state,msg);
          }
        }
        deviceInterface->state.progress = scaleValue(progress,0,100,0.0,1.0);         
      }
      else if (strcmp(topState, MS_CONNECTED) == 0) {
        // état CONNECTED
      }
      else if (strcmp(topState, MS_RECONNECTING) == 0) {
        // état RECONNECTING
      }
      else{
        setStateDisconnected();       // unknow state
        deviceInterface->state.label = topState;
        deviceInterface->state.subState = state;
        deviceInterface->state.details = concatStrings("UNHANDLED MS STATE:",deviceInterface->state.subState);
      }

    }

    void processMixersAvailable(char* rx, size_t len) {
      // Exemple : parse filtré pour réduire RAM/CPU
      JsonDocument filter;
      filter["body"]["consoles"][0]["consoleId"]     = true;
      filter["body"]["consoles"][0]["models"][0]     = true; // notation pour un array complet
      filter["body"]["consoles"][0]["manufacturer"]  = true;
      filter["body"]["consoles"][0]["name"]          = true;

      JsonDocument d;
      if (deserializeJson(d, rx, len, DeserializationOption::Filter(filter))) return;

      JsonArray consoles = d["body"]["consoles"].as<JsonArray>();
      for (JsonObject c : consoles) {
        int id = c["consoleId"] | -1;
        const char* name = c["name"] | "";
        const char* mfg  = c["manufacturer"] | "";
        JsonArray models = c["models"].as<JsonArray>();        

        (void)id; (void)name; (void)mfg;

        for (const char* m : models) {
            DEBUG_INFO("model=%d name=%s manufacturer=%s canSearch=%d",id, name, mfg, m);
        }        

      }
    }    

    //////////////////////////////////////////////////////////////
    ///                      DEVICE STATES                    ///
    /////////////////////////////////////////////////////////////
    void setStateConnected(){
      DEBUG_INFO("%s: setStateMSconnected()",interfaceName.c_str());
      DeviceInterface::setStatePending();
      state.details = "Connected to MixingStation";
      state.progress = -1;     
    }  
  
};

const MixingStation::Route MixingStation::routes[] = {
  { "/app/state",            RouteMode::JsonBody,  { .jh = &MixingStation::processState } },
  { "/app/mixers/available", RouteMode::RxMessage, { .rh = &MixingStation::processMixersAvailable } },
};


#endif