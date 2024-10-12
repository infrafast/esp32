#ifndef MixerServer_h
#define MixerServer_h

#include "HWComponents.h"
#include "mixerInterface.h"

#define MIXER_SET_KEY "setMixer"
#define MIXER_STATE_KEY "mixerState"

  #define MIXER_NAME_KEY "mixerName"
  #define MIXER_PORT_KEY "mixerPort"
  #define MIXER_MODEL_KEY "mixerModel"
  #define MIXER_CONNECTED_KEY "mixerConnected"

  #define SERVER_INTERFACE_KEY "interface"
  #define SERVER_IP_KEY "serverIP"
  #define SERVER_PORT_KEY "serverPort"
  #define SERVER_CONNECTED_KEY "serverConnected"


class MixerServer: public HWComponent2State{
  public:
    IPAddress serverIP;;
    String configIP="";
    int serverPort=0;
    Config* config;

    MixerInterface *mixerInterface;  // Pointeur vers l'interface abstraite (HAL)  

    DataRepository* dataRepository;
    Network* network;

    MixerServer(DataRepository* dataRepo,Network* net) : HWComponent2State(0), dataRepository(dataRepo), network(net){}

    void init(Config* conf){
      config = conf;
      disconnect(); // this sets default mixer as offline and force trying to reconnect at next refresh
    }

    bool refresh(bool force=false){    
      HWComponent2State::refresh(force);
      
      if (state == DISCONNECTED){        
        if (detectMixer()){
          DEBUG_PRINT("\nInterface detected: %s",config->getString(SERVER_INTERFACE_KEY)->c_str());          
        }
      }
     
      mixerInterface->update();

      return refreshed;
    }


    String getMixerState(){   
        //{"mixerState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}}

        JsonDocument json;
        json[MIXER_STATE_KEY][SERVER_INTERFACE_KEY] = *config->getString(SERVER_INTERFACE_KEY);
        json[MIXER_STATE_KEY][SERVER_IP_KEY] = serverIP.toString();
        json[MIXER_STATE_KEY][SERVER_PORT_KEY] = serverPort;
        json[MIXER_STATE_KEY][SERVER_CONNECTED_KEY] = state;

        json[MIXER_STATE_KEY][MIXER_NAME_KEY] = mixerInterface->name;
        json[MIXER_STATE_KEY][MIXER_PORT_KEY] = config->getInt(MIXER_PORT_KEY);
        json[MIXER_STATE_KEY][MIXER_MODEL_KEY] = *config->getString(MIXER_MODEL_KEY);
        json[MIXER_STATE_KEY][MIXER_CONNECTED_KEY] = mixerInterface->state == CONNECTED?true:false;

        String jsonString;
        serializeJson(json,jsonString);

        return jsonString;
    }

    // Connexion automatique à la table de mixage détectée
    bool connectToMixer() {
      if (mixerInterface->connect()) {
        state = true;
      } else {
        DEBUG_PRINT("Échec de connexion.");
        state = false;
      }
      return state;
    }

    // Envoi d'une commande générique
    bool sendCommand(const String &command) {
      if (!mixerInterface->state) {
        DEBUG_PRINT("Non connecté à une table de mixage.");
        return false;
      }

      return mixerInterface->sendCommand(command);
    }

    // Réception d'une réponse de la table de mixage
    String receiveResponse() {
      if (!mixerInterface->state) {
        return "Non connecté.";
      }

      return mixerInterface->receiveResponse();
    }

    // Déconnexion
    void disconnect() {
      if (mixerInterface != nullptr){
        DEBUG_PRINT("\nDisconnect mixer %s",*config->getString(MIXER_NAME_KEY))  ;
        delete mixerInterface;
      }      
      state = false;
      mixerInterface = new MixerOfflineInterface(dataRepository);
      config->setValue(MIXER_MODEL_KEY,"undef");
      mixerInterface->name = "auto";
      config->setValue(SERVER_INTERFACE_KEY,"Virtual");


    }


