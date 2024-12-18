#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#include "oscInterface.h"
#include <ArduinoJson.h>

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define MS_PING_DELAY         6000     
#define BEHRINGER_PING_DELAY  3000     

#define MS_RESTART_PERIOD           3000     
#define BEHRINGER_RESTART_PERIOD    9000     

#define ADDRESS_CHNAME "/ch/config/%d/name"
#define ADDRESS_BUSNAME "/bus/config/%d/name"

#define INTERFACES_FILENAME "/interfaces.json"

#define MIXING_STATION_MODEL "MixingStation"

enum{
  IDLE = CONNECTED+1,
  CONNECTING,
  SYNCING,
  BAD_OSC_PORT,
  BAD_REST_PORT,
  UNDEF_STATE
};

// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface : public DataProvider{
  public:
    String model;
    DataRepository* dataRepository = nullptr;
    String name="abstract";

    float progress = 0.0;  // connection progression indicator in %

    virtual uint8_t state()=0;
    virtual void pingInterface()=0;

    DataSet* chNames;
    DataSet* busNames;

    MixerInterface(DataRepository* dr,String _model){  
      if (_model == "") model = MIXING_STATION_MODEL;
      else model = _model;          

      DEBUG_PRINT("\nnew MixerInterface(%s)",model.c_str());

      dataRepository = dr;
      
    
      // Ouvrir le fichier JSON
      File file = LittleFS.open(INTERFACES_FILENAME, "r");
      if (!file) {
          DEBUG_PRINT("\nFailed to open JSON interface file");
          return;
      }


      JsonDocument interfaceDoc;
      // Désérialiser les données JSON directement à partir du flux de fichier
      auto error = deserializeJson(interfaceDoc, file);
      if (error) {
          DEBUG_PRINT("\nJSON interface parsing failed: %s",error.c_str());
          return;
      }

      file.close(); // Fermer le fichier

      // size of the data sets
      chSize = getValueByName("chSize",interfaceDoc,"6").toInt();      
      busSize = getValueByName("busSize",interfaceDoc,"2").toInt();
      // command strings

      chNames = new DataSet(ADDRESS_CHNAME,INPUTSELECT,this);
      busNames = new DataSet(ADDRESS_BUSNAME,INPUTSELECT,this);

      dataRepository->addDataSet(chNames);
      dataRepository->addDataSet(busNames);


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

    virtual void disconnect() = 0;                    // Déconnexion spécifique
    virtual void connect()=0;
    virtual bool sendCommand(String command, JsonDocument param) = 0;

  protected:
    uint16_t chSize,busSize;    

};


class OSCMixerInterface: public MixerInterface {

  public: 
    static String echoCommand;    
    bool mixerOnline = false;  

    OSCMixerInterface(DataRepository* dr,String m, OSCInterface* _osc, bool* _serverConnection): serverConnection(_serverConnection),osc(_osc), MixerInterface(dr,m){
      DEBUG_PRINT("\nnew OSCMixerInterface(%s %s,%d)",model.c_str(),osc->ip.toString().c_str(),osc->port);
    }

