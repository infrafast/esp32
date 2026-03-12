#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

#define ADDRESS_CONNECT_METHOD "/connect/%d"
#define INTERFACE_BEHRINGERXM "BehringerXM"

enum{
    CONNECT_MANUAL=1,                   // select always start at value 1
    CONNECT_DISCOVER,        
};

class ConnectMethodDS : public DataSet{
  
  public:

    #define ADDRESS_DISCOVERY_SIZE 2

    String options[ADDRESS_DISCOVERY_SIZE]={
        "manual",
        "discovery"
    };     

    ConnectMethodDS(Address* _address): DataSet(_address, INTERFACE_BEHRINGERXM, &IFdataRepo){}
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &options[index];    
    }  

    virtual uint16_t getSize(){
      return ADDRESS_DISCOVERY_SIZE;
    }
     
};

class BehringerXM: public NetworkOSC, public MixerInterface{
  public:

    #define XM_BEHRINGER_PING_CMD "/xinfo"

    int connectMethod = CONNECT_DISCOVER;

    String lastConnectedMixerName = "undef";

    #define DISCOVERY_PORT_COUNT 2 
    int discoveryPort[DISCOVERY_PORT_COUNT] = {10024, 10023};
    uint8_t discoveryIndex = 0;               // rolling index;

    ConnectMethodDS connectMethodDS = ConnectMethodDS(new DropDownAddress(ADDRESS_CONNECT_METHOD));
    
    BehringerXM():NetworkOSC(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,XM_BEHRINGER_PING_CMD,true,GROUP_INTERFACE_MIXER){
      
      unregisterDataSetAddress(&addressPing);              // we un-expose the ping parameter as user should not modify it 
      destination.echoCommand = XM_BEHRINGER_PING_CMD;     // instead we set it manually once for all

      registerDataSetAddress(connectMethodDS.address);    // add a connect method parameter

    }     

    virtual void serializeProperties(JsonArray& properties) override{

      char* buf = (char*)malloc(strlen(ADDRESS_CONNECT_METHOD)*sizeof(char*));
      sprintf(buf,ADDRESS_CONNECT_METHOD,connectMethod);
      properties.add(String(buf));
      free(buf); 

      // inherited parameters from NetworkOSC (IP+port) are useless when we are in discovery, we therefore do not use store/serialize them
      if (CONNECT_DISCOVER != connectMethod){
        NetworkOSC::serializeProperties(properties);        
      }

    }


    virtual bool restoreProperty(const char* property) override{          
      bool changed;

      if (sscanf(property, ADDRESS_CONNECT_METHOD, &connectMethod)){
        changed |= true;
      }
      // inherited parameters from NetworkOSC (IP+port) are useless when we are in discovery, we therefore do not recall them
      if (CONNECT_DISCOVER != connectMethod){
        changed |= NetworkOSC::restoreProperty(property);
      }
      return changed;
    }       

    virtual void handlePropertyChange(){
      NetworkOSC::handlePropertyChange();
      DEBUG_INFO("%s::handlePropertyChange() refresh config",interfaceName.c_str());    
      state.broadcastConfig = true; // it could be that the connect method has changed so we need to redisplay/hide the configuration fields  
    }    

    #define STRING_MAX_SIZE 64
    char stringBuffer[STRING_MAX_SIZE];

    virtual void process() override {

      const char* addr = osc.getAddress();
      if (!addr) return;

      // Routes fixes
      if (strEquals(addr, XM_BEHRINGER_PING_CMD)) {    

        // in case we are in discovery mode, we need to retrieve the destination IP and port from the UDP connection established with the mixer
        if (CONNECT_DISCOVER == connectMethod){
          // 1️⃣ retrieve mixer IP
          osc.getString(0, stringBuffer);         // index 0 = IP address
          IPAddress ip;
          if (ip.fromString(stringBuffer)) {
            mixerIP = ip;                  
            // 2️⃣ if the IP returned from the mixer differs from the on in the UDP packet,
            // this happens for instance when the mixer is on a different network (emulator on DSM for example)            
            // however we ignore and keep the address from the user if it is different but it was not a discovery
            // which would means in that case that there are some ip masquerading and we really ened to keep the one provided by user
            if (
              (UDPIP[0] != mixerIP[0] || UDPIP[1] != mixerIP[1] || UDPIP[2] != mixerIP[2] || UDPIP[3] != mixerIP[3])
              && !CONNECT_DISCOVER
            ){
              DEBUG_WARN("Mixer IP differs from UDP packet, switching to UDP IP instead. %s->%s",stringBuffer,UDPIP.toString());
              mixerIP = UDPIP;                      
            }
            //DEBUG_PRINT("\nPinging Mixer (%s) to retrieve latency from dynamic IP",mixerIP.toString().c_str());
            //blockingPing(mixerIP); // we retrieve the real latency, now that we have the mixer address      
          } else {
              DEBUG_ERROR("String reçue mais IP invalide");
          }
             
          // 3️⃣ check if mixerIP is different than the current identified destination
          if (destination.ip != mixerIP){
            destination.ip == mixerIP;
            remotePort = UDPPort;
            state.broadcastConfig = true; // inform the webapp to force displaying the acquired IP address in the configuration field
          }

        }     

        osc.getString(1, stringBuffer);     // index 1 = name
        mixerName = stringBuffer;

        osc.getString(2, stringBuffer);     // index 2 = model
        mixerModel = stringBuffer;

        // trigger mixer sync
        if (lastConnectedMixerName != mixerName){
          lastConnectedMixerName = mixerName;
          // resetsync()
        }

                        
      }
      fireEvent(evtOSC);      // propage les données      
    }

    virtual bool ping() override{
      // in case we are in discovery mode, we alternate the value of the port using 
      if (CONNECT_DISCOVER == connectMethod){        
        remotePort = discoveryPort[discoveryIndex];        
        discoveryIndex = (discoveryIndex + 1) % DISCOVERY_PORT_COUNT;
      }
      return NetworkOSC::ping();
    }

    virtual bool handleConnectService() override{
      DEBUG_INFO("%s::handleConnectService() set broadcastIP",interfaceName.c_str());

      // when connecting the service, if we are in discovery mode we use a broadcasting IP as destination
      // destination is then set to the real IP extracted from the OSC answer

      // warning: I suspect that if a mixer disconnects => put it offline
      // then put online another mixer 
      // this mecanism will not capture the fact that destination.ip should be put back to broadcast,
      // the monitoring loop will continue with the destination.ip assigned with old ip address and ping will continue pinging in "void"

      if (CONNECT_DISCOVER == connectMethod){
        destination.ip = hwm.network->IP;
        destination.ip[3] = 255;                   //broadcast            
      }

      return NetworkOSC::handleConnectService();
    }
    


};



#endif