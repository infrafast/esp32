#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

#define ADDRESS_CONNECT_METHOD "/connect/%d"
#define ADDRESS_MIXER_MODEL "/model/%s"

#define INTERFACE_XMSeries "XMSeries"

enum{
    CONNECT_MANUAL=1,                   // select always start at value 1
    CONNECT_DISCOVER,        
};

class ConnectMethodDS : public DataSet{
  
  public:

    #define ADDRESS_DISCOVERY_SIZE 2

    String options[ADDRESS_DISCOVERY_SIZE]={
        "manual",
        "discovery"
    };     

    ConnectMethodDS(Address* _address): DataSet(_address, INTERFACE_XMSeries, &IFdataRepo){}
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &options[index];    
    }  

    virtual uint16_t getSize(){
      return ADDRESS_DISCOVERY_SIZE;
    }
     
};

#define BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION "mix"

class XMmixerModelDS : public DataSet{
  
  public:
    std::vector<String> mixerModels;

    XMmixerModelDS(Address* _address): DataSet(_address, INTERFACE_XMSeries, &IFdataRepo){       
    }
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()) return &errorDatasetOOR;
      return &mixerModels[index];    
    }  

    void populate(){
      populateFileList(BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION,MIXER_DEFINITION_PATH,&mixerModels,"",false);     // no extension               
      if (getSize()<1) mixerModels.push_back("empty");
    }

    virtual uint16_t getSize(){
      return mixerModels.size();
    }

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

class XMSeries: public MixerDevice<NetworkOSC>{
  public:

    #define XMSERIES_PING_CMD "/xinfo"          //=> maybe go to mixer definition if not the same for all X/M....
    #define XMSERIES_XREMOTE_CMD "/xremote"     // called every 9 seconds

    int connectMethod = CONNECT_DISCOVER;
    
    String protocolName ="xair";
    /*
      protocolArgCount[] intégré, pour que process() sache à l’avance si chaque entrée de protocol[] attend :

      0 argument
      1 argument
      2 arguments

      Ça évite de deviner à chaque appel. Le principe reste compatible avec le fait que protocol[] soit chargé séquentiellement depuis xair.osc ou x32.osc
    */

    String protocol[XM_PROOTCOL_COUNT];         
    uint8_t protocolArgCount[XM_PROOTCOL_COUNT] = {0};

    /* */
    #define DISCOVERY_PORT_COUNT 2 
    int discoveryPort[DISCOVERY_PORT_COUNT] = {10024, 10023};
    uint8_t discoveryIndex = 0;               // rolling index;

    unsigned long lastXremote = 0;          // when di we last drop the xremote command
    #define XREMOTE_INTERVAL  9000           // 9 secondes

    ConnectMethodDS connectMethodDS = ConnectMethodDS(new DropDownAddress(ADDRESS_CONNECT_METHOD));
    XMmixerModelDS mixerModelDS = XMmixerModelDS(new DropDownAddress(ADDRESS_MIXER_MODEL));
    
    XMSeries():MixerDevice<NetworkOSC>(INTERFACE_XMSeries,8888,"192.168.0.1",10023,XMSERIES_PING_CMD,true,GROUP_INTERFACE_MIXER){
      
        unregisterDataSetAddress(&addressPing);              // we un-expose the ping parameter as user should not modify it 

        registerDataSetAddress(connectMethodDS.address);    // add a connect method parameter
        registerDataSetAddress(mixerModelDS.address);       // select mixer when offline

        destination.echoCommand = XMSERIES_PING_CMD;     // instead we set it manually once for all

        mixerDef = new BaseMixerDefinition();

    }     

    // method which is called when sync is finished so we can perform action like subscribing mixer changes...
    virtual void postSync(){
      // do nothing
    }

    virtual void monitorService() override{
      NetworkOSC::monitorService();
      unsigned long now = millis();

      if (now - lastXremote >= XREMOTE_INTERVAL) {
        oscOut.empty();
        oscOut.setAddress(XMSERIES_XREMOTE_CMD);   // si disponible dans ta version
        send(oscOut);    
        lastXremote = now;
      }      
    }

