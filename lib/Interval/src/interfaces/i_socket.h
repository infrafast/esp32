#ifndef _I_SOCKET_h
#define _I_SOCKET_h

#include "Interfaces.h"


#define INTERFACE_SOCKET "NetworkSocket"
class NetworkSocket: public NetworkDevice{
  public:
  
  bool dataAvailable = false;

  #define MAX_SOCKET_MESSAGE_LEN kBufferMaxSize
  #define SOCKET_QUEUE_SIZE 5
  #define SOCKET_MESSAGE_PROCESSED_AT_ONCE 3

  struct SocketDataCopy {
    uint16_t length;
    char message[MAX_SOCKET_MESSAGE_LEN];
    int code;
  };

  SocketDataCopy socketQueue[SOCKET_QUEUE_SIZE];
  volatile uint8_t queueHead = 0;   // prochain message à lire
  volatile uint8_t queueTail = 0;   // prochain slot d’écriture
  volatile uint8_t queueCount = 0;

  Event evtSock = Event(nullptr);

    NetworkSocket(String name=INTERFACE_SOCKET, String group=GROUP_INTERFACE_UTILS,String echoCommand="",int localP=8887,int remoteP=8888, bool exposed=false):NetworkDevice(name,group,localP,remoteP,echoCommand, exposed){
      //destination.host = "192.168.99.99"; 
    }

    virtual void route(const char *message,uint16_t length) = 0;

    virtual bool monitorService() override{   

      dataProcessed = false;

      SocketDataCopy* msg = nullptr;
      uint8_t processed = 0;

      while (processed < SOCKET_MESSAGE_PROCESSED_AT_ONCE && (msg = dequeueSocketMessage()) != nullptr) {
        route(msg->message, msg->length);
        dataProcessed = true;               // we received some data so we flag it
        processed++;
      }

      if (!dataProcessed){
        #define PULSE_CHECK_DELAY kTimeoutInterval*1.1        // we have to wait a bit longer than a socket receive timeout
        uint32_t now = millis();
        if (now - destination.lastRxTime > PULSE_CHECK_DELAY &&
            now - destination.lastPingTime > PULSE_CHECK_DELAY) {
          // ping the device if we have a valid socket
          if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN) _target(); // this sets dataProcessed to true
          else {
            if (handleConnectService()){
              destination.lastRxTime = now;  // wait next round
              setStatePending("green medium","Searching MixingStation");
            }
            else setStateError("Thread socket-open failed");
          }
        }
      }

      return dataProcessed;
    }  

    void virtual onSocketConnected() = 0; //called when socket connection has been established

    void onEvent(const Event& evt) override {
      NetworkDevice::onEvent(evt);
      destination.lastRxTime = millis();   // do not query device imemdiately, let time for the state to stabilize

      switch(evt.type){
        case EVENT_SOCKET_OPEN:
          // connexion has just been made to NetworkSocket
          //setStateConnected();
          setStatePending();      // the derived class will have to override state
          deviceInterface->state.details = "Querying application";
          onSocketConnected();    
        break;
        case EVENT_SOCKET_CLOSE:{
          SocketEvent* evtSock = (SocketEvent*)&evt;
          SocketData* socketData = (SocketData*)evtSock->data;        
          // this is called when closing socket => so could be by handleDisconnectService or because we lost the connection
          switch(socketData->code){
            case WebSocketCloseCode::MESSAGE_TOO_BIG:
              setStateError("Unsupported mixer: too big!");
            break;
            case WebSocketCloseCode::NORMAL_CLOSURE:
              setStatePending();  // continue normally
            break;
            default:
              setStateError(concatStrings("Unsupported mixer: closure code:",socketData->code).c_str());
          }
        }
        break;
        case EVENT_SOCKET_ERROR:
          setStateDisconnected();    
        break;
        case EVENT_SOCKET_MESSAGE:
          SocketData* data = (SocketData*)evt.data;
          enqueueSocketMessage(data);
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
      destination.lastRxTime = millis();                                // do not make an immediate pulse check to avoid monitor() looping on connect/disconnect
      hwm.network->clientSocketClose(); 
      if (state.enabled == false) setStateDisabled();
      return true;
    }    

  protected:
    bool enqueueSocketMessage(const SocketData* data) {
    if (data == nullptr || data->message == nullptr) return false;

    if (queueCount >= SOCKET_QUEUE_SIZE) {
      DEBUG_WARN("%s::enqueueSocketMessage() queue full, dropping message", interfaceName.c_str());
      return false;
    }

    SocketDataCopy& slot = socketQueue[queueTail];

    uint16_t copyLen = data->length;
    if (copyLen >= MAX_SOCKET_MESSAGE_LEN) {
      copyLen = MAX_SOCKET_MESSAGE_LEN - 1;
    }

    memcpy(slot.message, data->message, copyLen);
    slot.message[copyLen] = '\0';
    slot.length = copyLen;
    slot.code = data->code;

    queueTail = (queueTail + 1) % SOCKET_QUEUE_SIZE;
    queueCount++;
    dataAvailable = (queueCount > 0);

    return true;
  }

  SocketDataCopy* dequeueSocketMessage() {
    if (queueCount == 0) {
      dataAvailable = false;
      return nullptr;
    }

    SocketDataCopy* out = &socketQueue[queueHead];
    queueHead = (queueHead + 1) % SOCKET_QUEUE_SIZE;
    queueCount--;
    dataAvailable = (queueCount > 0);

    return out;
}
 
 
};


#endif
