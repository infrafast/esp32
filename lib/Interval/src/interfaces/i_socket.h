#ifndef _I_SOCKET_h
#define _I_SOCKET_h

#include "Interfaces.h"


#define INTERFACE_SOCKET "NetworkSocket"
class NetworkSocket: public NetworkDevice{
  public:
  
    bool dataAvailable = false;

    #define MAX_SOCKET_MESSAGE_LEN kBufferMaxSize
    // Structure interne avec copie réelle du message
    struct SocketDataCopy {
      uint16_t length;
      char message[MAX_SOCKET_MESSAGE_LEN];
      int code;
    }socketCopy;    

    Event evtSock = Event(&socketCopy);  // we always fire the event using the copy

    NetworkSocket(String name=INTERFACE_SOCKET, String group=GROUP_INTERFACE_UTILS,String echoCommand="",int localP=8887,int remoteP=8888, bool exposed=false):NetworkDevice(name,group,localP,remoteP,echoCommand, exposed){
      //destination.host = "192.168.99.99"; 
    }

    virtual void route(const char *message,uint16_t length) = 0;

    virtual void monitorService(){   

      if (dataAvailable){
        // recupere la donnée reçue et stockée  
        route(socketCopy.message,socketCopy.length);      // informe qui de droit          
        dataAvailable = false;
      }

      #define PULSE_CHECK_DELAY kTimeoutInterval*1.1        // kTimeoutInterval is the timeout of opening a socket from 
      // delay expired, we didn't receive any incoming socket message...
      // time to ping the device to see if it's still alive or if we have to attempt reconnecting      
      if (millis()-destination.lastQueryTime > PULSE_CHECK_DELAY){                
          destination.lastQueryTime = millis(); // reset timer
          if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN){
            // if we have a socket opened with MS, we inquiry the app state to wake up the socket          
            ping();
          }else{
            // if we have no socket connection or we loose it, we try to esbalish one
            if (!handleConnectService()) setStateError("Thread socket-open failed");
            else setStatePending();
          }        
      }                                                                   
    }
  

    void virtual onSocketConnected() = 0; //called when socket connection has been established

    void onEvent(const Event& evt) override {
      NetworkDevice::onEvent(evt);
     
      if (dataAvailable) return; // previous data has not been ocnsummed yet so we ignore it (we could make a queue)

      switch(evt.type){
        case EVENT_SOCKET_OPEN:
          // connexion has just been made to NetworkSocket
          setStateConnected();
          destination.lastQueryTime = 0;   // request an immediate /app/state to ping
          onSocketConnected();    
        break;
        case EVENT_SOCKET_CLOSE:
          // this is called when closing socket => so could be by handleDisconnectService or because we lost the connection
          setStatePending();
        break;
        case EVENT_SOCKET_ERROR:
          setStateDisconnected();
        break;
        case EVENT_SOCKET_MESSAGE:
          destination.lastQueryTime = millis();   // no need to query device

          SocketData* data = (SocketData*)evt.data;        

          // Comme la data va etre consommée dans la prochaine boucle, les pointeurs seront perdus, il faut donc faire une copie profonde (unique... sinon on peut faire une queue) qu'on utilisera plus tard
          uint16_t copyLen = data->length;

          if (copyLen >= MAX_SOCKET_MESSAGE_LEN) {
            copyLen = MAX_SOCKET_MESSAGE_LEN - 1; // garde une place pour '\0'
          }

          if (data->message != nullptr && copyLen > 0) {
            memcpy(socketCopy.message, data->message, copyLen);
          }
          socketCopy.message[copyLen] = '\0';
          socketCopy.length = copyLen;
          
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
      if (state.enabled == false) setStateDisabled();
      return true;
    }    
 
 
};


#endif