    virtual bool begin() override{
      DEBUG_INFO("%s::begin() => user setting up XMmixerModelDS/model and dataset pattern",interfaceName.c_str());      
      mixerModelDS.populate();        // feeding the list of available mixer definitions for when we are offline and want to select a mixer      
      // check if mixerID.model is in the list, otherwise set it to the first item from it
      if (!mixerModelDS.contains(mixerID.model)){
        const char* firstValidModel = mixerModelDS.getValuePtr(0)->c_str();
        DEBUG_WARN("%s is not a valid model, use %s as default",mixerID.model,firstValidModel);
        setModel(firstValidModel);
      }else setModel(mixerID.model.c_str());       // setting up the last connected mixer
      return true;
    }

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

    virtual bool restoreMixerProfile(){
      DEBUG_INFO("%s::restoreMixerProfile() => deserializing definition and protocol",interfaceName.c_str()); 
      String mixerDefinitionFilename = concatStrings(mixerID.model,".",BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION);
      bool success = deserializeMixerDefinitionFromFile(mixerDefinitionFilename.c_str());
      success &= deserializeMixerProtocolFromFile(protocolName.c_str());
      return success;
    }

    bool deserializeMixerProtocolFromFile(const char* filename) {
    /*
      ouvre le fichier de protocole
      lit chaque ligne avec readStringUntil('\n')
      enlève les éventuels \r et espaces parasites via trim()
      stocke chaque ligne dans protocol[index]
      s’arrête si :
      - le fichier est fini
      - ou si XM_PROOTCOL_COUNT lignes ont été lues    
    */
      #define XMSERIES_PROTOCOL_FILE_EXTENSION ".osc"

      String fullFilename = concatStrings(MIXER_DEFINITION_PATH,filename,XMSERIES_PROTOCOL_FILE_EXTENSION);   
      DEBUG_INFO("%s::deserializeMixerProtocolFromFile(%s) to mixerProtocol",interfaceName.c_str(), fullFilename.c_str());

      File file = LittleFS.open(fullFilename, "r");
      if (!file) {
        DEBUG_ERROR("%s::deserializeMixerProtocolFromFile -> impossible d'ouvrir %s",interfaceName.c_str(), fullFilename.c_str());
        return false;
      }

      /*
        countPlaceholders pour que process() sache à l’avance si chaque entrée de protocol[] attend :
        0 argument
        1 argument
        2 arguments
        Ça évite de deviner à chaque appel.
      */
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


      DEBUG_INFO("channelCount:%u busCOunt:%u auxCount:%u fxCount:%u mainCount:%u mtxCount:%u DCACount:%u fxSndCount:%u groupCount:%u",mixerDef->channelCount, mixerDef->busCount, mixerDef->auxCount, mixerDef->fxCount, mixerDef->mainCount, mixerDef->mtxCount, mixerDef->DCAcount, mixerDef->fxSndCount, mixerDef->groupCount);

      return true;
    }

    virtual void switchDevice(bool _enable) override{
        NetworkDevice::switchDevice(_enable);
        //we capture the device switching because we do not display the same properties if we are enable or disable (see serializeProperties), hence have to broadcast the change
        state.broadcastConfig = true;
    }

    virtual void handlePropertyChange(){
      NetworkOSC::handlePropertyChange();
      DEBUG_INFO("%s::handlePropertyChange() refresh config",interfaceName.c_str());    
      state.broadcastConfig = true; // it could be that the connect method has changed so we need to redisplay/hide the configuration fields
      if (state.enabled) handleConnectService();
    }    

    // used to extract values from the path    
    #define STRING_MAX_SIZE 64
    char pathBuffer[STRING_MAX_SIZE];
    int i1,i2;    // index

        //////////////////////////////////////////////////////////////////////////////////////////////////
       ////////////////////////////////                                       ///////////////////////////
      ///////////////////////////////// READ INPUT OSC MESSAGE AND ROUTE THEM //////////////////////////
     //////////////////////////////////                                       /////////////////////////    
    //////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void process() override {    

