/**
 * @file m_xmseries.h
 * @brief Behringer X32/X-Air backend and OSC protocol matching.
 */

#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

/**
 * @brief Behringer OSC connection and profile paths.
 *
 * X32/M32 consoles use OSC over UDP (normally port 10023); X-Air discovery
 * also probes port 10024. `/xinfo` identifies a console and includes its
 * network address, name, model and firmware. Model profiles then select the
 * channel counts and OSC address patterns used by this backend.
 * Protocol reference: https://tostibroeders.nl/wp-content/uploads/2020/02/X32-OSC.pdf
 */
#define ADDRESS_CONNECT_METHOD "/connect/%d"
#define ADDRESS_MIXER_MODEL "/model/%s"

#define INTERFACE_XMSeries "XMSeries"

enum{
    CONNECT_MANUAL=1,                   ///< select always start at value 1
    CONNECT_DISCOVER,        
};

/**
 * @brief Dataset of mixer connection-method choices.
 */
class ConnectMethodDS : public DataSet{
  
  public:

    #define ADDRESS_DISCOVERY_SIZE 2

    String options[ADDRESS_DISCOVERY_SIZE]={
        "manual",
        "discovery"
    };     

    /**
     * @brief Initialize ConnectMethodDS and its configured member bindings.
     */
    ConnectMethodDS(Address* _address): DataSet(_address, INTERFACE_XMSeries, &IFdataRepo){}
    
    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &options[index];    
    }  

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return ADDRESS_DISCOVERY_SIZE;
    }
     
};

#define BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION "mix"

/**
 * @brief Dataset of supported X32/X-Air mixer-model choices.
 */
class XMmixerModelDS : public DataSet{
  
  public:
    std::vector<String> mixerModels;

    /**
     * @brief Initialize XMmixerModelDS and its configured member bindings.
     */
    XMmixerModelDS(Address* _address): DataSet(_address, INTERFACE_XMSeries, &IFdataRepo){       
    }
    
    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()) return &errorDatasetOOR;
      return &mixerModels[index];    
    }  

    /** @brief Fill the supported mixer-model choices. */
    void populate(){
      populateFileList(BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION,MIXER_DEFINITION_PATH,&mixerModels,"",false);     // no extension               
      if (getSize()<1) mixerModels.push_back("empty");
    }

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return mixerModels.size();
    }

    /** @brief Test whether the requested item exists in this collection. */
    bool contains(const String& value) const {
      for (const auto& model : mixerModels) {
        if (model == value) {
          return true;
        }
      }
      return false;
    }    
     
};

enum{
  XM_MAIN_CONFIG_NAME,
  XM_DCA_CONFIG_NAME,
  XM_CH_CONFIG_NAME,
  XM_BUS_CONFIG_NAME,
  XM_AUX_CONFIG_NAME,
  XM_FX_CONFIG_NAME,
  XM_CH_BUS_MIX_LEVEL,
  XM_CH_BUS_MUTE,
  XM_CH_LR_MUTE,
  XM_BUS_MUTE,
  XM_PROOTCOL_COUNT
};

/**
 * @brief Translate common mixer commands into Behringer X32/X-Air OSC messages.
 *
 * Inheritance has three layers: XMSeries adds the device's address patterns and
 * discovery; MixerDevice<NetworkOSC> adds shared mixer synchronization;
 * NetworkOSC adds OSC messaging over the network. MixerDevice also inherits
 * MixerBase, the command contract seen by strips. NetworkOSC reaches
 * DeviceInterface through NetworkDevice, the connection view used by the app.
 *
 * For example, a strip asks requestCHBUSlevel(5, 2) through MixerBase. This
 * backend formats the selected protocol's OSC address and sends the request.
 * process() decodes the reply and calls the common handleChBusMixLevel(), which
 * updates the shared value read by every strip targeting that send.
 *
 * Create XMSeries in application setup (see MixerApplication.h), then select
 * that interface for a mixer strip. Do not create MixerDevice directly: the
 * concrete backend provides its missing protocol operations. Model profiles
 * supply counts and OSC patterns; /xremote is refreshed every nine seconds to
 * keep device updates active. postSync() currently adds no extra subscription.
 */
