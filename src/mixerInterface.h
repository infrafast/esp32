#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#include "oscInterface.h"
#include <ArduinoJson.h>
#include <vector>

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define ADDRESS_CHNAME "/ch/config/%d/name"
#define ADDRESS_BUSNAME "/bus/config/%d/name"


/***************************************************************/
/*********************** BEHRINGER DEFINITIONS ******************/
/***************************************************************/
#define BEHRINGER_RESTART_PERIOD    9000     
#define BEHRINGER_PING_DELAY        3000   

#define BEHRINGER_PING          "/info"
#define BEHRINGER_QUERY         "/xinfo"
#define OSC_BEHRINGER_REMOTE    "/xremote"


/*********************************************************************/
/*********************** MIXING STATION DEFINITIONS ******************/
/*******************************************************************/
#define MIXING_STATION_MODEL "MixingStation"
#define MS_RESTART_PERIOD     3000     
#define MS_PING_DELAY         3000     

#define OSC_MS_FORMAT "n"            // v => plain The actual value of the parameter (e.g. -5dB)    n => normalized The value is normalized within the range 0-1
#define OSC_MS_REMOTE       "/hi/" OSC_MS_FORMAT
#define OSC_MS_PRE          "/con/" OSC_MS_FORMAT
#define OSC_MS_PING         "/ch.0.solo"


//               REST API
#define RESTCALL_APP_STATE        "/app/state"
#define RESTCALL_CONSOLE_INFO     "/console/information"
#define RESTCALL_CONNECT          "/app/mixers/connect"
#define RESTCALL_AVAILABLE_MODELS "/app/mixers/available"
#define RESTCALL_MIXTARGETS       "/console/mixTargets"
#define RESTCALL_MIXER_CURRENT    "/app/mixers/current"

//              JSON API DATA CONTENT DEFINITION
#define MS_CHANNEL_TYPES                  "channelTypes"
  #define MS_TYPE_SHORTNAME_KEY   "shortName"
    #define MS_CH_SHORTNAME               "Ch"
    #define MS_BUS_SHORTNAME               "Mix"
/*********************************************************************/
/*********************** JSON WEBAPP DEFINITION ******************/
/*******************************************************************/

#define INTERFACE_STATE_KEY   "interfaceState"
  #define MIXER_PORT_KEY      "mixerPort"
  #define MIXER_CONNECTED_KEY "mixerConnected"
  #define MIXER_AUTOMODEL_KEY "autoConnectModel"
  #define MIXER_AUTOIP_KEY    "autoConnectIP"

/*********************************************************************/
/*********************** JSON INTERFACE DEFINITIONS ******************/
/*******************************************************************/
#define INTERFACES_FILENAME   "/webapp/interfaces.json"
#define INTERFACES_FILENAMEGZ INTERFACES_FILENAME ".gz"
#define INTERFACE_KEY         "interfaces"
#define MODEL_KEY             "model"
  #define CH_SIZE_KEY         "chSize"
  #define BUS_SIZE_KEY        "busSize"
  #define PROTOCOL_KEY        "protocol"
#define CONSOLE_KEY           "consoles"
  #define CONSOLEID_KEY       "consoleId"
#define NAME_KEY              "name"
#define PROTOCOLS_KEY         "protocols"


/*********************************************************************/
/*********************** INTERFACE STATES DEFINITIONS ******************/
/*******************************************************************/
enum{
  IDLE = CONNECTED+1,
  CONNECTING,
  SYNCING_MS,
  MIXER_UNREACHABLE,
  MS_UNREACHABLE,
  SEARCHING,
  CONNECTFAILED,
  SYNCING_CONSOLE,
  EOF_STATE
};

String stateNames[EOF_STATE]={
  "disconnected",
  "connected",
  "idle",
  "connecting",
  "syncing",
  "console unreachable",
  "mixing s. unreachable",
  "searching",
  "connect failed",
  "syncing console"
};

#define UNSYNC_PATTERN "~"

class DataSetPattern: public DataSet{
  
  public:
    String* values = nullptr;
    uint16_t size = 0;
    String pattern = "";

    DataSetPattern(String addr,String obj, DataProvider* dp) : DataSet(addr,obj, dp){}

    void init(uint16_t _size,String defVal){
      size = _size;
      DEBUG_PRINT("\nDataSetPattern init(%d,%s)",size,defVal.c_str());
      // Supprimer le tableau existant s'il y en a un
      if (values != nullptr) delete[] values; // Libérer la mémoire précédemment allouée
      // Créer un nouveau tableau dynamique avec la taille passée
      values = new String[size];
      // Initialiser les valeurs (par exemple, 0)
      for (int i = 0; i < size; ++i) {
          values[i] = defVal+String(i);
      }
    }
};

// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface : public DataProvider{
  public:
    String model;
    DataRepository* dataRepository = nullptr;
    String name="abstract";
    uint8_t interfaceState = DISCONNECTED;
    String OSCprotocol="";

    String unsyncPattern="~";

    float progress = 0.0;  // connection progression indicator in %

    virtual void queryState()=0;

    DataSetPattern* chNames;
    DataSetPattern* busNames;  

    MixerInterface(DataRepository* dr,String _model){  
      if (_model == "") model = MIXING_STATION_MODEL;
      else model = _model;          

      DEBUG_PRINT("\nnew MixerInterface(%s)",model.c_str());

      dataRepository = dr;

    };

    void init(){
      JsonDocument interfaceDoc;
      deserialize(&interfaceDoc);
      parseInterfaceProtocol(interfaceDoc);      
    }

    virtual void parseInterfaceProtocol(JsonDocument& interfaceDoc){
      // getting the properties of the interface in the json document
      //DEBUG_PRINT("\n\n\n\tAPPEL DE BASE");
      uint16_t chSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,CH_SIZE_KEY,"6").toInt();      
      uint16_t busSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,BUS_SIZE_KEY,"2").toInt();

      chNames = new DataSetPattern(ADDRESS_CHNAME,INPUTSELECT,this);
      busNames = new DataSetPattern(ADDRESS_BUSNAME,INPUTSELECT,this);

      chNames->init(chSize,UNSYNC_PATTERN+String("CH"));
      busNames->init(busSize,UNSYNC_PATTERN+String("BUS"));

      //DEBUG_PRINT("\n\n\n");

      dataRepository->addDataSet(chNames);
      dataRepository->addDataSet(busNames);        

    }

    virtual String getValue(String addrr, uint16_t index){
      String v=String(index)+"?";
      if (addrr == ADDRESS_CHNAME){
        if (index<chNames->size) v = chNames->values[index];
        else v="CH OOR-"+String(index);
      }else if (addrr == ADDRESS_BUSNAME){
        if (index<busNames->size) v = busNames->values[index];
        else v="BUS OOR-"+String(index);
      } 
      return v;
    }

    void deserialize(JsonDocument* interfaceDoc){
      
      ByteBuffer decompressedJson;
      size_t dataSize = 0;

      String interfaceFileNameGz = INTERFACES_FILENAME+String(".gz");      
      dataSize = readGzFileToBuffer(interfaceFileNameGz.c_str(),&decompressedJson);
      if (dataSize == 0){ // uncompress failed
        DEBUG_PRINT("\nFailed to decompress JSON interface file %s, using non compressed file",INTERFACES_FILENAME);
        File file = LittleFS.open(INTERFACES_FILENAME,"r");
        if (!file) {
            DEBUG_PRINT("\nFailed to open JSON interface file");
            file.close(); // Fermer le fichier
            return;
        }else{
          auto error = deserializeJson(*interfaceDoc, file);
          if (error) {
              DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());             
              file.close(); // Fermer le fichier
              return;
          }
          file.close(); // Fermer le fichier          
        }       
      }else{
        // Désérialiser les données JSON
        auto error = deserializeJson(*interfaceDoc, decompressedJson.buffer);
        if (error) {
            DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());
            return;
        }        
      }      
    }


    virtual ~MixerInterface(){

      dataRepository->removeDataSet(chNames);
      delete chNames;

      dataRepository->removeDataSet(busNames);      
      delete busNames;

      //DEBUG_PRINT("\n\n~MixerInterface()");
    }

    uint16_t size(String addrr){
      uint16_t n=0;
      if (addrr == ADDRESS_CHNAME) return chNames->size;
      else if (addrr == ADDRESS_BUSNAME) return busNames->size;
      else return 0;
      //DEBUG_PRINT("\nSizeof(BehringerMixer,%s)=%i",addrr.c_str(),n);
    }

    virtual bool update()=0;
    
    String getAdjacentValueByKeyFromRootArray(JsonDocument& doc, String root, String key, String keyValue, String adjacent, String defaultValue="") {  // key = "model" , adjacent = "size"
        // Vérifier si le document contient une clé "consoles"
        if (!doc[root].is<JsonArray>()) {
            DEBUG_PRINT("\nRoot '%s' not found or not an array in JSON",root.c_str());
            return defaultValue;
        }

        // Parcourir le tableau "root"
        JsonArray array = doc[root];
        for (JsonObject elem : array) {
            // Vérifier si la clé "name" correspond
            if (elem[key] == keyValue) {
                // Retourner la valeur de la clé spécifiée si elle existe
                if (elem.containsKey(adjacent.c_str())) {
                    String s = elem[adjacent.c_str()];
                    //DEBUG_PRINT("\n%s:%s = %s",keyValue.c_str(),adjacent.c_str(),s.c_str());
                    return s;
                } else {
                    DEBUG_PRINT("\nadjacent:%s   not found.",adjacent.c_str());
                    return defaultValue;
                }
            }
        }

        DEBUG_PRINT("\n<%s>=<%s> not found, returning default %s = %s",keyValue.c_str(),key.c_str(),adjacent.c_str(),defaultValue.c_str());
        return defaultValue;
    }


    // Fonction pour modifier la clé adjacente dans le JsonDocument
    // modifyAdjacentKeyValueIntOfRootArray(doc, "interfaces", "model", "XR12", BUS_SIZE_KEY, 6);

    void modifyAdjacentKeyValueIntOfRootArray(JsonDocument& doc, String root, String modelKey, String modelValue, String key, int newValue) {
      // Parcours des interfaces dans le tableau
      JsonArray interfaces = doc[root];
      for (JsonObject interfaceObj : interfaces) {
        // Vérifie si le modèle correspond
        if (interfaceObj[modelKey] == modelValue) {
          // Vérifie si la clé existe
          if (interfaceObj.containsKey(key)) {
            // Modifie la valeur de la clé spécifiée
            interfaceObj[key] = newValue;
            DEBUG_PRINT("\nValeur modifiée avec succès !");
            return;
          } else {
            DEBUG_PRINT("\nClé non trouvée !");
            return;
          }
        }
      }
      DEBUG_PRINT("\nModèle non trouvé !");
    }


