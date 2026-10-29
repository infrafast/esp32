/**
 * @file m_mstation.h
 * @brief Mixing Station backend, mixer definitions and incoming message routing.
 */

#ifndef _MSTATION_h
#define _MSTATION_h

#include "interfaces/i_socket.h"
#include "mixer.h"


/**
 * @brief Mixing Station paths shared by requests and response routing.
 *
 * `get(MSPATH_APP_STATE)` creates a request for `/app/state`; `route()` uses
 * the same constant to dispatch the reply to `processState()`. Dynamic data
 * suffixes are described separately by `MSDATA_*` and `msPathSpecs`.
 */
#define MSPATH_APP_STATE "/app/state"
#define MSPATH_MIXER_IDENTITY "/app/mixers/current"                   // mixer name, model, address IP
#define MSPATH_MIXER_DEFINITION "/console/information"                // mixer attributes like  number of channel...
#define MSPATH_SUBSCRIBE "/console/data/subscribe"
#define MSPATH_UNSUBSCRIBE "/console/data/unsubscribe"
#define MSPATH_MIXERS_AVAILABLE "/app/mixers/available"
#define MSPATH_MIXER_GET_DATA "/console/data/get"
#define MSPATH_MIXER_SET_DATA "/console/data/set"

/**
 * @brief Dynamic Mixing Station data-path templates.
 *
 * The `%d` placeholders are replaced with zero-based API indices. The same
 * templates are used to subscribe to values, decode `/console/data/get/...`
 * replies and build `/console/data/set/...` updates. For example,
 * `MSDATA_CH_BUS_LVL` with `(3, 1)` addresses
 * `ch.3.mix.sends.1.lvl`; `msPathSpecs` uses the template to recognize that
 * reply and `setdataQuery()` uses it to write the value back.
 */
#define MSDATA_CH_CONFIG_NAME  "ch.%d.cfg.name"
#define MSDATA_CH_BUS_MUTE  "ch.%d.mix.sends.%d.on"
#define MSDATA_BUS_MUTE  "ch.%d.mix.on"
#define MSDATA_CH_BUS_LVL  "ch.%d.mix.sends.%d.lvl"

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


/** @brief Return the label for a Mixing Station element type. */
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


/**
 * @brief Add Mixing Station address offsets to the common mixer element counts.
 *
 * BaseMixerDefinition says how many inputs, buses and other elements exist.
 * This derived data structure also says where each group starts in Mixing
 * Station's combined channel list. For example, a bus's global channel index
 * uses busOffset, while a send's bus index is relative to the send list.
 * This is data inheritance, separate from the backend's behavior hierarchy.
 */
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
/** @brief Print mixer capabilities for diagnostics. */
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
/**
 * @brief Translate common mixer commands into Mixing Station WebSocket JSON.
 *
 * MixingStation adds API paths, message routing and model-specific offsets.
 * Its parent MixerDevice<NetworkSocket> shares mixer names, synchronization and
 * refresh logic. That parent inherits both NetworkSocket (the connection and
 * message transport) and MixerBase (the mixer commands used by strips).
 * NetworkSocket reaches DeviceInterface through NetworkDevice. These are two
 * views of one backend object, linked by MixerDevice's binding registry.
 *
 * Create MixingStation in application setup (see MixerApplication.h), then
 * assign that interface to a mixer strip. The strip can stay unchanged when
 * switching to XMSeries because it only calls MixerBase's commands.
 *
 * For example, requestCHBUSlevel(5, 2) converts the one-based selections to
 * Mixing Station indices and subscribes to the send path in normalized format.
 * Routed replies update the common shared values read by strips. A request
 * here may establish a subscription, rather than just fetch a single reply.
 * Model offsets keep device-specific addressing inside this backend; do not
 * apply them in a generic strip. Existing setters and lifecycle code have their
 * own limitations; inheritance alone does not guarantee every operation works.
 */
class MixingStation:  public MixerDevice<NetworkSocket>{
  public:

    #define MS_PING_COMMAND MSPATH_MIXER_IDENTITY
  
    /** @brief Initialize MixingStation with the supplied configuration and bindings. */
    MixingStation():MixerDevice<NetworkSocket>(INTERFACE_MIXINGSTATION, GROUP_INTERFACE_MIXER, MS_PING_COMMAND, 8887, 8088, true){
      icon = ICON_USER_MIXINGSTATION;
      //destination.host = "192.168.99.99";
      mixerID.name = "MSStation";
     
      mixerDef = new MSmixerDefinition();
      strcpy(((MSmixerDefinition*)mixerDef)->model, mixerID.model.c_str());

      // sets the mixer definition and calculate its hash to be able to detect when it changes
      msMixer = (MSmixerDefinition*)mixerDef;
      lastMixerDefHash = computeHash(*((MSmixerDefinition*)mixerDef));

    }
    
    /**
     * @brief Request the currently selected mixer from Mixing Station.
     *
     * The `/app/mixers/current` response is the identity/heartbeat used by
     * `echo()`. When a mixer is connected it may contain `manufacturer`,
     * `currentModel` and `ipAddress`; while no mixer is selected, the response
     * can be incomplete and the state request is used instead.
     */
    virtual bool ping() override{
      hwm.network->clientSocketSend(get(destination.echoCommand.c_str()));
      destination.lastPingTx = millis();
      return true;
    }

