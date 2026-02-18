#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"
#include "OSCMessage.h"

#define GROUP_INTERFACE_OSC "OSC"
/*class OSCInterface{
  public:
};*/

#define ADDRESS_OSCPORT "/oscport/%d"

class OSCPing: public PingEcho{
  public:
    OSCPing(String host, String _command):PingEcho(host), pingCommand(_command){}
    String pingCommand; //the command used to monitor the connection    
};

class NetworkOSC: public DeviceInterface, public NetWorkInterface {  // should derive OSCInterface

  public:
    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    FreeTextAddress* addressOSCport = new FreeTextAddress(ADDRESS_OSCPORT,"OSCport", REGEX_0to65535);
    
    int OSC_remotePort;
    int OSC_localPort;

    OSCMessage osc;

    virtual const String& getProviderNameForData(){ return interfaceName;}   // we are our own provider do not forget it!

    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_remotePort, String pingCommand, bool _configurable=false):OSC_localPort(_OSC_localPort), OSC_remotePort(_OSC_remotePort), DeviceInterface(name, GROUP_INTERFACE_OSC,_configurable){
      icon = "g";     
      attachDeviceInterface(this);

      destination = new OSCPing(_OSC_IPaddress,pingCommand);
      

      if (!(OSCPing*)destination->ip.fromString(_OSC_IPaddress)) destination->ip = IPAddress(0,0,0,0);

      registerDataSetAddress(addressOSCport);
      registerDataSetAddress(&addressIP);           // global      
      registerDataSetAddress(&addressPing);         // global      

      jsonInputRepo.add(this);        
      
      data = &osc;

    }

    ~NetworkOSC(){
      delete destination;
    }

    virtual void serializeProperties(JsonArray& properties) override{
      char* buf = (char*)malloc((strlen(ADDRESS_OSCPORT)+6)*sizeof(char*)+1);
      sprintf(buf,ADDRESS_OSCPORT,OSC_remotePort);
      properties.add(String(buf)); 
      free(buf);

      buf = (char*)malloc((strlen(ADDRESS_IP)+destination->ip.toString().length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,destination->ip.toString());
      properties.add(String(buf)); 
      free(buf);    
      
      buf = (char*)malloc((strlen(ADDRESS_PING)+((OSCPing*)destination)->pingCommand.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,((OSCPing*)destination)->pingCommand);
      properties.add(String(buf)); 
      free(buf);        
    }

    virtual void handlePropertyChange(){
      DEBUG_PRINT("\n%s::handlePropertyChange()",interfaceName.c_str());
      disconnectDeviceFromNetwork();
    }

    virtual bool restoreProperty(const char* property) override{
      bool changed = false;
      uint16_t OSCport;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];      
      if (sscanf(property, ADDRESS_OSCPORT, &OSCport) == 1) {
        if (OSCport != OSC_remotePort)   changed = true;
        OSC_remotePort = OSCport;       
      }else if (sscanf(property, ADDRESS_IP, &inputStr)) {
        IPAddress ip;
        if (ip.fromString(inputStr)){
          if (destination->ip != ip)  changed = true;
          destination->ip = ip;      
        }else{
          DEBUG_PRINT("\ninvalid IP address:%s",inputStr);
        }
      }else if (sscanf(property, ADDRESS_PING, &inputStr)) {
        if (((OSCPing*)destination)->pingCommand != inputStr)  changed = true;
        ((OSCPing*)destination)->pingCommand = inputStr;    
      }
      return changed;
    }        

    virtual void monitor(){           
      monitorNetworkInterface();
    }    

    virtual void connectedNetworkMonitor(){
      // Check if packet was received
      dataAvailable = readOSC();

      if (dataAvailable > 0){                                     // data has come
        if (state.code != IF_READY) setStateConnected();          // set the device to connected
        destination->lastQueryTime =  millis();                   // so no need to trigger the ping as we know the device is alive
        connectAttemptCount = 0;                                  // reset attempt counter                
        return;                                                   
      }

      if (dataAvailable == -1){                                   // lost UDP connectivity
        if (state.code != IF_NOT_READY) setStateDisconnected();   // set the device to disconnected
        destination->lastQueryTime =  millis();     
        return;
      }

      // dataAvailable == 0 : we have a valid UDP connection, so we ping the device to see if it's still alive after certain time 
      #define OSC_PING_DELAY 3000
      // delay expired
      if (millis()-destination->lastQueryTime > OSC_PING_DELAY){        
        //should we give it a new chance or set state
        if (connectAttemptCount++ == 1){
          setStateQueryingDevice();
          connectAttemptCount = 0;          
        }else{
          if (!ping()) setStateDisconnected();                    // we lost our UDP socket
        }
      }      

    }

    int readOSC(){
        UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);   // request UDP slot
        if (udpSlot == NULL) return -1;                

        // from that point in time, semaphore is taken
        UDP* udp = udpSlot->udp;
        if (udp == nullptr) return -1;

        int size = udp->parsePacket();

        if (size > 0){
            osc.empty();

            while (size--){
                osc.fill(udp->read());                                
                delay(0);               
            }

            //xSemaphoreGive(udpSlot->mutex);

            if (osc.hasError()) {
                DEBUG_PRINT("\nreadOSC: %s\n", osc.getError());
                return 0;   // message invalide
            }else{
              char addr[64];
              osc.getAddress(addr, 0);
              DEBUG_PRINT("\n<<%s(%s,%i) ",addr,destination->ip.toString().c_str(),OSC_remotePort);
              if ( ((OSCPing*)destination)->pingCommand == addr ){
                /* we got a pingCommand echo, so we calculate the latency
                Il y a plusieurs façons correctes de moyenner une latence, selon ce que tu veux obtenir. Je te donne les 3 méthodes classiques, de la plus simple à la plus propre pour un système embarqué.
                ✅ Méthode 1 — Moyenne glissante exponentielle (RECOMMANDÉE) très utilisé en réseau (TCP, Wi-Fi, etc.)                
                */
                unsigned long rawLatency = millis() - destination->lastQueryTime;

                // filtre les valeurs incoherentes , par exemple si l'interface est deconnectée
                if (rawLatency < 500){
                  // α entre 0.1 (très lisse) et 0.3 (réactif)
                  #define LAT_ALPHA 0.2f                                 

                  if (rawLatency < 500) {
                      destination->latency =
                          destination->latency * (1.0f - LAT_ALPHA) +
                          rawLatency * LAT_ALPHA;

                      destination->latency = (uint16_t)destination->latency;

                      float delta = abs((int)rawLatency - (int)destination->latency);

                      destination->jitter =
                          destination->jitter * 0.75f +
                          delta * 0.25f;                      
                      }                  

                }
                DEBUG_PRINT("\t latency: %i, jitter:%i",destination->latency,destination->jitter);
              }
            }              
            // ✔ message valide
            // retour : au moins 1, même si aucun argument
            int argCount = osc.size();
            return (argCount == 0 ? 1 : argCount);
        }
        
        //xSemaphoreGive(udpSlot->mutex);
        return 0;
    }


    bool sendOSCMessage(OSCMessage *msg){           
      
      bool success = false;

      DEBUG_PRINT("\n>>%s(%s,%i->%i) ",msg->getAddress(),(OSCPing*)destination->ip.toString().c_str(),OSC_localPort, OSC_remotePort);

      UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);

      if (udpSlot == nullptr){
          DEBUG_PRINT(" **** FAIL (UdpSlot null) ****",msg->getAddress());
          return false;
      }            
     
      UDP* udp = udpSlot->udp; 

      if (udp != nullptr){
        udp->beginPacket(destination->ip, OSC_remotePort);
        msg->send(*udp);
        udp->endPacket();
        success = true;
      }else{
        DEBUG_PRINT(" **** FAIL (udp null) ****",msg->getAddress());        
      }

      return success;          
  }


    virtual bool begin(){
      return true;
    }

    virtual bool end(){
      return true;
    }

    virtual bool connectDeviceToNetwork(){
      // network connection is established so we now try to connect the device to its protocol
      destination->lastQueryTime = 0;                         // force ping to be triggered    
      udpID = hwm.network->requestUDPslotID(OSC_localPort);   // the network will create or reuse a udp slot and give its ID to us
      if (udpID == -1) return false;                          // can't get any UDP connectivity
      setStateQueryingDevice();                               // we are not yet connected to the OSC device, we try to
      return true;
    }

   virtual bool disconnectDeviceFromNetwork(){
      destination->lastQueryTime = 0;               // force immediate ping attempt
      hwm.network->stopUDP(udpID);                  // release UDP
      return true;
    }

    void setStateConnected(){            
      //DEBUG_PRINT("\n%s: setStateConnected()",interfaceName.c_str());
      state.code = IF_READY;      
      state.label = IF_STATE_CONNECTED;
      state.subState = destination->ip.toString();      
      state.details = *lang.getString("Connected");
      state.led = "green";
      state.progress = -1;      
    }

    void setStateQueryingDevice(){
      //DEBUG_PRINT("\n%s: setStateQueryingDevice()",interfaceName.c_str());
      // network is connected but not the OSC device is not answering
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.subState = destination->ip.toString();      
      state.details = *lang.getString("Trying to connect");
      state.led = "red medium";
      state.progress = -1;      
    }

    virtual void setStateDisconnected() override{
      if (!deviceInterface) return;
      deviceInterface->state.code = IF_NOT_READY;
      deviceInterface->state.label = IF_STATE_DISCONNECTED;
      deviceInterface->state.details = concatStrings(*lang.getString("UDP failed"));      
      deviceInterface->state.led = "red";
      deviceInterface->state.progress = -1;
    }     

    bool ping(){
      destination->lastQueryTime = millis();       
      OSCMessage msg(((OSCPing*)destination)->pingCommand.c_str());
      return sendOSCMessage(&msg);
    }

  private:    
    uint8_t connectAttemptCount = 0;
    int udpID = -1;
    
    //UDP* udp = nullptr; // DIRECT UDP

};



#endif