/*
    std::vector<String> getAllValuesByKey(const String& key) {
        std::vector<String> values;

        // Vérifier si le document contient une clé "consoles"
        if (!doc["consoles"].is<JsonArray>()) {
            Serial.println("Key 'consoles' not found or not an array in JSON");
            return values;
        }

        // Parcourir le tableau "consoles"
        JsonArray consoles = doc["consoles"];
        for (JsonObject console : consoles) {
            // Ajouter la valeur de la clé spécifiée si elle existe
            if (console.containsKey(key.c_str())) {
                values.push_back(String(console[key.c_str()].as<const char*>()));
            }
        }

        return values;
    }
*/


/*
    // Exemple d'utilisation : obtenir toutes les valeurs d'une clé
    std::vector<String> names = parser.getAllValuesByKey("name");
    Serial.println("All console names:");
    for (const auto& name : names) {
        Serial.println(name);
    }
*/

    virtual void connect(Config* conf)=0;
    virtual bool sendCommand(String command, JsonDocument param) = 0;
    virtual void disconnect()=0;

  protected:
    //uint16_t chSize=1,busSize=1;    

};


class OSCMixerInterface: public MixerInterface {

  public: 
    static String echoCommand;    
    bool OSClive = true;

    // offset for bus, chanel and other  names
    uint8_t chOffset = 0;
    uint8_t busOffset = 0;


    struct DataSetsSync {
        uint8_t ch = 0;
        uint8_t bus = 0;
        OSCMixerInterface* parent;

        DataSetsSync(OSCMixerInterface* _parent):parent(_parent){}

        uint8_t progress(){
          uint16_t totalToSync = parent->chNames->size + parent->busNames->size;
          uint16_t syncedSoFar = ch + bus;
          return map(syncedSoFar, 0, totalToSync, 0, 100);
        }
    };

    DataSetsSync dataSetSync;

    OSCMixerInterface(DataRepository* dr,String m, OSCInterface* _osc): osc(_osc), MixerInterface(dr,m),dataSetSync(this){
      DEBUG_PRINT("\nnew OSCMixerInterface(%s %s,%d)",model.c_str(),osc->ip.toString().c_str(),osc->port);
    }

    virtual void syncDataSet()=0;

    virtual bool monitor(){

      // restart monitoring value
      if (interfaceState == CONNECTED){
        if (millis()-lastRestartMonitorTime > restartMonitorDelay){
          osc->SendCmd2OSC(restartMonitorCommand.c_str(), false);          // otherwise we ask mixer to send message for the next 10 sec (see behringer protocol)
          lastRestartMonitorTime = millis();
        }
      }

      // processing OSC queue
      if (osc->OSCReadIn()){
        lastOSCCheck = millis();
        lastQueryTime = millis();   // REMOVE THIS ONE IF YOU STILL WANT TO CHECK REST API WHEN OSC IS RESPONDING (CONSUMME MORE TIME)        
        OSClive = true;
        //DEBUG_PRINT("\t* * * OSCLIVE true * * *\t");

        //in case there is a common prefix (like for mixing station, all OSC starts with /con/? ), we have to remove it to prevent scanf matching the first characters to be in common
        /*String addressWithoutPrefix = removePattern(osc->addressStr.c_str(), OSC_MS_PRE "%s") ;  //"/con/n/%s"         
        int val1;
        
        // data sets
        String patternWithoutPrefix = removePattern(chNames->pattern.c_str(), OSC_MS_PRE "%s");
        DEBUG_PRINT("\n<<%s versus %s",addressWithoutPrefix.c_str(),patternWithoutPrefix.c_str());
        if (sscanf(addressWithoutPrefix.c_str(), patternWithoutPrefix.c_str(), &val1) == 1){
            DEBUG_PRINT("\nGOT chname %d = %d \t%s <=> %s",val1,osc->iValue, addressWithoutPrefix.c_str(),chNames->pattern.c_str());
        }*/
        return true;
      }

      if (millis()-lastOSCCheck > queryDelay*2){
        lastOSCCheck = millis();
        OSClive = false;
        //DEBUG_PRINT("\t!!! OSCLIVE false !!!\t");
        //LogUDP::sendLog("OSCLIVE false",hwm.network->udp);        
      } 
            
      /////// MANAGE CONNECTION ////////////////////////////////////////////////////   

      if (millis()-lastQueryTime > queryDelay){
        lastQueryTime = millis();
        queryState();
        osc->SendCmd2OSC(echoCommand.c_str());        
      }     

      //////// DATA SET SYNC
      if (dataSetSync.progress()<100 ){         //&& interfaceState == SYNCING_CONSOLE
        syncDataSet();
      }


      return false;
    }

