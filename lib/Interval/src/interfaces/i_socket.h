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
  #define SOCKET_TX_QUEUE_SIZE 12
  #define SOCKET_TX_MESSAGE_SENT_AT_ONCE 8

  struct SocketDataCopy {
    uint16_t length;
    char message[MAX_SOCKET_MESSAGE_LEN];
    int code;
  };

  SocketDataCopy socketQueueRx[SOCKET_QUEUE_SIZE];
  volatile uint8_t queueHeadRx = 0;   // prochain message RX à lire
  volatile uint8_t queueTailRx = 0;   // prochain slot RX d’écriture
  volatile uint8_t queueCountRx = 0;

  // Queue statique d’émission : évite de flooder la socket quand plusieurs setters
  // appellent hwm.network->clientSocketSend() en rafale.
  SocketDataCopy socketQueueTx[SOCKET_TX_QUEUE_SIZE];
  volatile uint8_t queueHeadTx = 0;   // prochain message TX à envoyer
  volatile uint8_t queueTailTx = 0;   // prochain slot TX d’écriture
  volatile uint8_t queueCountTx = 0;
  uint32_t lastSocketTxTime = 0; // dernier envoi TX différé, utilisé pour respecter destination.latency

  Event evtSock = Event(nullptr);

    NetworkSocket(String name=INTERFACE_SOCKET, String group=GROUP_INTERFACE_UTILS,String echoCommand="",int localP=8887,int remoteP=8888, bool exposed=false):NetworkDevice(name,group,localP,remoteP,echoCommand, exposed){
      //destination.host = "192.168.99.99"; 
    }

    virtual void route(const char *message,uint16_t length) = 0;

    virtual bool monitorService() override{   

      dataProcessed = false;

      SocketDataCopy* msg = nullptr;
      uint8_t processed = 0;

      while (processed < SOCKET_MESSAGE_PROCESSED_AT_ONCE && (msg = dequeueSocketMessageRx()) != nullptr) {
        destination.lastRxTime = millis();  // timestamp of data reception, used to calculate latency and jitter
        route(msg->message, msg->length);
        dataProcessed = true;               // we received some data so we flag it
        processed++;
      }

      // Envoi différé : on ne sort qu’un petit nombre de messages par passage
      // dans monitorService(), au lieu d’envoyer immédiatement depuis les setters.
      uint8_t sent = flushSocketMessageTx();
      if (sent > 0) dataProcessed = true;

      if (!dataProcessed){
        #define PULSE_CHECK_DELAY kTimeoutInterval*1.1        // we have to wait a bit longer than a socket receive timeout
        uint32_t now = millis();
        if (now - destination.lastRxTime > PULSE_CHECK_DELAY &&
            now - destination.lastPingTx > PULSE_CHECK_DELAY) {
          // ping the device if we have a valid socket
          if (hwm.network->clientSocketState() == WebSocketReadyState::OPEN) pulseCheck(); // this sets dataProcessed to true
          else {
            if (handleConnectService()){
              destination.lastPingTx = now;  // wait next round
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
      destination.lastRxTime = millis();   // do not query device immediately, let time for the state to stabilize

      switch(evt.type){
        case EVENT_SOCKET_OPEN:
          // connexion has just been made to NetworkSocket
          //setStateConnected();
          setStatePending();      // the derived class will have to override state
          deviceInterface->state.details = "Querying application";
          onSocketConnected();    
        break;
        case EVENT_SOCKET_CLOSE:{
          clearSocketMessageTx();  // évite d’envoyer d’anciens ordres après reconnexion
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
          enqueueSocketMessageRx(data);
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
      destination.lastPingTx = millis();                                // do not make an immediate pulse check to avoid monitor() looping on connect/disconnect
      hwm.network->clientSocketClose(); 
      if (state.enabled == false) setStateDisabled();
      return true;
    }    

  protected:
    bool enqueueSocketMessageTx(const char* message) {
      if (message == nullptr) return false;

      uint16_t copyLen = strlen(message);
      if (copyLen >= MAX_SOCKET_MESSAGE_LEN) {
        copyLen = MAX_SOCKET_MESSAGE_LEN - 1;
        DEBUG_WARN("%s::enqueueSocketMessageTx() message truncated", interfaceName.c_str());
      }

      // Si la queue est pleine, on envoie directement le nouveau message.
      if (queueCountTx >= SOCKET_TX_QUEUE_SIZE) {
        DEBUG_WARN(
          "%s::enqueueSocketMessageTx() queue full (%d/%d), sending newest TX message directly",
          interfaceName.c_str(),
          queueCountTx,
          SOCKET_TX_QUEUE_SIZE
        );

        if (hwm.network != nullptr &&
            hwm.network->clientSocketState() == WebSocketReadyState::OPEN) {
          hwm.network->clientSocketSend(message);
          lastSocketTxTime = millis();
          return true;
        }

        DEBUG_WARN("%s::enqueueSocketMessageTx() direct send failed: socket not open", interfaceName.c_str());
        return false;
      }

      SocketDataCopy& slot = socketQueueTx[queueTailTx];
      memcpy(slot.message, message, copyLen);
      slot.message[copyLen] = '\0';
      slot.length = copyLen;
      slot.code = 0;

      queueTailTx = (queueTailTx + 1) % SOCKET_TX_QUEUE_SIZE;
      queueCountTx++;

      return true;
    }

    SocketDataCopy* dequeueSocketMessageTx() {
      if (queueCountTx == 0) return nullptr;

      SocketDataCopy* out = &socketQueueTx[queueHeadTx];
      queueHeadTx = (queueHeadTx + 1) % SOCKET_TX_QUEUE_SIZE;
      queueCountTx--;

      return out;
    }

    uint8_t flushSocketMessageTx() {
      if (hwm.network->clientSocketState() != WebSocketReadyState::OPEN) return 0;
      if (queueCountTx == 0) return 0;

      uint32_t now = millis();
      uint32_t txDelay = (destination.latency > 0) ? (uint32_t)destination.latency : 0;

      // Non bloquant : si le délai mesuré avec MixingStation n'est pas écoulé,
      // on garde le message en queue et monitorService() retentera au prochain passage.
      if (lastSocketTxTime != 0 && (now - lastSocketTxTime) < txDelay) return 0;

      uint8_t sent = 0;
      SocketDataCopy* msg = nullptr;

      while (sent < SOCKET_TX_MESSAGE_SENT_AT_ONCE &&
             (msg = dequeueSocketMessageTx()) != nullptr) {
        hwm.network->clientSocketSend(msg->message);
        lastSocketTxTime = now;
        sent++;

        // Avec SOCKET_TX_MESSAGE_SENT_AT_ONCE > 1, on évite quand même d'envoyer
        // plusieurs messages dans le même tour si destination.latency impose un délai.
        if (txDelay > 0) break;
      }

      return sent;
    }

    void clearSocketMessageTx() {
      queueHeadTx = 0;
      queueTailTx = 0;
      queueCountTx = 0;
      lastSocketTxTime = 0;
    }

    bool enqueueSocketMessageRx(const SocketData* data) {
    if (data == nullptr || data->message == nullptr) return false;

    if (queueCountRx >= SOCKET_QUEUE_SIZE) {
      DEBUG_WARN("%s::enqueueSocketMessageRx() queue full, dropping message", interfaceName.c_str());
      return false;
    }

    SocketDataCopy& slot = socketQueueRx[queueTailRx];

    uint16_t copyLen = data->length;
    if (copyLen >= MAX_SOCKET_MESSAGE_LEN) {
      copyLen = MAX_SOCKET_MESSAGE_LEN - 1;
    }

    memcpy(slot.message, data->message, copyLen);
    slot.message[copyLen] = '\0';
    slot.length = copyLen;
    slot.code = data->code;

    queueTailRx = (queueTailRx + 1) % SOCKET_QUEUE_SIZE;
    queueCountRx++;
    dataAvailable = (queueCountRx > 0);

    return true;
  }

  SocketDataCopy* dequeueSocketMessageRx() {
    if (queueCountRx == 0) {
      dataAvailable = false;
      return nullptr;
    }

    SocketDataCopy* out = &socketQueueRx[queueHeadRx];
    queueHeadRx = (queueHeadRx + 1) % SOCKET_QUEUE_SIZE;
    queueCountRx--;
    dataAvailable = (queueCountRx > 0);

    return out;
}
 
 
};


#endif
