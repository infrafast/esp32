#ifndef _MSTATION_h
#define _MSTATION_h

#include "interfaces/i_socket.h"
#include "mixer.h"


// that the protocol
#define PATH_APP_STATE "/app/state"
#define PATH_MIXER_IDENTITY "/app/mixers/current"                   // mixer name, model, address IP
#define PATH_MIXER_DEFINITION "/console/information"                // mixer attributes like  number of channel...
#define PATH_SUBSCRIBE "/console/data/subscribe"
#define PATH_MIXERS_AVAILABLE "/app/mixers/available"
#define PATH_MIXER_GET_DATA "/console/data/get"

#define MSDATA_CH_CONFIG_NAME  "ch.%d.cfg.name"

#define MS_MIXER_PROFILE_FILENAME "/mixerDef.ms"

// data types
enum ChannelType {
  MS_TYPE_INPUT = 0,
  MS_TYPE_AUX = 1,
  MS_TYPE_FXSEND = 2,
  MS_TYPE_FXRETURN = 3,
  MS_TYPE_MIXBUS = 4,
  MS_TYPE_GROUP = 5,
  MS_TYPE_MAINLR = 6,
  MS_TYPE_MTX = 7,
  MS_TYPE_DCA = 8,
  MS_TYPE_TALKBACK = 9,
  MS_TYPE_MAX,
  MS_TYPE_UNDEF
//  MS_TYPE_MASTER = 100,
//  MS_TYPE_POP_GROUP = 101
};


const char* MStypeToString(uint8_t type) {
    static const char* const types[MS_TYPE_MAX] PROGMEM = {
        "INPUT",
        "AUX",
        "FXSEND",
        "FXRETURN",
        "MIXBUS",
        "GROUP",
        "MAINLR",
        "MATRIX",
        "DCA",
        "TALKBACK"
    };

    if (type < MS_TYPE_MAX) {
        return types[type];
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "UNDEFTYPE:%u", type);
    return buffer;
}


struct MSmixerDefinition : public BaseMixerDefinition{
  char model[64] = {};
  uint8_t channelOffset = 0;
  uint8_t busOffset = 0 ;
  uint8_t auxOffset = 0;
  uint8_t fxOffset = 0; 
  uint8_t mtxOffset = 0;
  uint8_t mainOffset = 0;
  uint8_t DCAOffset = 0;
  uint8_t fxSndOffset = 0;
  uint8_t groupOffset = 0;
  uint8_t talkbackOffset = 0;

};