  private:
    bool discovery = false;
    uint8_t mixerLatency = 20;

    #define MAXMIXERPORT 2
    uint16_t port[MAXMIXERPORT]={10024,10023};
    uint8_t mixerPortCounter = 0;
    uint8_t nextIPDigit = 1;
    

    //OSC AND MESSAGING MANAGEMENT
    uint16_t bSize = 20, oldbSize = bSize ;
    uint8_t * bValue = new uint8_t[bSize];
    int iValue            = -1;           // values returned by the mixer
    float fValue          = -0.1;
    char   PacketBuffer[UDP_TX_PACKET_MAX_SIZE];
    String CommandStr;
    char   TypeStr;
    String ExpectedStr;
    #define OSCLEN 40
    char sValue[OSCLEN+10]; char sValue2[OSCLEN]; char sValue1[OSCLEN];


    // Fonction privée pour détecter le type de mixeur
    
    bool detectMixer() {    
      //DEBUG_PRINT("\n * * * Detect mixer * * *");
      if (network->state != CONNECTED) return false; // no connection

      state = DISCONNECTED ;

      const String* ipValue = config->getString(SERVER_IP_KEY);
      serverIP.fromString(*ipValue);
      discovery=false;             
      if ((serverIP[0] == 0 && serverIP[1] == 0 && serverIP[2] == 0 && serverIP[3] == 0) || *ipValue == "") discovery = true;

      IPAddress testIP;
      uint16_t mixingStationPort;

      // *************** autodetect:  **********************
      if (discovery){
        //DEBUG_PRINT(" - discovery - ");
        // for behringer native : we uses brodcast on UDP
        serverIP = network->IP;
        serverIP[3] = 255 ; // broadcast              
        // for mixing station, we uses TCP next IP
        testIP = network->IP;
        testIP[3] = nextIPDigit;
        serverPort = port[mixerPortCounter];
        mixingStationPort = 8080;
      }else{
        //DEBUG_PRINT(" - userIP - ");
        // *************** manual :  **********************        
        serverPort = config->getInt(SERVER_PORT_KEY); 
        mixingStationPort = config->getInt(SERVER_PORT_KEY); 
      }
        
      //DEBUG_PRINT(" %s UDP:%s/%i TCP:%s/8080 ",ipValue->c_str(),serverIP.toString().c_str(),serverPort,testIP.toString().c_str());

      // TESTING BEHRINGER
      SendCmd2OSC("/xinfo");
      if (OSCReadIn()) {
        if (mixerInterface != nullptr) delete mixerInterface;
        mixerInterface = new BehringerMixer(dataRepository);  // Utilisation du HAL pour MixingStation
        state = CONNECTED;
        //sValue frame from OSC contains mixer IP
        if (serverIP.fromString(sValue)) {
          IPAddress UDPIP = network->udp->remoteIP();
          if (UDPIP[0] != serverIP[0]){
            // the IP returned from the mixer differs from the on in the UDP packet,
            // this happens for instance when the mixer is on a different network (emulator on DSM for example)
            // therefore, we force the IP to be fixed and the one from UDP packet
            serverIP = UDPIP;
            DEBUG_PRINT("\nMixer IP differs from UDP packet, switching to UDP IP instead.");
          }
          config->setValue(MIXER_MODEL_KEY,sValue2);
          mixerInterface->name = sValue1;
          config->setValue(SERVER_INTERFACE_KEY,"MusicTribe");
        }
      } else {
        mixerPortCounter++ ; // we iterate to the next Behringer mixer model
        // we test with a different latency and model
        if (mixerPortCounter == MAXMIXERPORT){
          mixerPortCounter = 0;
          // as the broadcast message (/xinfo) do not increase the latency in case of failure, we have to do it here if mixers where not responding
          //mixerLatency *= 2;  // the check of maxlatency is done in oscread
        }       
        // TEST MIXING STATION
        // for mixing station:  REST API is over HTTP (so uses TCP), hence we cannot broadact. We have to test all IP address from the subnet range and see if we can connect the client provided by network hardware layer
        // which is not realistic, hence we test the dfault TCP port 8080
        if (network->client->connect(testIP, mixingStationPort)) {   // Port par defaut pour MixingStation
          if (mixerInterface != nullptr) delete mixerInterface;
          mixerInterface = new MixingStation(dataRepository,config->getInt(MIXER_PORT_KEY));  // Utilisation du HAL pour MixingStation
          serverPort = mixingStationPort;
          serverIP = testIP;

          config->setValue(MIXER_MODEL_KEY,"undef");
          mixerInterface->name = "undef";
          config->setValue(SERVER_INTERFACE_KEY,"MixingStation");

          state = CONNECTED;          
        }else{
          nextIPDigit ++;        
          if (nextIPDigit == 254){
            nextIPDigit = 1;
          }
        }
      }
      return state;
    }

