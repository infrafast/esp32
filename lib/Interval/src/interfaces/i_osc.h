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

class NetworkOSC: public DeviceInterface, public NetWorkInterface {  // should derive OSCInterface

  public:
    NetworkInterfaceMonitor netHelper = NetworkInterfaceMonitor(this,this);

    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    FreeTextAddress* addressOSCport = new FreeTextAddress(ADDRESS_OSCPORT,"OSCport", REGEX_0to65535);

    IPAddress OSC_IP;
    String OSC_ping;        // the command used to monitor the connection
    int OSC_clientPort;
    int OSC_localPort;

    OSCMessage osc;

    virtual const String& getDataProviderNameForThisInterface(){ return interfaceName;}

    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_clientPort, String ping, bool _configurable=false):OSC_localPort(_OSC_localPort), OSC_ping(ping), OSC_clientPort(_OSC_clientPort), DeviceInterface(name, GROUP_INTERFACE_OSC,_configurable){
      icon = "g";     

      if (!OSC_IP.fromString(_OSC_IPaddress)) OSC_IP = IPAddress(0,0,0,0);

      registerDataSetAddress(addressOSCport);
      registerDataSetAddress(&addressIP);           // global      
      registerDataSetAddress(&addressPing);         // global      

      jsonInputRepo.add(this);        
      
      data = &osc;

    }

    virtual void serializeProperties(JsonArray& properties) override{
      char* buf = (char*)malloc((strlen(ADDRESS_OSCPORT)+6)*sizeof(char*)+1);
      sprintf(buf,ADDRESS_OSCPORT,OSC_clientPort);
      properties.add(String(buf)); 
      delete buf;

      buf = (char*)malloc((strlen(ADDRESS_IP)+OSC_IP.toString().length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,OSC_IP.toString());
      properties.add(String(buf)); 
      delete buf;    
      
      buf = (char*)malloc((strlen(ADDRESS_PING)+OSC_ping.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,OSC_ping);
      properties.add(String(buf)); 
      delete buf;        
    }

    virtual void handlePropertyChange(){
      DEBUG_PRINT("\n%s::handlePropertyChange() : call disconnect",interfaceName.c_str());
      disconnect();
    }

    virtual void restoreProperty(const char* property){
      uint16_t OSCport;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];      
      if (sscanf(property, ADDRESS_OSCPORT, &OSCport) == 1) {
        if (OSCport != OSC_clientPort)   state.trigger.handleChange = true;
        OSC_clientPort = OSCport;       
      }else if (sscanf(property, ADDRESS_IP, &inputStr)) {
        IPAddress ip;
        if (ip.fromString(inputStr)){
          if (OSC_IP != ip)  state.trigger.handleChange = true;
          OSC_IP = ip;      
        }else{
          DEBUG_PRINT("\ninvalid IP address:%s",inputStr);
        }
      }else if (sscanf(property, ADDRESS_PING, &inputStr)) {
        if (OSC_ping != inputStr)  state.trigger.handleChange = true;
        OSC_ping = inputStr;    
      }
    }        

    virtual void monitor(){           
      netHelper.monitor();
    }    

    virtual void derivedMonitor(){
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

        if (udpSlot == NULL){
            return 0;                
        } 

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
              DEBUG_PRINT("\n<<%s(%s,%i) ",addr,OSC_IP.toString().c_str(),OSC_clientPort);
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
      DEBUG_PRINT("\n%s: setStateConnected()",interfaceName.c_str());
      state.code = IF_READY;      
      state.label = IF_STATE_CONNECTED;
      state.subState = OSC_IP.toString();      
      state.details = *lang.getString("Connected");
      state.led = "green";
      state.progress = -1;      
    }

    void setStateQueryingDevice(){
      DEBUG_PRINT("\n%s: setStateQueryingDevice()",interfaceName.c_str());
      // network is connected but not the OSC device is not answering
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.subState = OSC_IP.toString();      
      state.details = *lang.getString("Trying to connect");
      state.led = "red medium";
      state.progress = -1;      
    }

    void ping(){
      lastQueryTime = millis();       
      OSCMessage msg(OSC_ping.c_str());
      sendOSCMessage(&msg);
    }

    bool sendOSCMessage(OSCMessage *msg){           
      
      bool success = false;

      DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),OSC_IP.toString().c_str(),OSC_clientPort);

      UdpSlot* udpSlot = hwm.network->getUDPslot(udpID);
      // sempahore protected section
      if (udpSlot == nullptr){
          DEBUG_PRINT(" **** FAIL (UdpSlot null) ****",msg->getAddress());
          return false;
      }             

     
      UDP* udp = udpSlot->udp;

      if (udp != nullptr){
        udp->beginPacket(OSC_IP, OSC_clientPort);
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

    virtual bool connect(){
      // network connection has just been established :-)
      setStateQueryingDevice();         // we are not yet connected to the OSC device
      lastQueryTime = 0; // force ping to be triggered
      udpID = hwm.network->requestUDPslotID(OSC_localPort);      // the network will create or reuse a udp slot and give its ID to us      
      if (udpID != -1) return true;
      return false;
    }

    virtual bool disconnect(){
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