    void dropNextSyncQuery(){
      if (dataSetSync.ch<chNames->size){ // are channel complete ?
        DEBUG_PRINT("\nSyncing CH%d",dataSetSync.ch);
        if (chNames->values[dataSetSync.ch].indexOf(UNSYNC_PATTERN) != -1){  // if not sync, we sync it.
          sprintf(OSCCommand, chNames->pattern.c_str(), dataSetSync.ch+chOffset);
          if (osc->ping(OSCCommand)){
            chNames->values[dataSetSync.ch] = osc->sValue;
            DEBUG_PRINT("\t: %s",osc->sValue);
            dataSetSync.ch++;    // next one
          }
        }else dataSetSync.ch++;
      }
      if (dataSetSync.bus<busNames->size){ // are buses complete ?
        DEBUG_PRINT("\nSyncing BUS%d =>",dataSetSync.bus);
        if (busNames->values[dataSetSync.bus].indexOf(UNSYNC_PATTERN) != -1){  // if not sync, we sync it.
          sprintf(OSCCommand, busNames->pattern.c_str(), dataSetSync.bus+busOffset);
          if (osc->ping(OSCCommand)){
            busNames->values[dataSetSync.bus] = osc->sValue;
            DEBUG_PRINT("\t: %s",osc->sValue);
            dataSetSync.bus++;    // next one
          }
        }else dataSetSync.bus++;      
      }      
    }

    // Envoi d'une commande générique    
    virtual bool sendCommand(String command, JsonDocument param){
      bool success = true;
      
      if (command == "testMixingOSC"){

        String msg = param["msg"];
        IPAddress ip;
        ip.fromString(param["ip"].as<String>());
        int port = param["port"];

        osc->SendCmd2OSC(msg.c_str());
        //osc->OSCReadIn();
      }else success = false;
      return success;

    }    

  protected:
    uint16_t restartMonitorDelay=0, queryDelay=0;

    String restartMonitorCommand ="";
    OSCInterface* osc;

    char OSCCommand[OSC_MAX_LEN];

    unsigned long lastQueryTime = 0, lastRestartMonitorTime = 0, lastOSCCheck = 0;   
};

String OSCMixerInterface::echoCommand = "";

//bool OSCMixerInterface::monitoringSuccessful = false; 
//uint8_t OSCMixerInterface::connectAttempt = 0;

class MixerOfflineInterface: public MixerInterface {
  public:

    MixerOfflineInterface(DataRepository* dr,String m): MixerInterface(dr,m){
      DEBUG_PRINT("\n\nNew MixerOfflineInterface(<%s>)\n\n",model.c_str());
      name="Offline";
      interfaceState = DISCONNECTED;
    }

    virtual ~MixerOfflineInterface(){
      DEBUG_PRINT("\n~MixerOfflineInterface()");   
    }

    virtual bool update(){
      return true;
    } 

    virtual void connect(Config* conf){
      // do nothing
    }

    virtual void disconnect(){}

    virtual void queryState(){}               

    virtual bool sendCommand(String command, JsonDocument param){return false;} // Envoi de commande spécifique

    /*virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="ch"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="bus"+String(index);
      return v;
    }*/

    virtual void parseInterfaceProtocol(JsonDocument& doc){
      //DEBUG_PRINT("\n\n\n\tAPPEL DERIVE OFFLINE");
      MixerInterface::parseInterfaceProtocol(doc);
    }

  };


// Implémentation spécifique pour Behringer

class BehringerMixer : public OSCMixerInterface {
  
  public:

    BehringerMixer(DataRepository* dr,String model,OSCInterface* _osc): OSCMixerInterface(dr,model,_osc){
      DEBUG_PRINT("\n\nNew BehringerMixer(<%s>)\n\n",model.c_str());
      name="MusicTribe";
      echoCommand = BEHRINGER_PING;
      restartMonitorCommand = OSC_BEHRINGER_REMOTE;
      restartMonitorDelay = BEHRINGER_RESTART_PERIOD;
      queryDelay = BEHRINGER_PING_DELAY;
      interfaceState = CONNECTED;
    }

    virtual ~BehringerMixer(){
      DEBUG_PRINT("\n~BehringerMixer()");   
    }

    virtual void queryState(){
      if (!OSClive){
        DEBUG_PRINT("\t second chance");
        if (!osc->ping(echoCommand)) interfaceState = DISCONNECTED;     // second chance;
        else OSClive = true;
        lastQueryTime = millis();
      } 
    }

    virtual void connect(Config* conf){
      DEBUG_PRINT("\nConnected to %s",name.c_str());
    }

    virtual void disconnect(){}

    virtual void syncDataSet(){      
      dropNextSyncQuery();
    }

    bool sendCommand(String command, JsonDocument param) override {
      bool success;
      success = OSCMixerInterface::sendCommand(command, param);
      if (success) return true; // generic command has been consummed 
      
      success = true;

      // Traduction et envoi des commandes spécifiques à Behringer
      // Accéder à la seule et premiere clé qui est le nom de la command    

      // Buffer pour la chaîne formatée
      char formattedString[50];
 
      int chID = param["ch"];
      int busID = param["bus"];
      float value = param["value"];      

      if (command == "setChGain"){        
        float v;
        if (model == "X32"){
          sprintf(formattedString, "/ch/%02d/preamp/trim", chID);          
          v = value;    // use formula to convert
        }else{
          sprintf(formattedString, "/headamp/%02d/gain", chID);
          v = value;    // use formula to convert          
        }
        osc->SendFloat2OSC(formattedString,v);
      }else if (command == "setChBusMute"){
        bool mute = param["on"];
        if (model == "X32"){
          sprintf(formattedString,"/ch/%02d/mix/%02d/on", chID, busID);                     
        }else{
          sprintf(formattedString, "/ch/%02d/mix/on", chID);
          // check if we are allowed to mute a channel which is not mine
        }        
        osc->SendInt2OSC(formattedString,mute);
      }        
      //template
      else if (command =="CNAME"){
        if (model == "X32"){

        }else{
          
        }
      }
      else{
        success = false;
        DEBUG_PRINT("/n%s unsupported command: %s",name.c_str(),command.c_str());
      }
      return success;
    }

    virtual bool update(){
      if (monitor()){
        DEBUG_PRINT("\nBehringer processs QUEUE");
        if (osc->addressStr == "value"){

        }else if (osc->addressStr == "value2"){

        }
        return true;
      }
      return false;
    }

    /*virtual String getValue(String addrr, uint16_t index){
      String value="";
      
      if (addrr == ADDRESS_CHNAME){
        value="CH"+String(index); // default
        sprintf(OSCCommand, chNames->pattern.c_str(), index);        
      } 
      else if (addrr == ADDRESS_BUSNAME){
        value="BUS"+String(index);  // default
        sprintf(OSCCommand, busNames->pattern.c_str(), index);
      }

      if (osc->ping(OSCCommand)){
        value = osc->sValue;
      }

      return value;
    } */  

    virtual void parseInterfaceProtocol(JsonDocument& interfaceDoc){
      MixerInterface::parseInterfaceProtocol(interfaceDoc);
      OSCprotocol = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,PROTOCOL_KEY,"ms"); 

      chNames->pattern = "/ch/%02d/config/name";
      busNames->pattern = "/bus/%02d/config/name";
      /*parse following json:
      "ch":{
				"name":"/ch/config/%d/name",
				"busValue":"",
				"lrValue":""
			},*/      
    }


};

// Implémentation spécifique pour MixingStation
class MixingStation : public OSCMixerInterface {
/* get http://192.168.0.180:8080/console/data/definitions/ch.0.preamp.trim'   => 

{
  "definitions": {
    "ch.0.preamp.trim": {
      "path": "ch.0.preamp.trim",
      "definition": {
        "min": -18,
        "max": 18,
        "delta": 0.125,
        "type": "float"
      }
    }
  }
}
*/
  private:
    //WiFiClient client;    
    AsyncHttpClient msRESTAPIClient;
    
    uint16_t REST_API_port;
    Network* network;
    String RESTState="";

    struct SyncInfo {
        bool consoleInfo = false;
        bool availableModels = false;
        bool mixTargets = true;         //we skip it
        bool mixerCurrent = false;

        // Vérifie si tous les booléens sont vrais
        bool allSync() {
            if (
              consoleInfo & 
              availableModels &
              mixTargets &
              mixerCurrent          
            ) return true;
            else return false;
        }
    };

    SyncInfo mixerRESTSync;
       
  public:

    MixingStation(DataRepository* dr, OSCInterface* osc,Network* net,uint16_t _port): network(net), REST_API_port(_port), OSCMixerInterface(dr,MIXING_STATION_MODEL, osc){      
      
      DEBUG_PRINT("\n\nNew MixingStation()\n\n");   
      name="undef";                    // to be replaced by the outcome of a REST query 
      echoCommand = getOSC(OSC_MS_PING);
      restartMonitorCommand = OSC_MS_REMOTE;
      restartMonitorDelay = MS_RESTART_PERIOD;
      queryDelay = MS_PING_DELAY;      

      msRESTAPIClient.host = osc->ip;
      msRESTAPIClient.port = REST_API_port; 
      msRESTAPIClient.timeOut = queryDelay-1000; // maximum duration of the request is the period of check minus one second
      msRESTAPIClient.client = network->client;

      msRESTAPIClient.callback =  [&](String param) {processRESTcallback(param);};

      interfaceState = SEARCHING;
      progress = 0.0;

    }

    String getOSC(String command){
      return String(OSC_MS_PRE)+command;
    }


    virtual ~MixingStation(){
      disconnect();
      DEBUG_PRINT("\n~MixingStation()");   
    }

    virtual uint8_t state(){
      return interfaceState;
    }


    void processRESTcallback(String param){
      
      DEBUG_PRINT("\nprocessRESTcallback(%s...)",param.substring(0,100).c_str());

      JsonDocument doc;    
      auto error = deserializeJson(doc, param);
      if (error) {
          DEBUG_PRINT("\nprocessRESTcallback parsing failed: %s",error.c_str()); 
          return;
      }

      uint8_t status = doc[HTTP_REST_STATUS];
      JsonDocument data = doc[HTTP_REST_DATA];
      String origin = doc[HTTP_REST_ORIGIN];     

      /*
      String dataString;
      serializeJson(data, dataString);      
      DEBUG_PRINT("\n\tSTATUS: <%s>",status==HTTP_REST_OK?"ok":"error");
      DEBUG_PRINT("\n\tORIGIN: <%s>",origin.c_str());
      DEBUG_PRINT("\n\tDATA: <%s>",dataString.substring(0,50).c_str());*/

      if (origin == RESTCALL_CONSOLE_INFO){
        if (status == HTTP_REST_OK){
          
          uint16_t chSize = getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,MS_CH_SHORTNAME,"count",String(chSize)).toInt();
          uint16_t busSize = getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,MS_BUS_SHORTNAME,"count",String(busSize)).toInt();      

          chNames->init(chSize,UNSYNC_PATTERN+String("MSCH"));
          busNames->init(busSize,UNSYNC_PATTERN+String("MSCH"));

          chOffset = getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,MS_CH_SHORTNAME,"offset","0").toInt();
          busOffset = getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,MS_BUS_SHORTNAME,"offset","48").toInt();

          lastQueryTime = 0; // speed up the call to ping with no delay in between
          mixerRESTSync.consoleInfo = true;
        }else{
          setConnectFailed();
          mixerRESTSync.consoleInfo = false;      
        }
      }
      
      else if (origin == RESTCALL_APP_STATE){
        if (status == HTTP_REST_OK){
          RESTState = data["state"].as<String>();
          progress = data["progress"].as<int>();
          DEBUG_PRINT("\n\t\t\t\t\t\t ---- REST API STATE %s %d% ----", RESTState.c_str(), (int)progress);
          progress = progress * 1.1; // we fill the gap due to the latency of http checks
          if (progress > 100) progress = 100; 
          if (interfaceState == SYNCING_MS) lastQueryTime = 0; // speed up the call to ping with no delay in between
        }else{
          RESTState = "offline";
          DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! REST API Offline !!!----");          
        }
      }
      
      else if (origin == RESTCALL_CONNECT){       
        if (status == HTTP_REST_OK){
          RESTState = "connecting";
          progress = 0.0;
          lastQueryTime = 0; // speed up the call to ping with no delay in between          
        }else{
          setConnectFailed();
        }
      }

      else if (origin == RESTCALL_AVAILABLE_MODELS){
        // this call is to retrieve all supported console by MS sor we can store them in the interfaces.json definition file in a compressed format
        if (status == HTTP_REST_OK){
          JsonDocument interfaceDoc;      
          MixerInterface::deserialize(&interfaceDoc);

          JsonArray interfaces = interfaceDoc[CONSOLE_KEY];
          interfaces.clear();

          // Accéder au tableau "consoles"
          if (!data[CONSOLE_KEY].is<JsonArray>()) {
                DEBUG_PRINT("\n '%s' not found or not an array in JSON",CONSOLE_KEY);
                return;
            }
          JsonArray consoles = data[CONSOLE_KEY];      
          // Boucle pour lire chaque objet dans le tableau
          for (JsonObject console : consoles) {       
            JsonObject newInterface = interfaces.createNestedObject(); // Créer un nouvel objet dans le tableau
            // Ajouter des valeurs dans la nouvelle console
            newInterface[CONSOLEID_KEY] = console[CONSOLEID_KEY];      // Ajouter une clé "consoleId"
            newInterface[NAME_KEY] = console[NAME_KEY];  // Ajouter une clé "name"
          }

          // save the file
          String dataToCompress;
          serializeJson(interfaceDoc,dataToCompress);      
          DEBUG_PRINT("\ndataToCompress->%s",dataToCompress.c_str());
          size_t dataSize = writeBufferToGzFile(INTERFACES_FILENAMEGZ,&dataToCompress);     
          mixerRESTSync.availableModels = true;
          lastQueryTime = 0; // speed up the call to ping with no delay in between          
        }else{
          setConnectFailed();
          mixerRESTSync.availableModels = false;      
        }
      }

      else if (origin == RESTCALL_MIXTARGETS){
        if (status == HTTP_REST_OK){            
          mixerRESTSync.mixTargets = true;
        }else{
          setConnectFailed();
          mixerRESTSync.mixTargets = false;      
        }        

      }

      else if (origin == RESTCALL_MIXER_CURRENT){
        if (status == HTTP_REST_OK){                    
          /*
            {
              "consoleId": 0,
              "models": [
                "X32/M32"
              ],
              "supportedHardwareModels": [
                "X32/M32"
              ],
              "ipAddress": "192.168.0.1",
              "name": "X32/M32",
              "currentModel": "X32/M32",
              "manufacturer": "Music Tribe"
            }          
          */
          name = data["name"].as<String>();
          mixerRESTSync.mixerCurrent = true;
        }else{
          setConnectFailed();
          mixerRESTSync.mixerCurrent = false;      
        }        

      }
     
      else if (origin == "Z"){
        
      }
      //data.clear();  // doesnt change
      doc.clear();
    }

    void setConnectFailed(){
        DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! COMMAND FAILED !!!----");                          
        interfaceState = CONNECTFAILED;
    }


    virtual void queryState(){       

      if (!msRESTAPIClient.isClientConnecting){         
          // this is where we drop REST API request
          msRESTAPIClient.method = HTTP_METHOD_GET;
          // by default we just perform the routine check
          msRESTAPIClient.URL = RESTCALL_APP_STATE;
          // unless we are needing some information from the mixer
          if (interfaceState == SYNCING_CONSOLE){
            if (!mixerRESTSync.consoleInfo) msRESTAPIClient.URL = RESTCALL_CONSOLE_INFO;           
            if (!mixerRESTSync.availableModels) msRESTAPIClient.URL = RESTCALL_AVAILABLE_MODELS;
            if (!mixerRESTSync.mixTargets) msRESTAPIClient.URL = RESTCALL_MIXTARGETS;
            if (!mixerRESTSync.mixerCurrent) msRESTAPIClient.URL = RESTCALL_MIXER_CURRENT;
          }
          network->asyncConnect(&msRESTAPIClient);
      }

      queryDelay = MS_PING_DELAY;

      //////////////////////////////////// REST API HAS RESPONDED

      if (RESTState == "syncing"){
        interfaceState = SYNCING_MS;
        //lastQueryTime = 0; // try the next one straight with no delay
      }
      else if (RESTState == "connecting"){
        interfaceState = CONNECTING;
      }
      else if (RESTState == "idle"){
        interfaceState = IDLE;
      }else{

        /////////////////////////////////////  REST API HAS !NOT RESPONDED! OR !IS CONNECTED!
        
        if (OSClive) {

          if (RESTState == "connected" || RESTState == "backgroundSync"){        
            if (!mixerRESTSync.allSync() && !dataSetSync.progress()<100){     // if the mixer days it is connected and we retreived its data, then we set the interface status to CONNECTED
              interfaceState = SYNCING_CONSOLE;
            }else{           
              interfaceState = CONNECTED;               
              queryDelay = MS_PING_DELAY * 2; // we slow down the check as we are connected                

              // now we save the mixing station interface properties so in case user starts offline, we are able to provide the virtual information like number of buses and channels, etc... from the last connected device
              static bool serialized = false;
              if (!serialized){ // only do it once at first connection
                if (serialize()>0){                //save the properties into interfaces.json file                              
                  DEBUG_PRINT("\nmixing station properties saved into interface file");
                  serialized = true;
                }else{
                  DEBUG_PRINT("\nError savingmixing station properties into interface file");
                }
              }
            }
          }   

          else if (RESTState == "offline"){
            interfaceState = MS_UNREACHABLE;  // inconsistent
          }      

        }else{

          if (RESTState == "connected"){
            interfaceState = MIXER_UNREACHABLE;        // inconsistent
            LogUDP::sendLog("MIXER_UNREACHABLE",hwm.network->udp);        
          } 
          else if (RESTState == "offline"){
            disconnect();
          }
        }           

      }

      DEBUG_PRINT("\n\t\tqueryState()=> %d\n",interfaceState);

    }

    virtual void syncDataSet(){
      // data sets can only be made available once the mixer info and mix targets has been retrieved, especially offset of the names etc...
      if (mixerRESTSync.consoleInfo && mixerRESTSync.mixTargets){
        dropNextSyncQuery();
      }
      return;
    }

    size_t serialize(){
      JsonDocument interfaceDoc;
      deserialize(&interfaceDoc);

      modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, BUS_SIZE_KEY, busNames->size);
      modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, CH_SIZE_KEY, chNames->size);

      String dataToCompress;
      serializeJson(interfaceDoc,dataToCompress);      
      //DEBUG_PRINT("\ndataToCompress->%s",dataToCompress.c_str());
      size_t dataSize = writeBufferToGzFile("/webapp/interfaces.json.gz",&dataToCompress);

      return dataSize;
      
    }

    virtual void disconnect(){
      interfaceState = DISCONNECTED;     // no REST API and no OSC
      DEBUG_PRINT("\nWaiting semaphore release in disconnect()");
      if (xSemaphoreTake(msRESTAPIClient.mutex, portMAX_DELAY)) { // Prendre le mutex
        
          DEBUG_PRINT("\ndeleting task");
          //vTaskDelete(msRESTAPIClient.taskHandle); // Supprimer la tâche
          //msRESTAPIClient.taskHandle = NULL;       // Mettre à jour le pointeur
          if (msRESTAPIClient.client != NULL) msRESTAPIClient.client->stop();
          DEBUG_PRINT("\nclient stoppped");
          //msRESTAPIClient.taskHandle  = nullptr;
        
        xSemaphoreGive(msRESTAPIClient.mutex); // Relâcher le mutex
      }
    }

    virtual void connect(Config* config){       
      // launch the  REST API command to connect in case user config say so.
      if (config->getInt(MIXER_AUTOMODEL_KEY) != -1){       

        if (msRESTAPIClient.isClientConnecting) return;
        if (RESTState != "idle") return;
        if (interfaceState != IDLE) return;

        msRESTAPIClient.method = HTTP_METHOD_POST;
        msRESTAPIClient.URL = RESTCALL_CONNECT;
        JsonDocument postParam;
        postParam["consoleId"] = config->getInt(MIXER_AUTOMODEL_KEY);
        postParam["ip"] = *config->getString(MIXER_AUTOIP_KEY);
        serializeJson(postParam, msRESTAPIClient.post);

        if (network->asyncConnect(&msRESTAPIClient)){
          progress = 0.0; 
          RESTState = "connecting";
          DEBUG_PRINT("\nCONNECT MS TO %s\n", config->getString(MIXER_AUTOIP_KEY)->c_str());      
          LogUDP::sendLog("AUTO CONNECT MS",hwm.network->udp);
          lastQueryTime = 0; // try the next one straight with no delay
        }
      }
    }

    virtual bool update(){      
      if (monitor()){        
        // process queue
        //DEBUG_PRINT("\nMixing station processs QUEUE");
        if (osc->addressStr == "value"){

        }else if (osc->addressStr == "value2"){

        }
       
        return true;

      }      
      return false;
    }

    /*virtual String getValue(String addrr, uint16_t index){
      String value="";
      
      if (addrr == ADDRESS_CHNAME){
        value="CHMS"+String(index);
        index += chOffset;
      } 
      else if (addrr == ADDRESS_BUSNAME){
        value="BUSMS"+String(index);
        index += busOffset;        
      }

      // /con/n/ch.18.cfg.name,s,newname8

      sprintf(OSCCommand, chNames->pattern.c_str(), index);
      if (osc->ping(OSCCommand)){
        value = osc->sValue;
      }

      return value;
    }  */   

    virtual bool sendCommand(String command, JsonDocument param) {

      bool success;
      if (state() == DISCONNECTED) {
        DEBUG_PRINT("\nNon connecté a Mixing Station");
        return false;
      }else success = OSCMixerInterface::sendCommand(command, param);
      if (success) return true; // command has been consummed
      success = true;

        /// IMPLEMENT YOUR CODE HERE

      return success;
    }

    virtual void parseInterfaceProtocol(JsonDocument& interfaceDoc){
      MixerInterface::parseInterfaceProtocol(interfaceDoc);

      chNames->pattern = getOSC("/ch.%d.cfg.name");
      busNames->pattern = getOSC("/ch.%d.cfg.name");

      //DEBUG_PRINT("\n\n\n\tAPPEL DERIVE MS");
      //String getNameOSCcommand = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,"getName","ms"); 
      //DEBUG_PRINT("\n\n\n");          
    }


};

#endif