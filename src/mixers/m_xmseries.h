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
  XM_CH_CONFIG_NAME,
  XM_BUS_CONFIG_NAME,
  XM_AUX_CONFIG_NAME,
  XM_FX_CONFIG_NAME,
  XM_CH_BUS_MIX_LEVEL,
  XM_PROOTCOL_COUNT
};

class XMSeries: public MixerDevice<NetworkOSC>{
  public:

    #define XMSERIES_PING_CMD "/xinfo"          //=> maybe go to mixer definition if not the same for all X/M....
    #define XMSERIES_XREMOTE_CMD "/xremote"     // called every 9 seconds

    int connectMethod = CONNECT_DISCOVER;
    
    String protocolName ="xair";
    String protocol[XM_PROOTCOL_COUNT];         

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
      String fullFilename = concatStrings(MIXER_DEFINITION_PATH,filename);   
      DEBUG_INFO("%s::deserializeMixerProtocolFromFile(%s) to mixerProtocol",interfaceName.c_str(), fullFilename.c_str());

      File file = LittleFS.open(fullFilename, "r");
      if (!file) {
        DEBUG_ERROR("%s::deserializeMixerProtocolFromFile -> impossible d'ouvrir %s",interfaceName.c_str(), fullFilename.c_str());
        return false;
      }

      int index = 0;
      while (file.available() && index < XM_PROOTCOL_COUNT) {
        String line = file.readStringUntil('\n');
        // Nettoyage des fins de ligne Windows (\r\n)
        line.trim();
        protocol[index] = line;
        DEBUG_INFO("protocol[%d] = %s", index, protocol[index].c_str());
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
      DEBUG_INFO("%s:deserializeMixerDefinitionFromFile(%s)",this->interfaceName.c_str(),filename);       

      String fullFilename = concatStrings(MIXER_DEFINITION_PATH,filename); 

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
      protocolName           = doc["protocol"]     | protocolName;

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
      handleConnectService();
    }    

    // used to extract values from the path    
    #define STRING_MAX_SIZE 64
    char stringBuffer[STRING_MAX_SIZE];
    int i1,i2;
    float f1;

    virtual void process() override {
      

        // fonction principale de routage
        const char* addr = oscIn.getAddress();        
        if (!addr) return;

     
        // Routes fixes
        if (strEquals(addr, XMSERIES_PING_CMD)) {    
          IPAddress ip;
          // in case we are in discovery mode, we need to retrieve the destination IP and port from the UDP connection established with the mixer
          if (CONNECT_DISCOVER == connectMethod){
            /* 1️⃣ retrieve mixer IP
                method: DISCOVER => Always use the IP of the Mixer
                method: MANUAL =>  if UDP IP differs from Mixer, use UDP IP
            */
            oscIn.getString(0, stringBuffer);         // index 0 = IP address
            if (ip.fromString(stringBuffer)) {
            // 2️⃣ if the IP returned from the mixer differs from the destinationIP (which is the one from UDP packet)
            // this happens for instance when the mixer is on a different network (emulator on DSM for example)            
            if (
                ip[0] != destination.ip[0] || ip[1] != destination.ip[1] || ip[2] != destination.ip[2] || ip[3] != destination.ip[3]
            ){
                DEBUG_WARN("Mixer IP differs from UDP packet %s->%s",stringBuffer,UDPIP.toString());
                // if we are on a different subnet, then we have to keep the destination ip we used to connect                  
                //destination.ip = ip;
            }
            //DEBUG_PRINT("\nPinging Mixer (%s) to retrieve latency from dynamic IP",mixerIP.toString().c_str());
            //blockingPing(mixerIP); // we retrieve the real latency, now that we have the mixer address      
            } else {
                DEBUG_ERROR("String reçue mais IP invalide");
            }   
            
            remotePort = UDPPort;               // we use the port from the UDP socket that successfully connected the mixer

          }     

          oscIn.getString(1, stringBuffer);     // index 1 = name
          String name = stringBuffer;   // used by mixerHasChanged to perform the comparison against mixerID.name. Hence we can detect a change

          oscIn.getString(2, stringBuffer);     // index 2 = model
          String model = mapModelToSeries(stringBuffer);  // used by mixerHasChanged to perform the comparison against mixerID.name. Hence we can detect a change

          setMixer(name,model,destination.ip);
        
        }else if (sscanf(addr, protocol[XM_CH_BUS_MIX_LEVEL].c_str(), &i1, &i2) == 2) {
          f1 = oscIn.getFloat(0);
          DEBUG_INFO("XM_CH_BUS_MIX_LEVEL(%d,%d) = %.2f",i1,i2,f1);
          //fireEvent();
        }else if (sscanf(addr, protocol[XM_CH_CONFIG_NAME].c_str(), &i1) == 1) {
          oscIn.getString(0, stringBuffer);
          DEBUG_INFO("XM_CH_CONFIG_NAME(%d) = %s",i1,stringBuffer);   // i1 = channel id, stringBuffer = channel name
          chNamesDS->set(i1-1,stringBuffer);        // set the data, ID starts at 0 in datasets
        }else if (sscanf(addr, protocol[XM_BUS_CONFIG_NAME].c_str(), &i1) == 1) {
          oscIn.getString(0, stringBuffer);
          DEBUG_INFO("XM_BUS_CONFIG_NAME(%d) = %s",i1,stringBuffer);   // i1 = bus id, stringBuffer = bus name
          busNamesDS->set(i1-1,stringBuffer);        // set the data, ID starts at 0 in datasets
        }else if (sscanf(addr, protocol[XM_AUX_CONFIG_NAME].c_str(), &i1) == 1) {
          oscIn.getString(0, stringBuffer);
          DEBUG_INFO("XM_AUX_CONFIG_NAME(%d) = %s",i1,stringBuffer);   // i1 = aux id, stringBuffer = aux name
          auxNamesDS->set(i1-1,stringBuffer);        // set the data, ID starts at 0 in datasets
        }else if (sscanf(addr, protocol[XM_FX_CONFIG_NAME].c_str(), &i1) == 1) {
          oscIn.getString(0, stringBuffer);
          DEBUG_INFO("XM_FX_CONFIG_NAME(%d) = %s",i1,stringBuffer);   // i1 = fx id, stringBuffer = fx name
          fxNamesDS->set(i1-1,stringBuffer);        // set the data, ID starts at 0 in datasets
        }
         
    }


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

    /////////////////////////////////// MIXER RELATED REQUEST ////////////////////////////
    virtual void dropDSnameRequest(DataSetSynced* dataset, uint8_t index) override{
      // this method drop the request to get a name

      index += 1 ; // always starts at 1 for Behringer

      char path[64];
      const char* pattern;      

      if (dataset == chNamesDS) pattern = protocol[XM_CH_CONFIG_NAME].c_str();
      else if (dataset == busNamesDS) pattern = protocol[XM_BUS_CONFIG_NAME].c_str();
      else if (dataset == auxNamesDS) pattern = protocol[XM_AUX_CONFIG_NAME].c_str();
      else if (dataset == fxNamesDS) pattern = protocol[XM_FX_CONFIG_NAME].c_str();

      buildPathIndex(path, sizeof(path), index, pattern);

      oscOut.empty();
      oscOut.setAddress(path);   // si disponible dans ta version
      send(oscOut);

      //DEBUG_INFO("🔄%s",path);
    }
    
    //construit la chaîne en remplaçant les placeholders %d et %s
    void buildPathIndex(char* destination, size_t bufferSize, int index, const char* pattern) {
        snprintf(destination, bufferSize, pattern, index);
    }    

  protected:

};



#endif