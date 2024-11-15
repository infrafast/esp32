#ifndef MixerInterface_h
#define MixerInterface_h

#include <Arduino.h>
#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#define ADDRESS_CHNAME "/ch/config/%d/name"
#define ADDRESS_BUSNAME "/bus/config/%d/name"

// Classe abstraite pour implémenter les commandes et la gestion de connexion spécifique à chaque table de mixage
class MixerInterface : public DataProvider{
  public:
    String name;
    DataRepository* dataRepository = nullptr;
    uint8_t state = DISCONNECTED;

    enum{
      DATA_SYNC = CONNECTED+1,
      SYNCED
    };

    DataSet* chNames;
    DataSet* busNames;

    virtual ~MixerInterface(){
      dataRepository->removeDataSet(chNames);
      delete chNames;

      dataRepository->removeDataSet(busNames);      
      delete busNames;

      //DEBUG_PRINT("\n\n~MixerInterface()");
    }

    MixerInterface(DataRepository* dr,uint16_t port=0): dataRepository(dr){
      chNames = new DataSet(ADDRESS_CHNAME,INPUTSELECT,this);
      busNames = new DataSet(ADDRESS_BUSNAME,INPUTSELECT,this);

      dataRepository->addDataSet(chNames);
      dataRepository->addDataSet(busNames);  
      //DEBUG_PRINT("\n\nNew MixerInterface with its data set\n\n");    
    };

    void update(){
        switch (state){
          case DISCONNECTED:
            connect();
          break;
          case DATA_SYNC:

          break;
          case SYNCED:

          break;
        }        
    }

    virtual bool connect() = 0;                       // Connexion spécifique au mixeur
    virtual void disconnect() = 0;                    // Déconnexion spécifique
    virtual bool sendCommand(const String &command) = 0; // Envoi de commande spécifique
    virtual String receiveResponse() = 0;             // Réception de la réponse spécifique
  };

class MixerOfflineInterface: public MixerInterface {
  public:

    MixerOfflineInterface(DataRepository* dr): MixerInterface(dr){
      DEBUG_PRINT("\n\nNew MixerOfflineInterface()\n\n");
    }

    virtual ~MixerOfflineInterface(){
      DEBUG_PRINT("\n~MixerOfflineInterface()");   
      disconnect();   
    }

    virtual bool connect(){return false;}  // Connexion spécifique au mixeur
    virtual void disconnect(){state = DISCONNECTED;}                    // Déconnexion spécifique
    virtual bool sendCommand(const String &command){return false;} // Envoi de commande spécifique
    virtual String receiveResponse(){return "";}             // Réception de la réponse spécifique

    virtual void update(){
      MixerInterface::update();
    }


    virtual uint16_t size(String addrr){
      uint16_t n=0;
      if (addrr == ADDRESS_CHNAME) n=100;
      else if (addrr == ADDRESS_BUSNAME) n=20;
      //DEBUG_PRINT("\nSizeof(MixerOfflineInterface,%s)=%i",addrr.c_str(),n);
      return n;
    }

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="ch"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="bus"+String(index);
      return v;
    }


  };


// Implémentation spécifique pour Behringer
class BehringerMixer : public MixerInterface {
  private:
    //WiFiClient client;

  public:
    BehringerMixer(DataRepository* dr): MixerInterface(dr){
      DEBUG_PRINT("\n\nNew BehringerMixer()\n\n");      
    }

    virtual ~BehringerMixer(){
      DEBUG_PRINT("\n~BehringerMixer()");   
      disconnect();   
    }

    bool connect() override {      
      state = CONNECTED;
      DEBUG_PRINT("\tConnecté du mixeur Behringer.");
      return true;
    }

    virtual void update(){
      MixerInterface::update();
    }

    void disconnect() override {
      //client.stop();
      state = false;
      DEBUG_PRINT("\tDéconnecté du mixeur Behringer.");
    }

    bool sendCommand(const String &command) override {
      if (!state) {
        DEBUG_PRINT("Non connecté au mixeur Behringer.");
        return false;
      }

      // Traduction et envoi des commandes spécifiques à Behringer
      String behringerCommand = command;
      if (command == "MUTE") {
        behringerCommand = "/ch/01/mix/on 0";  // Exemple de commande spécifique
      } else if (command == "UNMUTE") {
        behringerCommand = "/ch/01/mix/on 1";  // Exemple de commande spécifique
      }
      
      //client.print(behringerCommand);
      return true;
    }

    String receiveResponse() override {
      String response = "";
      /*while (client.available()) {
        char c = client.read();
        response += c;
      }*/
      return response;
    }

    virtual uint16_t size(String addrr){
      uint16_t n=0;
      if (addrr == ADDRESS_CHNAME) n=100;
      else if (addrr == ADDRESS_BUSNAME) n=20;
      //DEBUG_PRINT("\nSizeof(BehringerMixer,%s)=%i",addrr.c_str(),n);
      return n;
    }

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="CHBehr"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="BUSBehr"+String(index);
      return v;
    }   
};

// Implémentation spécifique pour MixingStation
class MixingStation : public MixerInterface {
  private:
    //WiFiClient client;


  public:
    MixingStation(DataRepository* dr, uint16_t port): MixerInterface(dr,port){
      DEBUG_PRINT("\n\nNew MixingStation()\n\n");      

    }

    virtual ~MixingStation(){
      DEBUG_PRINT("\n~MixingStation()");   
      disconnect();   
    }

    virtual void update(){
      MixerInterface::update();
    }

    bool connect() override {
      return false;
    }

    void disconnect() override {
      //client.stop();
      state = false;
      DEBUG_PRINT("Déconnecté du mixeur MixingStation.");
    }

    bool sendCommand(const String &command) override {
      if (!state) {
        DEBUG_PRINT("Non connecté au mixeur MixingStation.");
        return false;
      }

      // Traduction et envoi des commandes spécifiques à MixingStation
      String mixingStationCommand = command;
      if (command == "MUTE") {
        mixingStationCommand = "{\"type\":\"mute\",\"channel\":1,\"state\":false}";  // Exemple JSON
      } else if (command == "UNMUTE") {
        mixingStationCommand = "{\"type\":\"mute\",\"channel\":1,\"state\":true}";
      }

      //client.print(mixingStationCommand);
      return true;
    }

    String receiveResponse() override {
      String response = "";
      /*while (client.available()) {
        char c = client.read();
        response += c;
      }*/
      return response;
    }

    virtual uint16_t size(String addrr){
      uint16_t n=0;
      if (addrr == ADDRESS_CHNAME) n=100;
      else if (addrr == ADDRESS_BUSNAME) n=20;
      //DEBUG_PRINT("\nSizeof(BehringerMixer,%s)=%i",addrr.c_str(),n);
      return n;
    }

    virtual String getValue(String addrr, uint16_t index){
      String v="";
      if (addrr == ADDRESS_CHNAME) v="CHMS"+String(index);
      else if (addrr == ADDRESS_BUSNAME) v="BUSMS"+String(index);
      return v;
    }     

};


#endif