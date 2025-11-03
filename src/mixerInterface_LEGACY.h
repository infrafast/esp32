#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#include <StringConcat.h>
#include "Interfaces.h"
#include <ArduinoJson.h>
#include <vector>
#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define ADDRESS_CHNAME "/ch/%d/name"
#define ADDRESS_BUSNAME "/bus/%d/name"
#define ADDRESS_AUXNAME "/aux/%d/name"
#define ADDRESS_FXRETNAME "/fxret/%d/name"
#define ADDRESS_MAINNAME "/main/%d/name"

#define ADDRESS_CHMUTE "/ch/%d/mute"
#define ADDRESS_BUSMUTE "/bus/%d/mute"
#define ADDRESS_AUXMUTE "/aux/%d/mute"
#define ADDRESS_FXMUTE "/fxret/%d/mute"
#define ADDRESS_MAINMUTE "/main/%d/mute"

// https://mixingstation.app/tasks/T5194


#define EXEC_TESTOSC "testosc"
#define EXEC_FRAME "oscframe"
#define EXEC_IP "ip"
#define EXEC_PORT "port"


/********************************************************************/
/*********************** INTERFACE ADDRESS INDEXES ******************/
/********************************************************************/
enum {
  ADDR_CHBUSMUTE,
  ADDR_CHBUSVALUE,
  ADDR_BUSMUTE,
  ADDR_CHGAIN,
  ADDR_MAX
};

enum {
  INSTR_TESTOSC,
  INSTR_SETADDRESSVALUE,
  INSTR_GETADDRESSVALUE,
};

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

#define OSC_MS_FORMAT       "n"            // v => plain The actual value of the parameter (e.g. -5dB)    n => normalized The value is normalized within the range 0-1
#define OSC_MS_REMOTE       "/hi/" OSC_MS_FORMAT
#define OSC_MS_PRE          "/con/" OSC_MS_FORMAT
#define OSC_MS_PING         "/ch.0.solo"


//               API CALLS
#define APICALL_CONNECT          "/app/mixers/connect"
#define APICALL_APP_STATE        "/app/state"

#define APICALL_SYNC_CONSOLE_INFO     "/console/information"
#define APICALL_SYNC_AVAILABLE_MODELS "/app/mixers/available"
#define APICALL_SYNC_MIXTARGETS       "/console/mixTargets"
#define APICALL_SYNC_MIXER_CURRENT    "/app/mixers/current"

#define MS_API_PATH              "path"
#define MS_API_ERROR             "error"

//              JSON API DATA CONTENT DEFINITION
#define MS_CHANNEL_TYPES                  "channelTypes"
  #define MS_TYPE_SHORTNAME_KEY   "shortName"
    #define MS_CH_SHORTNAME         "Ch"
    #define MS_BUS_SHORTNAME        "Bus"
    #define MS_AUX_SHORTNAME        "Aux"       
    #define MS_FXRET_SHORTNAME      "FxRet"
    #define MS_MAIN_SHORTNAME       "Main"

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
  #define AUX_SIZE_KEY        "auxSize"
  #define FXRET_SIZE_KEY      "fxRetSize"
  #define MAIN_SIZE_KEY       "mainSize"
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
  SYNCING_SOFTWARE,
  MIXER_UNREACHABLE,
  SOFTWARE_UNREACHABLE,
  SEARCHING,
  CONNECTFAILED,
  SYNCING_DATA,
  EOF_STATE
};

String stateNames[EOF_STATE]={
  "disconnected",
  "connected",
  "idle",
  "connecting",
  "syncing software",
  "console unreachable",
  "software unreachable",
  "searching",
  "connect failed",
  "syncing data",
};

#define UNSYNC_PATTERN "~"


class DataSetSynced: public DataSet{
  public: 
    String* values = nullptr;
    uint16_t syncIndex = 0;
    //std::function<void(DataSetSynced*, OSCValues)> callback;
    unsigned long lastSyncTime = 0; 
    int offset = 0;
    String defaultName="";
    String pattern="";

    DataSetSynced(String _shortName, String addr,String obj, uint16_t _size,String defVal, int _offset) : DataSet(addr,obj), shortName(_shortName){
      init(_size, defVal, _offset);
    }

    String shortName;

    void set(uint16_t index, String value){
      if (index>size) return;
      
      if (values[index] != value){
        values[index] = value;
        //brodcast
      }
       
    }

    void init(uint16_t _size,String defVal, int _offset){
      offset = _offset;
      defaultName = defVal;
      //DEBUG_PRINT("\nDataSetSynced init(%d,%s,%d)",size,defVal.c_str(),offset);

      if (size != _size){
        size = _size;
        // Supprimer le tableau existant s'il y en a un
        if (values != nullptr) delete[] values; // Libérer la mémoire précédemment allouée
        // Créer un nouveau tableau dynamique avec la taille passée
        values = new String[size];
        // Initialiser les valeurs (par exemple, 0)
      }
     
      for (int i = 0; i < size; ++i) {
          values[i] = concatStrings(defVal, i);
      }
    }


    virtual const String* getValuePtr(uint16_t i){    
      if (i < size) return &values[i];
      else return &defaultName;
    }

    /*virtual String serialize(){
      JsonDocument doc;
      doc[address][concatStrings("getDS", shortName)] = size;  // function name that is called by javascript to populate the data by indicating the size example: getDSCh(32)  //{"/ch/%d/config/name": {"getDSCh":32}}
      String data;
      serializeJson(doc,data);
      return data;
    } */ 
       

};


// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface: public DeviceInterface{
  public:
    String model;
    DataRepository* dataRepository = nullptr;
    String addresses[ADDR_MAX];

    String interfaceSubState = "";

    int idStart = 0;

    String unsyncPattern="~";

    virtual void queryState()=0;

    DataSetSynced* chNames;
    DataSetSynced* busNames;  
    DataSetSynced* auxNames;  
    DataSetSynced* fxRetNames;
    DataSetSynced* mainNames;

    MixerInterface(String _model, uint8_t _capture = CAPTURE_OUT):DeviceInterface(_model,_capture){  
      dataRepository = &app.dataRepository;
      if (_model == "") model = MIXING_STATION_MODEL;
      else model = _model;          

      //DEBUG_PRINT("\nnew MixerInterface(%s)",model.c_str());
    };

    void init(){
      JsonDocument interfaceDoc;
      deserialize(&interfaceDoc);
      parseInterfaceProtocol(interfaceDoc.as<JsonVariant>());      
    }

    virtual void parseInterfaceProtocol(JsonVariant interfaceDoc){
      // getting the properties of the interface in the json document
      //DEBUG_PRINT("\n\n\n\tAPPEL DE BASE");
      uint16_t chSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,CH_SIZE_KEY,"12").toInt();      
      uint16_t busSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,BUS_SIZE_KEY,"4").toInt();
      uint16_t auxSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,AUX_SIZE_KEY,"2").toInt();      
      uint16_t fxRetSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,FXRET_SIZE_KEY,"4").toInt();
      uint16_t mainSize = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,MAIN_SIZE_KEY,"1").toInt();

      chNames = new DataSetSynced(MS_CH_SHORTNAME,ADDRESS_CHNAME,INPUTSELECT,chSize,UNSYNC_PATTERN "CH",1);       // default offset is 1 
      busNames = new DataSetSynced(MS_BUS_SHORTNAME,ADDRESS_BUSNAME,INPUTSELECT,busSize,UNSYNC_PATTERN "BUS",1);
      auxNames = new DataSetSynced(MS_AUX_SHORTNAME,ADDRESS_AUXNAME,INPUTSELECT,auxSize,UNSYNC_PATTERN "AUX",1);
      fxRetNames = new DataSetSynced(MS_FXRET_SHORTNAME,ADDRESS_FXRETNAME,INPUTSELECT,fxRetSize,UNSYNC_PATTERN "FX",1);
      mainNames = new DataSetSynced(MS_MAIN_SHORTNAME,ADDRESS_MAINNAME, INPUTSELECT,mainSize,UNSYNC_PATTERN "MAIN",1);

      dsNames = { chNames, busNames, auxNames, fxRetNames,  mainNames};
      for (const auto& dsName : dsNames) dataRepository->addDataSet(dsName);      


    }

    void deserialize(JsonDocument* interfaceDoc){
      
      ByteBuffer decompressedJson;
      size_t dataSize = 0;

      String interfaceFileNameGz = concatStrings(INTERFACES_FILENAME, ".gz");
      dataSize = readGzFileToBuffer(interfaceFileNameGz.c_str(),&decompressedJson);
      if (dataSize == 0){ // uncompress failed
        //DEBUG_PRINT("\nFailed to decompress JSON interface file %s, using non compressed file",INTERFACES_FILENAME);
        File file = LittleFS.open(INTERFACES_FILENAME,"r");
        if (!file) {
            //DEBUG_PRINT("\nFailed to open JSON interface file");
            file.close(); // Fermer le fichier
            return;
        }else{
          auto error = deserializeJson(*interfaceDoc, file);
          if (error) {
              //DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());             
              file.close(); // Fermer le fichier
              return;
          }
          file.close(); // Fermer le fichier          
        }       
      }else{
        // Désérialiser les données JSON
        auto error = deserializeJson(*interfaceDoc, decompressedJson.buffer);
        if (error) {
            //DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());
            return;
        }        
      }      
    }


    virtual ~MixerInterface(){
      for (const auto& dsName : dsNames){
        dataRepository->removeDataSet(dsName);
        delete dsName;
      } 
      //DEBUG_PRINT("\n\n~MixerInterface()");
    }
   
    String getAdjacentValueByKeyFromRootArray(JsonVariant& doc, String root, String key, String keyValue, String adjacent, String defaultValue="") {  // key = "model" , adjacent = "size"
        // Vérifier si le document contient une clé "consoles"
        if (!doc[root].is<JsonArray>()) {
            //DEBUG_PRINT("\nRoot '%s' not found or not an array in JSON",root.c_str());
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
                    //DEBUG_PRINT("\nadjacent:%s   not found.",adjacent.c_str());
                    return defaultValue;
                }
            }
        }

        //DEBUG_PRINT("\n<%s>=<%s> not found, returning default %s = %s",keyValue.c_str(),key.c_str(),adjacent.c_str(),defaultValue.c_str());
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
            //DEBUG_PRINT("\nValeur modifiée avec succès !");
            return;
          } else {
            //DEBUG_PRINT("\nClé non trouvée !");
            return;
          }
        }
      }
      //DEBUG_PRINT("\nModèle non trouvé !");
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

  protected:
    //uint16_t chSize=1,busSize=1;    
    std::vector<DataSetSynced*> dsNames;

};


class OSCMixerInterface: public MixerInterface {

  public: 
    static String echoCommand;    
    String OSCprotocol="";
    uint16_t latency;

    OSCMixerInterface(String _model, uint8_t capture, OSCInterface* _osc): osc(_osc), MixerInterface(_model, capture){
      //DEBUG_PRINT("\nnew OSCMixerInterface(%s %s,%d)",model.c_str(),osc->ip.toString().c_str(),osc->port);
      delayBetween2SyncFrames = osc->latency*dsNames.size()*2;      
    }

    virtual void syncDataSet()=0;

