#ifndef MixerServer_h
#define MixerServer_h

#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#include "HWComponents.h"
#include "mixerInterface.h"

#define MIXER_SET_KEY "setMixer"
#define MIXER_STATE_KEY "mixerState"
#define SERVER_STATE_KEY "serverState"
#define INTERFACE_STATE_KEY "interfaceState"
#define MIXER_DEMO_KEY "demoMixer"

  #define MIXER_PORT_KEY "mixerPort"
  #define MIXER_MODEL_KEY "mixerModel"
  #define MIXER_CONNECTED_KEY "mixerConnected"

  #define SERVER_INTERFACE_KEY "interface"
  #define SERVER_IP_KEY "serverIP"
  #define SERVER_PORT_KEY "serverPort"
  #define SERVER_CONNECTED_KEY "serverConnected"
  #define SERVER_NAME_KEY "serverName"



class MixerServer: public HWComponent2State{
  public:
    IPAddress serverIP;
    int serverPort=0;
    IPAddress mixingStationIP;
    uint16_t mixingStationPort;
    String configIP="";
    String name;
    Config* config;
    MixerInterface *mixerInterface;  // Pointeur vers l'interface abstraite (HAL)  
    DataRepository* dataRepository;
    Network* network;
    OSCInterface osc;

    MixerServer(DataRepository* dataRepo,Network* net) : HWComponent2State(0), dataRepository(dataRepo), network(net){}

    ~MixerServer(){}

    void init(Config* conf){
      DEBUG_PRINT("\n Init MixerServer");
      config = conf;
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
    }

    bool refresh(bool force=false){    
      HWComponent2State::refresh(force);
            
      if (network->state == CONNECTED){
        if (state == DISCONNECTED){        
          if (millis()-lastDetectionAttemptTime > MIXER_DETECTION_PERIOD){
            lastDetectionAttemptTime = millis();
            detectMixer();
          }
        }
        mixerInterface->update();        
      }

      if (network->refreshed){
        // network as changed state
        if (network->state == DISCONNECTED && state == CONNECTED) disconnect();
      }
      
      return refreshed;
    }


    String getMixerState(){   
        //{"interfaceState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}}

        JsonDocument json;
        json[SERVER_STATE_KEY][SERVER_INTERFACE_KEY] = mixerInterface->name;
        json[SERVER_STATE_KEY][SERVER_IP_KEY] = serverIP.toString();
        json[SERVER_STATE_KEY][SERVER_PORT_KEY] = serverPort;
        json[SERVER_STATE_KEY][SERVER_CONNECTED_KEY] = state;
        json[SERVER_STATE_KEY][SERVER_NAME_KEY] = name;

        
        json[INTERFACE_STATE_KEY][MIXER_PORT_KEY] = mixingStationPort;
        json[INTERFACE_STATE_KEY][MIXER_MODEL_KEY] = mixerInterface->model;
        json[INTERFACE_STATE_KEY][MIXER_CONNECTED_KEY] = mixerInterface->state() == CONNECTED?true:false;

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
      if (mixerInterface != nullptr){
        delete mixerInterface;
      }      
      DEBUG_PRINT("\nServer disconnected")  ;      
      state = DISCONNECTED;
      mixerInterface = new MixerOfflineInterface(dataRepository,*config->getString(MIXER_MODEL_KEY),&osc);
      name = "auto";
    }

  private:    

    bool discovery = false;

    uint8_t mixerPortCounter = 0;
    
    bool mixingStationFlag=false;

    uint8_t nextMixingStationIPDigit = 2;
    
    #define MAXMIXERPORT 2
    uint16_t behringerPort[MAXMIXERPORT]={10024,10023};
   
    unsigned long lastDetectionAttemptTime = 0;

    AsyncClient msHttpClientParam;
  
    void detectMixer() {    

      DEBUG_PRINT("\n * * * Detect mixer * * *");
      state = DISCONNECTED ;

                 
      String configIP = *config->getString(SERVER_IP_KEY); 
      if (!serverIP.fromString(configIP)) serverIP = IPAddress(0,0,0,0);

      DEBUG_PRINT("\n * * SERVER_IP_KEY = %s, serverIP = %s",configIP.c_str(),serverIP.toString().c_str());

      discovery=false; //if IP address is 0 or not set, we use auto detection 
      if ((serverIP[0] == 0 && serverIP[1] == 0 && serverIP[2] == 0 && serverIP[3] == 0) || configIP == "") discovery = true;

      // *************** autodetect:  **********************      
      if (discovery){
        DEBUG_PRINT(" - discovery - ");
        
        serverIP = network->IP;
        mixingStationIP = serverIP; // base address for mixingStation IP is common
        
        // for behringer native : we uses brodcast on UDP
        serverIP[3] = 255 ; // broadcast              
        serverPort = behringerPort[mixerPortCounter];
        // for mixing station, we uses TCP next IP      
                
        mixingStationIP[3] = nextMixingStationIPDigit;        
        mixingStationPort = 8088;    // IN CASE WE SHIFT TO OSC DETECTION ALGORITHM , WE WOULD GAVE TO CHANGE THAT AND APPLY THE SAME AS BEHRINGER 

        osc.settings(serverIP,serverPort,network->udp);

      }else{
        DEBUG_PRINT(" - userIP - ");
        // *************** manual :  **********************        
        serverPort = config->getInt(SERVER_PORT_KEY); 
        mixingStationPort = serverPort;     
        mixingStationIP = serverIP;            
      }
      
      DEBUG_PRINT(" UDP:%s/%i TCP:%s/%i ",serverIP.toString().c_str(),serverPort,mixingStationIP.toString().c_str(),mixingStationPort);

      if (!testBehringer())
      testMixingStation();

      if (mixingStationFlag) setMixingStationMixer();

      return;
    }

