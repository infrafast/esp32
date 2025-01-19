#ifndef MixerServer_h
#define MixerServer_h

#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#include "HWComponents.h"
#include "mixerInterface.h"

#define MIXER_SET_KEY "setMixer"
#define MIXER_STATE_KEY "mixerState"    // overall key name containing INTERFACE_STATE_KEY and SERVER_STATE_KEY subkeys

#define MIXER_DEMO_KEY "demoMixer"

#define SERVER_STATE_KEY "serverState"
  #define SERVER_INTERFACE_KEY "interface"
  #define SERVER_IP_KEY "serverIP"
  #define SERVER_PORT_KEY "serverPort"
  #define SERVER_DISCOVERY_KEY "serverDiscovery"
  #define SERVER_MODEL_KEY "serverModel"
  #define SERVER_CONNECTED_KEY "serverConnected"
  #define SERVER_NAME_KEY "serverName"

#define MIXER_DETECTION_PERIOD 3000

class MixerServer: public HWComponent2State{
  public:
    IPAddress serverIP;
    int serverUDPPort, behringerUDPPort = 10023, msUDPPort, msAPIPort;

    bool msAPIOnline = false;

    String name;
    Config* config;
    MixerInterface *mixerInterface;  // Pointeur vers l'interface abstraite (HAL)  
    DataRepository* dataRepository;
    Network* network;
    OSCInterface osc;

    MixerServer(DataRepository* dataRepo) : HWComponent2State(0), dataRepository(dataRepo){}

    ~MixerServer(){}

    void init(Config* conf, Network* net){
      //DEBUG_PRINT("\n Init MixerServer");
      config = conf;
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
      network = net;
      
      msHttpClientParam.callback = [&](const char* param) { processRESTcallback(param); };
      msHttpClientParam.timeOut = MIXER_DETECTION_PERIOD-1000; // maximum duration of the request is the period of check minus one second    
      msHttpClientParam.URL = RESTCALL_APP_STATE;  

    }

    bool refresh(bool force=false){    
      HWComponent2State::refresh(force);

      if (network->refreshed){
        // network has changed state
        //if (network->state == DISCONNECTED && state == CONNECTED) disconnect();
        osc.setNetwork(network->udp,network->LANlatency);

        //maybe put here mixerInterface->disconnect() and esnure that it contains the code to stop httpclient
      }

      if (!mixerInterface->update()){
        // we may do this after 2 attempts
        if (mixerInterface->interfaceState == DISCONNECTED) disconnect();
      }

      if (network->state == CONNECTED){                
               
          if (millis()-lastDetectionAttemptTime > MIXER_DETECTION_PERIOD){
            lastDetectionAttemptTime = millis();
            if (state == DISCONNECTED) connect();            
          }
          if (state == CONNECTED){
            if (mixerInterface->interfaceState == IDLE){
              mixerInterface->connect(config);            
            }
          }          
      }
     
      return refreshed;
    }

    String getMixerState(){   
        //{"interfaceState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}}

        JsonDocument json;
        json[SERVER_STATE_KEY][SERVER_INTERFACE_KEY] = mixerInterface->name;
        json[SERVER_STATE_KEY][SERVER_MODEL_KEY] = mixerInterface->model;
        json[SERVER_STATE_KEY][SERVER_IP_KEY] = serverIP.toString();
        json[SERVER_STATE_KEY][SERVER_DISCOVERY_KEY] = getDiscoveryMode()?"auto":"manual";
        json[SERVER_STATE_KEY][SERVER_PORT_KEY] = serverUDPPort;
        json[SERVER_STATE_KEY][SERVER_CONNECTED_KEY] = state;
        json[SERVER_STATE_KEY][SERVER_NAME_KEY] = name;
        
        json[INTERFACE_STATE_KEY][MIXER_PORT_KEY] = msAPIPort;        
        json[INTERFACE_STATE_KEY][MIXER_CONNECTED_KEY] = stateNames[mixerInterface->interfaceState] ;
        json[INTERFACE_STATE_KEY][MIXER_AUTOMODEL_KEY] = config->getInt(MIXER_AUTOMODEL_KEY);        
        json[INTERFACE_STATE_KEY][MIXER_AUTOIP_KEY] = *config->getString(MIXER_AUTOIP_KEY);        

        String jsonString;
        serializeJson(json,jsonString);

        return jsonString;
    }