    virtual bool requiresEcho() = 0; // used to inform if the mixer need an echo after sending a command, this is the case for X32

    virtual uint8_t progress(){
      uint16_t totalToSync = 0;
      uint16_t syncedSoFar = 0;
      for (const auto& dsName : dsNames){
        totalToSync += dsName->getSize();
        syncedSoFar += dsName->syncIndex;
      }      
      return map(syncedSoFar, 0, totalToSync, 0, 100);      
    }  

    virtual bool parseIn(JsonObject& data){
      
      // restart monitoring value
      if (interfaceState == CONNECTED){
        if (millis()-lastRestartMonitorTime > restartMonitorDelay){
          osc->SendCmd2OSC(restartMonitorCommand.c_str());          // otherwise we ask mixer to send message for the next 10 sec (see behringer protocol)
          lastRestartMonitorTime = millis();
        }
      }

      //////// DATA SET SYNC
      if (interfaceState == SYNCING_DATA ){         //we should not try to sync if we are not connected
        syncDataSet();
      }
      
      // PROCESS OSC QUEUE
      if (osc->monitor(data)){        
        /// match names pattern
        String prevPattern="";
        for (const auto& dsName : dsNames){
          if (prevPattern == dsName->pattern) break;
          else{
            prevPattern = dsName->pattern;
            if (osc->matchPattern(dsName->pattern,data[EXEC_ADDRESS])){
              if (updateName(dsName,data)) break;
            }
          }

        } 

        lastQueryTime = millis();   // remove THIS ONE IF YOU STILL WANT TO CHECK REST API WHEN OSC IS RESPONDING (CONSUMME MORE TIME)                
        OSCechoAttempt = 0;                        
        return true; 
      } 

      /////// MANAGE CONNECTION ////////////////////////////////////////////////////   
      if (millis()-lastQueryTime > queryDelay){        
        lastQueryTime = millis();
        queryState();
        OSCechoAttempt++;
        osc->SendCmd2OSC(echoCommand.c_str());                      
        //DEBUG_PRINT("\t OSCechoAttempt: %d",OSCechoAttempt);
      } 
      return changedState;
    }    

    virtual bool updateName(DataSetSynced* ds, JsonObject& val){
      // this is the specific part that uses scanf to extract the value => we have to know in advance that we want ot retrieve an int.
      int index = 0;
      if (!sscanf(val[EXEC_ADDRESS], ds->pattern.c_str(), &index) == 1){
        //DEBUG_PRINT("\nparse %s failed",ds->pattern.c_str());
      }
      //DEBUG_PRINT("\nUpdate index %d - offset(%d) = ",index,ds->offset);      
      ////////////////////////////////////
      index = index - ds->offset;
      if (index < 0) index = 0;
      if (index >= ds->getSize()){
        //DEBUG_PRINT("\tNO DS IMPLEMENTED FOR THIS INDEX");
        return false;
      }
      //DEBUG_PRINT("%d",index);
      // first we check the value of IVal which indicate which channel has been received      
      if (ds->values[index].indexOf(UNSYNC_PATTERN) != -1){ // the channel was notsync so we flag it as sync.        
        ds->syncIndex++;    // next one
        //DEBUG_PRINT("\n%s=>%d/%d",ds->shortName,ds->syncIndex,ds->getSize());
        //DEBUG_PRINT("\t SYNCED");
        ds->lastSyncTime = 0;
      }
      // and in all cases we update the value // sValue
      String sValue = val[EXEC_VALUE][0];
      if ( sValue == "") ds->set(index,String(index+1));  // if we receive an empty string we define the name as the index (we add one to get  a value starting at one and not 0)
      else ds->set(index,sValue) ;      
      //DEBUG_PRINT("\t%s(%d)=>%s",ds->shortName.c_str(),index, val.sValue);      
      
      return true;
    }

    void dropParrallelAsyncQuery(){
      strcpy(OSCCommand,"");   

      if (progress() >= 100){
        lastQueryTime = 0;        // do not wait and trigger immediate queryState if we are complete
        return;
      }

      for (const auto& dsName : dsNames){
        if (dsName->syncIndex < dsName->getSize()){ // complete ?                                         
          if (dsName->values[dsName->syncIndex].indexOf(UNSYNC_PATTERN) != -1){  // if not sync, we sync it.            
            unsigned long elapsedTime = millis() - dsName->lastSyncTime;
            if (elapsedTime >= delayBetween2SyncFrames){  // > dsNames.size * latency
              //DEBUG_PRINT("\nSyncing %s%d =>",dsName->shortName.c_str(),dsName->syncIndex);                                             
              dsName->lastSyncTime = millis();
              sprintf(OSCCommand, dsName->pattern.c_str(), dsName->syncIndex+dsName->offset);
              osc->SendCmd2OSC(OSCCommand);
              delayBetween2SyncFrames = elapsedTime;
            }else{
              //DEBUG_PRINT("\t-X(%s/ %i < %i)-",dsName->shortName.c_str(),elapsedTime,delayBetween2SyncFrames);
            }
          }   
        }
      } 
    }