    /**
     * @brief Start the API state handshake after the WebSocket opens.
     *
     * This sends a GET request for `/app/state`. Subsequent state notifications
     * drive the lifecycle and eventually trigger `/console/information`.
     */
    virtual void onSocketConnected() override{
      hwm.network->clientSocketSend(get(MSPATH_APP_STATE));  // forge a MS get json query with echoCommand as the path
    }
 
    /**
     * @brief Load the cached virtual-mixer definition.
     *
     * Mixing Station exposes one protocol with changing channel layouts, so
     * the cache stores the last `/console/information` result rather than a
     * hardware-specific protocol profile.
     */
    virtual bool restoreMixerProfile(){
      DEBUG_INFO("%s::restoreMixerProfile() => only deserializing definition, protocol is unique and hard wired",interfaceName.c_str());       
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return deserializeMixerDefinitionFromFile(MS_MIXER_PROFILE_FILENAME);
    }    

    /**
     * @brief Mark the backend as virtual and select its persisted definition.
     *
     * The actual network handshake is driven by `onSocketConnected()` and
     * `processState()`; this method does not query the remote API directly.
     */
    virtual void beginMixer() override{
      DEBUG_INFO("%s::beginMixer() => setting model and dataset pattern",interfaceName.c_str());            
      setModel("virtual");      // "virtual" is different than default value "offline" and this forces setModel to reload and set the model from the definition file
    }

