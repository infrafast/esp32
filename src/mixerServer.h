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
  #define SERVER_OFFLINE_KEY "serverOffline"
  #define SERVER_SELECTED_KEY "serverSelected"

#define MIXER_DETECTION_PERIOD 3000

class MixerServer: public HWComponent2State{
  public:
    IPAddress serverIP;
    int serverUDPPort, behringerUDPPort = 10024, msUDPPort, msAPIPort;

    JsonDocument pingData;

    bool msOnline = false;
    bool behringerOnline = false;
    bool offline = false, previousOffline = false;

    String name;
    MixerInterface *mixerInterface;  // Pointeur vers l'interface abstraite (HAL)  
    DataRepository* dataRepository;
    OSCInterface* osc;

    MixerServer() : HWComponent2State(0){}

    ~MixerServer(){}

    void init(/*Config* conf, Network* net*/){      
      //network = net;     
      //config = conf;
      dataRepository = &app.dataRepository;
      osc = new OSCInterface("mixerOSC",NO_CAPTURE);
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
      
      //
      msHttpClientParam.callback = [&](const char* param) { processJsonData(param); };
      msHttpClientParam.timeOut = MIXER_DETECTION_PERIOD-1000; // maximum duration of the request is the period of check minus one second    
      msHttpClientParam.URL = APICALL_APP_STATE;  
      //
      userWSclientOnMessage = std::bind(&MixerServer::wsOnMessageCallback, this, std::placeholders::_1);
      
      pingParam.osc = osc;
      pingParam.data = pingData.to<JsonObject>();
      pingParam.timeOut = osc->latency;
      pingParam.callback = [this](bool success, int port) {
        this->pingCallBack(success, port);
      };

      pingLatency = hwm.network->LANlatency;
      
    }