    virtual bool parseOut(const JsonObject& command){
      String s;      
      serializeJson(command, s);
      DEBUG_PRINT("\n%s parseOut %s",name.c_str(),s.c_str());

      // param ressemble a : {"osc":{"address":"/ch/03/level/05/","value": 0.5 }}
      // ou {"instruction":{data}}
      /*String s;      
      serializeJson(param, s);
      DEBUG_PRINT("\nexecute %s",s.c_str());*/
      
      bool success = true;
      
      JsonPair kv = *command.begin();  // pointeur sur l’unique entrée

      const char* instruction = kv.key().c_str(); // ex: "osc"
      JsonObject data = kv.value().as<JsonObject>(); // ex: {"address": "...", "value": ...}
      
      if (strcmp(EXEC_OSC,instruction) == 0){
        
        // {"/ch/03/level/05/": 0.5 }
        JsonDocument doc;
        String address = command[EXEC_OSC][EXEC_ADDRESS];
        JsonVariant value = command[EXEC_OSC][EXEC_VALUE]; // value could be null => it's a get address and not set address, that's why we use varian and not Stirg otherwise String would be filled with "null"
        // doc["/ch/03/level/05/"] = 0.5;              
        doc[address] = value;				
        osc->execute(doc.as<JsonObject>());

        //Xair automatically echo the command but not X/M
        //we use the parse and not execute to get the echo to avoid that it is recorded twice by the InterfaceCapture
        if (!value.isNull() && requiresEcho())  {        
          JsonDocument echo;
          echo[address] = nullptr;
          osc->parseOut(echo.as<JsonObject>());
        }




      }else if (instruction == EXEC_TESTOSC){


      }else if (instruction == "WHATEVER"){


      }else success = false;
      
      return success;

    }     

  protected:
    // utilities
    char formattedCommandString[50];
    uint16_t restartMonitorDelay=0, queryDelay=0;
    uint8_t OSCechoAttempt = 0;
    String restartMonitorCommand ="";
    OSCInterface* osc;
    unsigned long delayBetween2SyncFrames;

    #define OSC_MAX_LEN 64
    char OSCCommand[OSC_MAX_LEN];

    unsigned long lastQueryTime = 0, lastRestartMonitorTime = 0;   
};

String OSCMixerInterface::echoCommand = "";

//bool OSCMixerInterface::monitoringSuccessful = false; 
//uint8_t OSCMixerInterface::connectAttempt = 0;

class MixerOfflineInterface: public MixerInterface {
  public:

    MixerOfflineInterface(String m): MixerInterface(m){
      //DEBUG_PRINT("\n\nNew MixerOfflineInterface(<%s>)\n\n",model.c_str());
      name="Offline";
      interfaceState = DISCONNECTED;
      for (uint8_t i=0;i<ADDR_MAX-1;i++) addresses[i] = concatStrings("address", i);
    }

    bool read(JsonObject& data){
      return false;
    }  
    

    virtual ~MixerOfflineInterface(){
      //DEBUG_PRINT("\n~MixerOfflineInterface()");   
    }

    virtual void connect(){
      // do nothing
    }

    virtual bool parseOut(const JsonObject& data){return false;}
    
    virtual bool parseIn(JsonObject& data){
      return false;
    }  

    virtual uint8_t progress(){return 0;}

    virtual void disconnect(){}

    virtual void queryState(){}               

    /*virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v = concatStrings("ch", index);
      else if (addrr == ADDRESS_BUSNAME) v = concatStrings("bus", index);
      return v;
    }*/

    virtual void parseInterfaceProtocol(JsonVariant doc) override{
      //DEBUG_PRINT("\n\n\n\tAPPEL DERIVE OFFLINE");
      MixerInterface::parseInterfaceProtocol(doc);
    }

    // commands
    virtual void setChGain(uint8_t chID, float value){};
    virtual void setChBusMute(uint8_t chID, uint8_t busID, bool mute){};    

    virtual void getChBusMute(uint8_t chID, uint8_t busID){};
    virtual void getChBusValue(uint8_t chID, uint8_t busID){};
    virtual void getBusMute(uint8_t busID){};

  };


// Implémentation spécifique pour Behringer

class BehringerMixer : public OSCMixerInterface {
    
  public:

    BehringerMixer(String model,uint8_t capture, OSCInterface* _osc): OSCMixerInterface(model,capture, _osc){
      //DEBUG_PRINT("\n\nNew BehringerMixer(<%s>)\n\n",model.c_str());
      name="MusicTribe";
      echoCommand = BEHRINGER_PING;
      restartMonitorCommand = OSC_BEHRINGER_REMOTE;
      restartMonitorDelay = BEHRINGER_RESTART_PERIOD;
      queryDelay = BEHRINGER_PING_DELAY;
      interfaceState = SYNCING_DATA;

      idStart = 1;

      // addresses definition ===> this could be moved into parseInterfaceProtocol      
      //common
      addresses[ADDR_CHBUSVALUE] =  "/ch/%02d/mix/%02d/level";
      addresses[ADDR_BUSMUTE] =     "/bus/%02d/mix/on";
      //specific to X32
      if (model == "X32"){
        addresses[ADDR_CHBUSMUTE] =   "/ch/%02d/mix/%02d/on";
        addresses[ADDR_CHGAIN] =   "/ch/%02d/preamp/trim";
      }else{
        addresses[ADDR_CHBUSMUTE] =   addresses[ADDR_BUSMUTE];
        addresses[ADDR_CHGAIN] =   "/headamp/%02d/gain";
      }

    }

    virtual bool requiresEcho(){
      if (model == "XR12" || model == "XR16" || model == "XR18") return false;
      else return true;
    }

    virtual ~BehringerMixer(){
      //DEBUG_PRINT("\n~BehringerMixer()");   
    }

    virtual void queryState(){
      if (OSCechoAttempt == 2) interfaceState = DISCONNECTED;     
      else if (interfaceState == SYNCING_DATA){
        if (progress() >= 100) {
          interfaceState = CONNECTED;
        }
      }
    }

    virtual void connect(){
      //DEBUG_PRINT("\nConnected to %s",name.c_str());
    }

    virtual void disconnect(){}

    virtual void syncDataSet(){   
      dropSequentialSyncQuery();
    }