#ifdef DEBUG
char* dumpMixerDef(const MSmixerDefinition &def, char* buffer, size_t bufferSize) {

  snprintf(buffer, bufferSize,
      "channelCount=%u busCount=%u auxCount=%u fxCount=%u mtxCount=%u mainCount=%u DCAcount=%u fxSndCount=%u groupCount=%u talkbackCount=%u"
      "channelOffset=%u busOffset=%u auxOffset=%u fxOffset=%u mtxOffset=%u mainOffset=%u DCAoffset=%u fxSndOffset=%u groupOffset:%u talkbackOffset=%u"
      "model=%s",
      def.channelCount,
      def.busCount,
      def.auxCount,
      def.fxCount,
      def.mtxCount,
      def.mainCount,
      def.DCAcount,
      def.fxSndCount,
      def.groupCount,
      def.talkbackCount,
      def.channelOffset,
      def.busOffset,
      def.auxOffset,
      def.fxOffset,
      def.mtxOffset,
      def.mainOffset,
      def.DCAOffset,
      def.fxSndOffset,
      def.groupOffset,
      def.talkbackOffset,
      def.model);

  return buffer;
}
#endif

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation:  public MixerDevice<NetworkSocket>{
  public:
  
    MixingStation():MixerDevice<NetworkSocket>(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,PATH_MIXER_IDENTITY,8887,8088,true){
      //destination.host = "192.168.99.99";
      mixerID.name = "MSStation";
     
      mixerDef = new MSmixerDefinition();
      strcpy(((MSmixerDefinition*)mixerDef)->model, mixerID.model.c_str());

    }

    virtual bool ping() override{
      hwm.network->clientSocketSend(get(destination.echoCommand.c_str()));  // forge a MS get json query with echoCommand as the path
      return true;
    }

    virtual void onSocketConnected() override{
      hwm.network->clientSocketSend(get(PATH_APP_STATE));  // forge a MS get json query with echoCommand as the path
    }
 
    virtual bool restoreMixerProfile(){
      DEBUG_INFO("%s::restoreMixerProfile() => only deserializing definition, protocol is unique and hard wired",interfaceName.c_str());       
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return deserializeMixerDefinitionFromFile(MS_MIXER_PROFILE_FILENAME);
    }    

    virtual bool begin() override{
      DEBUG_INFO("%s::begin() => setting model and dataset pattern",interfaceName.c_str());      
      setModel("virtual");      // "virtual" is different than default value "offline" and this forces setModel to reload and set the model from the definition file
      return true;
    }
    
    bool saveMixerDefinition(){
      DEBUG_INFO("%s::saveMixerDefinition()",interfaceName.c_str());       
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return saveStruct(MS_MIXER_PROFILE_FILENAME, *((MSmixerDefinition*)mixerDef));
    }    

    virtual bool deserializeMixerDefinitionFromFile(const char* filename) override{
      #ifdef DEBUG
        char buffer[256];      
        DEBUG_INFO("%s::deserializeMixerDefinitionFromFile() => before load() :\t%s",interfaceName.c_str(),dumpMixerDef(*((MSmixerDefinition*)mixerDef),buffer, sizeof(buffer)));   
      #endif
      bool success = loadStruct(filename, *((MSmixerDefinition*)mixerDef));
      if (success) mixerID.model = ((MSmixerDefinition*)mixerDef)->model; // mixing station is particular in that sense that the model is stored directly in the definition because it is "virtual". Long story short.
      else mixerID.model = "Mixing Station";
      #ifdef DEBUG
        DEBUG_INFO("%s::deserializeMixerDefinitionFromFile() => after load() :\t%s",interfaceName.c_str(),dumpMixerDef(*((MSmixerDefinition*)mixerDef),buffer, sizeof(buffer)));   
      #endif
      return success;
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

    #define ROUTES_COUNT 5

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
        DEBUG_INFO("←←←🌐 %s::path ✅ %s",interfaceName.c_str(),path);
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
      #define MS_SYNCING      "syncing"
      #define MS_BGSYNC       "backgroundSync"      //MixingStation::processState: msg:,progress:88,state:backgroundSync,topState:connected

      const char* msg      = body["msg"]      | "";
      int progress         = body["progress"] | 0;
      const char* state    = body["state"]    | "";
      const char* topState = body["topState"] | "";
      DEBUG_INFO("%s::processState: msg:%s,progress:%i,state:%s,topState:%s",interfaceName.c_str(),msg,progress,state,topState);

      if (strcmp(topState, MS_IDLE) == 0) {
        if (deviceInterface->state.label = MS_IDLE){
          setStatePending();
          deviceInterface->state.label = MS_IDLE;               // do not invert details/label as label=IDLE forces state refresh for the GUI compared to label=PENDING which is also set by device scan
          deviceInterface->state.details = destinationDetails();  // connexion details of mixing station
        }
      }
      else if (strcmp(topState, MS_CONNECTING) == 0 || strcmp(topState, MS_RECONNECTING) == 0) {
        if (strcmp(state, MS_SYNCING) == 0){
          if (deviceInterface->state.subState != IF_STATE_SYNCING){
            setStateSyncing(MS_SYNCING);      
            deviceInterface->state.details = destinationDetails();
          }
        }else{
          if (deviceInterface->state.subState != String(state)){
            setStateConnecting(state,msg);
          }
        }
        deviceInterface->state.progress = progress;                 
      }
      else if (strcmp(topState, MS_CONNECTED) == 0) {
        // we would now query the mixer
        // EXCEPT if the state comes from a background sync, we just ignore it, oherwise we kill the capacity
        // it means in that case that the progress is != 0 
        //       MixingStation::path ✅ /app/state
        //ℹ       MixingStation::processState: msg:,progress:94,state:backgroundSync,topState:connected
        // we wait that any background sync or progress is finished before querying the mixer definition, otherwise we would create a mess in the capacity with all the incoming data while we are syncing
        if (strcmp(state, MS_BGSYNC) == 0){
          if (progress == 1) setStateBGSyncing();
          else if (progress == 99) setStateLive(); // we are live when background sync is finished, even if we are not fully synced yet, as we receive data and can already start working, the mixer definition will be updated when progress is at 100% and then we will be fully synced
          else deviceInterface->state.progress = progress;  // we just update the progress and wait for the next state update to query the mixer definition when progress is at 100% (when background sync is finished)        
        }else if (progress == 0) getMixerDefinition();  // if progress is 0 it means that we are just connected and not syncing, so we can directly query the mixer definition, otherwise if progress is > 0 it means that we are already syncing and we will query the mixer definition when syncing is finished (when progress is at 100%)
      }
      /*else if (strcmp(topState, MS_RECONNECTING) == 0) {
        setStatePending("green medium",concatStrings(state).c_str());  
        deviceInterface->state.label = topState;
        deviceInterface->state.subState = state;      
      }*/
      else{
        // we do not alter main state code, only report what we received
        setStatePending("green medium",concatStrings("UNHANDLED MS STATE:",state).c_str());
        deviceInterface->state.label = topState;
        deviceInterface->state.subState = state;        
      }

    }

    void setStateBGSyncing(){
      DEBUG_INFO("%s: setStateBGSyncing()",deviceInterface->interfaceName.c_str());
      setStateLive();
      deviceInterface->state.details = concatStrings("Background Syncing... ", deviceInterface->state.details);
    } 


    void getMixerDefinition(){      
      hwm.network->clientSocketSend(get(PATH_MIXER_DEFINITION)); // retrieve name, etc...            
    }

    void processMixerDefinition(char* rx, size_t len) {
      #define MS_CHANNEL_TYPES    "channelTypes"
        #define MS_OFFSET   "offset"
        #define MS_COUNT    "count"
        #define MS_TYPE     "type"


      JsonDocument filter;
      filter["body"][MS_CHANNEL_TYPES][0][MS_OFFSET]     = true;
      filter["body"][MS_CHANNEL_TYPES][0][MS_COUNT]     = true;
      filter["body"][MS_CHANNEL_TYPES][0][MS_TYPE]     = true;

      JsonDocument d;

      DeserializationError e;
      e = deserializeJson(d, rx, len, DeserializationOption::Filter(filter));
      if (e){
        DEBUG_ERROR("JSON error: %s", e.c_str());        
        return;
      }

      //feed the mixerDef
      JsonArray channelTypes = d["body"][MS_CHANNEL_TYPES].as<JsonArray>();
      if (channelTypes.isNull()) {
        DEBUG_ERROR("JSON error: %s missing",MS_CHANNEL_TYPES);
        return;
      }

      // Valeurs par défaut si certaines sections n'existent pas
      mixerDef->channelCount = 0;
      mixerDef->busCount     = 0;
      mixerDef->auxCount     = 0;
      mixerDef->fxCount      = 0;
      ((MSmixerDefinition*)mixerDef)->channelOffset = 0;
      ((MSmixerDefinition*)mixerDef)->busOffset     = 0;
      ((MSmixerDefinition*)mixerDef)->auxOffset    = 0;
      ((MSmixerDefinition*)mixerDef)->fxOffset      = 0;

      for (JsonObject ct : channelTypes) {
        uint8_t type  = ct[MS_TYPE]  | 255;
        uint8_t count = ct[MS_COUNT] | 0;
        uint8_t offset = ct[MS_OFFSET] | 0;

        switch (type) {
          case MS_TYPE_INPUT: // Channel / Input
            if (count > MAX_INPUT) count = MAX_INPUT; // avoid overflow 
            mixerDef->channelCount = count;
            ((MSmixerDefinition*)mixerDef)->channelOffset = offset;
            break;
          case MS_TYPE_AUX: // Aux
            if (count > MAX_AUX) count = MAX_AUX; // avoid overflow
            mixerDef->auxCount = count;
            ((MSmixerDefinition*)mixerDef)->auxOffset = offset;
            break;
          case MS_TYPE_FXRETURN: // FX Return
            if (count > MAX_FX) count = MAX_FX; // avoid overflow
            mixerDef->fxCount = count;
            ((MSmixerDefinition*)mixerDef)->fxOffset = offset;
            break;
          case MS_TYPE_MIXBUS: // Bus
            if (count > MAX_BUSES) count = MAX_BUSES; // avoid overflow
            mixerDef->busCount = count;
            ((MSmixerDefinition*)mixerDef)->busOffset = offset;
            break;
          case MS_TYPE_FXSEND: // not used
            if (count > MAX_FX) count = MAX_FX; // avoid overflow
            mixerDef->fxSndCount = count;
            ((MSmixerDefinition*)mixerDef)->fxSndOffset = offset;          
            break;
          case MS_TYPE_GROUP: // not used
            mixerDef->groupCount = count;
            ((MSmixerDefinition*)mixerDef)->groupOffset = offset;          
            break;
          case MS_TYPE_MAINLR: // Main L/R
            if (count > MAX_MAIN) count = MAX_MAIN; // avoid overflow
            mixerDef->mainCount = count;
            ((MSmixerDefinition*)mixerDef)->mainOffset = offset;
            break;  
          case MS_TYPE_MTX: // Matrix
            mixerDef->mtxCount = count;
            ((MSmixerDefinition*)mixerDef)->mtxOffset = offset;
            break;  
          case MS_TYPE_DCA: // DCA
            if (count > MAX_DCA) count = MAX_DCA; // avoid overflow
            mixerDef->DCAcount = count;            
            ((MSmixerDefinition*)mixerDef)->DCAOffset = offset;
            break;    
          case MS_TYPE_TALKBACK: // DCA
            mixerDef->talkbackCount = count;
            ((MSmixerDefinition*)mixerDef)->talkbackOffset = offset;
            break;                      
          default:
            break;
        }
      }

      // now serialize this onfo into the MS_MIXER_PROFILE_FILENAME in binary form
      saveMixerDefinition();

      // now get the mixer identity
      hwm.network->clientSocketSend(get(PATH_MIXER_IDENTITY)); // retrieve name, etc...            

    }


    void processMixerIdentity(const char* path, JsonVariant body) {
      // got the connected mixer informartion
      #define OFFLINE "offline"
      IPAddress ip;
      ip.fromString(body["ipAddress"] | "");
      //String name   = body["name"]    | OFFLINE;        // name is the same as model, better to use manufacturer + model which is more unique and more relevant for the user
      String name   = body["manufacturer"]    | OFFLINE;
      String model  = body["currentModel"] | "";

      if (name != OFFLINE){        
        setMixer(name,model,ip);                
      }else{
        hwm.network->clientSocketSend(get(PATH_APP_STATE));   // force getting the state as a backup in case mixerQuery didn't pass thru
      }

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
      String data;

      /*
      "path": "/console/data/get/DATAPATH",
      "method": "GET",
      "body": {
        "format": "val",
        "value": "Voc-Claude"
      }*/
      const char* value =  body["value"];     

      if (extractPath(path,PATH_MIXER_GET_DATA,data)){
        // data = /console/data/get/DATAPATH 
        DEBUG_INFO("%s::data: %s \"%s\"",interfaceName.c_str(),data.c_str(),value);
        int index = -1;
        if (sscanf(data.c_str(), "/"MSDATA_CH_CONFIG_NAME, &index) == 1) {
          // DATAPATH => ch.0.cfg.name/val
          // now based on the index, we know which datasets we have to update          
          MSmixerDefinition* msDef = (MSmixerDefinition*)mixerDef;
          int type = getTypeFromIndex(msDef,index);          
          switch (type) {
            case MS_TYPE_INPUT: // Channel / Input
              inputNamesDS->set(index-msDef->channelOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            case MS_TYPE_AUX: // Aux
              auxNamesDS->set(index-msDef->auxOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            case MS_TYPE_FXRETURN: // FX Return
              fxNamesDS->set(index-msDef->fxOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            case MS_TYPE_MIXBUS: // Bus
              mixbusNamesDS->set(index-msDef->busOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            case MS_TYPE_DCA: // DCA
              dcaNamesDS->set(index-msDef->DCAOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            case MS_TYPE_MAINLR: // Main L/R
              mainNamesDS->set(index-msDef->mainOffset,value);        // store the data, index is index - offset as all datasets starts at zero and if we use the index from path we would be out of bound
            break;
            // Types présents mais non utilisés ici
            case MS_TYPE_FXSEND: 
            case MS_TYPE_GROUP: 
            case MS_TYPE_MTX: 
            case MS_TYPE_TALKBACK: 
            default:
              DEBUG_WARN("processDataGet() unmanaged type: %s",MStypeToString(type));
            break;
          }          
        }      
      }  


      //float value          =  body["value"]   | 0.0;
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
      static char buffer[255];

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

    /////////////////////////////////////////////
    // method which is called when sync is finished so we can perform action like subscribing mixer changes...
    virtual void postSync(){
      String cfgNames = MSDATA_CH_CONFIG_NAME;
      cfgNames.replace("%d","*");
      hwm.network->clientSocketSend(post(PATH_SUBSCRIBE,bodyPathFormat(cfgNames.c_str(),"val")));  // forge a MS get json query
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////                                     ////////////////////////////
    /////////////////////////////////// CORE FUNCTION MIXER RELATED REQUEST ////////////////////////////
    ///////////////////////////////////                                     ////////////////////////////    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////////
    /////////// GETTERS ///////////
    ///////////////////////////////
    virtual void getDSname(DataSetSynced* dataset, uint8_t index) override{

      if      (dataset == inputNamesDS) index += ((MSmixerDefinition*)mixerDef)->channelOffset;
      else if (dataset == mixbusNamesDS) index += ((MSmixerDefinition*)mixerDef)->busOffset;
      else if (dataset == auxNamesDS) index += ((MSmixerDefinition*)mixerDef)->auxOffset;
      else if (dataset == fxNamesDS) index += ((MSmixerDefinition*)mixerDef)->fxOffset;
      else if (dataset == mainNamesDS) index += ((MSmixerDefinition*)mixerDef)->mainOffset;
      else if (dataset == dcaNamesDS) index += ((MSmixerDefinition*)mixerDef)->DCAOffset;

      char data[64];  
      snprintf(data,  sizeof(data), MSDATA_CH_CONFIG_NAME, index);
      String path = concatStrings(PATH_MIXER_GET_DATA,"/",data,"/val");

      hwm.network->clientSocketSend(get(path.c_str()));  // forge a MS get json query with echoCommand as the path     
      

    }

    virtual void getCHBUSmute(uint8_t chIndex, uint8_t busIndex) override{

    }

    virtual void getBUSmute(uint8_t index) override{

    }

    virtual void getCHBUSlevel(uint8_t chIndex, uint8_t busIndex) override{

    }
    //////////////////////////////////////
    /////////////// SETTERS //////////////
    //////////////////////////////////////
    virtual void setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level){

    }

    virtual void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, bool mute) override{    

    }

    ////////////////////////////////////////

    // utility
    bool extractPath(const String& full, const String& base, String& remainder) {
        if (full.startsWith(base)) {
            remainder = full.substring(base.length());
            return true;
        }
        return false;
    }

    // à partir d’un index, retrouver le type (CHANNEL, AUX, FX, BUS, …)
    int getTypeFromIndex(MSmixerDefinition* mixerDef, int index) {

        if (index >= mixerDef->channelOffset &&
            index < mixerDef->channelOffset + mixerDef->channelCount) {
            return MS_TYPE_INPUT;
        }

        if (index >= mixerDef->auxOffset &&
            index < mixerDef->auxOffset + mixerDef->auxCount) {
            return MS_TYPE_AUX;
        }

        if (index >= mixerDef->fxSndOffset &&
            index < mixerDef->fxSndOffset + mixerDef->fxSndOffset) {
            return MS_TYPE_FXSEND;
        }

        if (index >= mixerDef->fxOffset &&
            index < mixerDef->fxOffset + mixerDef->fxCount) {
            return MS_TYPE_FXRETURN;
        }

        if (index >= mixerDef->busOffset &&
            index < mixerDef->busOffset + mixerDef->busCount) {
            return MS_TYPE_MIXBUS;
        }

        if (index >= mixerDef->groupOffset &&
            index < mixerDef->groupOffset + mixerDef->groupCount) {
            return MS_TYPE_GROUP;
        }

        if (index >= mixerDef->mainOffset &&
            index < mixerDef->mainOffset + mixerDef->mainCount) {
            return MS_TYPE_MAINLR;
        }

        if (index >= mixerDef->mtxOffset &&
            index < mixerDef->mtxOffset + mixerDef->mtxCount) {
            return MS_TYPE_MTX;
        }        

        if (index >= mixerDef->DCAOffset &&
            index < mixerDef->DCAOffset + mixerDef->DCAcount) {
            return MS_TYPE_DCA;
        }        

        if (index >= mixerDef->talkbackOffset &&
            index < mixerDef->DCAOffset + mixerDef->talkbackCount) {
            return MS_TYPE_TALKBACK;
        }   

        return MS_TYPE_UNDEF; // ou un type INVALID si tu en as un
    }

  
};

const MixingStation::Route MixingStation::routes[ROUTES_COUNT] = {
  { PATH_APP_STATE,            RouteMode::JsonBody,   { .jh = &MixingStation::processState },           false },
  { PATH_MIXER_IDENTITY,       RouteMode::JsonBody,   { .jh = &MixingStation::processMixerIdentity },   false },  
  { PATH_MIXER_DEFINITION,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixerDefinition }, false },  
  { PATH_MIXERS_AVAILABLE,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixersAvailable }, false },
  { PATH_MIXER_GET_DATA"/*",   RouteMode::JsonBody,   { .jh = &MixingStation::processDataGet },         true },
};

#endif