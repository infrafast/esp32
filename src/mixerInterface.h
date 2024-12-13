#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#include "oscInterface.h"
#include <ArduinoJson.h>

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define MIXER_DETECTION_PERIOD 3000     // detect mixer every seconds

#define ADDRESS_CHNAME "/ch/config/%d/name"
#define ADDRESS_BUSNAME "/bus/config/%d/name"

#define INTERFACES_FILENAME "/interfaces.json"

#define MIXING_STATION_MODEL "MixingStation"

// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface : public DataProvider{
  public:
    OSCInterface* osc;

    String model;
    DataRepository* dataRepository = nullptr;
    String name="abstract";

    enum{
      DATA_SYNC = CONNECTED+1,
      SYNCED
    };

    virtual uint8_t state()=0;

    DataSet* chNames;
    DataSet* busNames;

    MixerInterface(DataRepository* dr,String _model,OSCInterface* _osc): osc(_osc){  
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
      protocol = getValueByName("protocol",interfaceDoc);

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

    void update(){
        switch (state()){
          // Warning BehringerINterface state is linked to MixerServer state which is based on 2 state component and therefore only return CONNECT AND DISCONNECT
          case DISCONNECTED:
            connect();
          break;
          case DATA_SYNC:

          break;
          case CONNECTED:
            monitor();
          break;
        }        
    }

    virtual bool connect()=0;
    virtual bool monitor()=0;
    
    // Envoi d'une commande générique    
    virtual bool sendCommand(String command, JsonDocument param){
      bool success = true;
      
      if (command == "testMixingOSC"){

        String msg = param["msg"];
        IPAddress ip;
        ip.fromString(param["ip"].as<String>());
        int port = param["port"];

        osc->SendCmd2OSC(msg.c_str(),ip,port);
        //osc->OSCReadIn();
      }else success = false;
      return success;

    }


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
            if (String(interface["model"].as<const char*>()) == model) {
                // Retourner la valeur de la clé spécifiée si elle existe
                if (interface.containsKey(key.c_str())) {
                    String s = interface[key.c_str()].as<String>();
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
    virtual String receiveResponse() = 0;             // Réception de la réponse spécifique

  protected:
    uint16_t chSize,busSize;
    
    // commands string
    String protocol;
    

};

class MixerOfflineInterface: public MixerInterface {
  public:

    MixerOfflineInterface(DataRepository* dr,String m, OSCInterface* osc): MixerInterface(dr,m, osc){
      DEBUG_PRINT("\n\nNew MixerOfflineInterface(<%s>)\n\n",model.c_str());
      name="Offline";
    }

    virtual ~MixerOfflineInterface(){
      DEBUG_PRINT("\n~MixerOfflineInterface()");   
      disconnect();   
    }

    virtual bool connect(){
      return true;
    }  

    virtual bool monitor(){
      return true;
    } 

    virtual uint8_t state(){
      return DISCONNECTED;
    }

    void disconnect() override{
      DEBUG_PRINT("\nMixerOfflineInterface disconnected");
    }                   

    virtual bool sendCommand(String command, JsonDocument param){return false;} // Envoi de commande spécifique
    virtual String receiveResponse(){return "";}             // Réception de la réponse spécifique

    /*virtual void update(){
      MixerInterface::update();
    }*/

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="ch"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="bus"+String(index);
      return v;
    }

  };


// Implémentation spécifique pour Behringer
class BehringerMixer : public MixerInterface {
  
  public:
    bool* ServerState;

    BehringerMixer(DataRepository* dr,String model,OSCInterface* _osc,bool* s): MixerInterface(dr,model,_osc){
      DEBUG_PRINT("\n\nNew BehringerMixer(<%s>)\n\n",model.c_str());
      ServerState = s;      
      name="MusicTribe";
    }

    virtual ~BehringerMixer(){
      DEBUG_PRINT("\n~BehringerMixer()");   
      disconnect();   
    }

    bool connect() override {          
      /*if (state()==CONNECTED) return CONNECTED;
      else{
        DEBUG_PRINT("\t\nTentative de connection à l'interface Behringer.");        
        // do nothing... as the behringer is conenctd directly thru the server is connected 
      }*/
      return state() == CONNECTED?CONNECTED:DISCONNECTED;
    }

    virtual uint8_t state(){
      // here the state is only based on the server but we may imagine to process other state like syncing
      return *ServerState == CONNECTED ? CONNECTED:DISCONNECTED;
    }

    /*virtual void update(){
      MixerInterface::update();
    }*/

    virtual bool monitor(){
      // restart querying periodic feedback
      #define MIXER_XREMOTE_DELAY 8000      
      if (millis()-lastXRemoteTime > MIXER_XREMOTE_DELAY){
        osc->SendCmd2OSC("/xremote");          // otherwise we ask mixer to send message for the next 10 sec (see behringer protocol)
        lastXRemoteTime = millis();
      }

      //test if mixer is still reachable
      if (millis()-lastMonitorTime > MIXER_DETECTION_PERIOD){        
        lastMonitorTime = millis();
        if (monitoringSuccessful){
          osc->SendCmd2OSC("/info");
          monitoringSuccessful = false;
        }else{
          DEBUG_PRINT("\n **** Mixer didn't respond within %dms",MIXER_DETECTION_PERIOD);         
          disconnect();
          return false;
        }
      } 
      
      // processing OSC queue
      if (osc->OSCReadIn()){
        if (osc->CommandStr == "/info"){
          monitoringSuccessful = true;          
        }else if (osc->CommandStr == "/toto"){

        }


      }
      return true;
    } 


    void disconnect() override {
      //client.stop();
      *ServerState = DISCONNECTED;
      DEBUG_PRINT("\nBehringerMixer disconnected");
    }

    bool sendCommand(String command, JsonDocument param) override {
      bool success;
      if (state() == DISCONNECTED) {
        DEBUG_PRINT("\nNon connecté au mixeur Behringer.");
        return false;
      }else success = MixerInterface::sendCommand(command, param);
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

    String receiveResponse() override {
      String response = "";
      /*while (client.available()) {
        char c = client.read();
        response += c;
      }*/
      return response;
    }

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="CHBehr"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="BUSBehr"+String(index);
      return v;
    }   

  private:
    bool monitoringSuccessful = true;   // flag which indicates if the mixer replied to periodic monitoring command
    unsigned long lastMonitorTime = 0;
    unsigned long lastXRemoteTime = 0;

};

// Implémentation spécifique pour MixingStation
class MixingStation : public MixerInterface {
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
    uint8_t interfaceState = DISCONNECTED;
    uint16_t port = 0;

  public:
    MixingStation(DataRepository* dr, String _model, uint16_t p, OSCInterface* osc): port(port), MixerInterface(dr,_model, osc){
      DEBUG_PRINT("\n\nNew MixingStation()\n\n");   
      name=MIXING_STATION_MODEL; 
    }

    virtual ~MixingStation(){
      DEBUG_PRINT("\n~MixingStation()");   
      disconnect();   
    }

    virtual uint8_t state(){
      return interfaceState;
    }

    /*virtual void update(){
      MixerInterface::update();
    }*/

    virtual bool monitor(){
      // PUT THE CODE THAT RESTART MONITORING OSC VALUE AND PING MIXINGSTATION FOR LIVE
      return true;
    }     

    bool connect() override {
      // HERE WE MAY NEED TO OPEN THE OSC SOCKET

      // WE ALSO HAVE TO COLLECT ALL THE SIZE OF THE DIFFERENTS DATA SET LIKE CHSIZE BUSSIZE 
      // USING THE ALGORITHM OF "DEDUCTION" THEN STORE THE VALUE IN THE JSON INTERFACES FILE SO
      // IN CASE THE USER WORKS OFFLINE IT USES STILL THE DATA SETS POPULATED WITH THE RIGHT NUMBER OF ITEMS
      // HAVING THEIR NAMES DEFAULTED TO "CH"+i AS PER THE OFFLINE INTERFACE SERVICE PROVIDER

      //interfaceState = CONNECTED;
      return interfaceState;
    }

    void disconnect() override {
      //client.stop();
      interfaceState = DISCONNECTED;
      DEBUG_PRINT("\nMixingStation disconnected");
    }

    String receiveResponse() override {
      String response = "";
      /*while (client.available()) {
        char c = client.read();
        response += c;
      }*/
      return response;
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
        DEBUG_PRINT("\nNon connecté au mixeur Behringer.");
        return false;
      }else success = MixerInterface::sendCommand(command, param);
      if (success) return true; // command has been consummed
      success = true;

        /// IMPLEMENT YOUR CODE HERE

      return success;
    }


};


#endif