    void dropSequentialSyncQuery(){      
      if (dsNameIndex >= dsNames.size()){ // we've synced it all
        lastQueryTime = 0;        // do not wait and trigger immediate queryState
        return;
      }
      DataSetSynced* dsName = dsNames[dsNameIndex]; //sync in serial, next dsName

      if (dsName->syncIndex < dsName->getSize()){ // then are buses complete ?
        //DEBUG_PRINT("\nSyncing %s%d =>",dsName->shortName.c_str(),dsName->syncIndex);
        if (dsName->values[dsName->syncIndex].indexOf(UNSYNC_PATTERN) != -1){  // if not sync, we sync it.
          sprintf(OSCCommand, dsName->pattern.c_str(), dsName->syncIndex+dsName->offset);

          JsonDocument doc;
          JsonObject input = doc.to<JsonObject>();

          if (osc->blockingPing(OSCCommand, input, latency)){
            latency = osc->latency; // recall the latency from the last ping
            updateName(dsName,input);
          }
          else latency = (latency + 20) % 250;
        }   
      }else dsNameIndex++;
    }

    virtual void parseInterfaceProtocol(JsonVariant interfaceDoc) override{
      MixerInterface::parseInterfaceProtocol(interfaceDoc);
      OSCprotocol = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,PROTOCOL_KEY,"ms"); 

      chNames->pattern = "/ch/%02d/config/name";
      busNames->pattern = "/bus/%02d/config/name";
      auxNames->pattern = "/auxin/%02d/config/name";
      fxRetNames->pattern = "/fxrtn/%02d/config/name";
      mainNames->pattern = "/main/st/config/name";

      //for (const auto& dsName : dsNames) dsName->offset = 1; //index starts at 1 // this is initialized by default when creating names in init of Base Class
      /*parse following json:
      "ch":{
				"name":"/ch/config/%d/name",
				"busValue":"",
				"lrValue":""
			},*/      
    }

  private:
    uint8_t dsNameIndex = 0;

};