class XMSeries: public MixerDevice<NetworkOSC>{
  public:

    #define XMSERIES_PING_CMD "/xinfo"          //=> maybe go to mixer definition if not the same for all X/M....
    #define XMSERIES_XREMOTE_CMD "/xremote"     // called every 9 seconds

    int connectMethod = CONNECT_DISCOVER;
    
    String protocolName ="xair";
    /**
     * OSC address patterns loaded from the model's `.osc` profile. Entries
     * follow the `XM_*` enum order; `protocolArgCount` records whether a
     * pattern is exact, has one index, or has two indices.
     */
    String protocol[XM_PROOTCOL_COUNT];         
    uint8_t protocolArgCount[XM_PROOTCOL_COUNT] = {0};

    /** X32 and X-Air discovery ports, tried alternately. */
    #define DISCOVERY_PORT_COUNT 2 
    int discoveryPort[DISCOVERY_PORT_COUNT] = {10024, 10023};
    uint8_t discoveryIndex = 0;               ///< rolling index;

    unsigned long lastXremote = 0;          ///< Last `/xremote` renewal time.
    /** `/xremote` expires after about 10 seconds; renew slightly earlier. */
    #define XREMOTE_INTERVAL  9000

    ConnectMethodDS connectMethodDS = ConnectMethodDS(new DropDownAddress(ADDRESS_CONNECT_METHOD));
    XMmixerModelDS mixerModelDS = XMmixerModelDS(new DropDownAddress(ADDRESS_MIXER_MODEL));
    
    /** @brief Initialize XMSeries with the supplied configuration and bindings. */
    XMSeries():MixerDevice<NetworkOSC>(INTERFACE_XMSeries, 8888, "192.168.0.1", 10023, XMSERIES_PING_CMD, true, GROUP_INTERFACE_MIXER){      
        icon = ICON_USER_BEHRINGER;
        unregisterDataSetAddress(&addressPing);              // we un-expose the ping parameter as user should not modify it 

        registerDataSetAddress(connectMethodDS.address);    // add a connect method parameter
        registerDataSetAddress(mixerModelDS.address);       // select mixer when offline

        destination.echoCommand = XMSERIES_PING_CMD;     // instead we set it manually once for all

        mixerDef = new BaseMixerDefinition();

    }     

    /**
     * @brief method which is called when sync is finished so we can perform action like subscribing mixer changes...
     */
    virtual void postSync(){
      // do nothing
    }

    /**
     * @brief Renew `/xremote` and run the normal OSC network monitor.
     *
     * The X32 keeps change notifications active for about 10 seconds after
     * `/xremote`; this backend renews it every 9 seconds and sends the
     * keepalive immediately.
     */
    virtual bool monitorService() override {
      unsigned long now = millis();

      if (now - lastXremote >= XREMOTE_INTERVAL) {
        /*
        oscOut.empty();
        oscOut.setAddress(XMSERIES_XREMOTE_CMD);   // envoi differé
        send(oscOut);           
        */
        sendOSC(XMSERIES_XREMOTE_CMD, true);   // envoi immediat Sinon un backlog sortant peut retarder le keepalive, Comme c’est un keepalive critique, je l'envoi directement au lieu de passer par send() :
        lastXremote = now;
      }

      return NetworkOSC::monitorService();
    }