    /** @brief End the interface session and release its active connection. */
    virtual bool end() override{
      DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());            
      return true;
    }
    
    /** @brief Persist the detected mixer capabilities when they have changed. */
    bool saveMixerDefinition(){
      DEBUG_INFO("%s::saveMixerDefinition()",interfaceName.c_str());       
      // version générique pour ESP32 Arduino, afin de sauvegarder / charger n’importe quelle struct POD avec une seule paire de fonctions :
      return saveStruct(MS_MIXER_PROFILE_FILENAME, *((MSmixerDefinition*)mixerDef));
    }    

    /** @brief Load mixer capabilities from the saved model definition. */
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
    // MIXER DEFINITION 
    MSmixerDefinition* msMixer;

    /**
     * @brief used to see if mixer has changed, so we only save it if that's the case
     */
    uint32_t lastMixerDefHash = 0;
    /** @brief Calculate a hash of mixer-definition data to detect changes before saving. */
    uint32_t computeHash(const MSmixerDefinition& def) {
        const uint8_t* data = (const uint8_t*)&def;
        size_t size = sizeof(MSmixerDefinition);

        uint32_t hash = 5381; // djb2 simple
        for (size_t i = 0; i < size; i++) {
            hash = ((hash << 5) + hash) + data[i];
        }
        return hash;
    }

    ///////////////////////////////////////
    // JSON and ROUTE HANDLING DEFINITION
    //////////////////////////////////////
    static constexpr size_t RX_MAX = kBufferMaxSize + 1;
    // Current supported data paths are below 32 bytes; keep room for future
    // Mixing Station path components while avoiding a heap-backed String.
    static constexpr size_t DATA_PATH_MAX = 128;
    char rxBuf[RX_MAX];
    JsonDocument docHeader;
    JsonDocument docFull;    
    // Stable route filter reused for every incoming envelope. The network
    // callback only queues messages; route() consumes them sequentially.
    JsonDocument routeFilter;
    bool routeFilterReady = false;

    enum class RouteMode : uint8_t { JsonBody, RxMessage };

    using JsonRouteHandler = void (MixingStation::*)(const char* path,JsonVariant body);
    using RxRouteHandler   = void (MixingStation::*)(char* rx, size_t len);

    /**
     * @brief Incoming Mixing Station path and its handler.
     */
    struct Route {
      const char* path;
      RouteMode mode;
      union { JsonRouteHandler jh; RxRouteHandler rh; } fn;
    };

    static const Route routes[];

    #define ROUTES_COUNT 5

    /** @brief Test whether an incoming path matches a registered route. */
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

    /** @brief Find the handler associated with an incoming mixer message path. */
    const Route* findRoute(const char* path) const {
      for (size_t i = 0; i < ROUTES_COUNT; ++i) {
        if (routeMatches(routes[i].path, path)) {
          return &routes[i];
        }
      }
      return nullptr;
    }
  
    /**
     * @brief Parse a Mixing Station WebSocket envelope and dispatch its path.
     *
     * Mixing Station replies use an envelope whose `path` selects the handler
     * and whose `body` contains the payload. Errors and unknown paths are
     * rejected before they can change mixer state.
     *
     * Example state reply:
     * @code
     * {"path":"/app/state","body":{"state":"connected",
     *   "topState":"connected","progress":0,"msg":""},"error":null}
     * @endcode
     * Example data reply (the exact path may contain a channel index):
     * @code
     * {"path":"/console/data/get/ch.1.cfg.name",
     *   "body":{"format":"txt","value":"Lead vocal"},"error":null}
     * @endcode
     *
     * The first filter keeps only `path` and `error` for routing. `JsonBody`
     * handlers then parse the envelope body; `RxMessage` handlers receive the
     * raw message and apply a narrower filter when the response is large, such
     * as `/console/information`.
     *
     * @param message Complete JSON envelope received from the transport.
     * @param length Number of bytes in @p message, excluding the terminator.
     */
    virtual void route(const char *message,uint16_t length) override{
   
      if (length + 1 > RX_MAX){
        DEBUG_ERROR("%s::process(...,%i) exceed buffer size:%i",interfaceName.c_str(),length,RX_MAX);
        return;
      } 

      if (!routeFilterReady) {
        routeFilter["path"]  = true;
        routeFilter["error"] = true;
        routeFilterReady = true;
      }

      docHeader.clear();
      DeserializationError e = deserializeJson(
        docHeader,
        message,
        length,
        DeserializationOption::Filter(routeFilter)
      );
      if (e){
        DEBUG_ERROR("JSON error: %s", e.c_str());        
        return;
      }

      if (!docHeader["error"].isNull()) {
        const char* error = docHeader["error"];
        DEBUG_ERROR("Path error:%s",error);        
        return;
      }

      const char* path = docHeader["path"] | "";
      if (!path[0]){
        DEBUG_WARN("\tNo Path");
        return;
      }else{
        DEBUG_DETAIL("←←←🌐 %s::path ✅ %s (%lu)",interfaceName.c_str(),path, millis());
      }

      const Route* r = findRoute(path);
      if (!r){
        DEBUG_WARN("No route found for path:%s",path);
        return;
      }

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

    }  

    /**
     * @brief Apply an `/app/state` notification to the interface lifecycle.
     *
     * `topState` is the connection workflow and `state` is its current step.
     * The handler projects them onto the firmware state. A connected state with
     * zero progress requests the mixer definition; `backgroundSync` delays that
     * request until synchronization is complete.
     *
     * Example connection notification:
     * @code
     * {"path":"/app/state","body":{"state":"connecting",
     *   "topState":"connecting","progress":35,"msg":"Connecting"}}
     * @endcode
     * Example completed connection notification:
     * @code
     * {"path":"/app/state","body":{"state":"connected",
     *   "topState":"connected","progress":0,"msg":""}}
     * @endcode
     *
     * @param path Matched route path; retained for the common handler API.
     * @param body Decoded JSON body from the WebSocket envelope.
     */
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
      //DEBUG_INFO("%s::processState: msg:%s,progress:%i,state:%s,topState:%s",interfaceName.c_str(),msg,progress,state,topState);

      if (strcmp(topState, MS_IDLE) == 0) {
        if (deviceInterface->state.label == MS_IDLE){
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
          if (progress >= 99) setStateLive(); // we are live when background sync is finished, even if we are not fully synced yet, as we receive data and can already start working, the mixer definition will be updated when progress is at 100% and then we will be fully synced
          else{
            if (deviceInterface->state.subState != MS_BGSYNC) setStateBGSyncing();
            deviceInterface->state.progress = progress;  // we just update the progress and wait for the next state update to query the mixer definition when progress is at 100% (when background sync is finished)                      
          }
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

    /**
     * @brief Parse `/console/information` and update the cached channel layout.
     *
     * The API reports one combined channel index space. Each `channelTypes`
     * entry supplies a type, an offset and a count; those ranges are converted
     * into the offsets consumed by the generic mixer datasets. Counts are
     * clamped to the firmware limits before the definition is persisted. The
     * handler filters the response before deserialization because the complete
     * information document also contains colors, frequencies and other fields
     * that are irrelevant to this firmware definition.
     */
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
      uint32_t newHash = computeHash(*((MSmixerDefinition*)mixerDef));
      // only do it if the mixerDef has changed, we detect that by checking if its hash has changed
      if (newHash != lastMixerDefHash) {
        saveMixerDefinition();
        lastMixerDefHash = newHash;
      }      

      // now get the mixer identity
      hwm.network->clientSocketSend(get(MSPATH_MIXER_IDENTITY)); // retrieve name, etc...            

    }

    /**
     * @brief Process the response to the `/app/mixers/current` presence query.
     *
     * A connected mixer supplies its manufacturer, model and (when available)
     * `ipAddress`. If no mixer is selected, the response is not treated as an
     * identity and the state handshake is retried instead.
     */
    void echo(void* data) {
      auto body = *static_cast<JsonVariant*>(data);

      IPAddress ip;
      ip.fromString(body["ipAddress"] | "");

      String name  = body["manufacturer"] | "offline";
      String model = body["currentModel"] | "";

      if (name != "offline") {
        setMixer(name, model, ip);                                // process the data 
      } else {
        hwm.network->clientSocketSend(get(MSPATH_APP_STATE));     //echo contains wrong data, we ask the mixer state
      }
    }

    /**
     * @brief Apply the `/app/mixers/current` response as a heartbeat.
     *
     * `pulseBeat()` consumes the common identity fields. The route is kept
     * separate from `echo()` because the latter also handles the raw response
     * used by the transport's presence check.
     */
    void processMixerIdentity(const char* path, JsonVariant body) {
      pulseBeat(&body);  
    }

    /**
     * @brief Read `/app/mixers/available` and log advertised console models.
     *
     * This endpoint describes the models Mixing Station supports; it is not a
     * network discovery result and does not provide a console IP address.
     */
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

    /**
     * @brief Route `/console/data/get/...` replies to name, level and mute handlers.
     *
     * Decode full paths into element types and indices before updating shared
     * values. The API returns the representation selected by the subscription
     * in `body.format`; the handler consumes `body.value` and translates the
     * combined API index back into the corresponding dataset index.
     */
    void processDataGet(const char* path, JsonVariant body) {
          /*
          "path": "/console/data/get/DATAPATH",
          "method": "GET",
          "body": {
            "format": "val",
            "value": "Voc-Claude"
          }*/
      char data[DATA_PATH_MAX];

      if (!extractPath(path, MSPATH_MIXER_GET_DATA, data, sizeof(data))) {
        DEBUG_WARN("%s::data: extractPath failed for:%s", interfaceName.c_str(), path);
        return;
      }

      // Keep the value in the response document. Converting it to String for
      // every update caused a heap allocation even for numeric and boolean data.
      JsonVariant value = body["value"];
      const char* valueText = value | "";

      //DEBUG_INFO("%s::data: %s <%s>", interfaceName.c_str(), data, valueText);

      MSPathMatch match;
      if (!decodeMSDataPath(data, match)) {
        DEBUG_WARN("%s::data: unmatched path:%s", interfaceName.c_str(), data);
        return;
      }

      /*DEBUG_INFO("%s::pattern matched: %s (argc=%u, a=%d, b=%d, c=%d)",
                interfaceName.c_str(),
                valueText,
                match.argCount,
                match.values[0],
                match.values[1],
                match.values[2]);*/

      switch (match.type) {

        case MSDataPattern::ChConfigName: {
          int index = match.values[0];
          int type = getTypeFromIndex(msMixer, index);

          const char* name = valueText;

          switch (type) {
            case MS_TYPE_INPUT:
              inputNamesDS->set(index - msMixer->channelOffset, name);
              break;
            case MS_TYPE_AUX:
              auxNamesDS->set(index - msMixer->auxOffset, name);
              break;
            case MS_TYPE_FXRETURN:
              fxNamesDS->set(index - msMixer->fxOffset, name);
              break;
            case MS_TYPE_MIXBUS:
              mixbusNamesDS->set(index - msMixer->busOffset, name);
              break;
            case MS_TYPE_DCA:
              dcaNamesDS->set(index - msMixer->DCAOffset, name);
              break;
            case MS_TYPE_MAINLR:
              mainNamesDS->set(index - msMixer->mainOffset, name);
              break;
            default:
              DEBUG_WARN("%s::data: unmanaged cfg.name type=%s index=%d",
                        interfaceName.c_str(), MStypeToString(type), index);
              break;
          }
          break;
        }

        case MSDataPattern::ChBusMute: {
          int chIndex  = match.values[0];
          int busIndex = match.values[1];

          int mute = value.is<bool>() ? (value.as<bool>() ? 1 : 0)
                                      : (strcmp(valueText, "true") == 0 ? 1 : 0);
          DEBUG_DETAIL("%s::CH_BUS_MUTE ch=%d bus=%d mute=%s",interfaceName.c_str(), chIndex, busIndex, mute ? "true" : "false");
          //fireChBusMute(chIndex+1, busIndex+1, mute);      // mstation channel and buses index starts at 0 which are compared to dropdown of strips that read the event starts that starts at 1, therefore, we add 1
          handleChBusMute(chIndex+1, busIndex+1, mute);       // mstation channel and buses index starts at 0 which are compared to dropdown of strips that read the event starts that starts at 1, therefore, we add 1
          break;
        }

        case MSDataPattern::BusMute: {
          int busIndex = match.values[0];
          busIndex -= msMixer->busOffset;     

          int mute = value.is<bool>() ? (value.as<bool>() ? 1 : 0)
                                      : (strcmp(valueText, "true") == 0 ? 1 : 0);
          DEBUG_DETAIL("%s::BUS_MUTE bus=%d mute=%s",interfaceName.c_str(), busIndex, mute ? "true" : "false");
          handleBusMute(busIndex+1,mute); // mstation channel and buses index starts at 0 which are compared to dropdown of strips that read the event starts that starts at 1, therefore, we add 1
          break;
        }

        case MSDataPattern::ChBusLevel: {
          int chIndex  = match.values[0];
          int busIndex = match.values[1];

          float lvl = value.as<float>();

          //DEBUG_INFO("%s::CH_BUS_LEVEL ch=%d bus=%d lvl=%.3f",interfaceName.c_str(), chIndex, busIndex, lvl);
          //fireChBusMixLevel(chIndex+1, busIndex+1, lvl);      // mstation channel and buses index starts at 0 which are compared to dropdown of strips that read the event starts that starts at 1, therefore, we add 1
          handleChBusMixLevel(chIndex+1,busIndex+1,lvl);        // mstation channel and buses index starts at 0 which are compared to dropdown of strips that read the event starts that starts at 1, therefore, we add 1
          break;
        }

        default:
          DEBUG_WARN("%s::data: unsupported decoded path:%s", interfaceName.c_str(), data);
          break;
      }
    }    

    /**
     * @brief Request `/console/information` after the state handshake.
     *
     * The response contains `channelTypes` entries with `type`, `offset` and
     * `count`. `processMixerDefinition()` converts those ranges into the
     * offsets used by the common mixer datasets.
     */
    void getMixerDefinition(){      
      hwm.network->clientSocketSend(get(MSPATH_MIXER_DEFINITION)); // retrieve name, etc...            
    }



    /////////////////////////////////////////////////////////////
    ///                      DEVICE STATES                    ///
    ////////////////////////////////////////////////////////////
    void setStateBGSyncing(){
      DEBUG_INFO("%s: setStateBGSyncing()",deviceInterface->interfaceName.c_str());
      setStateLive();
      deviceInterface->state.subState = MS_BGSYNC ;
      deviceInterface->state.details = concatStrings("Background Syncing... ", deviceInterface->state.details);
    } 

    /** @brief Publish the framework-facing disconnected state and reason. */
    void setStateDisconnected() override{
      DeviceInterface::setStateDisconnected(lang.getString("MixingStation unreachable")->c_str());      
    }

    /////////////////////////////////////////////
    // HELPER TO BUILD AND RETURN A SOCKET REQUEST
    /**
     * @brief Build the common Mixing Station request envelope.
     *
     * The API uses `{path, method, body}` for both REST and WebSocket calls.
     * This helper only serializes the envelope; the caller decides whether it
     * is sent immediately or placed in the socket transmit queue.
     */
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

    /** @brief Build (but do not send) a Mixing Station GET request envelope. */
    const char* get(const char* path, const char* body=nullptr) {
      return build(path, "GET",body);
    }

    /** @brief Build (but do not send) a Mixing Station POST request envelope. */
    const char* post(const char* path,const char* body=nullptr) {
      return build(path, "POST",body);
    }    

    /**
     * @brief Build the body used by `/console/data/subscribe` or `unsubscribe`.
     *
     * `format` is normally `val`; level subscriptions use `norm` when the
     * shared dataset expects a normalized value in the range 0.0 to 1.0.
     */
    const char* bodyPathFormat(const char* path, const char* format="val") {
        static char buffer[160];

        snprintf(buffer, sizeof(buffer),
                "{"
                "\"path\":\"%s\","
                "\"format\":\"%s\""
                "}",
                path, format);

        return buffer;
    }        

    /**
     * @brief Hook for post-synchronization subscriptions.
     *
     * Mixing Station subscriptions are currently issued on demand by the
     * request methods below, so the default implementation intentionally does
     * nothing.
     */
    virtual void postSync(){      
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////                                     ////////////////////////////
    ////////////////////////////////// CORE FUNCTION MIXER RELATED REQUEST ////////////////////////////
    //////////////////////////////////                                     ////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////
    
    ///////////////////////////////
    ////////// GETTERS ///////////
    //////////////////////////////

    /**
     * @brief Subscribe to a channel-name data path.
     *
     * Dataset indices are one-based at the common mixer interface and are
     * translated to Mixing Station's offset-based combined channel list.
     */
    virtual void requestDSname(DataSetSynced* dataset, uint8_t index) override{
            
      if      (dataset == inputNamesDS) index += msMixer->channelOffset;
      else if (dataset == mixbusNamesDS) index += msMixer->busOffset;
      else if (dataset == auxNamesDS) index += msMixer->auxOffset;
      else if (dataset == fxNamesDS) index += msMixer->fxOffset;
      else if (dataset == mainNamesDS) index += msMixer->mainOffset;
      else if (dataset == dcaNamesDS) index += msMixer->DCAOffset;

      snprintf(pathBuffer, sizeof(pathBuffer), MSDATA_CH_CONFIG_NAME, index);
      subscribeQuery(pathBuffer);
    }

    /** @brief Subscribe to `ch.<index>.mix.on` for a bus mute state. */
    virtual void requestBUSmute(uint8_t busIndex) override{
      busIndex -= 1; // index of mixing station starts at 0

      snprintf(pathBuffer, sizeof(pathBuffer), MSDATA_BUS_MUTE, busIndex + msMixer->busOffset );  
      subscribeQuery(pathBuffer);
    }

    /** @brief Subscribe to `ch.<index>.mix.sends.<bus>.on`. */
    virtual void requestCHBUSmute(uint8_t chIndex, uint8_t busIndex) override{
      chIndex -= 1; // the requestCHBUSlevel is called from a strip which provide its selectID , since they starts at 1 but MS staation channels starts at zero, we have to deduct 1
      busIndex -= 1; // same for bus index, we have to deduct 1 to match MS station protocol

      snprintf(pathBuffer, sizeof(pathBuffer), MSDATA_CH_BUS_MUTE,
              chIndex + msMixer->channelOffset,      // requestCHBUSmute is called from strip with Select1ID as the parameter which starts at 1, hence we deduct 1 since MS channel starts at 0
              busIndex );      // bus index is not offseted for level and mute, we only deduct one which is the offset of the select2id dropdownm value

      subscribeQuery(pathBuffer);
    }

    /**
     * @brief Subscribe to a send level using Mixing Station's `norm` format.
     *
     * The resulting value is delivered immediately and on subsequent changes.
     */
    virtual void requestCHBUSlevel(uint8_t chIndex, uint8_t busIndex) override{

      chIndex -= 1; // the requestCHBUSlevel is called from a strip which provide its selectID , since they starts at 1 but MS staation channels starts at zero, we have to deduct 1
      busIndex -= 1; // same for bus index, we have to deduct 1 to match MS station protocol

      snprintf(pathBuffer, sizeof(pathBuffer), MSDATA_CH_BUS_LVL,
              chIndex + msMixer->channelOffset,
              busIndex);      // bus index is not offseted for level and mute, only for the path to subscribeQuery to the change of this data, which is a bit weird but that's how MS do it, we just adapt to their protocol

      subscribeQuery(pathBuffer,"norm");   // we want a 0.0 -> 1.0 return, hence we use norm
    }

    ///// getters helpers
    /**
     * @brief Subscribe to a data path and request its current value.
     *
     * The generated request is equivalent to:
     * @code
     * {"path":"/console/data/subscribe","method":"POST",
     *  "body":{"path":"ch.0.mix.sends.0.lvl","format":"norm"}}
     * @endcode
     */
    void subscribeQuery(const char* path, const char* format="val"){         
      // drop the request to subscribeQuery to a change of this data and get immediate value
      //hwm.network->clientSocketSend(post(MSPATH_SUBSCRIBE,bodyPathFormat(path,format)));  // direct send
      enqueueSocketMessageTx(post(MSPATH_SUBSCRIBE,bodyPathFormat(path,format)));

    }    


    /** @brief Remove a `/console/data/subscribe` registration for a data path. */
    void unsubscribeQuery(const char* path, const char* format="val"){         
      // drop the request to subscribeQuery to a change of this data and get immediate value
      //hwm.network->clientSocketSend(post(MSPATH_UNSUBSCRIBE,bodyPathFormat(path,format)));  // direct send
      enqueueSocketMessageTx(post(MSPATH_UNSUBSCRIBE,bodyPathFormat(path,format)));
    } 

    /////////////////////////////////////
    ////////////// SETTERS //////////////
    /////////////////////////////////////
    /**
     * @brief Set a send level through `/console/data/set/<path>/norm`.
     *
     * The common mixer layer supplies one-based channel and bus selections;
     * Mixing Station receives zero-based indices for this data path.
     */
    virtual void setCHBUSlevel(int chIndex, int busIndex, float level) override {      
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex - 1, busIndex - 1, MSDATA_CH_BUS_LVL);
      // level est une valeur normalisée 0.0 -> 1.0
      setdataQuery(pathBuffer, level, "norm");      // immediate send to avoid "letency" when moving the fader
    }

    /**
     * @brief Set the mute state of an input send to a bus.
     *
     * The value is sent through `/console/data/set/<path>/val` and queued for
     * transmission, preserving the common socket back-pressure behavior.
     */
    virtual void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, int mute) override {    
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex - 1, busIndex - 1, MSDATA_CH_BUS_MUTE);
      setdataQuery(pathBuffer, mute, "val");
    }

    ///// setters helpers
    /** @brief Format a floating-point value as a JSON number for Mixing Station. */
    static int formatSetValue(char* buffer, size_t bufferSize, float value) {
      return snprintf(buffer, bufferSize, "{\"value\":%.6g}", (double)value);
    }

    static int formatSetValue(char* buffer, size_t bufferSize, double value) {
      return snprintf(buffer, bufferSize, "{\"value\":%.6g}", value);
    }

    template<typename T>
    static int formatSetValue(char* buffer, size_t bufferSize, T value) {
      return snprintf(buffer, bufferSize, "{\"value\":%ld}", (long)value);
    }

    template<typename T>
    /**
     * @brief Send a value update to a Mixing Station data path.
     * @tparam T Value type serialized into the request body.
     * The format selects the API representation (`val` or `norm`); immediate
     * bypasses the TX queue for latency-sensitive controls.
     */
    void setdataQuery(const char* path, T value, const char* format = "val", bool immediate=false) {
      /*
      {
        "path": "/console/data/set/path/val",
        "method": "POST",
        "body": {
          "value": value
        }
      }
      */      
      char fullPath[192];
      int pathLength = snprintf(
        fullPath,
        sizeof(fullPath),
        "%s/%s/%s",
        MSPATH_MIXER_SET_DATA,
        path,
        format
      );

      if (pathLength < 0 || static_cast<size_t>(pathLength) >= sizeof(fullPath)) {
        DEBUG_ERROR("%s::setdataQuery() path too long", interfaceName.c_str());
        return;
      }

      char bodyBuffer[64];
      int bodyLength = formatSetValue(bodyBuffer, sizeof(bodyBuffer), value);

      if (bodyLength < 0 || static_cast<size_t>(bodyLength) >= sizeof(bodyBuffer)) {
        DEBUG_ERROR("%s::setdataQuery() body too large", interfaceName.c_str());
        return;
      }

      const char* request = post(fullPath, bodyBuffer);

      if (immediate)
        hwm.network->clientSocketSend(request);  // direct send
      else
        enqueueSocketMessageTx(request);       // queued
    }    

    ///////////////////////////////////////

    // utility
    /**
     * @brief Extract a relative data path from a `/console/data/get` reply.
     *
     * The optional `/val` suffix is removed because the path-pattern decoder
     * matches the canonical data path only.
     */
    bool extractPath(const char* full,
                     const char* base,
                     char* remainder,
                     size_t remainderSize) {
        if (!full || !base || !remainder || remainderSize == 0) return false;

        const size_t baseLength = strlen(base);
        if (strncmp(full, base, baseLength) != 0) return false;

        const char* suffix = full + baseLength;
        size_t suffixLength = strlen(suffix);

        // Some replies append "/val" to the data path; it is not part of the
        // canonical pattern used by decodeMSDataPath().
        if (suffixLength >= 4 && strcmp(suffix + suffixLength - 4, "/val") == 0) {
            suffixLength -= 4;
        }

        if (suffixLength + 1 > remainderSize) return false;

        memcpy(remainder, suffix, suffixLength);
        remainder[suffixLength] = '\0';
        return true;
    }

    /**
     * @brief Resolve an element type from Mixing Station's combined index.
     *
     * This is the inverse of the offset assignment performed by
     * `processMixerDefinition()` and is used when a data reply must update a
     * type-specific dataset.
     */
    int getTypeFromIndex(MSmixerDefinition* mixerDef, int index) {

        if (index >= mixerDef->channelOffset &&
            index < mixerDef->channelOffset + mixerDef->channelCount)
            return MS_TYPE_INPUT;

        if (index >= mixerDef->auxOffset &&
            index < mixerDef->auxOffset + mixerDef->auxCount)
            return MS_TYPE_AUX;

        if (index >= mixerDef->fxSndOffset &&
            index < mixerDef->fxSndOffset + mixerDef->fxSndCount)
            return MS_TYPE_FXSEND;        

        if (index >= mixerDef->fxOffset &&
            index < mixerDef->fxOffset + mixerDef->fxCount)
            return MS_TYPE_FXRETURN;

        if (index >= mixerDef->busOffset &&
            index < mixerDef->busOffset + mixerDef->busCount)
            return MS_TYPE_MIXBUS;

        if (index >= mixerDef->groupOffset &&
            index < mixerDef->groupOffset + mixerDef->groupCount)
            return MS_TYPE_GROUP;

        if (index >= mixerDef->mainOffset &&
            index < mixerDef->mainOffset + mixerDef->mainCount)
            return MS_TYPE_MAINLR;
        
        if (index >= mixerDef->mtxOffset &&
            index < mixerDef->mtxOffset + mixerDef->mtxCount) 
            return MS_TYPE_MTX;
        
        if (index >= mixerDef->DCAOffset &&
            index < mixerDef->DCAOffset + mixerDef->DCAcount)
            return MS_TYPE_DCA;

        if (index >= mixerDef->talkbackOffset &&
            index < mixerDef->talkbackOffset + mixerDef->talkbackCount)
            return MS_TYPE_TALKBACK;

        return MS_TYPE_UNDEF; // ou un type INVALID si tu en as un
    }

    //**************** PATTERN MATCHING FOR DATA PATHS *****************
    enum class MSDataPattern : uint8_t {
      None = 0,
      ChConfigName,
      ChBusMute,
      BusMute,
      ChBusLevel
    };

    /**
     * @brief Description of a supported Mixing Station path pattern.
     */
    struct MSPathSpec {
      MSDataPattern type;
      const char* scanFormat;
      uint8_t argCount;   ///< nombre de %d attendus
    };

    /**
     * @brief Result fields extracted from a Mixing Station path.
     */
    struct MSPathMatch {
      MSDataPattern type = MSDataPattern::None;
      int values[3] = { -1, -1, -1 };
      uint8_t argCount = 0;
    };

    static const MSPathSpec msPathSpecs[];
    static constexpr size_t msPathSpecsCount = 4;


    /**
     * @brief Accept a data path only when the whole expected pattern matches.
     *
     * Extract numeric indices while rejecting extra suffixes, so similarly prefixed
     * paths are not routed to the wrong value handler.
     */
    bool matchDataPathExact(const char* dataPath,
                            const char* scanFormat,
                            uint8_t argCount,
                            int* outValues) {
      if (!dataPath || !scanFormat) return false;
      if (argCount > 3) return false;

      int consumed = 0;
      const int dataPathLength = (int)strlen(dataPath);

      int a = -1, b = -1, c = -1;
      int matched = 0;

      switch (argCount) {
        case 0:
          matched = sscanf(dataPath, scanFormat, &consumed);
          return (matched == 0 && consumed == dataPathLength);

        case 1:
          matched = sscanf(dataPath, scanFormat, &a, &consumed);
          if (matched == 1 && consumed == dataPathLength) {
            outValues[0] = a;
            return true;
          }
          return false;

        case 2:
          matched = sscanf(dataPath, scanFormat, &a, &b, &consumed);
          if (matched == 2 && consumed == dataPathLength) {
            outValues[0] = a;
            outValues[1] = b;
            return true;
          }
          return false;

        case 3:
          matched = sscanf(dataPath, scanFormat, &a, &b, &c, &consumed);
          if (matched == 3 && consumed == dataPathLength) {
            outValues[0] = a;
            outValues[1] = b;
            outValues[2] = c;
            return true;
          }
          return false;
      }

      return false;
    }

    /**
     * @brief Classify an incoming data path without accepting a partial match.
     *
     * Exact matching prevents a valid prefix such as `ch.1.mix.sends.0.on`
     * from accidentally consuming a different or future endpoint with an
     * extra suffix.
     */
    bool decodeMSDataPath(const char* dataPath, MSPathMatch& out) {
      if (!dataPath) return false;

      for (size_t i = 0; i < msPathSpecsCount; ++i) {
        const MSPathSpec& spec = msPathSpecs[i];

        int values[3] = { -1, -1, -1 };
        if (matchDataPathExact(dataPath, spec.scanFormat, spec.argCount, values)) {
          out.type = spec.type;
          out.argCount = spec.argCount;
          out.values[0] = values[0];
          out.values[1] = values[1];
          out.values[2] = values[2];
          return true;
        }
      }

      out.type = MSDataPattern::None;
      out.argCount = 0;
      out.values[0] = -1;
      out.values[1] = -1;
      out.values[2] = -1;
      return false;
    }

    /** @brief Update the quality-of-service indicator used by the display. */
    virtual void determineQoS() override{    
      if (state.code != IF_READY) state.QoSlevel = QoS::UNDETERMINED;
      else
      if (destination.latency <= 50 && destination.jitter <= 10) state.QoSlevel = QoS::GOOD;    
      else
      if (destination.latency <= 100 && destination.jitter <= 20) state.QoSlevel = QoS::MEDIUM;    
      else
      if (destination.latency <= 200 && destination.jitter <= 40)  state.QoSlevel = QoS::POOR;    // yellow
      else
      // Au-delà : problème réseau
      state.QoSlevel = QoS::CRITICAL;    
    }    
        
    
};