// Implémentation spécifique pour MixingStation   
// v2.2.2
// 
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
    //WebSocketServer<EthernetServer, EthernetClient> server;

    uint16_t ms_API_port;
  
    uint8_t syncSoftwareProgress = 0;

    String API_Mixer_State="";

    class SyncInfo {
      public:
        String APIcall ="";
        bool synced = false;
        SyncInfo(String apiCall, bool forceSynced=false):APIcall(apiCall),synced(forceSynced){}
    };
    
    bool serialized = false;
 
    std::vector<SyncInfo> mixerAPISync; 
    uint8_t APIsyncIndex = 0;

    SyncInfo syncConsoleInfo = SyncInfo(APICALL_SYNC_CONSOLE_INFO);
    SyncInfo syncMixerCurrent = SyncInfo(APICALL_SYNC_MIXER_CURRENT);
    SyncInfo syncAvailableModels = SyncInfo(APICALL_SYNC_AVAILABLE_MODELS);
    SyncInfo syncMixTargets = SyncInfo(APICALL_SYNC_MIXTARGETS,true);       

  public:

    MixingStation(uint8_t capture, OSCInterface* osc,uint16_t _port): ms_API_port(_port), OSCMixerInterface(MIXING_STATION_MODEL, capture, osc){      
      
      //DEBUG_PRINT("\n\nNew MixingStation()\n\n");   
      // to be replaced by the outcome of a REST query 
      echoCommand = OSC_MS_PRE OSC_MS_PING;
      restartMonitorCommand = OSC_MS_REMOTE;
      restartMonitorDelay = MS_RESTART_PERIOD;
      queryDelay = MS_PING_DELAY;      

      // prepare REST API callback
      msRESTAPIClient.host = osc->ip;
      msRESTAPIClient.port = ms_API_port; 
      msRESTAPIClient.timeOut = queryDelay-1000; // maximum duration of the request is the period of check minus one second
      msRESTAPIClient.client = hwm.network->client;
      msRESTAPIClient.callback =  [&](const char* param) {processJsonData(param);};

      mixerAPISync = {syncAvailableModels, syncConsoleInfo, syncMixerCurrent, syncMixTargets};
      

      // prepare websocket API callback
      userWSclientOnMessage = std::bind(&MixingStation::wsOnMessageCallback, this, std::placeholders::_1);
      hwm.network->clientSocketClose();                
      
      // addresses definition
      addresses[ADDR_CHBUSMUTE] = OSC_MS_PRE "/ch.%d.mix.sends.%d.on";
      addresses[ADDR_CHBUSVALUE] = OSC_MS_PRE "/ch.%d.mix.sends.%d.lvl";
      addresses[ADDR_BUSMUTE] = OSC_MS_PRE "/ch.0.mix.on";

      idStart = 0;

      interfaceState = SEARCHING;


    }

    virtual uint8_t progress(){
      if (interfaceState == SYNCING_SOFTWARE){
          return syncSoftwareProgress; 
      }else if (interfaceState == SYNCING_DATA){
        uint16_t syncWeight = mixerAPISync.size()*10;
        uint16_t totalToSync = 100+syncWeight;
        float syncedSoFar = OSCMixerInterface::progress();

        for (auto&sync : mixerAPISync){
          if (sync.synced) syncedSoFar += syncWeight/mixerAPISync.size();
        }
        return map(syncedSoFar, 0, totalToSync, 0, 100);
      }
      return 0;
    }

    String _OSCprefix(String command){
      return concatStrings(OSC_MS_PRE, command);
    }


    virtual ~MixingStation(){
      //disconnect();
      //DEBUG_PRINT("\n~MixingStation()");   
    }

    virtual bool requiresEcho(){
      return true;    //mixing station must echo any command with value which is sent to feedback the interface that it has been correctly processed
    }


    virtual uint8_t state(){
      return interfaceState;
    }

    bool getSynced(String s){
      for (const auto& sync : mixerAPISync) if (sync.APIcall == s) return sync.synced;
      return false;
    }

    void setSynced(String s, bool b=true){
      for (auto&sync : mixerAPISync){
        if (sync.APIcall == s){
          sync.synced = b;
          return;
        }
      }
    }

    void wsOnMessageCallback(const char* message){
      /*
        {
          "path": "/app/state",
          "method": "GET",
          "body": {
            "msg": "",
            "progress": 0,
            "state": "connected",
            "topState": "connected"
          }
        }    

        or

        {
          "method": "GET",
          "error": "Could not parse json message: Could not assign value body  to field 'body'"
        }

      */
      //DEBUG_PRINT("\n\t\t\twsOnMessageCallback <<<<< %s",message);
      
      JsonDocument input;    
      auto error = deserializeJson(input, message);
      if (error) {
          //DEBUG_PRINT("\nwsMixerInterface OnMessageCallback parsing failed: %s",error.c_str()); 
          input.clear();
          return;
      }

      JsonDocument output;
      if (input.containsKey(MS_API_ERROR)){
        output[API_STATUS] = API_ERROR;        
        output[API_BODY] = input[MS_API_ERROR];
      }else{
        output[API_ORIGIN] = input[MS_API_PATH];
        output[API_STATUS] = API_OK;
        output[API_BODY] = input[API_BODY];
      }

      processJsonData(output.as<String>().c_str());

      input.clear();
      output.clear();
    }

    void processJsonData(const char* param){
      /*
        {
          "orign": "/app/state",
          "status": 1,
          "body": {}
        }
      */
      //DEBUG_PRINT("\nprocessJsonData(%s...)",param);
      
      JsonDocument doc;    
      auto error = deserializeJson(doc, param);
      if (error) {
          //DEBUG_PRINT("\nprocessJsonData parsing failed: %s",error.c_str()); 
          doc.clear();
          return;
      }

      uint8_t status = doc[API_STATUS];
      JsonVariant data = doc[API_BODY];
      String origin = doc[API_ORIGIN];     

      /*
      String dataString;
      serializeJson(data, dataString);      
      DEBUG_PRINT("\n\tSTATUS: <%s>",status==API_OK?"ok":"error");
      DEBUG_PRINT("\n\tORIGIN: <%s>",origin.c_str());
      DEBUG_PRINT("\n\tDATA: <%s>",dataString.substring(0,50).c_str());*/

      if (origin == APICALL_SYNC_CONSOLE_INFO){
        if (status == API_OK){
          
          chNames->init(
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,chNames->shortName,"count",String(2)).toInt(),
            UNSYNC_PATTERN "MSCH", 
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,chNames->shortName,"offset","0").toInt()
          );

          busNames->init(
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,busNames->shortName,"count",String(2)).toInt(),
            UNSYNC_PATTERN "MSBUS", 
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,busNames->shortName,"offset","0").toInt()
          );
          
          auxNames->init(
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,auxNames->shortName,"count",String(2)).toInt(),
            UNSYNC_PATTERN "MSAUX", 
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,auxNames->shortName,"offset","0").toInt()
          );
          
          fxRetNames->init(
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,fxRetNames->shortName,"count",String(2)).toInt(),
            UNSYNC_PATTERN "MSFX", 
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,fxRetNames->shortName,"offset","0").toInt()
          );
          
          mainNames->init(
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,mainNames->shortName,"count",String(2)).toInt(),
            UNSYNC_PATTERN "MSMAIN", 
            getAdjacentValueByKeyFromRootArray(data,MS_CHANNEL_TYPES,MS_TYPE_SHORTNAME_KEY,mainNames->shortName,"offset","0").toInt()
          );


          std::sort(dsNames.begin(), dsNames.end(), [](const DataSetSynced* a, const DataSetSynced* b) {
              return a->offset > b->offset; // Tri décroissant
          });


          lastQueryTime = 0; // speed up the call to ping with no delay in between

          setSynced(origin);
        }else{
          setConnectFailed();
          setSynced(origin,false);
        }
      }
      
      else if (origin == APICALL_APP_STATE){
        if (status == API_OK){
          API_Mixer_State = data["topState"].as<String>();
          interfaceSubState = data["state"].as<String>();
          syncSoftwareProgress = data["progress"].as<int>();
          //DEBUG_PRINT("\n\t\t\t\t\t\t ---- REST API STATE %s %d% ----", API_Mixer_State.c_str(), (int)syncSoftwareProgress);
          //syncSoftwareProgress = syncSoftwareProgress * 1.1; // we fill the gap due to the latency of http checks
          if (syncSoftwareProgress > 100) syncSoftwareProgress = 100; 
          if (interfaceState == SYNCING_SOFTWARE) lastQueryTime = 0; // speed up the call to ping with no delay in between
        }else{
          API_Mixer_State = "offline";
          //DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! REST API Offline !!!----");          
        }
      }
      
      else if (origin == APICALL_CONNECT){       
        if (status == API_OK){
          API_Mixer_State = "connecting";
          syncSoftwareProgress = 0;
          lastQueryTime = 0; // speed up the call to ping with no delay in between          
        }else{
          setConnectFailed();
        }
      }

      else if (origin == APICALL_SYNC_AVAILABLE_MODELS){
        // this call is to retrieve all supported console by MS sor we can store them in the interfaces.json definition file in a compressed format
        if (status == API_OK){
          JsonDocument interfaceDoc;      
          MixerInterface::deserialize(&interfaceDoc);

          JsonArray interfaces = interfaceDoc[CONSOLE_KEY];
          interfaces.clear();

          // Accéder au tableau "consoles"
          if (!data[CONSOLE_KEY].is<JsonArray>()) {
                //DEBUG_PRINT("\n '%s' not found or not an array in JSON",CONSOLE_KEY);
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
          if (serialize(interfaceDoc)){
            //DEBUG_PRINT("\nmixingstation serialized correctly");
          }else{
            //DEBUG_PRINT("\nmixingstation serialized failed");
          }
          setSynced(origin);
          lastQueryTime = 0; // speed up the call to ping with no delay in between          
        }else{
          setConnectFailed();
          setSynced(origin,false);
        }
      }

      else if (origin == APICALL_SYNC_MIXTARGETS){
        if (status == API_OK){            
          setSynced(origin);
        }else{
          setConnectFailed();
          setSynced(origin,false);
        }        

      }

      else if (origin == APICALL_SYNC_MIXER_CURRENT){
        if (status == API_OK){                    
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
          setSynced(origin);
        }else{
          setConnectFailed();
          setSynced(origin,false);
        }        

      }
     
      else if (origin == "Z"){
        
      }

      else {

          //DEBUG_PRINT("\n\t%s",doc.as<String>().c_str());

      }

      //data.clear();  // doesnt change
      doc.clear();
    }

    void setConnectFailed(){
        //DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! COMMAND FAILED !!!----");                          
        interfaceState = CONNECTFAILED;
    }

    bool getNextSyncAPICall(String* val){                      
      if (APIsyncIndex == mixerAPISync.size()){
        *val = "";
        return false;
      }
      *val = mixerAPISync[APIsyncIndex].APIcall;
      APIsyncIndex++; 
      return true;     
    }

    void resetSyncData(){
      //DEBUG_PRINT("\n\n\n\n\t\t\t\t\t\t!!! RESET SYNC DATA !!!\n\n\n\n\n");
      APIsyncIndex = 0;
      for (auto&sync : mixerAPISync) sync.synced = false;  
      for (const auto& dsName : dsNames) dsName->syncIndex = 0;
      serialized = false;
    }

    /*void dropAPI_http_requests(){
      if (!msRESTAPIClient.isClientConnecting){         
        // this is where we drop REST API request
        msRESTAPIClient.method = HTTP_METHOD_GET;
        if (interfaceState == SYNCING_DATA){
          // this is to drop the next request to retrieve data from the mixer during sync phase
          if (!getNextSyncAPICall(&msRESTAPIClient.URL)) msRESTAPIClient.URL = "";
        }else{
          // this is the request which is dropped periodically to monitor if application is still live
          msRESTAPIClient.URL = APICALL_APP_STATE;
        }
        hwm.network->asyncConnect(&msRESTAPIClient);                     
      }
    }*/


    void dropAPI_ws_requests(){
      String API_Call = APICALL_APP_STATE;      
      
      if (interfaceState == SYNCING_DATA){
        if (!getNextSyncAPICall(&API_Call)) API_Call = APICALL_APP_STATE;   // ======> bigger request use 7430 bytes => /console/mixTargets , use dropAPI_http_requests for this one
      }

      //{ "path": "/app/state", "method": "GET", "body": null }
      JsonDocument doc;
      doc[MS_API_PATH] = API_Call;   
      doc[API_BODY] = doc.createNestedObject(); //empty object {}

      if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN) hwm.network->clientSocketSend(doc.as<String>().c_str());
      else hwm.network->clientSocketOpen(osc->ip.toString().c_str(), ms_API_port, "/");
      
      doc.clear();
    }

    virtual void queryState(){       
      //dropAPI_http_requests();
      dropAPI_ws_requests();

      queryDelay = MS_PING_DELAY;
      //////////////////////////////////// REST API HAS RESPONDED
      if (interfaceSubState == "syncing"){
        // detect the state change
        if(interfaceState != SYNCING_SOFTWARE){
          resetSyncData();
        }
        interfaceState = SYNCING_SOFTWARE;
        //lastQueryTime = 0; // try the next one straight with no delay
      }
      else if (API_Mixer_State == "connecting" || API_Mixer_State == "reconnecting"){
        interfaceState = CONNECTING;
      }
      else if (API_Mixer_State == "idle"){
        interfaceState = IDLE;
      }else{
        /////////////////////////////////////  REST API HAS !NOT RESPONDED! OR !IS CONNECTED!
        if (OSCechoAttempt>1){
          if (API_Mixer_State == "connected"){
            interfaceState = MIXER_UNREACHABLE;        // inconsistent
            LogUDP::sendLog("MIXER_UNREACHABLE",hwm.network->udp);        
          } 
          else if (API_Mixer_State == "offline"){
            disconnect();
          }
        }else{
          if (API_Mixer_State == "connected"){        
            if (progress()<100){     // if the mixer days it is connected and we retreived its data, then we set the interface status to CONNECTED
              interfaceState = SYNCING_DATA;
            }else{           
              interfaceState = CONNECTED;               
              queryDelay = MS_PING_DELAY * 2; // we slow down the check as we are connected                
              // now we save the mixing station interface properties so in case user starts offline, we are able to provide the virtual information like number of buses and channels, etc... from the last connected device
              if (!serialized){ // only do it once at first connection
                JsonDocument interfaceDoc;
                deserialize(&interfaceDoc);
                modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, BUS_SIZE_KEY, busNames->getSize());
                modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, CH_SIZE_KEY, chNames->getSize());
                modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, AUX_SIZE_KEY, auxNames->getSize());
                modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, FXRET_SIZE_KEY, fxRetNames->getSize());
                modifyAdjacentKeyValueIntOfRootArray(interfaceDoc, INTERFACE_KEY, MODEL_KEY, model, MAIN_SIZE_KEY, mainNames->getSize());
                if (serialize(interfaceDoc)>0){                //save the properties into interfaces.json file                              
                  //DEBUG_PRINT("\nmixing station properties saved into interface file");
                  serialized = true;
                }else{
                  //DEBUG_PRINT("\nError savingmixing station properties into interface file");
                }
              }
            }
          }   
          else if (API_Mixer_State == "offline"){
            interfaceState = SOFTWARE_UNREACHABLE;  // inconsistent
          }      
        }          
      }
      DEBUG_PRINT("\n\t\tqueryState()=> %d\n",interfaceState);
    }

    virtual void syncDataSet(){
      // data sets can only be made available once the mixer info and mix targets has been retrieved, especially offset of the names etc...
      if (getSynced(APICALL_SYNC_CONSOLE_INFO) && getSynced(APICALL_SYNC_MIXTARGETS)){
        dropParrallelAsyncQuery(); // for mixing station => in case it push data every 3 seconds, there is no need to call for a syncQuery as it will come anyway
      }
      return;
    }
    
    size_t serialize(JsonVariant interfaceDoc){
      String dataToCompress;
      serializeJson(interfaceDoc,dataToCompress);      
      size_t dataSize = writeBufferToGzFile("/webapp/interfaces.json.gz",dataToCompress.c_str());
      return dataSize;
      
    }

    virtual void disconnect(){
      interfaceState = DISCONNECTED;     // no REST API and no OSC
      //DEBUG_PRINT("\nWaiting semaphore release in disconnect()");
      //if (xSemaphoreTake(msRESTAPIClient.httpAsyncMutex, portMAX_DELAY)) { // Prendre le mutex
      unsigned long t1 = millis();
      #define TIMEOUT_HTTP 500
      while (msRESTAPIClient.isClientConnecting == true && millis()-t1 < TIMEOUT_HTTP) yield();

      //DEBUG_PRINT("\ndeleting task");
      //vTaskDelete(msRESTAPIClient.taskHandle); // Supprimer la tâche
      //msRESTAPIClient.taskHandle = NULL;       // Mettre à jour le pointeur
      if (msRESTAPIClient.client != NULL) msRESTAPIClient.client->stop();
      hwm.network->clientSocketClose();
      //DEBUG_PRINT("\nnetwork client stoppped");
      //msRESTAPIClient.taskHandle  = nullptr;
        
        //xSemaphoreGive(msRESTAPIClient.httpAsyncMutex); // Relâcher le mutex
      
    }

    virtual void connect(){       
      // launch the  REST API command to connect in case user config say so.
      if (app.userConfig.getInt(MIXER_AUTOMODEL_KEY) != -1){       

        if (msRESTAPIClient.isClientConnecting) return;
        if (API_Mixer_State != "idle") return;
        if (interfaceState != IDLE) return;

        msRESTAPIClient.method = HTTP_METHOD_POST;
        msRESTAPIClient.URL = APICALL_CONNECT;
        JsonDocument postParam;
        postParam["consoleId"] = app.userConfig.getInt(MIXER_AUTOMODEL_KEY);
        postParam["ip"] = *app.userConfig.getString(MIXER_AUTOIP_KEY);
        serializeJson(postParam, msRESTAPIClient.post);

        if (hwm.network->asyncConnect(&msRESTAPIClient)){
          syncSoftwareProgress = 0; 
          API_Mixer_State = "connecting";
          //DEBUG_PRINT("\nCONNECT MS TO %s\n", app.userConfig.getString(MIXER_AUTOIP_KEY)->c_str());      
          LogUDP::sendLog("AUTO CONNECT MS",hwm.network->udp);
          lastQueryTime = 0; // try the next one straight with no delay
        }
      }
    }

    /*virtual String getValue(String addrr, uint16_t index){
      String value="";

      if (addrr == ADDRESS_CHNAME){
        value = concatStrings("CHMS", index);
        index += chOffset;
      }
      else if (addrr == ADDRESS_BUSNAME){
        value = concatStrings("BUSMS", index);
        index += busOffset;
      }

      // /con/n/ch.18.cfg.name,s,newname8

      sprintf(OSCCommand, chNames->pattern.c_str(), index);
      if (osc->ping(OSCCommand)){
        value = osc->sValue;
      }

      return value;
    }  */   

    virtual void parseInterfaceProtocol(JsonVariant interfaceDoc){
      MixerInterface::parseInterfaceProtocol(interfaceDoc);

      for (const auto& dsName : dsNames) dsName->pattern = OSC_MS_PRE "/ch.%d.cfg.name";

      //DEBUG_PRINT("\n\n\n\tAPPEL DERIVE MS => chNames->pattern = %s",chNames->pattern.c_str());
      //String getNameOSCcommand = getAdjacentValueByKeyFromRootArray(interfaceDoc,INTERFACE_KEY,MODEL_KEY,model,"getName","ms"); 
      //DEBUG_PRINT("\n\n\n");          
    }


    virtual bool updateName(DataSetSynced* ds, JsonObject& val) override{
      // since mixingstation use the same address for updating different properties (ch/bus...) 
      // the routing is performed based on the offset which tells which one is the target
      int index = 0;
      if (sscanf(val[EXEC_ADDRESS], ds->pattern.c_str(), &index) != 1){
        //DEBUG_PRINT("\nparse %s failed",ds->pattern.c_str());
      }

      for (const auto& dsName : dsNames) {          
          //dsName are sorted 
          if (index >= dsName->offset) if (OSCMixerInterface::updateName(dsName,val)) return true;
      }

      return false;
    }


};

#endif