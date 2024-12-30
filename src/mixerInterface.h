#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#include "oscInterface.h"
#include <ArduinoJson.h>

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define BEHRINGER_RESTART_PERIOD    9000     
#define BEHRINGER_PING_DELAY        3000     

#define MS_RESTART_PERIOD     3000     
#define MS_PING_DELAY         3000     


#define ADDRESS_CHNAME "/ch/config/%d/name"
#define ADDRESS_BUSNAME "/bus/config/%d/name"

#define INTERFACES_FILENAME "/webapp/interfaces.json.gz"

#define MIXING_STATION_MODEL "MixingStation"

#define INTERFACE_STATE_KEY "interfaceState"
  #define MIXER_PORT_KEY "mixerPort"
  #define MIXER_CONNECTED_KEY "mixerConnected"
  #define MIXER_AUTOMODEL_KEY "autoConnectModel"
  #define MIXER_AUTOIP_KEY "autoConnectIP"

enum{
  IDLE = CONNECTED+1,
  CONNECTING,
  SYNCING,
  MIXER_UNREACHABLE,
  MS_UNREACHABLE,
  SEARCHING,
  CONNECTFAILED,
  EOF_STATE
};

#define UNDEF "undef"

#define MS_API_STATE "/app/state"

String stateNames[EOF_STATE]={
  "disconnected",
  "connected",
  "idle",
  "connecting",
  "syncing",
  "mixer unreachable",
  "mixing s. unreachable",
  "searching",
  "connect failed"
};

// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface : public DataProvider{
  public:
    String model;
    DataRepository* dataRepository = nullptr;
    String name="abstract";
    uint8_t interfaceState = DISCONNECTED;

    float progress = 0.0;  // connection progression indicator in %

    virtual void queryState()=0;

    DataSet* chNames;
    DataSet* busNames;

    MixerInterface(DataRepository* dr,String _model){  
      if (_model == "") model = MIXING_STATION_MODEL;
      else model = _model;          

      DEBUG_PRINT("\nnew MixerInterface(%s)",model.c_str());

      dataRepository = dr;

      chNames = new DataSet(ADDRESS_CHNAME,INPUTSELECT,this);
      busNames = new DataSet(ADDRESS_BUSNAME,INPUTSELECT,this);

      dataRepository->addDataSet(chNames);
      dataRepository->addDataSet(busNames);      
      
    
      // Ouvrir le fichier JSON
      /*File file = LittleFS.open(INTERFACES_FILENAME, "r");
      if (!file) {
          DEBUG_PRINT("\nFailed to open JSON interface file");
          return;
      }*/

      ByteBuffer decompressedJson;
      size_t dataSize = 0;
      dataSize = readGzFileToBuffer(INTERFACES_FILENAME,&decompressedJson);
      if (dataSize == 0){ // uncompress failed
        DEBUG_PRINT("\nFailed to decompress JSON interface file");
        return;
      }

      JsonDocument interfaceDoc;
      // Désérialiser les données JSON directement à partir du flux de fichier
      auto error = deserializeJson(interfaceDoc, decompressedJson.buffer);
      if (error) {
          DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());
          return;
      }

      //file.close(); // Fermer le fichier

      // size of the data sets
      chSize = getValueByName("chSize",interfaceDoc,"6").toInt();      
      busSize = getValueByName("busSize",interfaceDoc,"2").toInt();

      // move code below on top to still be able to work in case the JSON file can not be opened




    };

    virtual ~MixerInterface(){

      dataRepository->removeDataSet(chNames);
      delete chNames;

      dataRepository->removeDataSet(busNames);      
      delete busNames;

      //DEBUG_PRINT("\n\n~MixerInterface()");
    }

    uint16_t size(String addrr){
      uint16_t n=0;
      if (addrr == ADDRESS_CHNAME) return chSize;
      else if (addrr == ADDRESS_BUSNAME) return busSize;
      else return 0;
      //DEBUG_PRINT("\nSizeof(BehringerMixer,%s)=%i",addrr.c_str(),n);
    }

    virtual bool update()=0;
    
    String getValueByName(String key, JsonDocument interfaceDoc, String defaultValue="") {
        // Vérifier si le document contient une clé "consoles"
        if (!interfaceDoc["interfaces"].is<JsonArray>()) {
            DEBUG_PRINT("\nKey 'interfaces' not found or not an array in JSON");
            return defaultValue;
        }

        // Parcourir le tableau "interfaces"
        JsonArray interfaces = interfaceDoc["interfaces"];
        for (JsonObject interface : interfaces) {
            // Vérifier si la clé "name" correspond
            if (interface["model"] == model) {
                // Retourner la valeur de la clé spécifiée si elle existe
                if (interface.containsKey(key.c_str())) {
                    String s = interface[key.c_str()];
                    DEBUG_PRINT("\nKey :%s = %s",key.c_str(),s.c_str());
                    return s;
                } else {
                    DEBUG_PRINT("\nKey :%s   not found.",key.c_str());
                    return defaultValue;
                }
            }
        }

        DEBUG_PRINT("\ninterface <%s> not found, returning default nKey :%s = %s",model.c_str(),key.c_str(),defaultValue.c_str());
        return defaultValue;
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
    uint16_t chSize=1,busSize=1;    

};


