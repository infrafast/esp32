#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"
#include "OSCMessage.h"


// ============================================================
// Helpers de parsing
// ============================================================

static bool parse2Digits(const char* p, uint8_t& value) {
  if (p[0] < '0' || p[0] > '9') return false;
  if (p[1] < '0' || p[1] > '9') return false;
  value = (p[0] - '0') * 10 + (p[1] - '0');
  return true;
}

static bool strEquals(const char* a, const char* b) {
  return strcmp(a, b) == 0;
}

static bool strStartsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

// serialization de message OSC
class MemoryPrint : public Print {
public:
  MemoryPrint(uint8_t* buf, size_t maxLen)
    : _buf(buf), _maxLen(maxLen), _len(0) {}

  size_t write(uint8_t b) override {
    if (_len >= _maxLen) return 0;
    _buf[_len++] = b;
    return 1;
  }

  size_t length() const { return _len; }

private:
  uint8_t* _buf;
  size_t _maxLen;
  size_t _len;
};

struct OscQueueNode {
  String address;
  uint8_t* data = nullptr;
  size_t len = 0;
  OscQueueNode* next = nullptr;
};

class OscMessageQueue {
public:
  OscMessageQueue()
    : head(nullptr), tail(nullptr), count(0), lastSendTime(0) {}

  ~OscMessageQueue() {
    clear();
  }

  bool enqueue(OSCMessage& msg) {
    // enqueue une copie d'un message OSC
    size_t msgLen = msg.size();

    // Sécurité minimale : allouer au moins 64 octet, meme si pas de parametres au message
    size_t allocLen = (msgLen > 0) ? msgLen : 64;

    uint8_t* temp = new uint8_t[allocLen];
    if (!temp) return false;

    MemoryPrint mp(temp, allocLen);
    msg.send(mp);

    size_t actualLen = mp.length();

    // Si rien n'a été écrit → invalide
    if (actualLen == 0) {
      delete[] temp;
      return false;
    }

    // Vérifie doublon
    for (OscQueueNode* cur = head; cur != nullptr; cur = cur->next) {
      if (cur->len == actualLen && memcmp(cur->data, temp, actualLen) == 0) {
        delete[] temp;
        return false;
      }
    }

    // Ajuste la taille si nécessaire
    uint8_t* finalBuf = temp;
    if (actualLen != allocLen) {
      finalBuf = new uint8_t[actualLen];
      if (!finalBuf) {
        delete[] temp;
        return false;
      }
      memcpy(finalBuf, temp, actualLen);
      delete[] temp;
    }

    // Création node
    OscQueueNode* node = new OscQueueNode();
    if (!node) {
      delete[] finalBuf;
      return false;
    }

    node->data = finalBuf;
    node->len = actualLen;
    node->next = nullptr;

    // FIFO
    if (!tail) {
      head = tail = node;
    } else {
      tail->next = node;
      tail = node;
    }

    count++;
    return true;
  }


  // Déqueue uniquement si delayBetweenTwoSend ms écoulées depuis le dernier envoi = latency
  bool dequeue(OSCMessage& outMsg, int delayBetweenTwoSend) {
    if (!head){
      return false;
    }

    unsigned long now = millis();
    if ((now - lastSendTime) < delayBetweenTwoSend) {
      return false;
    }

    OscQueueNode* node = head;

    for (size_t i = 0; i < node->len; i++) {
      outMsg.fill(node->data[i]);
    }

    head = head->next;
    if (!head) tail = nullptr;

    delete[] node->data;
    delete node;
    count--;

    lastSendTime = now;
    return true;
  }

  bool isEmpty() const {
    return head == nullptr;
  }

  size_t size() const {
    return count;
  }

  void clear() {
    while (head) {
      OscQueueNode* node = head;
      head = head->next;
      delete[] node->data;
      delete node;
    }
    tail = nullptr;
    count = 0;
  }

private:
  OscQueueNode* head;
  OscQueueNode* tail;
  size_t count;
  unsigned long lastSendTime;
};




#define GROUP_INTERFACE_OSC "OSC"

class NetworkOSC: public NetworkDevice {  // should derive OSCInterface

  public:
    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    OSCMessage oscIn;   // reception
    OSCMessage oscOut;  // envoi

    OscMessageQueue queue; // envoi compatible avec la latence