    bool refresh(bool force=false){    
      HWComponent2State::refresh(force);

      if (previousOffline != offline){
        previousOffline = offline;
        refreshed = true;
      }

      if (offline) return refreshed; // do not try to connect if offline parameter is set to true, we just stop here

      if (hwm.network->state == CONNECTED){                

          if (mixerInterface->changedState){
            if (mixerInterface->interfaceState == DISCONNECTED) 
              DEBUG_PRINT("\nmixer disconnect()");
              //disconnect();
          }        

          if (millis()-lastDetectionAttemptTime > MIXER_DETECTION_PERIOD){
            lastDetectionAttemptTime = millis();
            if (state == DISCONNECTED) 
            DEBUG_PRINT("\nmixer connect()");
            //connect();            
          }
          if (state == CONNECTED){
            if (mixerInterface->interfaceState == IDLE){
              //mixerInterface->connect();     
              DEBUG_PRINT("\ninterface connect()");       
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
        json[SERVER_STATE_KEY][SERVER_DISCOVERY_KEY] = discovery()?"auto":"manual";
        json[SERVER_STATE_KEY][SERVER_PORT_KEY] = serverUDPPort;
        json[SERVER_STATE_KEY][SERVER_CONNECTED_KEY] = state;
        json[SERVER_STATE_KEY][SERVER_NAME_KEY] = name;
        json[SERVER_STATE_KEY][SERVER_OFFLINE_KEY] = offline;
        
        
        json[INTERFACE_STATE_KEY][MIXER_PORT_KEY] = msAPIPort;        
        json[INTERFACE_STATE_KEY][MIXER_CONNECTED_KEY] = stateNames[mixerInterface->interfaceState] ;
        json[INTERFACE_STATE_KEY][MIXER_AUTOMODEL_KEY] = app.userConfig.getInt(MIXER_AUTOMODEL_KEY);        
        json[INTERFACE_STATE_KEY][MIXER_AUTOIP_KEY] = *app.userConfig.getString(MIXER_AUTOIP_KEY);        

        String jsonString;
        serializeJson(json,jsonString);

        return jsonString;
    }

    bool discovery(){
      bool discovery = false;
      String configIP = *app.userConfig.getString(SERVER_IP_KEY); 

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
        //DEBUG_PRINT("\nImpossible d'ouvrir %s",INTERFACES_FILENAME);
      }

      // Lire le contenu du fichier dans un String
      DeserializationError error = deserializeJson(interfaceDoc, file);

      if (error) {
        //DEBUG_PRINT("\nErreur de désérialisation Interfaces: %s",error.f_str());
      }

      file.close();    
      String jsonString;
      serializeJson(interfaceDoc["interfaces"],jsonString);
      return jsonString; 
    }

    // Déconnexion
    void disconnect() {      
      //DEBUG_PRINT("\n[-------server state > DISCONNECTED -----]");      
      if (mixerInterface != nullptr){
        mixerInterface->disconnect();
        delete mixerInterface;
      }      
      state = DISCONNECTED;
      msOnline = false;
      behringerOnline = false;
      mixerInterface = new MixerOfflineInterface(*app.userConfig.getString(SERVER_MODEL_KEY));
      mixerInterface->init();
      name = *app.lang.getString("searching");
    }

   private:    
    uint16_t pingLatency = 0;

    unsigned long lastDetectionAttemptTime = 0;

    AsyncHttpClient msHttpClientParam;
    OSCPingTaskParameters pingParam;


    void connect() { 
      DEBUG_PRINT("mixerServer.connect()");
      static bool toggleBehringerMSTest = true;   
            
      state = DISCONNECTED ;              

      if (msOnline){
        instantiateMixingStationMixer();
        return;
      }

      if (behringerOnline){
        instantiateBehringerMixer();
        return;
      }

      String configIP = *app.userConfig.getString(SERVER_IP_KEY);
      if (!serverIP.fromString(configIP) || configIP=="") serverIP = IPAddress(0,0,0,0);  

      DEBUG_PRINT("\n * * MIXER SERVER_IP_KEY = %s, serverIP = %s",configIP.c_str(),serverIP.toString().c_str());

      // *************** autodetect:  **********************      
      if (discovery()){
        DEBUG_PRINT(" - D I S C O V E R Y - ");
        
        serverIP = hwm.network->IP;      
        serverIP[3] = 255 ; // broadcast IP for Behringer and MS
       
        msAPIPort = 8080;    
        msUDPPort = 3000;

        if (!toggleBehringerMSTest){
          setBehringerTest();          
          //and then, test the next behringer port
          if (behringerUDPPort == 10024) behringerUDPPort = 10023;
          else behringerUDPPort = 10024;          
        }                  
        else;
          setMixingStationTest();

        toggleBehringerMSTest = !toggleBehringerMSTest;

      }else{
        DEBUG_PRINT(" - U S E R   D E F I N E D - ");
        // *************** manual :  **********************        
        behringerUDPPort = app.userConfig.getInt(SERVER_PORT_KEY); 
        msUDPPort = app.userConfig.getInt(SERVER_PORT_KEY); 
        msAPIPort = app.userConfig.getInt(MIXER_PORT_KEY);

        if (*app.userConfig.getString(SERVER_MODEL_KEY) == MIXING_STATION_MODEL) setMixingStationTest();
        else setBehringerTest();
  
      }   
      
      pingParam.ip = serverIP;
      if (!pingParam.isPinging){
        osc->asyncPing(&pingParam);
      }                
          
    }

    void testMS_REST_API(){
      //DEBUG_PRINT("\nTesting msREST IP:%s/%i ",serverIP.toString().c_str(),msAPIPort);      
      msHttpClientParam.host = serverIP;
      msHttpClientParam.port = msAPIPort; 
      hwm.network->asyncConnect(&msHttpClientParam);
    }

    /*void testMS_WS_API(){
      //DEBUG_PRINT("\nTesting msWS IP:%s/%i ",serverIP.toString().c_str(),msAPIPort);      
      if (hwm.network->clientSocketState() == WebSocketReadyState::CLOSED)
        hwm.network->clientSocketOpen(serverIP.toString().c_str(), msAPIPort, "/");

      String API_Call = APICALL_APP_STATE;      
      //{ "path": "/app/state", "method": "GET", "body": null }
      JsonDocument doc;
      doc[MS_API_PATH] = API_Call;   
      doc[API_BODY] = doc.createNestedObject(); //empty object {}

      hwm.network->clientSocketSend(doc.as<String>().c_str());
      doc.clear();

    }*/

    void setMixingStationTest(){      
      DEBUG_PRINT("\nsetMixingStationTest()");     
      pingParam.address = OSC_MS_PRE OSC_MS_PING;
      pingParam.port = msUDPPort;
      testMS_REST_API();  //trigger async answer               
      //testMS_WS_API();   // ==> very slow if clientSocketOpen(192.168.0.180,8080,/)Error in connection establishment: net::ERR_CONNECTION_REFUSED
    }

    void setBehringerTest(){    
      DEBUG_PRINT("\nsetBehringerTest()");
      pingParam.address = BEHRINGER_QUERY;
      pingParam.port = behringerUDPPort;          
    }

    void pingCallBack(bool _success, int _port){
      //DEBUG_PRINT("\npingCallBack %s:%d => %s",data->expectedStr,_port,_success?"OK":"failed");      
      lastDetectionAttemptTime = 0;   // don't wait next cycle      
      if(_success){
        // set the flags used by setBehringerTest() and testMS() to instantiate mixerinterface
        if (_port == 10023 || _port == 10024){
          behringerOnline = true;
          behringerUDPPort = _port;
        }else msOnline = true;        
        //DEBUG_PRINT("\nsetting flags: MS:%d BEHRINGER:%d",msOnline,behringerOnline);
      }else{
        pingLatency = (pingLatency + 20) % 250;             
        pingParam.timeOut = pingLatency;        
        //DEBUG_PRINT("\nincreasing latency");
      }     
    }

    bool testBehringer2(){      
      
      //DEBUG_PRINT("\nTesting BehringerUDP IP:%s/%i",serverIP.toString().c_str(),behringerUDPPort);
      //osc->setClient(serverIP,behringerUDPPort);
      if (osc->blockingPing(serverIP,behringerUDPPort,BEHRINGER_QUERY,pingParam.data,pingLatency)){
        instantiateBehringerMixer();
        return true;
      }

      if (behringerUDPPort == 10023) behringerUDPPort = 10024;
      else behringerUDPPort = 10023;

      return false;
    }

    void initInterface(const char* _name){
      String udpLogString="instantiate "+String(_name)+" interface";
      //DEBUG_PRINT("\n%s",udpLogString.c_str());
      state = CONNECTED;      
      LogUDP::sendLog(udpLogString.c_str(),hwm.network->udp);
      if (discovery()) serverIP = hwm.network->udp->remoteIP(); 
      if (mixerInterface != nullptr) delete mixerInterface;
      osc->setClient(serverIP,serverUDPPort);
      name = _name;
    }

    void instantiateMixingStationMixer(){
      serverUDPPort = msUDPPort;
      initInterface(app.lang.getString("software")->c_str());
      mixerInterface = new MixingStation(CAPTURE_OUT,osc,msAPIPort);  
      mixerInterface->init();
    } 

    void instantiateBehringerMixer(){
      //<</xinfo,s,172.17.0.2, X32OnlineIntervalDemo, 
      //sValue: 172.17.0.2
      //sValue1: X32OnlineIntervalDemo
      //sValue2: X32           
      IPAddress remoteMixerIP;
      remoteMixerIP.fromString(pingParam.data[EXEC_VALUE][0].as<String>());      //sValue
      if (serverIP[0] != remoteMixerIP[0]){
        //DEBUG_PRINT("\nRemoteMixer IP (%s)->(%s) differs from ServerIP (%s)",data.sValue,remoteMixerIP.toString().c_str(),serverIP.toString().c_str());
        //serverIP = remoteMixerIP;          
      }           
      serverUDPPort = behringerUDPPort; //serverUDPPort = osc->udp->remotePort();       
      initInterface(pingParam.data[EXEC_VALUE][1].as<const char*>());
      mixerInterface = new BehringerMixer(pingParam.data[EXEC_VALUE][2].as<String>(),CAPTURE_IN, osc); 
      mixerInterface->init();
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
          //DEBUG_PRINT("\nwsMixerServer OnMessageCallback parsing failed: %s",error.c_str()); 
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
      
      //DEBUG_PRINT("\nServerProcessRESTcallback: %s",param);
      lastDetectionAttemptTime = 0;   // don't wait next cycle to create interface      
      JsonDocument doc;
      
      auto error = deserializeJson(doc, param);
      if (error) {
          //DEBUG_PRINT("\nServerProcessRESTcallback parsing failed: %s",error.c_str());
          doc.clear();
          return;
      }
      uint8_t status = doc[API_STATUS];

      if (status == API_OK){
        //DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API is ON ----");
        msOnline = true;  // do not set state = CONNECTED as this will be done by setmixer otherwise connect() will not trigger creation of mixing station
      }else{
        //DEBUG_PRINT("\n\t\t\t\t\t\t ---- Mixing REST API not responding ----");
        msOnline = false;
      }
      doc.clear();
    }
    

};

#endif