    bool getDiscoveryMode(){
      bool discovery = false;
      String configIP = *config->getString(SERVER_IP_KEY); 

      IPAddress ip;
      if (!ip.fromString(configIP)) ip = IPAddress(0,0,0,0);  

      if ((ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) || configIP == "") discovery = true;
      return discovery;
    }

    String getInterfaces(){
      JsonDocument interfaceDoc;
      // Ouvrir le fichier JSON
      File file = LittleFS.open(INTERFACES_FILENAME, "r");
      if (!file) {
        DEBUG_PRINT("\nImpossible d'ouvrir %s",INTERFACES_FILENAME);
      }

      // Lire le contenu du fichier dans un String
      DeserializationError error = deserializeJson(interfaceDoc, file);

      if (error) {
        DEBUG_PRINT("\nErreur de désérialisation Interfaces: %s",error.f_str());
      }

      file.close();    
      String jsonString;
      serializeJson(interfaceDoc["interfaces"],jsonString);
      return jsonString; 
    }

    // Déconnexion
    void disconnect() {
      DEBUG_PRINT("\n[-------server state > DISCONNECTED -----]");      
      if (mixerInterface != nullptr){
        delete mixerInterface;
      }      
      state = DISCONNECTED;
      msAPIOnline = false;
      mixerInterface = new MixerOfflineInterface(dataRepository,*config->getString(SERVER_MODEL_KEY));
      mixerInterface->init();
      name = *app.lang.getString("searching");
    }

  private:    
  
    unsigned long lastDetectionAttemptTime = 0;

    //AsyncClient msHttpClientParam;
    AsyncHttpClient msHttpClientParam;


    void connect() {    

      //DEBUG_PRINT("\n * * * Detect mixer * * *");
      state = DISCONNECTED ;              

      String configIP = *config->getString(SERVER_IP_KEY);
      if (!serverIP.fromString(configIP) || configIP=="") serverIP = IPAddress(0,0,0,0);  

      DEBUG_PRINT("\n * * MIXER SERVER_IP_KEY = %s, serverIP = %s",configIP.c_str(),serverIP.toString().c_str());

      // *************** autodetect:  **********************      
      if (getDiscoveryMode()){
        DEBUG_PRINT(" - D I S C O V E R Y - ");
        
        serverIP = network->IP;      
        serverIP[3] = 255 ; // broadcast IP for Behringer and MS
       
        msAPIPort = 8080;    
        msUDPPort = 3000;
          
        if (!testBehringer()) testMixingStation();

      }else{
        DEBUG_PRINT(" - U S E R   D E F I N E D - ");
        // *************** manual :  **********************        
        behringerUDPPort = config->getInt(SERVER_PORT_KEY); 
        msUDPPort = config->getInt(SERVER_PORT_KEY); 
        msAPIPort = config->getInt(MIXER_PORT_KEY);

        if (*config->getString(SERVER_MODEL_KEY) == MIXING_STATION_MODEL) testMixingStation();
        else testBehringer();
  
      }   
    }

    void testMS_REST_API(){
      msHttpClientParam.host = serverIP;
      msHttpClientParam.port = msAPIPort; 
      network->asyncConnect(&msHttpClientParam);
    }

    bool testMixingStation(){      
      // TEST MIXING STATION
      // for mixing station:  REST API is over HTTP (so uses TCP), hence we cannot broadact. We have to test all IP address from the subnet range and see if we can connect the client provided by network hardware layer
      // which is not realistic, hence we test the dfault TCP port 8080       IN ASYNC HTTP REQUEST  
      if (msAPIOnline){  //gather async answer
        DEBUG_PRINT("\n =========> msAPIOnline");
        setMixingStationMixer(serverIP);    
        return true;
      }

      // first method
      osc.setClient(serverIP,msUDPPort);
      DEBUG_PRINT("\nTesting msUDP IP:%s/%i",serverIP.toString().c_str(),msUDPPort);    
      if (osc.blockingPing(OSC_MS_PING)){
        setMixingStationMixer(osc.udp->remoteIP());
        return true;
      }

      // second method, 
      DEBUG_PRINT("\nTesting msREST IP:%s/%i ",serverIP.toString().c_str(),msAPIPort);      
      testMS_REST_API();  //trigger async answer               

      return false;
    }

