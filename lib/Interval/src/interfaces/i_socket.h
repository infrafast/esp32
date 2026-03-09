#ifndef _I_SOCKET_h
#define _I_SOCKET_h

#include "Interfaces.h"

/*
buffer circulaire statique de socket
Comme ton SocketData contient un const char* message, il ne faut pas empiler juste le pointeur, sinon la mémoire pointée peut devenir invalide plus tard.
Il faut donc faire une copie profonde du message dans une zone mémoire appartenant à la FIFO.
Le bon choix ici est donc :
une FIFO de taille fixe : 5 éléments
un stockage statique
un buffer texte fixe par élément
si la FIFO est pleine : on écrase l’élément le plus ancien
*/
// Paramètres FIFO
static constexpr size_t FIFO_SIZE = 5;
static constexpr size_t MAX_MESSAGE_LEN = kBufferMaxSize;   // from mWebscoket library

// Structure interne avec copie réelle du message
struct SocketDataCopy {
  uint16_t length;
  char message[MAX_MESSAGE_LEN];
  int code;
};


class SocketDataFifo {
public:
  SocketDataFifo() : head(0), tail(0), count(0) {}

  // Empile une donnée.
  // Si la FIFO est pleine, on écrase la plus ancienne.
  void push(const SocketData* data) {
    if (data == nullptr) {
      return;
    }

    SocketDataCopy& slot = buffer[head];

    // Copie sécurisée du message
    uint16_t copyLen = data->length;

    if (copyLen >= MAX_MESSAGE_LEN) {
      copyLen = MAX_MESSAGE_LEN - 1; // garde une place pour '\0'
    }

    if (data->message != nullptr && copyLen > 0) {
      memcpy(slot.message, data->message, copyLen);
    }
    slot.message[copyLen] = '\0';

    slot.length = copyLen;
    slot.code   = data->code;

    // Avance head
    head = (head + 1) % FIFO_SIZE;

    if (count < FIFO_SIZE) {
      count++;
    } else {
      // FIFO pleine -> on écrase le plus ancien
      tail = (tail + 1) % FIFO_SIZE;
    }
  }

  // Indique s'il y a au moins une donnée disponible
  bool available() const {
    return count > 0;
  }

  // Dépile la plus ancienne donnée
  // Retourne true si une donnée a été lue, false sinon
  bool pop(SocketDataCopy& out) {
    if (count == 0) {
      return false;
    }

    out = buffer[tail];
    tail = (tail + 1) % FIFO_SIZE;
    count--;

    return true;
  }

  // Nombre d'éléments présents
  size_t size() const {
    return count;
  }

  // Vide complètement la FIFO
  void clear() {
    head = 0;
    tail = 0;
    count = 0;
  }

private:
  SocketDataCopy buffer[FIFO_SIZE];
  size_t head;
  size_t tail;
  size_t count;
};


#define INTERFACE_SOCKET "NetworkSocket"
class NetworkSocket: public NetworkDevice{
  public:
  
    NetworkSocket(String name=INTERFACE_SOCKET, String group=GROUP_INTERFACE_UTILS,String echoCommand="ping",int localP=8887,int remoteP=8888, bool exposed=false):NetworkDevice(name,group,localP,remoteP,echoCommand, exposed){
      //destination.host = "192.168.99.99"; 
    }

    virtual void monitorService(){
      
        // check if we have a data socket in the queue
        // ...
        //
        //if (socket.available()) process();

        if (gSocketFifo.available()) {                                      //can be a while...
            if (gSocketFifo.pop(data)) {
                process();
            }
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
  

    virtual void process() = 0;  // called when socketData are available

    void virtual setStateConnected() = 0;

    void onEvent(const Event& evt) override {
      NetworkDevice::onEvent(evt);
     
      switch(evt.type()){
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
            const auto& e = static_cast<const SockeMessageEvent&>(evt);
            const SocketData* d = e.data();    

            // push it to the queue (creates a copy)
            gSocketFifo.push(d);
        
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
 
  protected:
    SocketDataFifo gSocketFifo;
    SocketDataCopy data;
  
};


#endif