    void SendCmd2OSC(const char* address)       // Send OSC command (buffer), no value (status request)
    {
      DEBUG_PRINT("\n>>(%i,%i)", serverPort, mixerLatency);
      OSCMessage msg(address);
      ExpectedStr = address;
      network->udp->beginPacket(serverIP, serverPort);
      msg.send(*network->udp);
      network->udp->endPacket();
      //yield();
      delay(mixerLatency);
      msg.empty();
    }


    boolean OSCReadIn() {
      iValue = -1;
      fValue = -0.1;
      // Check if packet was received

    //  delay(latency);
      int size = network->udp->parsePacket();
      if (size < 1) return false;
    /*
      uint16_t waitRead = 0;
      int size = 0;
      do{
        size = OscUDP->parsePacket();
        delay(1);
        //DEBUG_PRINT("°");
      }while(waitRead++<=latency && size<1);
      if (size < 1){
        //DEBUG_PRINT("¦");
        return false;
      }*/
      //DEBUG_PRINT("@");

      // Get the packet
      //DEBUG_OSCMSG("\n<<:");
      OSCMessage msg;
      /*size = OscUDP->available();   
      OscUDP->read(udpRxBuffer, size); // Mise en tampon du paquet  
      msg.fill(udpRxBuffer,size);*/
      while (size--) msg.fill(network->udp->read());
      // Check for errors
      if (msg.hasError()) {
        //DEBUG_OSCMSG("error: %s\n", msg.getError());
        return false;
      }
      // Parse return OSC command
      msg.getAddress(PacketBuffer, 0);                // get message path
      CommandStr = String(PacketBuffer);              // assign to CommandStr
      TypeStr = msg.getType(0);                       // get data type

      if (msg.isInt(0)){
        iValue = msg.getInt(0);    // integer
        DEBUG_PRINT("\n<< %i",iValue);
      }
      if (msg.isFloat(0)){
        fValue = msg.getFloat(0);  // float
        DEBUG_PRINT("\n<< %.2f",fValue);
      }
      if (msg.isString(0)) {
        msg.getString(0, sValue);  // string
        msg.getString(1, sValue1);
        msg.getString(2, sValue2);
        DEBUG_PRINT("\n<< %s %s %s",sValue,sValue1,sValue2);
      }
      if (msg.isBlob(0)) {
        bSize = msg.getBlobLength(0);
        //DEBUG_OSCMSG("BLOB(%i)", bSize);
        if (bSize > oldbSize) {
          //DEBUG_PRINT("\nResizing blob buffer");
          delete bValue;
          bValue = new uint8_t[bSize];
          oldbSize = bSize;
        }
        if (msg.getBlob(0, bValue) != bSize) return false;
        //DEBUG_OSCMSG("\nbValue=(%s)", bValue);
      }
      msg.empty();     
      return true;
    }



};

#endif