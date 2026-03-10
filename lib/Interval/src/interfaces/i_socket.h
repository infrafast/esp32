#ifndef _I_SOCKET_h
#define _I_SOCKET_h

#include "Interfaces.h"


#define INTERFACE_SOCKET "NetworkSocket"
class NetworkSocket: public NetworkDevice{
  public:
  
    bool dataAvailable = false;
    SocketData* socketData;

    NetworkSocket(String name=INTERFACE_SOCKET, String group=GROUP_INTERFACE_UTILS,String echoCommand="ping",int localP=8887,int remoteP=8888, bool exposed=false):NetworkDevice(name,group,localP,remoteP,echoCommand, exposed){
      //destination.host = "192.168.99.99"; 
    }

    virtual void monitorService(){
      
        // check if we have a data socket in the queue
        // ...
        //
        //if (socket.available()) process();

      if (dataAvailable){
        process(socketData->message,socketData->length);
        dataAvailable = false;
      }

        #define PULSE_CHECK_DELAY 5000
        // delay expired, we didn't receive any incoming socket message...
        // time to ping the device to see if it's still alive or if we have to attempt reconnecting      
        if (millis()-destination.lastQueryTime > PULSE_CHECK_DELAY){                
            destination.lastQueryTime = millis(); // reset timer
            if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN){
                // if we have a socket opened with MS, we inquiry the app state to wake up the socket          
                ping();
            }else{
                // if we have no socket connection or we loose it, we try to esbalish one
                handleConnectService();
            }        
        }                                                                   
    }
  

    virtual void process(const char *message, uint16_t length) = 0;

    void virtual setStateConnected() = 0;

    void onEvent(const Event& evt) override {
      NetworkDevice::onEvent(evt);
     
      if (dataAvailable) return; // previous data has not been ocnsummed yet so we ignore it (we could make a queue)

      switch(evt.type){
        case EVENT_SOCKET_OPEN:
          // connexion has just been made to NetworkSocket
          setStateConnected();
          destination.lastQueryTime = 0;   // request an immediate /app/state to get state        
        break;
        case EVENT_SOCKET_CLOSE:
          // this is called when closing socket => so could be by handleDisconnectService or because we lost the connection
          deviceInterface->setStatePending();
        break;
        case EVENT_SOCKET_ERROR:
          deviceInterface->setStateDisconnected(*lang.getString("NetworkSocket unreachable"));
        break;
        case EVENT_SOCKET_MESSAGE:
          socketData = (SocketData*)evt.data;        
          dataAvailable = true;
        break;                
      }
    }    

    virtual bool handleConnectService(){
      DEBUG_INFO("%s::handleConnectService()",interfaceName.c_str());
      if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN){
        DEBUG_PRINT(":client already opened\n");
        return true;
      }else{
        return hwm.network->clientSocketOpen(destination.host.c_str(), remotePort, "/"); 
      }
    }

    virtual bool handleDisconnectService(){
      DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
      destination.lastQueryTime = millis();                                // do not make an immediate pulse check to avoid monitor() looping on connect/disconnect
      hwm.network->clientSocketClose(); 
      return false;
    }    
 
 
};


#endif