    /** @brief Start backend-specific identification or subscriptions. */
    virtual void beginMixer() override{
      DEBUG_INFO("%s::beginMixer() => user setting up XMmixerModelDS/model and dataset pattern",interfaceName.c_str());                  
      mixerModelDS.populate();        // feeding the list of available mixer definitions for when we are offline and want to select a mixer      
      // check if mixerID.model is in the list, otherwise set it to the first item from it
      if (!mixerModelDS.contains(mixerID.model)){
        const char* firstValidModel = mixerModelDS.getValuePtr(0)->c_str();
        DEBUG_WARN("%s is not a valid model, use %s as default",mixerID.model,firstValidModel);
        setModel(firstValidModel);
      }else setModel(mixerID.model.c_str());       // setting up the last connected mixer
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(JsonArray& properties) override{

        char* buf;

        if (state.enabled){
            // if mixer is enabled, we show normal parameter except choosing the model because it will be imposed by connection
            buf = (char*)malloc(strlen(ADDRESS_CONNECT_METHOD)*sizeof(char*));
            sprintf(buf,ADDRESS_CONNECT_METHOD,connectMethod);
            properties.add(String(buf));
            free(buf); 

            // inherited parameters from NetworkOSC (IP+port) are useless when we are in discovery, we therefore do not use store/serialize them
            if (CONNECT_DISCOVER != connectMethod){
                NetworkOSC::serializeProperties(properties);        
            }

        }else{
            // we only give the possibility to user to select the model if we are offline
            buf = (char*)malloc((strlen(ADDRESS_MIXER_MODEL)+64)*sizeof(char*));
            sprintf(buf,ADDRESS_MIXER_MODEL,mixerID.model.c_str());
            properties.add(String(buf));
            free(buf); 
        }

    }

    /** @brief Restore one property string and report whether its value changed. */
    virtual bool restoreProperty(const char* property) override{          
      bool changed = false;

      int newMethod;

      if (sscanf(property, ADDRESS_CONNECT_METHOD, &newMethod)){
        if (connectMethod != newMethod) changed = true;
        connectMethod = newMethod;
      }

      char model[64];
      if (sscanf(property, ADDRESS_MIXER_MODEL, &model)){
        if (mixerID.model != model) changed = true;        
        setModel(model);        
      }
      
      // inherited parameters from NetworkOSC (IP+port) are useless when we are in discovery, we therefore do not recall them
      if (CONNECT_DISCOVER != connectMethod){
        changed |= NetworkOSC::restoreProperty(property);
      }
      return changed;
    }       

    /**
     * @brief Load the selected model definition and its OSC address profile.
     *
     * The `.mix` file supplies counts and the protocol family; the matching
     * `.osc` file supplies the concrete OSC paths used by requests and routing.
     */
    virtual bool restoreMixerProfile(){
      DEBUG_INFO("%s::restoreMixerProfile() => deserializing definition and protocol",interfaceName.c_str()); 
      String mixerDefinitionFilename = concatStrings(mixerID.model,".",BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION);
      bool success = deserializeMixerDefinitionFromFile(mixerDefinitionFilename.c_str());
      success &= deserializeMixerProtocolFromFile(protocolName.c_str());
      return success;
    }

    /**
     * @brief Load the model's OSC address patterns and count their placeholders.
     *
     * Lines are read in `XM_*` order from the model `.osc` file; keep that
     * order aligned with the command constants used by reads and writes.
     */
    bool deserializeMixerProtocolFromFile(const char* filename) {
      #define XMSERIES_PROTOCOL_FILE_EXTENSION ".osc"

      String fullFilename = concatStrings(MIXER_DEFINITION_PATH,filename,XMSERIES_PROTOCOL_FILE_EXTENSION);   
      DEBUG_INFO("%s::deserializeMixerProtocolFromFile(%s) to mixerProtocol",interfaceName.c_str(), fullFilename.c_str());

      File file = LittleFS.open(fullFilename, "r");
      if (!file) {
        DEBUG_ERROR("%s::deserializeMixerProtocolFromFile -> impossible d'ouvrir %s",interfaceName.c_str(), fullFilename.c_str());
        return false;
      }

      int index = 0;
      while (file.available() && index < XM_PROOTCOL_COUNT) {
        String line = file.readStringUntil('\n');
        line.trim();

        protocol[index] = line;
        protocolArgCount[index] = countPlaceholders(line);

        DEBUG_INFO("protocol[%d] = %s (args=%d)",index,protocol[index].c_str(),protocolArgCount[index]);

        index++;
      }

      file.close();

      // Si le fichier contient moins de lignes que prévu
      if (index < XM_PROOTCOL_COUNT) {
        DEBUG_WARN("%s::deserializeMixerProtocolFromFile -> fichier incomplet (%d/%d lignes)",interfaceName.c_str(), index, XM_PROOTCOL_COUNT);
      }

      return true;
    }

    #define XAIR_PROTOCOL "xair"
    #define XM_PROTOCOL "x32"

    /**
     * @brief Load channel counts and protocol-family settings from a `.mix` file.
     */
    virtual bool deserializeMixerDefinitionFromFile(const char* filename){
      String fullFilename = concatStrings(MIXER_DEFINITION_PATH,filename); 

      DEBUG_INFO("%s:deserializeMixerDefinitionFromFile(%s)",this->interfaceName.c_str(),fullFilename.c_str());             

      File file = LittleFS.open(fullFilename, "r");
      if (!file) {
        DEBUG_PRINT("❌ can't open file");
        return false;
      }

      JsonDocument doc;

      DeserializationError error = deserializeJson(doc, file);
      file.close();

      if (error) {
        DEBUG_PRINT("❌ json error:%s",error.c_str());
        return false;
      }

      mixerDef->channelCount = doc["channelCount"] | mixerDef->channelCount;
      mixerDef->busCount     = doc["busCount"]     | mixerDef->busCount;
      mixerDef->auxCount     = doc["auxCount"]     | mixerDef->auxCount;
      mixerDef->fxCount      = doc["fxCount"]      | mixerDef->fxCount;
      mixerDef->mainCount    = doc["mainCount"]    | mixerDef->mainCount;
      mixerDef->mtxCount     = doc["mtxCount"]     | mixerDef->mtxCount;
      mixerDef->DCAcount     = doc["DCAcount"]     | mixerDef->DCAcount;
      mixerDef->fxSndCount    = doc["fxSndCount"]    | mixerDef->fxSndCount;
      mixerDef->groupCount    = doc["groupCount"]    | mixerDef->groupCount;
      
      protocolName           = doc["protocol"]     | protocolName;

      // depending on the protocol name, we set other internal properties
      if (protocolName == XAIR_PROTOCOL){
        // ...
      }else if (protocolName == XM_PROTOCOL){
        echoing = false;
      }else{
        // ...
      }


      DEBUG_INFO("channelCount:%u busCOunt:%u auxCount:%u fxCount:%u mainCount:%u mtxCount:%u DCACount:%u fxSndCount:%u groupCount:%u",mixerDef->channelCount, mixerDef->busCount, mixerDef->auxCount, mixerDef->fxCount, mixerDef->mainCount, mixerDef->mtxCount, mixerDef->DCAcount, mixerDef->fxSndCount, mixerDef->groupCount);

      return true;
    }

    /** @brief Apply the requested enabled state through the interface lifecycle. */
    virtual void switchDevice(bool _enable) override{
        NetworkDevice::switchDevice(_enable);
        //we capture the device switching because we do not display the same properties if we are enable or disable (see serializeProperties), hence have to broadcast the change
        state.broadcastConfig = true;
    }

    /** @brief React to changed connection settings, including reconnection when required. */
    virtual void handlePropertyChange(){
      NetworkOSC::handlePropertyChange();
      DEBUG_INFO("%s::handlePropertyChange() refresh config",interfaceName.c_str());    
      state.broadcastConfig = true; // it could be that the connect method has changed so we need to redisplay/hide the configuration fields
      if (state.enabled) handleConnectService();
    }    

    int i1,i2;    ///< index

    /**
     * @brief Match incoming OSC addresses and update the shared mixer datasets.
     *
     * The profile supplies the address patterns. For example,
     * `/ch/01/mix/02/on` updates a channel-to-bus mute and
     * `/ch/01/mix/02/level` updates its send level. Exact routes such as a fixed
     * X-Air main path are handled separately. `matchProtocol()` extracts the
     * numeric indices before the common handlers receive the value.
     */
    virtual void process() override {    

        const char* addr = oscIn.getAddress();        
        if (!addr) return;

        int m1 = -1;
        int m2 = -1;
        int match = MATCH_NONE;

        ///////////////////// CONFIG NAME ROUTES ////////////////////////

        match = matchProtocol(XM_CH_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleNameRoute(inputNamesDS, "XM_CH_CONFIG_NAME", m1);
          return;
        }

        match = matchProtocol(XM_BUS_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleNameRoute(mixbusNamesDS, "XM_BUS_CONFIG_NAME", m1);
          return;
        }

        match = matchProtocol(XM_AUX_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleNameRoute(auxNamesDS, "XM_AUX_CONFIG_NAME", m1);
          return;
        }
        if (match == MATCH_EXACT) {
          handleNameRoute(auxNamesDS, "XM_AUX_CONFIG_NAME", 1);   // path fixe XAir
          return;
        }

        match = matchProtocol(XM_FX_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleNameRoute(fxNamesDS, "XM_FX_CONFIG_NAME", m1);
          return;
        }

        match = matchProtocol(XM_MAIN_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_EXACT) {
          handleNameRoute(mainNamesDS, "XM_MAIN_CONFIG_NAME", 1);   // path fixe XAir/X32
          return;
        }

        match = matchProtocol(XM_DCA_CONFIG_NAME, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleNameRoute(dcaNamesDS, "XM_DCA_CONFIG_NAME", m1);   
          return;
        }

        ///////////////////// MUTE STATE ROUTES ////////////////////////
        match = matchProtocol(XM_BUS_MUTE,addr,m1,m2);
        if (match == MATCH_1INT){
          handleBusMute(m1,oscIn.getInt(0));
          return;
        }

        match = matchProtocol(XM_CH_BUS_MUTE, addr, m1, m2);
        if (match == MATCH_2INT) {
          handleChBusMute(m1, m2, oscIn.getInt(0));
          return;
        }

        match = matchProtocol(XM_CH_LR_MUTE, addr, m1, m2);
        if (match == MATCH_1INT) {
          
          // xair and x32 have different way of treating channel mute: Xair cannot mute channel on buses, there is only a main mute (the XM_CH_LR_MUTE = XM_CH_BUS_MUTE), hence this handler may be call by xair to mute a ch
          if (protocolName == XAIR_PROTOCOL) {
            // do the mute "virtually" on all buses
            for (int busIndex=0; busIndex<mixerDef->busCount; busIndex++){
              handleChBusMute(m1, busIndex+1, oscIn.getInt(0));                 // bus index starts at 1 on behringer
            }
          } else if (protocolName == XM_PROTOCOL){      
            handleChMainMute(m1, oscIn.getInt(0));        // normal processing
          }
          return;
        }

        ///////////////////// LEVEL ROUTES ////////////////////////

        match = matchProtocol(XM_CH_BUS_MIX_LEVEL, addr, m1, m2);
        if (match == MATCH_2INT) {
          //fireChBusMixLevel(m1, m2, oscIn.getFloat(0));
          handleChBusMixLevel(m1,m2,oscIn.getFloat(0));
          return;
        }
    }
    ////////////////////////////////
    /// PRISE EN CHARGE DES ROUTES
    ////////////////////////////////
    
    void handleNameRoute(DataSetSynced* ds, const char* label, int index) {
      oscIn.getString(0, pathBuffer);
      //DEBUG_INFO("%s(%d) = %s", label, index, pathBuffer);
      ds->set(index - 1, pathBuffer);
    }

    /**
     * @brief Decode the `/xinfo` discovery reply and select the mixer.
     *
     * The OSC reply contains the console IP, name and model. In discovery
     * mode the source UDP address is authoritative for the destination, while
     * the reported IP is still checked for diagnostics.
     */
    virtual void echo(void* data) override{
      
      // mixer replied to the ping command with some data, we know these data is an OSC message pointer
      NetworkOSC::echo(data);
      OSCMessage* osc = (OSCMessage*) data;

      // we will extract the IP address of the mixer
      IPAddress ip;

      // auto connect method = we have to sets the destination ip and retrieve port information
      if (CONNECT_DISCOVER == connectMethod) {
        osc->getString(0, pathBuffer);   // index 0 = IP address
        if (ip.fromString(pathBuffer)) {
          if (
              ip[0] != destination.ip[0] || ip[1] != destination.ip[1] ||
              ip[2] != destination.ip[2] || ip[3] != destination.ip[3]
          ){
              DEBUG_WARN("Mixer IP differs from UDP packet %s->%s", pathBuffer, UDPIP.toString());
              // we changed the ip address which is a broadcast one to the real ip of the mixer
              destination.ip = UDPIP;     // could be destination.ip = ip but this will not work
          }
        } else {
          DEBUG_ERROR("String reçue mais IP invalide");
        }
        remotePort = UDPPort;
      }

      osc->getString(1, pathBuffer);   // mixer name
      String name = pathBuffer;

      osc->getString(2, pathBuffer);   // mixer model
      String model = mapModelToSeries(pathBuffer);

      setMixer(name, model, destination.ip);
    }

    ///////////////////////////////

    #define XM32series "xm32"
    #define XMR18series "xmr18"
    #define XR12 "xr12"
    #define XR16 "xr16"

    /** @brief Choose the OSC protocol family for the detected mixer model. */
    String mapModelToSeries(const char* model) {
      // based on the token retrieved from the OSC answer, we deduct the right mixer model
      if (model == nullptr) return XM32series;
      String m = String(model);
      m.toUpperCase();
      // X32 / M32
      if (m.indexOf("32") >= 0 ) return XM32series;
      // XR12 
      if (m.indexOf("12") >= 0) return XR12;
      // XR16
      if (m.indexOf("16") >= 0) return XR16;
      // XR18 / X18 / MR18
      if (m.indexOf("18") >= 0 ) return XMR18series;
      // fallback
      return m;
    }

    /**
     * @brief Send the presence probe, alternating discovery ports when needed.
     */
    virtual bool ping() override{
      // in case we are in discovery mode, we alternate the value of the port using 
      if (CONNECT_DISCOVER == connectMethod && state.code != IF_READY){        
        remotePort = discoveryPort[discoveryIndex];        
        discoveryIndex = (discoveryIndex + 1) % DISCOVERY_PORT_COUNT;
      }
      return NetworkOSC::ping();
    }

    /**
     * @brief Start the OSC service, using broadcast for network discovery.
     *
     * Once `/xinfo` identifies a console, `echo()` replaces the broadcast
     * destination with the console's UDP source address.
     */
    virtual bool handleConnectService() override{
      DEBUG_INFO("%s::handleConnectService() set broadcastIP: %s",interfaceName.c_str(),CONNECT_DISCOVER == connectMethod?"yes":"no");

      // when connecting the service, if we are in discovery mode we use a broadcasting IP as destination
      // destination is then set to the real IP extracted from the OSC answer

      // warning: I suspect that if a mixer disconnects => put it offline
      // then put online another mixer 
      // this mecanism will not capture the fact that destination.ip should be put back to broadcast,
      // the monitoring loop will continue with the destination.ip assigned with old ip address and ping will continue pinging in "void"

      if (CONNECT_DISCOVER == connectMethod){
        destination.ip = hwm.network->IP;
        destination.ip[3] = 255;                   //broadcast            
      }

      return NetworkOSC::handleConnectService();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////                                     ////////////////////////////
    ////////////////////////////////// CORE FUNCTION MIXER RELATED REQUEST ////////////////////////////
    //////////////////////////////////                                     ////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    
    ////////////////////////////
    //////// GETTERS ///////////
    ////////////////////////////

    /**
     * @brief Request a dataset name using the profile's OSC address pattern.
     *
     * Common dataset indices are one-based; the profile determines whether the
     * request needs zero, one or two formatted arguments.
     */
    virtual void requestDSname(DataSetSynced* dataset, uint8_t index) override {
      index += 1; // Behringer starts at 1

      uint8_t protocolIndex = 0xFF;

      if (dataset == inputNamesDS) protocolIndex = XM_CH_CONFIG_NAME;
      else if (dataset == mixbusNamesDS) protocolIndex = XM_BUS_CONFIG_NAME;
      else if (dataset == auxNamesDS) protocolIndex = XM_AUX_CONFIG_NAME;
      else if (dataset == fxNamesDS) protocolIndex = XM_FX_CONFIG_NAME;
      else if (dataset == mainNamesDS) protocolIndex = XM_MAIN_CONFIG_NAME;   // no pattern for main, we use the aux one with fixed index 1
      else if (dataset == dcaNamesDS) protocolIndex = XM_DCA_CONFIG_NAME;    // no pattern for dca, we use the aux one with fixed index 1
      else return;

      const char* pattern = protocol[protocolIndex].c_str();

      if (protocolArgCount[protocolIndex] == 0) {
        snprintf(pathBuffer, sizeof(pathBuffer), "%s", pattern);
      } else if (protocolArgCount[protocolIndex] == 1) {
        snprintf(pathBuffer, sizeof(pathBuffer), pattern, index);
      } else {
        DEBUG_ERROR("Unexpected arg count for dataset name request: %d", protocolArgCount[protocolIndex]);
        return;
      }

      sendOSC(pathBuffer);
    }
    
    /** @brief Request a channel-to-bus mute using its two-index OSC pattern. */
    virtual void requestCHBUSmute(uint8_t chIndex, uint8_t busIndex) override{
      // this method drop the request to get a ch mute state on a bus, we need to find the right pattern and replace the 2 placeholders with chIndex and busIndex
      const char* pattern = protocol[XM_CH_BUS_MUTE].c_str();      
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);
      //DEBUG_INFO("%s requestCHBUSmute(%i,%i)",this->interfaceName.c_str(),chIndex,busIndex);
      sendOSC(pathBuffer);
    }

    /** @brief Request a bus mute using the profile's one-index OSC pattern. */
    virtual void requestBUSmute(uint8_t index) override{
      // this method drop the request to get a bus mute state, we need to find the right pattern and replace the placeholder with index
      const char* pattern = protocol[XM_BUS_MUTE].c_str();      
      buildPath1Int(pathBuffer, sizeof(pathBuffer), index, pattern);
      sendOSC(pathBuffer);
    }

    /** @brief Request a channel-to-bus level using its two-index OSC pattern. */
    virtual void requestCHBUSlevel(uint8_t chIndex, uint8_t busIndex) override{
      // this method drop the request to get a ch level on a bus, we need to find the right pattern and replace the 2 placeholders with chIndex and busIndex
      const char* pattern = protocol[XM_CH_BUS_MIX_LEVEL].c_str();      
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);
      sendOSC(pathBuffer);
    }