/**
 * @brief Map Mixing Station API paths to the corresponding response handlers.
 *
 * Each row contains the API path, decoding mode and handler. `/app/state`
 * passes its decoded body to `processState()`; `/console/information` passes
 * the raw message so `processMixerDefinition()` can filter `channelTypes`.
 *
 * The `/console/data/get/*` entry deliberately uses a trailing wildcard. A
 * reply such as `/console/data/get/ch.3.mix.sends.1.lvl` must reach one common
 * handler; registering one row per channel and bus would be impossible because
 * those indices are dynamic. `processDataGet()` removes the prefix and then
 * applies the stricter `msPathSpecs` patterns below.
 *
 * A table keeps endpoint registration declarative and leaves exact/wildcard
 * matching in one place (`findRoute()`).
 */
const MixingStation::Route MixingStation::routes[ROUTES_COUNT] = {
  { MSPATH_APP_STATE,            RouteMode::JsonBody,   { .jh = &MixingStation::processState }            },
  { MSPATH_MIXER_IDENTITY,       RouteMode::JsonBody,   { .jh = &MixingStation::processMixerIdentity }    },  
  { MSPATH_MIXER_DEFINITION,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixerDefinition }  },  
  { MSPATH_MIXERS_AVAILABLE,     RouteMode::RxMessage,  { .rh = &MixingStation::processMixersAvailable }  },
  { MSPATH_MIXER_GET_DATA"/*",   RouteMode::JsonBody,   { .jh = &MixingStation::processDataGet }          },
};