    virtual bool monitor(){

      if (millis()-lastPingTime > pingDelay){
        lastPingTime = millis();
        
        if (mixerOnline){          
          if ((state() == DISCONNECTED) && *serverConnection == CONNECTED) connect();
          /*else{
            if (state() == BAD_OSC_PORT) osc->ping(echoCommand);
          }*/
          else pingInterface();            // next pulse          
        }else{
          if (state() == DISCONNECTED) pingInterface();     // we were disconnected so we try to reconnect
          else disconnect();                                // we were connected so we disconnect
          return false;
        }
      }

      if (state()==CONNECTED){

        if (millis()-lastRestartMonitorTime > restartMonitorDelay){
          osc->SendCmd2OSC(restartMonitorCommand.c_str());          // otherwise we ask mixer to send message for the next 10 sec (see behringer protocol)
          lastRestartMonitorTime = millis();
        }
      
        // processing OSC queue
        if (osc->OSCReadIn()){
          lastPingTime = millis();
          /*if (osc->addressStr == echoCommand)
            monitoringSuccessful = true;*/
          return true;  // we manage to read an OSC command so we return true to be used by the update() method of the mixer interface
        }
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
    uint16_t restartMonitorDelay=0, pingDelay=0;

    String restartMonitorCommand ="";
    OSCInterface* osc;
    unsigned long lastPingTime = 0;
    //static bool monitoringSuccessful; 
    //static uint8_t connectAttempt;    
    unsigned long lastRestartMonitorTime = 0;   
    bool* serverConnection; 
};

String OSCMixerInterface::echoCommand = "";

//bool OSCMixerInterface::monitoringSuccessful = false; 
//uint8_t OSCMixerInterface::connectAttempt = 0;

class MixerOfflineInterface: public MixerInterface {
  public:

    MixerOfflineInterface(DataRepository* dr,String m): MixerInterface(dr,m){
      DEBUG_PRINT("\n\nNew MixerOfflineInterface(<%s>)\n\n",model.c_str());
      name="Offline";
    }

    virtual ~MixerOfflineInterface(){
      DEBUG_PRINT("\n~MixerOfflineInterface()");   
      disconnect();   
    }

    virtual bool update(){
      return true;
    } 

    virtual void connect(){
      // do nothing
    }

    virtual uint8_t state(){
      return DISCONNECTED;
    }

    virtual void pingInterface(){}

    void disconnect() override{
      DEBUG_PRINT("\nMixerOfflineInterface disconnected");
    }                   

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

    BehringerMixer(DataRepository* dr,String model,OSCInterface* _osc,bool* s): OSCMixerInterface(dr,model,_osc, s){
      DEBUG_PRINT("\n\nNew BehringerMixer(<%s>)\n\n",model.c_str());
      name="MusicTribe";
      echoCommand = "/info";
      restartMonitorCommand = "/xremote";
      restartMonitorDelay = BEHRINGER_RESTART_PERIOD;
      pingDelay = BEHRINGER_PING_DELAY;
    }

    virtual ~BehringerMixer(){
      DEBUG_PRINT("\n~BehringerMixer()");   
      disconnect();   
    }

    virtual void pingInterface(){
      *serverConnection = osc->ping(echoCommand);
      mixerOnline = true;
    }

    virtual uint8_t state(){
      // here the state is only based on the server but we may imagine to process other state like syncing
      return *serverConnection?CONNECTED:DISCONNECTED;
    }

    virtual void connect(){
      *serverConnection = CONNECTED;
      DEBUG_PRINT("\nConnected to %s",name.c_str());
    }

    void disconnect() override {
      //client.stop();
      *serverConnection = DISCONNECTED;
      DEBUG_PRINT("\nBehringerMixer disconnected");
    }

    bool sendCommand(String command, JsonDocument param) override {
      bool success;
      if (state() == DISCONNECTED) {
        DEBUG_PRINT("\nNon connecté au mixeur Behringer.");
        return false;
      }else success = OSCMixerInterface::sendCommand(command, param);
      if (success) return true; // command has been consummed
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
#define MIXING_STATION_CHECK "/con/v/mixer.rta.src"
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
    AsyncHttpClient msHttpClientParam;
    uint16_t REST_API_port;
    Network* network;

  public:
    uint8_t interfaceState = DISCONNECTED;

    MixingStation(DataRepository* dr, String _model, OSCInterface* osc,Network* net,uint16_t _port, bool* serverConnection): network(net), REST_API_port(_port), OSCMixerInterface(dr,_model, osc, serverConnection){
      DEBUG_PRINT("\n\nNew MixingStation()\n\n");   
      name=MIXING_STATION_MODEL; 
      echoCommand = MIXING_STATION_CHECK;
      restartMonitorCommand = "/hi/n";
      restartMonitorDelay = MS_RESTART_PERIOD;
      pingDelay = MS_PING_DELAY;      

    }

    virtual ~MixingStation(){
      DEBUG_PRINT("\n~MixingStation()");   
      disconnect();   
    }

    virtual uint8_t state(){
      return interfaceState;
    }

    /*bool OSCCheckOnce(){
      // this method is to cross check (only once) if OSC ping reply
      //as we may have the case that pingInterface() reply state: connect
      //while the OSC port is not answering and we need both
      //in case we do not need both we would just return true or treat the OSC state as a separate state of the mixer
            
      if (!osc->live){
        return osc->ping(echoCommand);
        DEBUG_PRINT("\n <<< OSCCheckOnce() >>> called because OSC is pending check");
      } 
      else return true;

    }*/

    virtual void pingInterface(){
      msHttpClientParam.host = osc->ip;
      msHttpClientParam.port = REST_API_port; 
      msHttpClientParam.timeOut = pingDelay-1000; // maximum duration of the request is the period of check minus one second
      msHttpClientParam.client = network->http;
      msHttpClientParam.URL = "/app/state";

      msHttpClientParam.onConnectedCallback = [&](String param) { 
        
        interfaceState = getJsonState(param, &progress); 
        mixerOnline = true;
        DEBUG_PRINT("\n\t\t\t\t\t\t ---- mixerOnline %d %d%----",interfaceState,(int)progress);
        progress = progress *1.1; // we fill the  gap due to the latency of http checks
        if (progress>100) progress = 100; 
        if (interfaceState == SYNCING) lastPingTime = 0; // speed up the call to ping with no delay in between 

        if (interfaceState == CONNECTED && !osc->live){
          interfaceState = BAD_OSC_PORT;
          osc->ping(echoCommand);          
        }
        
        /*if (interfaceState == CONNECTED){ 
          if (!OSCCheckOnce()) interfaceState = BAD_OSC_PORT; // read comment in OSCCheckOnce to understand why we put this call here
        }*/
        DEBUG_PRINT("\nInterfaceState: %d",interfaceState);
      };
      
      msHttpClientParam.failConnectCallback = [&]() { 
        // if we didn't have any answer from the HTTP rest, it means the server is also disconnected => no because we also have OSC ping
        //*serverConnection = DISCONNECTED;
        mixerOnline = false;
        DEBUG_PRINT("\n\t\t\t\t\t\t ----!!! no reply to ping !!!----");
        interfaceState = BAD_REST_PORT;        
      };           
      
      if (!network->asyncConnect(&msHttpClientParam)) lastPingTime = 0; // there is a pending request so we try the next one straight with no delay

    }

    static uint8_t getJsonState(String jsonString, float* prog){
      // Utiliser ArduinoJson pour parser le JSON
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, jsonString);     

      if (!error) {
        //doc["msg"];       // unused
        *prog = doc["progress"]; 
        String state = doc["state"];
        if (state == "connected") return CONNECTED;
        else if (state == "idle") return IDLE;
        else if (state == "connecting") return CONNECTING;
        else if (state =="syncing") return SYNCING;
      } else {
        DEBUG_PRINT("\nparseJsonState error");        
      }
      return UNDEF_STATE;
    }

    virtual void connect(){
      interfaceState = CONNECTED;                
      if (osc->live){
       // launch the  REST API command to connect in case user config say so.
       // do it independantly from the OSC live or not
       DEBUG_PRINT("\n\nREST API CONNECT YES/NO !\n");
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

    void disconnect() override {
      //client.stop();
      interfaceState = DISCONNECTED;
      osc->live = false;
      progress = 0;
      DEBUG_PRINT("\nMixingStation disconnected");
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