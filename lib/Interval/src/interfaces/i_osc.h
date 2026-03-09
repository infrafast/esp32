#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"
#include "OSCMessage.h"

#define GROUP_INTERFACE_OSC "OSC"
/*class OSCInterface{
  public:
};*/


class OscMessageEvent : public Event {
public:
  explicit OscMessageEvent(OSCMessage& msg) : _msg(msg) {}
  EventType type() const override {return EVENT_OSC_MESSAGE;}
  OSCMessage& message() const {return _msg;}
  const char* serialize() const override {return _msg.getAddress();}
private:
  OSCMessage& _msg;
};

class NetworkOSC: public NetworkDevice {  // should derive OSCInterface

  public:
    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    OSCMessage osc;
    OscMessageEvent oscEvent{osc};        //Comme OscMessageEvent contient une référence, il est obligatoire de l’initialiser dans la liste d’initialisation du constructeur, CCCCC

    virtual const String& getProviderNameForData(){ return interfaceName;}   // we are our own provider do not forget it!

    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_remotePort, String echoCommand, bool _configurable=false, String group = GROUP_INTERFACE_OSC):NetworkDevice(name, group ,_OSC_localPort,_OSC_remotePort,echoCommand, _configurable){
      icon = "g";        

      if (!destination.ip.fromString(_OSC_IPaddress)) destination.ip = IPAddress(0,0,0,0);
  
      registerDataSetAddress(&addressPing);         // global      
    
      //data = &osc;                                  // this interface manage OSCMessage as data 

    }

    ~NetworkOSC(){
    }

    virtual void serializeProperties(JsonArray& properties) override{
      NetworkDevice::serializeProperties(properties);
      
      buf = (char*)malloc((strlen(ADDRESS_PING)+destination.echoCommand.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_PING,destination.echoCommand);
      properties.add(String(buf)); 
      free(buf);              
    }

    virtual bool restoreProperty(const char* property) override{
      
      bool changed = false;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];   

      if (sscanf(property, ADDRESS_PING, &inputStr)) {
        if (destination.echoCommand != inputStr)  changed = true;
        destination.echoCommand = inputStr;    
      }

      return changed | NetworkDevice::restoreProperty(property);
    }        

     virtual void monitorService(){
      // Check if packet was received
      dataAvailable = readOSC();

      if (dataAvailable > 0){                                     // data has come
        if (state.code != IF_READY) setStateReady();          // set the device to connected
        destination.lastQueryTime =  millis();                   // so no need to trigger the ping as we know the device is alive
        connectAttemptCount = 0;                                  // reset attempt counter                
        return;                                                   
      }

      if (dataAvailable == -1){                                     // UDP not replying
        if (deviceInterface->state.label != IF_STATE_DISCONNECTED) setStateDisconnected();
      }

      // time to ping the device to see if it's still alive after certain time or if we have no valid UDP, we reconnect
      #define OSC_PING_DELAY 3000
      // delay expired
      if (millis()-destination.lastQueryTime > OSC_PING_DELAY){        
        destination.lastQueryTime = millis();
        if (connectAttemptCount++ == 1){            //put 2 if you give it a new chance or set state (2x try)
          setStateQueryingDevice();
          connectAttemptCount = 0;          
        }else{
          if (!ping()) handleConnectService();
        }                                                                 
      }                                                                   
    }

    int readOSC(){
        UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);   // request UDP slot
        if (udpSlot == NULL) return -1;                

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
                DEBUG_INFO("readOSC: %i\n", osc.getError());
                return 0;   // message invalide
            }else{
              char addr[64];
              osc.getAddress(addr, 0);
              DEBUG_INFO("<<%s(%s,%i) ",addr,destination.ip.toString().c_str(),remotePort);

              // see if it's a ping, we measure
              if ( destination.echoCommand == addr ) determineQoS();

              // we now forward the event
              // before that we may filter here some message (ex. the ping...)
              fireEvent(oscEvent);

            }              
            // ✔ message valide
            // retour : au moins 1, même si aucun argument
            int argCount = osc.size();
            return (argCount == 0 ? 1 : argCount);
        }
    
        return 0;
    }


    bool sendOSCMessage(OSCMessage *msg){           
      
      bool success = false;

      DEBUG_INFO(">>%s(%s,%i->%i) ",msg->getAddress(),destination.ip.toString().c_str(),localPort, remotePort);

      UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);

      if (udpSlot == nullptr){
          DEBUG_ERROR(" **** FAIL (UdpSlot null) ****");
          return false;
      }            
     
      UDP* udp = udpSlot->udp; 

      if (udp != nullptr){
        udp->beginPacket(destination.ip, remotePort);
        msg->send(*udp);
        udp->endPacket();
        success = true;
      }else{
        DEBUG_ERROR(" **** FAIL (udp null) ****");        
      }

      return success;          
    }

    virtual bool stop() override{          // disconnect the network for network interface
      DEBUG_INFO("%s::stop()",interfaceName.c_str());
      return handleDisconnectService();
    };  

    virtual bool isSwitchable(){
      return true;
    }    

    virtual bool handleConnectService(){
      DEBUG_INFO("%s::handleConnectService()",interfaceName.c_str());
      // network connection is established so we now try to connect the device to its protocol
      if (udpID == -1) {
        udpID = hwm.network->requestUDPslotID(localPort);   // the network will create or reuse a udp slot and give its ID to us
      }
      if (udpID == -1) return false;                            // can't get any UDP connectivity => try again until next queryTime expired
      
      DEBUG_INFO("udp(%i) got UDP slot #%i",localPort,udpID);      
      setStateQueryingDevice();                                 // we are not yet connected to the OSC device, we will try by sending a ping
      destination.lastQueryTime = 0;                           // with the valid UDP we just got so we force ping to be triggered immediately
      return true;
    }

    virtual bool handleDisconnectService(){
      DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
      destination.lastQueryTime = 0;                                // force immediate ping attempt
      if (udpID != -1) hwm.network->releaseUDPSlot(&udpID);          // release any valid UDP slot we would have acquired 
      //setStateDisconnected();
      return true;
    }    
  
    void setStateQueryingDevice(){
      //DEBUG_PRINT("\n%s: setStateQueryingDevice()",interfaceName.c_str());
      // network is connected but not the OSC device is not answering
      DeviceInterface::setStatePending();
      state.subState = destination.ip.toString();      
      state.progress = -1;      
    }

    virtual bool ping() override{
      destination.lastQueryTime = millis();       
      OSCMessage msg(destination.echoCommand.c_str());
      return sendOSCMessage(&msg);
    }

    void determineQoS(){      
      /* we got a echoCommand echo, so we calculate the latency
      Il y a plusieurs façons correctes de moyenner une latence, selon ce que tu veux obtenir. Je te donne les 3 méthodes classiques, de la plus simple à la plus propre pour un système embarqué.
      ✅ Méthode 1 — Moyenne glissante exponentielle (RECOMMANDÉE) très utilisé en réseau (TCP, Wi-Fi, etc.)                
      */
      unsigned long rawLatency = millis() - destination.lastQueryTime;

      // filtre les valeurs incoherentes , par exemple si l'interface est deconnectée
      if (rawLatency < 500){
        // α entre 0.1 (très lisse) et 0.3 (réactif)
        #define LAT_ALPHA 0.2f                                 

        if (rawLatency < 500) {
            destination.latency =
                destination.latency * (1.0f - LAT_ALPHA) +
                rawLatency * LAT_ALPHA;

            destination.latency = (uint16_t)destination.latency;

            float delta = abs((int)rawLatency - (int)destination.latency);

            destination.jitter =
                destination.jitter * 0.75f +
                delta * 0.25f;                      
            }                  

      }

      DEBUG_INFO("\t latency: %i, jitter:%i",destination.latency,destination.jitter);      

      // Sécurité bornes
      if (destination.latency > 500) destination.latency = 500;
      
      if (destination.latency <= 15 && destination.jitter <= 5)
        state.qualityColor = GUI::green;    
      else if (destination.latency <= 30 && destination.jitter <= 5)
        state.qualityColor = GUI::blue;    
      else if (destination.latency <= 50 && destination.jitter <= 20)
        state.qualityColor = GUI::yellow;    
      // Au-delà : problème réseau
      else
        state.qualityColor = GUI::red;    
    }      

  private:    
    uint8_t connectAttemptCount = 0;
    bool udpConnectionLost = false;
    int udpID = -1;
    
    //UDP* udp = nullptr; // DIRECT UDP

};

#endif