    Event evtOSC = Event(&oscIn);
    // from remote UDP
    IPAddress UDPIP;
    int UDPPort;
    UDP* udp = nullptr;

    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_remotePort, String echoCommand, bool _configurable=false, String group = GROUP_INTERFACE_OSC):NetworkDevice(name, group ,_OSC_localPort,_OSC_remotePort,echoCommand, _configurable){
      icon = "g";        

      if (!destination.ip.fromString(_OSC_IPaddress)) destination.ip = IPAddress(0,0,0,0);

      registerDataSetAddress(&addressPing);         //       
    
      evtOSC.type = EVENT_OSC_MESSAGE;              // thta's the only event type this interface is raising

    
    }

    ~NetworkOSC(){
    }

    virtual bool begin(){
      DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
      return true;
    }

    virtual bool end(){
      DEBUG_INFO("%s::end() calling stop()",interfaceName.c_str());            
      return stop();
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

    #define OSC_PING_DELAY 3000
    #define OSC_MESSAGE_PROCESSED_AT_ONCE 3 // process

    uint32_t lastPingTime = 0;

    virtual bool monitorService() override {
      bool dataProcessed = false;

      OSCMessage msgOut;
      if (queue.dequeue(msgOut, destination.latency)) {
        _sendOSCMessage(&msgOut);
        dataProcessed = true;
      }

      for (uint8_t i = 0; i < OSC_MESSAGE_PROCESSED_AT_ONCE; ++i) {
        int8_t dataAvailable = readOSC();
        if (dataAvailable <= 0) break;

        if (state.code != IF_READY) setStateConnected();
        connectAttemptCount = 0;

        process();
        dataProcessed = true;
      }

      if (!dataProcessed) {
        uint32_t now = millis();

        if (now - destination.lastRxTime > OSC_PING_DELAY &&
            now - lastPingTime > OSC_PING_DELAY) {

          if (connectAttemptCount >= 2) {
            if (state.label != IF_STATE_PENDING) setStateQueryingDevice();
            connectAttemptCount = 0;
          } else {
            if (ping()) {
              lastPingTime = now;
              connectAttemptCount++;
              dataProcessed = true;
            } else {
              handleConnectService();
            }
          }
        }
      }

      return dataProcessed;
    }

    
    bool send(OSCMessage& msg){
      return queue.enqueue(msg);            // queued send
      //return _sendOSCMessage(&msg);       // direct send
    }

    virtual void process(){
      fireEvent(evtOSC);  //simply propagate the data
    }    
    
    #ifdef DEBUG
      char oscDump[256];
    #endif

    int readOSC(){
        UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);   // request UDP slot
        udp = nullptr;
        
        if (udpSlot == NULL) return -1;                

        udp = udpSlot->udp;
        if (udp == nullptr) return -1;

        int size = udp->parsePacket();

        if (size > 0){
            oscIn.empty();

            while (size--){
                oscIn.fill(udp->read());                                
                delay(0);               
            }

            //xSemaphoreGive(udpSlot->mutex);

            if (oscIn.hasError()) {
                DEBUG_INFO("readOSC: %i\n", oscIn.getError());
                return 0;   // message invalide
            }else{
              char addr[64];
              oscIn.getAddress(addr, 0);

              destination.lastRxTime = millis();

              // see if it's a ping, we measure
              if (destination.echoCommand == addr) {
                if (destination.pingPending) {
                  unsigned long rawLatency = millis() - destination.lastPingSentAt;
                  calculateLatencyAndJitter(rawLatency);
                  destination.pingPending = false;
                }

                UDPIP = udp->remoteIP();
                UDPPort = udp->remotePort();
              }


              #ifdef DEBUG
                dump(oscIn, oscDump, sizeof(oscDump));              
                DEBUG_INFO("<<%s(%s,%i) %s",addr,destination.ip.toString().c_str(),remotePort, oscDump);              
              #endif
              
            }              
            // ✔ message valide
            // retour : au moins 1, même si aucun argument
            int argCount = oscIn.size();
            return (argCount == 0 ? 1 : argCount);
        }
    
        return 0;
    }

    char* dump(OSCMessage &msg, char* buffer, size_t bufferSize) {
      size_t offset = 0;
      for (int i = 0; i < msg.size(); i++) {
        if (offset >= bufferSize) break;
        if (msg.isInt(i)) offset += snprintf(buffer + offset, bufferSize - offset,"%d ", msg.getInt(i));
        else if (msg.isFloat(i)) offset += snprintf(buffer + offset, bufferSize - offset,"%f ", msg.getFloat(i));
        else if (msg.isString(i)) {
          char txt[128];
          if (msg.getString(i, txt, sizeof(txt)) > 0) offset += snprintf(buffer + offset, bufferSize - offset,"%s ", txt);
        }
        else if (msg.isBoolean(i)) offset += snprintf(buffer + offset, bufferSize - offset,"%s ", msg.getBoolean(i) ? "true" : "false");
        else if (msg.isDouble(i)) offset += snprintf(buffer + offset, bufferSize - offset,"%lf ", msg.getDouble(i));
        else if (msg.isBlob(i)) offset += snprintf(buffer + offset, bufferSize - offset,"[blob:%d] ", msg.getBlobLength(i));
        else offset += snprintf(buffer + offset, bufferSize - offset,"[type:%c] ", msg.getType(i));
      }
      return buffer;
    }

    virtual bool stop() override{          // disconnect the network for network interface
      DEBUG_INFO("%s::stop()",interfaceName.c_str());
      return handleDisconnectService();
    };  

    virtual bool isSwitchable(){
      return true;
    }    

    virtual bool handleConnectService() override{
      DEBUG_INFO("%s::handleConnectService()",interfaceName.c_str());
      // network connection is established so we now try to connect the device to its protocol
      if (udpID == -1) {
        udpID = hwm.network->requestUDPslotID(localPort);   // the network will create or reuse a udp slot and give its ID to us
      }
      if (udpID == -1) return false;                            // can't get any UDP connectivity => try again until next queryTime expired
      
      DEBUG_INFO("udp(%i) got UDP slot #%i",localPort,udpID);      
      setStateQueryingDevice();                                 // we are not yet connected to the OSC device, we will try by sending a ping
      destination.lastRxTime = 0;                             // with the valid UDP we just got so we force ping to be triggered immediately
      return true;
    }

    virtual bool handleDisconnectService() override{
      DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
      destination.lastRxTime = 0;                                // force immediate ping attempt
      destination.pingPending = false;          // clear any pending ping
      if (udpID != -1) hwm.network->releaseUDPSlot(&udpID);          // release any valid UDP slot we would have acquired 
      queue.clear();      
      return true;
    }    
  
    void setStateQueryingDevice(){
      //DEBUG_PRINT("\n%s: setStateQueryingDevice()",interfaceName.c_str());
      // network is connected but not the OSC device is not answering
      DeviceInterface::setStatePending();
      state.subState = destination.ip.toString();      
      state.progress = -1;      
    }

    /*virtual bool ping() override{
      destination.lastRxTime = millis();       
      OSCMessage msg(destination.echoCommand.c_str()); 
      return send(msg);
    }*/

    virtual bool ping() override {
      OSCMessage msg(destination.echoCommand.c_str());

      destination.lastPingSentAt = millis();
      destination.pingPending = true;

      return _sendOSCMessage(&msg);
    }    


    virtual void determineQoS() override{    
      if (state.code != IF_READY) state.QoSlevel = QoS::UNDETERMINED;
      else
      if (destination.latency <= 50 && destination.jitter <= 10) state.QoSlevel = QoS::GOOD;    
      else
      if (destination.latency <= 100 && destination.jitter <= 20) state.QoSlevel = QoS::MEDIUM;    
      else
      if (destination.latency <= 200 && destination.jitter <= 40)  state.QoSlevel = QoS::POOR;    // yellow
      else
      // Au-delà : problème réseau
      state.QoSlevel = QoS::CRITICAL;    
    }      

    bool _sendOSCMessage(OSCMessage *msg){           
      
      bool success = false;

      dumpOSCMessage(*msg);

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

  private:    
    uint8_t connectAttemptCount = 0;
    bool udpConnectionLost = false;
    int udpID = -1;
    
    //UDP* udp = nullptr; // DIRECT UDP


      void dumpOSCMessage(OSCMessage& msg){
        #ifdef DEBUG
          int argCount = msg.size();
          
          DEBUG_INFO(">>(%s,%i->%i) %s",destination.ip.toString().c_str(),localPort, remotePort, msg.getAddress());

          //DEBUG_PRINT(" | args:%d ", argCount);

          for (int i = 0; i < argCount; i++) {

            char type = msg.getType(i);

            switch(type) {

              case 'i': { // int32
                int32_t v = msg.getInt(i);
                DEBUG_PRINT("[i:%ld] ", (long)v);
                break;
              }

              case 'f': { // float
                float v = msg.getFloat(i);
                DEBUG_PRINT("[f:%f] ", v);
                break;
              }

              case 's': { // string
                char buffer[64];
                msg.getString(i, buffer, sizeof(buffer));
                DEBUG_PRINT("[s:%s] ", buffer);
                break;
              }

              case 'b': { // blob
                int size = msg.getBlobLength(i);
                const uint8_t* data = msg.getBlob(i);
                DEBUG_PRINT("[b:%d bytes] ", size);
                break;
              }

              case 'd': { // double
                double v = msg.getDouble(i);
                DEBUG_PRINT("[d:%f] ", v);
                break;
              }

              case 'T': DEBUG_PRINT("[T:true] "); break;
              case 'F': DEBUG_PRINT("[F:false] "); break;
              case 'N': DEBUG_PRINT("[N:nil] "); break;
              case 'I': DEBUG_PRINT("[I:inf] "); break;

              default:
                DEBUG_PRINT("[?:%c] ", type);
                break;
            }
          }
        #endif    
      }


};

#endif
