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
  #define SERVER_MODEL_KEY "serverModel"
  #define SERVER_CONNECTED_KEY "serverConnected"
  #define SERVER_NAME_KEY "serverName"

#define MIXER_DETECTION_PERIOD 3000

class MixerServer: public HWComponent2State{
  public:
    IPAddress serverIP;
    uint16_t serverUDPPort, behringerUDPPort, msUDPPort, msAPIPort;

    bool msAPIOnline = false;

    String configIP="";
    String name;
    Config* config;
    MixerInterface *mixerInterface;  // Pointeur vers l'interface abstraite (HAL)  
    DataRepository* dataRepository;
    Network* network;
    OSCInterface osc;

    MixerServer(DataRepository* dataRepo) : HWComponent2State(0), dataRepository(dataRepo){}

    ~MixerServer(){}

    void init(Config* conf, Network* net){
      DEBUG_PRINT("\n Init MixerServer");
      config = conf;
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
      network = net;
      
      msHttpClientParam.callback = [&](String param) { processRESTcallback(param); };
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
      name = "auto";
    }

  private:    

    bool discovery = false;

    uint8_t mixerPortCounter = 0;
      
    #define MAXMIXERPORT 2
    uint16_t UDPPortTotest[MAXMIXERPORT]={10024,10023};
   
    unsigned long lastDetectionAttemptTime = 0;

    //AsyncClient msHttpClientParam;
    AsyncHttpClient msHttpClientParam;


    void connect() {    

      DEBUG_PRINT("\n * * * Detect mixer * * *");
      state = DISCONNECTED ;

                 
      String configIP = *config->getString(SERVER_IP_KEY); 
      if (!serverIP.fromString(configIP)) serverIP = IPAddress(0,0,0,0);

      DEBUG_PRINT("\n * * SERVER_IP_KEY = %s, serverIP = %s",configIP.c_str(),serverIP.toString().c_str());

      discovery=false; //if IP address is 0 or not set, we use auto detection 
      if ((serverIP[0] == 0 && serverIP[1] == 0 && serverIP[2] == 0 && serverIP[3] == 0) || configIP == "") discovery = true;

      // *************** autodetect:  **********************      
      if (discovery){
        DEBUG_PRINT(" - D I S C O V E R Y - ");
        
        serverIP = network->IP;      
        serverIP[3] = 255 ; // broadcast IP for Behringer and MS

        // for behringer native : we uses brodcast on UDP

        behringerUDPPort = UDPPortTotest[mixerPortCounter];
        // for mixing station, we uses TCP next IP                      
        
        msAPIPort = 8080;    
        msUDPPort = 3000;

        mixerPortCounter = (mixerPortCounter+1) % MAXMIXERPORT;
     
        //in auto mode we test both
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

    void testMixingStation(){      
      // TEST MIXING STATION
      // for mixing station:  REST API is over HTTP (so uses TCP), hence we cannot broadact. We have to test all IP address from the subnet range and see if we can connect the client provided by network hardware layer
      // which is not realistic, hence we test the dfault TCP port 8080       IN ASYNC HTTP REQUEST  
      if (msAPIOnline){  //gather async answer
        DEBUG_PRINT("\n =========> msAPIOnline");
        setMixingStationMixer(serverIP);    
        return;
      }

      osc.setClient(serverIP,msUDPPort);
      DEBUG_PRINT("\tTesting msUDP IP:%s/%i",serverIP.toString().c_str(),msUDPPort);      
      if (osc.ping(MIXING_STATION_PING)){
        DEBUG_PRINT("\n =========> mixerServerPing MS OSC");
        setMixingStationMixer(osc.udp->remoteIP()); 
        return;
      }

      // second method, 
      DEBUG_PRINT("\tTesting msREST IP:%s/%i TCP:%i ",serverIP.toString().c_str(),msAPIPort);      
      testMS_REST_API();  //trigger async answer               
    }

    bool testBehringer(){      
      bool behringerIsLive = false;
      
      osc.setClient(serverIP,behringerUDPPort);
      DEBUG_PRINT("\tTesting BehringerUDP IP:%s/%i",serverIP.toString().c_str(),msUDPPort);            
      behringerIsLive = osc.ping(BEHRINGER_QUERY);
      
      if (behringerIsLive) setBehringerMixer();

      return behringerIsLive;
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

      name = serverIP.toString();

    } 

    void setBehringerMixer(){

      DEBUG_PRINT("\nsetBehringerMixer()");
      LogUDP::sendLog("setBehringerMixer",hwm.network->udp);

      state = CONNECTED;      

      //<</xinfo,s,172.17.0.2, X32OnlineIntervalDemo, 
      //sValue: 172.17.0.2
      //sValue1: X32OnlineIntervalDemo
      //sValue2: X32
      IPAddress UDPIP = network->udp->remoteIP();
      if (UDPIP[0] != serverIP[0]){
        // the IP returned from the mixer differs from the on in the UDP packet,
        // this happens for instance when the mixer is on a different network (emulator on DSM for example)
        // therefore, we force the IP to be fixed and the one from UDP packet
        DEBUG_PRINT("\nServerIP (%s) differs from UDPIP (%s)",serverIP.toString().c_str(),UDPIP.toString().c_str());
        //serverIP = UDPIP;
      }
      IPAddress remoteMixerIP = remoteMixerIP.fromString(osc.sValue);
      if (serverIP[0] != remoteMixerIP[0]){
        DEBUG_PRINT("\nRemoteMixer IP (%s) differs from ServerIP (%s)",remoteMixerIP.toString().c_str(),serverIP.toString().c_str());
        //serverIP = remoteMixerIP;          
      }

      serverUDPPort = behringerUDPPort; //network->udp->remotePort();
      //config->setValue(SERVER_MODEL_KEY,osc.sValue2);
      //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
      //config->setValue(SERVER_PORT_KEY,String(serverUDPPort));
      //config->save();

      name = osc.sValue1;
      if (mixerInterface != nullptr) delete mixerInterface;
      osc.setClient(serverIP,serverUDPPort);
      mixerInterface = new BehringerMixer(dataRepository,osc.sValue2,&osc); 

    }

    void processRESTcallback(String param){     
      JsonDocument doc;
      
      auto error = deserializeJson(doc, param);
      if (error) {
          DEBUG_PRINT("\nprocessRESTcallback parsing failed: %s",error.c_str());
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
    }

};

#endif