class OSCMixerInterface: public MixerInterface {

  public: 
    static String echoCommand;    
    bool OSClive = true;

    OSCMixerInterface(DataRepository* dr,String m, OSCInterface* _osc): osc(_osc), MixerInterface(dr,m){
      DEBUG_PRINT("\nnew OSCMixerInterface(%s %s,%d)",model.c_str(),osc->ip.toString().c_str(),osc->port);
    }

    virtual bool monitor(){

      // restart monitoring value
      if (interfaceState == CONNECTED){
        if (millis()-lastRestartMonitorTime > restartMonitorDelay){
          osc->SendCmd2OSC(restartMonitorCommand.c_str());          // otherwise we ask mixer to send message for the next 10 sec (see behringer protocol)
          lastRestartMonitorTime = millis();
        }
      }

      // processing OSC queue
      if (osc->OSCReadIn()){
        lastOSCCheck = millis();
        lastQueryTime = millis();   // REMOVE THIS ONE IF YOU STILL WANT TO CHECK REST API WHEN OSC IS RESPONDING (CONSUMME MORE TIME)        
        OSClive = true;
        DEBUG_PRINT("\t* * * OSCLIVE true * * *\t");
        return true;
      }

      if (millis()-lastOSCCheck > queryDelay*2){
        lastOSCCheck = millis();
        OSClive = false;
        DEBUG_PRINT("\t!!! OSCLIVE false !!!\t");
        LogUDP::sendLog("OSCLIVE false",hwm.network->udp);        
      } 
            
      /////// MANAGE CONNECTION ////////////////////////////////////////////////////   

      if (millis()-lastQueryTime > queryDelay){
        lastQueryTime = millis();
        queryState();
          //DEBUG_PRINT("\t--- ----\t");
          //OSClive = true;     // we reset the flag and assume that OSC is reachable
          osc->SendCmd2OSC(echoCommand.c_str());        
      }     

      return false;
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

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="ch"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="bus"+String(index);
      return v;
    }

  };


// Implémentation spécifique pour Behringer
class BehringerMixer : public OSCMixerInterface {
  
  public:

    BehringerMixer(DataRepository* dr,String model,OSCInterface* _osc): OSCMixerInterface(dr,model,_osc){
      DEBUG_PRINT("\n\nNew BehringerMixer(<%s>)\n\n",model.c_str());
      name="MusicTribe";
      echoCommand = "/info";
      restartMonitorCommand = "/xremote";
      restartMonitorDelay = BEHRINGER_RESTART_PERIOD;
      queryDelay = BEHRINGER_PING_DELAY;
      interfaceState = CONNECTED;
    }

    virtual ~BehringerMixer(){
      DEBUG_PRINT("\n~BehringerMixer()");   
    }

    virtual void queryState(){
      //if (!osc->ping(echoCommand)) interfaceState = DISCONNECTED;
      if (!OSClive) interfaceState = DISCONNECTED;
    }

    virtual void connect(Config* conf){
      DEBUG_PRINT("\nConnected to %s",name.c_str());
    }

    virtual void disconnect(){}

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

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="CHBehr"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="BUSBehr"+String(index);
      return v;
    }   

};