        const char* addr = oscIn.getAddress();        
        if (!addr) return;

        int m1 = -1;
        int m2 = -1;
        int match = MATCH_NONE;

        ////////////////////// PING ////////////////////////
        if (strEquals(addr, XMSERIES_PING_CMD)) {    
          IPAddress ip;

          if (CONNECT_DISCOVER == connectMethod) {
            oscIn.getString(0, pathBuffer);   // index 0 = IP address
            if (ip.fromString(pathBuffer)) {
              if (
                  ip[0] != destination.ip[0] || ip[1] != destination.ip[1] ||
                  ip[2] != destination.ip[2] || ip[3] != destination.ip[3]
              ){
                  DEBUG_WARN("Mixer IP differs from UDP packet %s->%s", pathBuffer, UDPIP.toString());
                  // destination.ip = ip;
              }
            } else {
              DEBUG_ERROR("String reçue mais IP invalide");
            }

            remotePort = UDPPort;
          }

          oscIn.getString(1, pathBuffer);   // mixer name
          String name = pathBuffer;

          oscIn.getString(2, pathBuffer);   // mixer model
          String model = mapModelToSeries(pathBuffer);

          setMixer(name, model, destination.ip);
          return;
        }

        ////////////////////// CONFIG NAME ROUTES ////////////////////////

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

        ////////////////////// MUTE STATE ROUTES ////////////////////////

        match = matchProtocol(XM_CH_BUS_MUTE, addr, m1, m2);
        if (match == MATCH_2INT) {
          handleMuteEventChBus(m1, m2, oscIn.getInt(0));
          return;
        }

        match = matchProtocol(XM_CH_LR_MUTE, addr, m1, m2);
        if (match == MATCH_1INT) {
          handleMuteEventChMain(m1, oscIn.getInt(0));
          return;
        }

        ////////////////////// LEVEL ROUTES ////////////////////////