    bool testBehringer(){      
      
      uint16_t latency = osc.latency;

      DEBUG_PRINT("\nTesting BehringerUDP IP:%s/%i",serverIP.toString().c_str(),behringerUDPPort);
      
      osc.setClient(serverIP,behringerUDPPort);
      if (osc.blockingPing(BEHRINGER_QUERY,latency)){
        setBehringerMixer();
        return true;
      }

      latency = (latency + 10) % 250;

      if (behringerUDPPort == 10023) behringerUDPPort = 10024;
      else behringerUDPPort = 10023;

      return false;
    }

    void setMixingStationMixer(IPAddress _ip){
      //DEBUG_PRINT("\nsetMixingStationMixer(%d,%s)",interfaceState,_ip.toString().c_str());
      DEBUG_PRINT("\nsetMixingStationMixer(%s)",_ip.toString().c_str());
      LogUDP::sendLog("setMixingStationMixer",hwm.network->udp);
      state = CONNECTED;            // we connected mixing station

      //config->setValue(SERVER_MODEL_KEY,MIXING_STATION_MODEL);
      //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
      //config->setValue(MIXER_PORT_KEY,String(msAPIPort));
      //config->setValue(SERVER_PORT_KEY,String(msUDPPort));
      //config->save();

      serverIP = _ip;
      serverUDPPort = msUDPPort;
      if (mixerInterface != nullptr) delete mixerInterface;
      osc.setClient(serverIP,serverUDPPort); 
      mixerInterface = new MixingStation(dataRepository,&osc, network, msAPIPort);  
      mixerInterface->init();
      name = *app.lang.getString("software");

    } 

    void setBehringerMixer(){

      DEBUG_PRINT("\nsetBehringerMixer()");
      LogUDP::sendLog("setBehringerMixer",hwm.network->udp);

      state = CONNECTED;      

      //<</xinfo,s,172.17.0.2, X32OnlineIntervalDemo, 
      //sValue: 172.17.0.2
      //sValue1: X32OnlineIntervalDemo
      //sValue2: X32
      //IPAddress UDPIP = 
      serverIP = osc.udp->remoteIP();
      IPAddress remoteMixerIP = remoteMixerIP.fromString(osc.data.sValue); // <======== get 1.0.0.0 in remoteServerIP, why ?
      if (serverIP[0] != remoteMixerIP[0]){
        DEBUG_PRINT("\nRemoteMixer IP (%s) differs from ServerIP (%s)",remoteMixerIP.toString().c_str(),serverIP.toString().c_str());
        //serverIP = remoteMixerIP;          
      }

      serverUDPPort = osc.udp->remotePort(); 
      //config->setValue(SERVER_MODEL_KEY,osc.sValue2);
      //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
      //config->setValue(SERVER_PORT_KEY,String(serverUDPPort));
      //config->save();

      name = osc.data.sValue1;
      if (mixerInterface != nullptr) delete mixerInterface;
      osc.setClient(serverIP,serverUDPPort);
      mixerInterface = new BehringerMixer(dataRepository,osc.data.sValue2,&osc); 
      mixerInterface->init();
    }

    void processRESTcallback(const char* param){     
      
      DEBUG_PRINT("\nServerProcessRESTcallback: %s",param);

      JsonDocument doc;
      
      auto error = deserializeJson(doc, param);
      if (error) {
          DEBUG_PRINT("\nServerProcessRESTcallback parsing failed: %s",error.c_str());
          doc.clear();
          return;
      }
      uint8_t status = doc[HTTP_REST_STATUS];

      if (status == HTTP_REST_OK){
        DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API is ON ----");
        msAPIOnline = true;  // do not set state = CONNECTED as this will be done by setmixer otherwise connect() will not trigger creation of mixing station
        lastDetectionAttemptTime = 0; // we detected the mixer so we shorten the delay to treat the information
      }else{
        DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API not responding ----");
        msAPIOnline = false;
      }
      doc.clear();
    }
    

};

#endif