// Implémentation spécifique pour MixingStation
#define MIXING_STATION_CHECK "/con/v/ch.0.solo"
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
    String RESTState=UNDEF;

  public:

    MixingStation(DataRepository* dr, String _model, OSCInterface* osc,Network* net,uint16_t _port): network(net), REST_API_port(_port), OSCMixerInterface(dr,_model, osc){      
      
      DEBUG_PRINT("\n\nNew MixingStation()\n\n");   
      name=MIXING_STATION_MODEL; 
      echoCommand = MIXING_STATION_CHECK;
      restartMonitorCommand = "/hi/n";
      restartMonitorDelay = MS_RESTART_PERIOD;
      queryDelay = MS_PING_DELAY;      

      msRESTAPIClient.host = osc->ip;
      msRESTAPIClient.port = REST_API_port; 
      msRESTAPIClient.timeOut = queryDelay-1000; // maximum duration of the request is the period of check minus one second
      msRESTAPIClient.client = network->client;

      interfaceState = SEARCHING;
      progress = 0.0;

    }

    virtual ~MixingStation(){
      DEBUG_PRINT("\n~MixingStation()");   
    }

    virtual uint8_t state(){
      return interfaceState;
    }

    // call back for REST API state check
    void RESTAPIState(String param) {
        if (param == "fail"){
          // if we didn't have any answer from the HTTP rest, it means the server is also disconnected
          RESTState = "offline";
          DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! REST API Offline !!!----");          
        }else{
          RESTState = getJsonState(param, &progress);         
          DEBUG_PRINT("\n\t\t\t\t\t\t ---- REST API STATE %s %d% ----", RESTState.c_str(), (int)progress);
          progress = progress * 1.1; // we fill the gap due to the latency of http checks
          if (progress > 100) progress = 100; 
          if (interfaceState == SYNCING) lastQueryTime = 0; // speed up the call to ping with no delay in between
        }
    }

    // call back for REST API command send
    void RESTAPICommand(String param) {
      if (param == "failed"){
        // if we didn't have any answer from the HTTP rest, it means the server is also disconnected
        interfaceState = CONNECTFAILED;
        DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! AUTO CONNECT FAILED !!!----");          
      }else{
        DEBUG_PRINT("\n\t\t\t\t\t\t ---- REST API COMMAND %s ----", param.c_str());
        RESTState = "connecting";
        progress = 0.0;
      }
    }


    virtual void queryState(){       

      if (!msRESTAPIClient.isClientConnecting){
        msRESTAPIClient.method = HTTP_METHOD_GET;
        msRESTAPIClient.URL = MS_API_STATE;
        msRESTAPIClient.post = "";
        msRESTAPIClient.onConnectedCallback = [&](String param) {RESTAPIState(param);};    
        msRESTAPIClient.failConnectCallback = [&]() {RESTAPIState("fail");};   
        network->asyncConnect(&msRESTAPIClient);
      }
      //if (RESTState == UNDEF) return; // we wait completion of the first REST API query to continue detrmining the full status

      queryDelay = MS_PING_DELAY;

      //////////////////////////////////// REST API HAS RESPONDED

      if (RESTState == "syncing"){
        interfaceState = SYNCING;
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
            interfaceState = CONNECTED;   // all good
            queryDelay = MS_PING_DELAY * 2; // we slow down the check as we are connected

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
            interfaceState = DISCONNECTED;     // no REST API and no OSC
            if (msRESTAPIClient.taskHandle != nullptr){
              DEBUG_PRINT("\ndeleting task");
              vTaskDelete(msRESTAPIClient.taskHandle);
              DEBUG_PRINT("\ntask deleted");
              if (msRESTAPIClient.client != NULL) msRESTAPIClient.client->stop();
              DEBUG_PRINT("\nclient stoppped");
              msRESTAPIClient.taskHandle  = nullptr;
            }   
          }
          //else interfaceState == SEARCHING;
        }           

      }

      DEBUG_PRINT("\n\n\t\tqueryState()=> %d\n\n",interfaceState);

    }

    virtual void disconnect(){
     
    }

    static String getJsonState(String jsonString, float* prog){
      // Utiliser ArduinoJson pour parser le JSON
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, jsonString);     
      if (!error) {
        //doc["msg"];       // unused
        *prog = doc["progress"]; 
        return doc["state"];
      }
      DEBUG_PRINT("\nparseJsonState error");        
      return "error";            
    }

    virtual void connect(Config* config){       
      // launch the  REST API command to connect in case user config say so.
      if (config->getInt(MIXER_AUTOMODEL_KEY) == -1){
        // do nothing, just wait
      }else{       
        
        if (msRESTAPIClient.isClientConnecting) return;
        if (RESTState != "idle") return;
        if (interfaceState != IDLE) return;

        msRESTAPIClient.method = HTTP_METHOD_POST;
        msRESTAPIClient.URL = "/app/mixers/connect";
        JsonDocument postParam;
        postParam["consoleId"] = config->getInt(MIXER_AUTOMODEL_KEY);
        //postParam["mixTarget"] = -1; // no restrictions   // not used anymore
        postParam["ip"] = *config->getString(MIXER_AUTOIP_KEY);
        serializeJson(postParam, msRESTAPIClient.post);

        msRESTAPIClient.onConnectedCallback = [&](String param) {RESTAPICommand(param);};    
        msRESTAPIClient.failConnectCallback = [&]() {RESTAPICommand("failed");};

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
        DEBUG_PRINT("\nMixing station processs QUEUE");
        if (osc->addressStr == "value"){

        }else if (osc->addressStr == "value2"){

        }
        return true;
      }
      return false;
    }

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="CHMS"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="BUSMS"+String(index);
      return v;
    }     

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


};

#endif