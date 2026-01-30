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
    
    int OSC_clientPort;
    int OSC_localPort;

    OSCMessage osc;

    virtual const String& getProviderNameForData(){ return interfaceName;}   // we are our own provider do not forget it!

    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_clientPort, String pingCommand, bool _configurable=false):OSC_localPort(_OSC_localPort), OSC_clientPort(_OSC_clientPort), DeviceInterface(name, GROUP_INTERFACE_OSC,_configurable){
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
      sprintf(buf,ADDRESS_OSCPORT,OSC_clientPort);
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
      DEBUG_PRINT("\n%s::handlePropertyChange() : call disconnect",interfaceName.c_str());
      disconnectDeviceFromNetwork();
    }

    virtual bool restoreProperty(const char* property) override{
      bool changed = false;
      uint16_t OSCport;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];      
      if (sscanf(property, ADDRESS_OSCPORT, &OSCport) == 1) {
        if (OSCport != OSC_clientPort)   changed = true;
        OSC_clientPort = OSCport;       
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

      if (dataAvailable > 0){
        // set the device to connected
        if (state.code != IF_READY) setStateConnected();
        lastQueryTime =  millis();             // and no need to trigger the verification
        connectAttemptCount = 0;               // reset attempt counter                
        return;
      }
             
      #define OSC_PING_DELAY 3000
      // delay expired
      if (millis()-lastQueryTime > OSC_PING_DELAY){        
        //should we give it a new chance or set state
        if (connectAttemptCount++ == 1){
          setStateQueryingDevice();
          connectAttemptCount = 0;          
        }else{
          ping();       
        }
      }      

    }

    int readOSC(){

        UdpSlot* udpSlot = hwm.network->getUDPslot(udpID);   // request UDP slot

        if (udpSlot == NULL) return 0;                

        UDP* udp = udpSlot->udp;

        int size = udp->parsePacket();

        if (size > 0){
            osc.empty();

            while (size--){
                osc.fill(udp->read());
                yield();
            }

            xSemaphoreGive(udpSlot->mutex);

            if (osc.hasError()) {
                DEBUG_PRINT("\nreadOSC: %s\n", osc.getError());
                return 0;   // message invalide
            }else{
              char addr[64];
              osc.getAddress(addr, 0);
              DEBUG_PRINT("\n<<%s(%s,%i) ",addr,destination->ip.toString().c_str(),OSC_clientPort);
              if ( ((OSCPing*)destination)->pingCommand == addr ){
                /* we got a pingCommand echo, so we calculate the latency
                Il y a plusieurs façons correctes de moyenner une latence, selon ce que tu veux obtenir. Je te donne les 3 méthodes classiques, de la plus simple à la plus propre pour un système embarqué.
                ✅ Méthode 1 — Moyenne glissante exponentielle (RECOMMANDÉE) très utilisé en réseau (TCP, Wi-Fi, etc.)                
                */
                unsigned long latency = millis()-lastQueryTime;

                // filtre les valeurs incoherentes , par exemple si l'interface est deconnectée
                if (latency > 500) latency = destination->latency;

                // α entre 0.1 (très lisse) et 0.3 (réactif)
                #define LAT_ALPHA 0.2f                
                destination->latency =
                    destination->latency * (1.0f - LAT_ALPHA) +
                    latency * LAT_ALPHA;                

                DEBUG_PRINT("\t latency: %i",destination->latency);
              }
            }              
            // ✔ message valide
            // retour : au moins 1, même si aucun argument
            int argCount = osc.size();
            return (argCount == 0 ? 1 : argCount);
        }
        
        xSemaphoreGive(udpSlot->mutex);
        return 0;
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

    void ping(){
      lastQueryTime = millis();       
      OSCMessage msg(((OSCPing*)destination)->pingCommand.c_str());
      sendOSCMessage(&msg);
    }

    bool sendOSCMessage(OSCMessage *msg){           
      
      bool success = false;

      DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),(OSCPing*)destination->ip.toString().c_str(),OSC_clientPort);

      UdpSlot* udpSlot = hwm.network->getUDPslot(udpID);
      // sempahore protected section
      if (udpSlot == nullptr){
          DEBUG_PRINT(" **** FAIL (UdpSlot null) ****",msg->getAddress());
          return false;
      }             

     
      UDP* udp = udpSlot->udp;

      if (udp != nullptr){
        udp->beginPacket(destination->ip, OSC_clientPort);
        msg->send(*udp);
        udp->endPacket();
        success = true;
      }else{
        success = false;
        DEBUG_PRINT(" **** FAIL (udp null) ****",msg->getAddress());        
      }

      xSemaphoreGive(udpSlot->mutex);
      // sempahore released section
      return success;          
  }


    virtual bool begin(){
      return true;
    }

    virtual bool end(){
      return true;
    }

    virtual bool connectDeviceToNetwork(){
      // network connection has just been established :-)
      setStateQueryingDevice();         // we are not yet connected to the OSC device
      lastQueryTime = 0; // force ping to be triggered
      udpID = hwm.network->requestUDPslotID(OSC_localPort);      // the network will create or reuse a udp slot and give its ID to us      
      if (udpID != -1) return true;
      return false;
    }

   virtual bool disconnectDeviceFromNetwork(){
      lastQueryTime = 0; // force immediate ping attempt
      setStateQueryingDevice();
      return true;
    }

  private:
    unsigned long lastQueryTime ;
    uint8_t connectAttemptCount = 0;
    uint8_t udpID = 0;

};



#endif