    void testMixingStation(){
      // TEST MIXING STATION
      // for mixing station:  REST API is over HTTP (so uses TCP), hence we cannot broadact. We have to test all IP address from the subnet range and see if we can connect the client provided by network hardware layer
      // which is not realistic, hence we test the dfault TCP port 8080       IN ASYNC HTTP REQUEST

      // SECOND OPTION: TEST THE OSC CONNECTION WHICH COULE BE MORE RELIABLE AND DO NOT REQUITE AN ASYNC HTTP CONNECZ

      // THE RISK IS THAT THERE MAY BE A SERVER IN THE LAN THAT REPLIES TO 8080 CONNECT SO WE MAY CHANGE IT TO TEST A PAGE OR 
      msHttpClientParam.host = mixingStationIP;
      msHttpClientParam.port = mixingStationPort; 
      msHttpClientParam.client = network->client;
      mixingStationFlag = false;
      msHttpClientParam.onConnectedCallback = [&]() { foundMixingStation(); };
      msHttpClientParam.failConnectCallback = [&]() { nextMixingStationMixer(); };      
      network->asyncConnect(&msHttpClientParam);
    }

    bool testBehringer(){
      bool success = false;      
      osc.SendCmd2OSC("/xinfo");
      delay(network->LANlatency);      
      if (osc.OSCReadIn()) {
        setBehringerMixer();
        success = true;
      } else {
        mixerPortCounter++ ; // we iterate to the next Behringer mixer model
        // we test with a different latency and model
        if (mixerPortCounter == MAXMIXERPORT){
          mixerPortCounter = 0;
          // as the broadcast message (/xinfo) do not increase the latency in case of failure, we have to do it here if mixers where not responding
          //mixerLatency *= 2;  // the check of maxlatency is done in oscread
        }       
      }    
      return success;  
    }

    void setMixingStationMixer(){
      DEBUG_PRINT("\nsetMixingStationMixer()");

      serverPort = mixingStationPort;
      serverIP = mixingStationIP;

      config->setValue(MIXER_MODEL_KEY,MIXING_STATION_MODEL);
      //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
      config->setValue(MIXER_PORT_KEY,String(mixingStationPort));
      config->setValue(SERVER_PORT_KEY,String(serverPort));

      config->save();

      if (mixerInterface != nullptr) delete mixerInterface;
      mixerInterface = new MixingStation(dataRepository,MIXING_STATION_MODEL,mixingStationPort, &osc);  // Utilisation du HAL pour MixingStation

      name = serverIP.toString();
      mixerInterface->connect();  
      state = CONNECTED;         

    } 

    void setBehringerMixer(){
      //sValue frame from OSC contains mixer IP
      if (serverIP.fromString(osc.sValue)) {
        IPAddress UDPIP = network->udp->remoteIP();
        if (UDPIP[0] != serverIP[0]){
          // the IP returned from the mixer differs from the on in the UDP packet,
          // this happens for instance when the mixer is on a different network (emulator on DSM for example)
          // therefore, we force the IP to be fixed and the one from UDP packet
          serverIP = UDPIP;
          DEBUG_PRINT("\nMixer IP differs from UDP packet, switching to UDP IP instead.");
        }
        //<</xinfo,s,172.17.0.2, X32OnlineIntervalDemo, X32
        config->setValue(MIXER_MODEL_KEY,osc.sValue2);
        //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
        config->setValue(SERVER_PORT_KEY,String(serverPort));


        name = osc.sValue1;
        mixerInterface->connect();
        config->save();

        if (mixerInterface != nullptr) delete mixerInterface;
        osc.settings(serverIP,serverPort,network->udp);
        mixerInterface = new BehringerMixer(dataRepository,*config->getString(MIXER_MODEL_KEY),&osc, &state);  // Utilisation du HAL pour MixingStation
        state = CONNECTED;      
      }
    }

    void nextMixingStationMixer(){
      DEBUG_PRINT("\n\t\tnextMixingStationMixer()");
      nextMixingStationIPDigit ++;        
      if (nextMixingStationIPDigit == 254){
        nextMixingStationIPDigit = 1;
      }      
    }

    void foundMixingStation(){
      DEBUG_PRINT("\n\t\tfoundMixingStation()");
      mixingStationFlag = true;      
    }

};

#endif