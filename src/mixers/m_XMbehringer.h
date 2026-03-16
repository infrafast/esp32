#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

#define ADDRESS_CONNECT_METHOD "/connect/%d"
#define ADDRESS_MIXER_MODEL "/model/%s"
#define INTERFACE_BEHRINGERXM "BehringerXM"

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

    ConnectMethodDS(Address* _address): DataSet(_address, INTERFACE_BEHRINGERXM, &IFdataRepo){}
    
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

    XMmixerModelDS(Address* _address): DataSet(_address, INTERFACE_BEHRINGERXM, &IFdataRepo){       
    }
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()) return &errorDatasetOOR;
      return &mixerModels[index];    
    }  

    void populate(){
      populateFileList(BEHRINGER_MIXER_PROFILE_FILENAME_EXTENSION,MIXER_DEFINITION_PATH,&mixerModels,"",false);     // no extension               
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



class BehringerXM: public MixerDevice<NetworkOSC>{
  public:

    #define XM_BEHRINGER_PING_CMD "/xinfo"    //=> maybe go to mixer definition if not the same for all X/M....

    int connectMethod = CONNECT_DISCOVER;
    String protocol;

    /* */
    #define DISCOVERY_PORT_COUNT 2 
    int discoveryPort[DISCOVERY_PORT_COUNT] = {10024, 10023};
    uint8_t discoveryIndex = 0;               // rolling index;

    ConnectMethodDS connectMethodDS = ConnectMethodDS(new DropDownAddress(ADDRESS_CONNECT_METHOD));
    XMmixerModelDS mixerModelDS = XMmixerModelDS(new DropDownAddress(ADDRESS_MIXER_MODEL));
    
    BehringerXM():MixerDevice<NetworkOSC>(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,XM_BEHRINGER_PING_CMD,true,GROUP_INTERFACE_MIXER){
      
        unregisterDataSetAddress(&addressPing);              // we un-expose the ping parameter as user should not modify it 

        registerDataSetAddress(connectMethodDS.address);    // add a connect method parameter
        registerDataSetAddress(mixerModelDS.address);       // select mixer when offline

        destination.echoCommand = XM_BEHRINGER_PING_CMD;     // instead we set it manually once for all

    }     

    virtual bool begin() override{
      DEBUG_INFO("%s::begin() => user setting up XMmixerModelDS",interfaceName.c_str());      
      mixerModelDS.populate();        // feeding the list of available mixer definitions for when we are offline and want to select a mixer      

      // check if mixerID.model is in the list, otherwise set it to the first item from it
      if (!mixerModelDS.contains(mixerID.model)){
        const char* firstValidModel = mixerModelDS.getValuePtr(0)->c_str();
        DEBUG_WARN("%s is not a valid model, use %s as default",mixerID.model,firstValidModel);
        setModel(firstValidModel);
        return true;
      }
      
      return restoreMixerProfile();       // setting up the last connected mixer
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
      success &= deserializeMixerProtocolFromFile(protocol.c_str());
      return success;
    }

    bool deserializeMixerProtocolFromFile(const char* filename){
      DEBUG_INFO("%s::deserializeMixerProtocolFromFile(%s) to mixerProtocol",interfaceName.c_str(),filename); 
      return true;
    }

    virtual bool deserializeMixerDefinitionFromFile(const char* filename){
      DEBUG_INFO("%s:deserializeMixerDefinitionFromFile(%s)",this->interfaceName.c_str(),filename);       
      // ...  restore mixerDef

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

      mixerDef.channelCount = doc["channelCount"] | mixerDef.channelCount;
      mixerDef.busCount     = doc["busCount"]     | mixerDef.busCount;
      mixerDef.auxCount     = doc["auxCount"]     | mixerDef.auxCount;
      mixerDef.fxCount      = doc["fxCount"]      | mixerDef.fxCount;
      protocol              = doc["protocol"]     | protocol;

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
    }    

    #define STRING_MAX_SIZE 64
    char stringBuffer[STRING_MAX_SIZE];

    virtual void process() override {

        const char* addr = osc.getAddress();
        if (!addr) return;

        // Routes fixes
        if (strEquals(addr, XM_BEHRINGER_PING_CMD)) {    

            // in case we are in discovery mode, we need to retrieve the destination IP and port from the UDP connection established with the mixer
            if (CONNECT_DISCOVER == connectMethod){
                /* 1️⃣ retrieve mixer IP
                    method: DISCOVER => Always use the IP of the Mixer
                    method: MANUAL =>  if UDP IP differs from Mixer, use UDP IP
                */
                osc.getString(0, stringBuffer);         // index 0 = IP address
                IPAddress ip;
                if (ip.fromString(stringBuffer)) {
                // 2️⃣ if the IP returned from the mixer differs from the on in the UDP packet,
                // this happens for instance when the mixer is on a different network (emulator on DSM for example)            
                // however we ignore and keep the address from the user if it is different but it was not a discovery
                // which would means in that case that there are some ip masquerading and we really ened to keep the one provided by user
                if (
                    UDPIP[0] != mixerID.ip[0] || mixerID.ip[1] != mixerID.ip[1] || mixerID.ip[2] != mixerID.ip[2] || UDPIP[3] != mixerID.ip[3]
                ){
                    DEBUG_WARN("Mixer IP differs from UDP packet, switching to UDP IP instead. %s->%s",stringBuffer,UDPIP.toString());
                    ip = UDPIP;                      
                }
                //DEBUG_PRINT("\nPinging Mixer (%s) to retrieve latency from dynamic IP",mixerIP.toString().c_str());
                //blockingPing(mixerIP); // we retrieve the real latency, now that we have the mixer address      
                } else {
                    DEBUG_ERROR("String reçue mais IP invalide");
                }
                    
                // 3️⃣ check if mixerIP is different than the current identified destination =>  
                if (destination.ip != ip){
                    remotePort = UDPPort;               // we use the port from the UDP socket that successfully connected the mixer
                    destination.ip = ip;
                }

            }     

            osc.getString(1, stringBuffer);     // index 1 = name
            String name = stringBuffer;   // used by mixerHasChanged to perform the comparison against mixerID.name. Hence we can detect a change

            osc.getString(2, stringBuffer);     // index 2 = model
            String model = stringBuffer;  // used by mixerHasChanged to perform the comparison against mixerID.name. Hence we can detect a change

            if (isNewMixer(name,model,destination.ip))  state.broadcastConfig = true;  
            
        }
        else
            fireEvent(evtOSC);      // propage les données      
    }

    virtual void startMixerDataSync() override{
        // do whatever is required here      
        setStatePendingDataSync();                      
    }

    virtual bool ping() override{
      // in case we are in discovery mode, we alternate the value of the port using 
      if (CONNECT_DISCOVER == connectMethod){        
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
    


};



#endif