    ////////////////////////////
    //////// SETTERS ///////////
    ////////////////////////////
    /**
     * @brief Set a channel-to-bus level as an OSC float.
     *
     * X32 OSC fader/send values use normalized floats; when the selected
     * profile does not echo writes, the shared value is mirrored locally.
     */
    virtual void setCHBUSlevel(int chIndex, int busIndex, float level) override{
      // this method sends the command to set a channel level on a bus
      // we need to find the right pattern and replace the 2 placeholders
      const char* pattern = protocol[XM_CH_BUS_MIX_LEVEL].c_str(); 
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);         
      sendOSC(pathBuffer, level, true);   // immediate send
      if (!echoing) allInput2MixbusValues.get(chIndex,busIndex)->value = level; // as X32 is not echoing the command, the value has to be mirrored manually assuming it was changed.
    }

    /**
     * @brief Set a channel-to-bus mute and mirror it when echo is disabled.
     */
    virtual void setCHBUSmute(uint8_t chIndex, uint8_t busIndex, int mute) override{
      // this method sends the command to set a channel mute state on a bus
      // we need to find the right pattern and replace the 2 placeholders
      const char* pattern = protocol[XM_CH_BUS_MUTE].c_str();
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);
      sendOSC(pathBuffer, mute);    
      if (!echoing) allInput2MixbusValues.get(chIndex,busIndex)->toggle = mute; // as X32 is not echoing the command, the value has to be mirrored manually assuming it was changed.
    }

  protected:    

    /** @brief Count numeric placeholders when loading an OSC protocol pattern. */
    uint8_t countPlaceholders(const String& s) {
      uint8_t count = 0;

      for (size_t i = 0; i < s.length(); ++i) {
        if (s[i] == '%') {
          // ignorer %% éventuel
          if ((i + 1) < s.length() && s[i + 1] == '%') {
            ++i;
            continue;
          }
          count++;
        }
      }

      return count;
    }

    enum ProtocolMatchResult {
      MATCH_NONE  = -1,
      MATCH_EXACT = 0,
      MATCH_1INT  = 1,
      MATCH_2INT  = 2
    };

    /**
     * @brief Require a complete OSC address match for the selected protocol pattern.
     *
     * A successful numeric conversion alone is insufficient: `sscanf` can
     * return one even when trailing text does not match. `%n` records the
     * consumed length so partial addresses are rejected.
     * For example, a channel-name pattern must not accept the prefix of a
     * channel mute address.
     */
    int matchProtocol(uint8_t protocolIndex, const char* addr, int& out1, int& out2) {
      if (!addr) return MATCH_NONE;
      if (protocolIndex >= XM_PROOTCOL_COUNT) return MATCH_NONE;

      out1 = -1;
      out2 = -1;

      const char* pattern = protocol[protocolIndex].c_str();
      int endPos = 0;

      switch (protocolArgCount[protocolIndex]) {
        case 0:
          return strEquals(addr, pattern) ? MATCH_EXACT : MATCH_NONE;

        case 1: {
          String fmt = String(pattern) + "%n";
          int n = sscanf(addr, fmt.c_str(), &out1, &endPos);
          if (n == 1 && addr[endPos] == '\0') return MATCH_1INT;
          return MATCH_NONE;
        }

        case 2: {
          String fmt = String(pattern) + "%n";
          int n = sscanf(addr, fmt.c_str(), &out1, &out2, &endPos);
          if (n == 2 && addr[endPos] == '\0') return MATCH_2INT;
          return MATCH_NONE;
        }

        default:
          return MATCH_NONE;
      }
    }

    /**
     * @brief Send an OSC address, optionally with a value.
     *
     * Requests without a value are reads; requests with one OSC argument are
     * writes. `immediate` is used for keepalives and latency-sensitive faders.
     */
    inline void sendOSC(const char* address, bool immediate = false) {
      oscOut.empty();
      oscOut.setAddress(address);
      send(oscOut, immediate);
    }    

    /** @brief Format the protocol address and send the resulting OSC command. */
    template<typename T>
    inline void sendOSC(const char* address, T value, bool immediate = false) {
      oscOut.empty();
      oscOut.setAddress(address);
      oscOut.add(value);
      send(oscOut,immediate);    // send immediately, do not wait for the end of the loop to send the message
    }    

};



#endif