/**
 * @brief Supported data-path patterns for `/console/data/get/...` replies.
 *
 * These patterns cover only values exposed through the generic mixer datasets.
 * `argCount` states how many numeric `%d` components must be extracted.
 *
 * Examples:
 * - `ch.3.cfg.name` matches `ChConfigName` with `{3}`.
 * - `ch.3.mix.sends.1.lvl` matches `ChBusLevel` with `{3, 1}`.
 * - `ch.3.mix.sends.1.lvl.extra` matches nothing because matching is exact;
 *   silently accepting that suffix could route a future API field to the
 *   wrong dataset.
 *
 * `routes` selects the top-level handler; this table classifies its dynamic
 * data suffix. Keeping those steps separate localizes validation and index
 * conversion in `decodeMSDataPath()`.
 */
/**
 * @brief Declare one data-path entry and derive its exact sscanf format.
 *
 * The public path model is used to build subscriptions and requests. The
 * matching format needs the leading slash and a trailing `%n` so the parser
 * can verify that the complete path was consumed. Keeping both forms here
 * prevents them from drifting apart when a path is added or renamed.
 */
#define MSTATION_PATH_SPEC(type, pattern, count) \
  { type, "/" pattern "%n", count }

const MixingStation::MSPathSpec MixingStation::msPathSpecs[MixingStation::msPathSpecsCount] = {
  MSTATION_PATH_SPEC(MSDataPattern::ChConfigName, MSDATA_CH_CONFIG_NAME, 1),
  MSTATION_PATH_SPEC(MSDataPattern::ChBusMute,    MSDATA_CH_BUS_MUTE,    2),
  MSTATION_PATH_SPEC(MSDataPattern::BusMute,      MSDATA_BUS_MUTE,       1),
  MSTATION_PATH_SPEC(MSDataPattern::ChBusLevel,   MSDATA_CH_BUS_LVL,     2),
};

#undef MSTATION_PATH_SPEC

#endif