        match = matchProtocol(XM_CH_BUS_MIX_LEVEL, addr, m1, m2);
        if (match == MATCH_2INT) {
          handleChBusMixLevel(m1, m2, oscIn.getFloat(0));
          return;
        }
    }
    /////////////////////////////////
    /// PRISE EN CHARGE DES ROUTES
    /////////////////////////////////
    void handleChBusMixLevel(int ch, int bus, float level) {
      /*payload[0] = STRIP_CH_BUS_MIX_LEVEL;
      payload[1] = ch;
      payload[2] = bus;
      payload[3] = level;
      fireEvent(evtMix);*/
      inputNamesDS->values[ch-1] = level;   // update the ch level on the bus in the dataset, the strip will capture this change and update the fader level
    }

    void handleNameRoute(DataSetSynced* ds, const char* label, int index) {
      oscIn.getString(0, pathBuffer);
      //DEBUG_INFO("%s(%d) = %s", label, index, pathBuffer);
      ds->set(index - 1, pathBuffer);
    }

    void handleMuteEventChMain(int ch, int muteState) {
      /*payload[0] = STRIP_CH_MAIN_MUTE;
      payload[1] = ch;
      payload[2] = muteState;
      fireEvent(evtMix);*/
      mainNamesDS->mutes[ch-1] = muteState;   // update the ch mute state on the main in the dataset, the strip will capture this change and update the toggle state
    }

    void handleMuteEventChBus(int ch, int bus, int muteState) {
      /*payload[0] = STRIP_CH_BUS_MUTE;
      payload[1] = ch;
      payload[2] = bus;
      payload[3] = muteState;
      fireEvent(evtMix);*/
      inputNamesDS->mutes[ch-1] = muteState;   // update the ch mute state on the bus in the dataset, the strip will capture this change and update the toggle state


    }

    ////////////////////////////////

    #define XM32series "xm32"
    #define XMR18series "xmr18"
    #define XR12 "xr12"
    #define XR16 "xr16"

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

    virtual bool ping() override{
      // in case we are in discovery mode, we alternate the value of the port using 
      if (CONNECT_DISCOVER == connectMethod && state.code != IF_READY){        
        remotePort = discoveryPort[discoveryIndex];        
        discoveryIndex = (discoveryIndex + 1) % DISCOVERY_PORT_COUNT;
      }
      return NetworkOSC::ping();
    }

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

    /////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////                                     ////////////////////////////
    /////////////////////////////////// CORE FUNCTION MIXER RELATED REQUEST ////////////////////////////
    ///////////////////////////////////                                     ////////////////////////////    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    
    /////////////////////////////
    ///////// GETTERS ///////////
    /////////////////////////////
    virtual void getDSname(DataSetSynced* dataset, uint8_t index) override {
      /*
        Puisque certains patterns peuvent avoir 0 argument, 1 argument, etc., tu peux aussi éviter d’envoyer un index inutile si le pattern est fixe :    
      */
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
    
    virtual void getCHBUSmute(uint8_t chIndex, uint8_t busIndex) override{
      // this method drop the request to get a ch mute state on a bus, we need to find the right pattern and replace the 2 placeholders with chIndex and busIndex
      const char* pattern = protocol[XM_CH_BUS_MUTE].c_str();      
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);

      sendOSC(pathBuffer);
    }

    virtual void getBUSmute(uint8_t index) override{
      // this method drop the request to get a bus mute state, we need to find the right pattern and replace the placeholder with index
      const char* pattern = protocol[XM_BUS_MUTE].c_str();      
      buildPath1Int(pathBuffer, sizeof(pathBuffer), index, pattern);

      sendOSC(pathBuffer);
    }

    virtual void getCHBUSlevel(uint8_t chIndex, uint8_t busIndex) override{
      // this method drop the request to get a ch level on a bus, we need to find the right pattern and replace the 2 placeholders with chIndex and busIndex
      const char* pattern = protocol[XM_CH_BUS_MIX_LEVEL].c_str();      
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);

      sendOSC(pathBuffer);
    }

    /////////////////////////////
    ///////// SETTERS ///////////
    /////////////////////////////
    virtual void setCHBUSlevel(uint8_t chIndex, uint8_t busIndex, float level) override{
      // this method sends the command to set a channel level on a bus
      // we need to find the right pattern and replace the 2 placeholders
      const char* pattern = protocol[XM_CH_BUS_MIX_LEVEL].c_str();
      buildPath2Int(pathBuffer, sizeof(pathBuffer), chIndex, busIndex, pattern);

      sendOSC(pathBuffer, level);
    }


  protected:
    /////////////////////////////////// UTILITIES ////////////////////////////

    //construit la chaîne en remplaçant les placeholders %d et %s
    void buildPath1Int(char* destination, size_t bufferSize, int index, const char* pattern) {
        snprintf(destination, bufferSize, pattern, index);
    }   
    
    void buildPath2Int(char* destination, size_t bufferSize, int index1, int index2, const char* pattern) {
        snprintf(destination, bufferSize, pattern, index1, index2);
    }       

    /*
      pré-analyser chaque pattern au chargement des protocoles
      mémoriser si le pattern a 0, 1 ou 2 placeholders    
    */
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

    /*
        sscanf("/ch/01/mix/02/on", "/ch/%02d/config/name", &i1)

        sscanf :

        lit /ch/
        lit %02d → i1 = 1
        arrive sur /mix/02/on
        attend /config/name
        ça ne matche plus

        Mais comme un champ a déjà été assigné, la valeur de retour est 1.
        Du coup ton test :

        if (sscanf(addr, protocol[XM_CH_CONFIG_NAME].c_str(), &i1) == 1)

        passe à tort.

      D'ou l'implementation  exigeant que toute la chaîne soit consommée et l'utilisation de %n pour vérifier que le pattern correspond à l'intégralité de la chaîne.
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

    ////////////////////////////
    inline void sendOSC(const char* address) {
      oscOut.empty();
      oscOut.setAddress(address);
      send(oscOut);
    }    

    template<typename T>
    inline void sendOSC(const char* address, T value) {
      oscOut.empty();
      oscOut.setAddress(address);
      oscOut.add(value);
      send(oscOut);
    }    

};



#endif