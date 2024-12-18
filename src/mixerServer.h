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

#define MIXER_DETECTION_PERIOD 5000

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

    MixerServer(DataRepository* dataRepo,Network* net) : HWComponent2State(0), dataRepository(dataRepo), network(net){}

    ~MixerServer(){}

    void init(Config* conf){
      DEBUG_PRINT("\n Init MixerServer");
      config = conf;
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
    }

    bool refresh(bool force=false){    
      HWComponent2State::refresh(force);

      if (refreshed){
        if (state == CONNECTED){
          DEBUG_PRINT("\n[------- server state > CONNECTED -----]");
        }else{
          disconnect();
        }
      }

      if (network->refreshed){
        // network as changed state
        if (network->state == DISCONNECTED && state == CONNECTED) disconnect();
        osc.setNetwork(network->udp,network->LANlatency);
      }
  
      if (network->state == CONNECTED){                
        if (state == DISCONNECTED){
          if (millis()-lastDetectionAttemptTime > MIXER_DETECTION_PERIOD){
            lastDetectionAttemptTime = millis();
            connect();
          }
        }
        mixerInterface->update();        
      }
     
      return refreshed;
    }


    String getMixerState(){   
        //{"interfaceState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}}

        JsonDocument json;
        json[SERVER_STATE_KEY][SERVER_INTERFACE_KEY] = mixerInterface->name;
        json[SERVER_STATE_KEY][SERVER_IP_KEY] = serverIP.toString();
        json[SERVER_STATE_KEY][SERVER_PORT_KEY] = serverUDPPort;
        json[SERVER_STATE_KEY][SERVER_CONNECTED_KEY] = state;
        json[SERVER_STATE_KEY][SERVER_NAME_KEY] = name;

        
        json[INTERFACE_STATE_KEY][MIXER_PORT_KEY] = msUDPPort;
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
      DEBUG_PRINT("\n[-------server state > DISCONNECTED -----]");      
      if (mixerInterface != nullptr){
        delete mixerInterface;
      }      
      state = DISCONNECTED;
      msAPIOnline = false;
      mixerInterface = new MixerOfflineInterface(dataRepository,*config->getString(MIXER_MODEL_KEY));
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
     
      }else{
        DEBUG_PRINT(" - U S E R   D E F I N E D - ");
        // *************** manual :  **********************        
        behringerUDPPort = config->getInt(SERVER_PORT_KEY); 
        msUDPPort = config->getInt(SERVER_PORT_KEY); 
        msAPIPort = config->getInt(MIXER_PORT_KEY);
      }
    
      DEBUG_PRINT("IP:%s UDP:%i/%i TCP:%i ",serverIP.toString().c_str(),behringerUDPPort,msUDPPort,msAPIPort);

      //if (!testBehringer()) testMixingStation();
      //if (!testMixingStation()) testBehringer();
     
      testMixingStation();

    }

    void testMS_REST_API(){
      msHttpClientParam.host = serverIP;
      msHttpClientParam.port = msAPIPort; 
      msHttpClientParam.timeOut = MIXER_DETECTION_PERIOD-1000; // maximum duration of the request is the period of check minus one second
      //msHttpClientParam.client = network->client;      
      msHttpClientParam.client = network->http;
      msHttpClientParam.URL = "/app/state";
      msHttpClientParam.onConnectedCallback = [&](String param) { msREST_API_on(param); };
      msHttpClientParam.failConnectCallback = [&]() { msREST_API_off(); };      
      network->asyncConnect(&msHttpClientParam);
    }

    void testMixingStation(){      
      // TEST MIXING STATION
      // for mixing station:  REST API is over HTTP (so uses TCP), hence we cannot broadact. We have to test all IP address from the subnet range and see if we can connect the client provided by network hardware layer
      // which is not realistic, hence we test the dfault TCP port 8080       IN ASYNC HTTP REQUEST  
      if (msAPIOnline == true){  //async answer
        DEBUG_PRINT("\n =========> msAPIOnline");
        setMixingStationMixer(msUDPPort);    // CONNECTED, SYNCING, DISCONNECTED...
        return;
      }

      osc.setClient(serverIP,msUDPPort);
      if (osc.ping(MIXING_STATION_CHECK)){
        DEBUG_PRINT("\n =========> mixerServerPing MS OSC");
        setMixingStationMixer(osc.udp->remoteIP()); // speed up the next iteration        
        return;
      }

      // second method, 
      testMS_REST_API();           
    }

    bool testBehringer(){      
      bool behringerIsLive = false;
      
      osc.setClient(serverIP,behringerUDPPort);
      behringerIsLive = osc.ping(BehringerMixer::echoCommand);
      
      if (behringerIsLive) setBehringerMixer();

      return behringerIsLive;
    }

    void setMixingStationMixer(IPAddress _ip){
      //DEBUG_PRINT("\nsetMixingStationMixer(%d,%s)",interfaceState,_ip.toString().c_str());
      DEBUG_PRINT("\nsetMixingStationMixer(%s)",_ip.toString().c_str());
      state = CONNECTED;            // we connected mixing station

      config->setValue(MIXER_MODEL_KEY,MIXING_STATION_MODEL);
      //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
      config->setValue(MIXER_PORT_KEY,String(msAPIPort));
      config->setValue(SERVER_PORT_KEY,String(msUDPPort));

      config->save();

      serverIP = _ip;
      serverUDPPort = msUDPPort;

      osc.setClient(serverIP,serverUDPPort); 


      if (mixerInterface != nullptr) delete mixerInterface;
      mixerInterface = new MixingStation(dataRepository,MIXING_STATION_MODEL,&osc, network, msAPIPort,&state);  

      name = serverIP.toString();

/*      ((MixingStation*)mixerInterface)->interfaceState = interfaceState; // that's the one we got from hte async http request OR from OSC test (which means CONNECTED)
      ((MixingStation*)mixerInterface)->mixerOnline = (interfaceState == CONNECTED?true:false);*/
    } 

    void setBehringerMixer(){
      state = CONNECTED;      
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
        serverUDPPort = network->udp->remotePort();
        //<</xinfo,s,172.17.0.2, X32OnlineIntervalDemo, X32
        config->setValue(MIXER_MODEL_KEY,osc.sValue2);
        //config->setValue(SERVER_IP_KEY,serverIP.toString()); we never do that as it would override the default user setting 0.0.0.0 auto for instance
        config->setValue(SERVER_PORT_KEY,String(serverUDPPort));

        name = osc.sValue1;

        config->save();

        if (mixerInterface != nullptr) delete mixerInterface;

        osc.setClient(serverIP,serverUDPPort);
        /*mixerInterface = new BehringerMixer(dataRepository,*config->getString(MIXER_MODEL_KEY),&osc, &state);  // Utilisation du HAL pour MixingStation
        ((BehringerMixer*)mixerInterface)->mixerOnline = true;*/
      }
    }

    void msREST_API_off(){
      DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API not responding ----");
      msAPIOnline = false;
      state = DISCONNECTED;
    }

    void msREST_API_on(String s){
      DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API is ON ----");
      msAPIOnline = true;  // do not set state = CONNECTED as this will be done by setmixer otherwise connect() will not trigger creation of mixing station
      float progress;
      lastDetectionAttemptTime = 0; // speed up to the next iteration
    }

};

#endif