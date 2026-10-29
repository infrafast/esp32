/**
 * @file i_osc.h
 * @brief OSC message buffering and network transport.
 */

#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"
#include "OSCMessage.h"


// ============================================================
// Helpers de parsing
// ============================================================

/** @brief Read the numeric index encoded by two address characters. */
static bool parse2Digits(const char* p, uint8_t& value) {
  if (p[0] < '0' || p[0] > '9') return false;
  if (p[1] < '0' || p[1] > '9') return false;
  value = (p[0] - '0') * 10 + (p[1] - '0');
  return true;
}

/** @brief Test whether two strings contain identical text. */
static bool strEquals(const char* a, const char* b) {
  return strcmp(a, b) == 0;
}

/** @brief Test a string prefix without allocating a temporary String. */
static bool strStartsWith(const char* s, const char* prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

// serialization de message OSC
// ============================================================
// Helpers de sérialisation OSC
// ============================================================

/**
 * @brief Print adapter writing encoded OSC bytes into caller-provided memory.
 */
class MemoryPrint : public Print {
public:
  /**
   * @brief Initialize MemoryPrint and its configured member bindings.
   */
  MemoryPrint(uint8_t* buf, size_t maxLen)
    : _buf(buf), _maxLen(maxLen), _len(0), _overflow(false) {}

  /**
   * @brief Copy output bytes into the supplied memory without exceeding its capacity.
   *
   * @return Number of bytes accepted; overflow() records a capacity shortfall.
   */
  size_t write(uint8_t b) override {
    if (_len >= _maxLen) {
      _overflow = true;
      return 0;
    }
    _buf[_len++] = b;
    return 1;
  }

  /** @brief Return the number of bytes stored or counted. */
  size_t length() const { return _len; }
  /** @brief Report whether output exceeded the caller-provided buffer. */
  bool overflow() const { return _overflow; }

private:
  uint8_t* _buf;
  size_t _maxLen;
  size_t _len;
  bool _overflow;
};

/**
 * @brief Counts encoded output bytes without storing them.
 */
class CountingPrint : public Print {
public:
  /**
   * @brief Initialize CountingPrint and its configured member bindings.
   */
  CountingPrint() : _len(0) {}

  /**
   * @brief Count output bytes without allocating or storing a message buffer.
   */
  size_t write(uint8_t) override {
    ++_len;
    return 1;
  }

  /** @brief Return the number of bytes stored or counted. */
  size_t length() const { return _len; }

private:
  size_t _len;
};

/**
 * @brief Owned encoded bytes for one outgoing command in the OSC FIFO.
 */
struct OscQueueNode {
  uint8_t* data = nullptr;
  size_t len = 0;
  OscQueueNode* next = nullptr;
};

/**
 * @brief Outgoing OSC command FIFO with duplicate suppression and paced removal.
 */
class OscMessageQueue {
public:
  /**
   * @brief Initialize OscMessageQueue and its configured member bindings.
   */
  OscMessageQueue()
    : head(nullptr), tail(nullptr), count(0), lastSendTime(0) {}

  /** @brief Release resources held by this object. */
  ~OscMessageQueue() {
    clear();
  }

  /**
   * @brief Encode an outgoing OSC command and append it unless already queued.
   *
   * Count encoded bytes first, allocate exactly that buffer, then serialize and
   * verify the byte count. Byte-identical commands already in the queue are rejected.
   * The queue owns both node and encoded bytes; no peer address is stored per entry.
   * @return True only when a new command was appended.
   */
  bool enqueue(OSCMessage& msg) {
    if (msg.hasError()) {
      return false;
    }

    // 1) Mesure exacte de la taille sérialisée OSC
    CountingPrint counter;
    msg.send(counter);
    size_t serializedLen = counter.length();

    if (serializedLen == 0) {
      return false;
    }

    // 2) Allocation buffer exact
    uint8_t* buf = new uint8_t[serializedLen];
    if (!buf) {
      return false;
    }

    // 3) Sérialisation réelle
    MemoryPrint mp(buf, serializedLen);
    msg.send(mp);

    if (mp.overflow() || mp.length() != serializedLen) {
      delete[] buf;
      return false;
    }

    // 4) Détection doublon (optionnel, comme dans votre version)
    for (OscQueueNode* cur = head; cur != nullptr; cur = cur->next) {
      if (cur->len == serializedLen && memcmp(cur->data, buf, serializedLen) == 0) {
        delete[] buf;
        return false;
      }
    }

    // 5) Création noeud
    OscQueueNode* node = new OscQueueNode();
    if (!node) {
      delete[] buf;
      return false;
    }

    node->data = buf;
    node->len = serializedLen;
    node->next = nullptr;

    // 6) FIFO append
    if (tail == nullptr) {
      head = tail = node;
    } else {
      tail->next = node;
      tail = node;
    }

    ++count;
    return true;
  }

  // Déqueue uniquement si delayBetweenTwoSend ms écoulées depuis le dernier envoi
  /**
   * @brief Decode and remove the oldest queued command after the send delay.
   *
   * Delay is measured since the last successfully dequeued command. The node is
   * released even if decoding fails, and outMsg is emptied on failure.
   * @param outMsg Destination message; cleared before loading queued bytes.
   * @param delayBetweenTwoSend Minimum interval in milliseconds.
   * @return Whether a due command was decoded successfully.
   */
  bool dequeue(OSCMessage& outMsg, int delayBetweenTwoSend) {
    if (head == nullptr) {
      return false;
    }

    unsigned long now = millis();
    if ((unsigned long)(now - lastSendTime) < (unsigned long)delayBetweenTwoSend) {
      return false;
    }

    OscQueueNode* node = head;

    // Important : repartir d'un message vide
    outMsg.empty();

    for (size_t i = 0; i < node->len; ++i) {
      outMsg.fill(node->data[i]);
    }

    bool ok = !outMsg.hasError();

    head = node->next;
    if (head == nullptr) {
      tail = nullptr;
    }

    delete[] node->data;
    delete node;
    --count;

    if (ok) {
      lastSendTime = now;
    } else {
      outMsg.empty();
    }

    return ok;
  }

  /**
   * @brief Test whether queued output can be sent under the inter-command delay.
   */
  bool peekIsDue(int delayBetweenTwoSend) const {
    if (head == nullptr) {
      return false;
    }

    unsigned long now = millis();
    return ((unsigned long)(now - lastSendTime) >= (unsigned long)delayBetweenTwoSend);
  }

  /** @brief Report whether the queue contains no entries. */
  bool isEmpty() const {
    return head == nullptr;
  }

  /** @brief Return the current number of entries. */
  size_t size() const {
    return count;
  }

  /** @brief Release stored entries and reset the container state. */
  void clear() {
    while (head != nullptr) {
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

/**
 * @brief OSC network transport with message events and presence monitoring.
 */
class NetworkOSC: public NetworkDevice {  // should derive OSCInterface

  public:
    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    OSCMessage oscIn;   ///< reception
    OSCMessage oscOut;  ///< envoi

    OscMessageQueue queue; ///< envoi compatible avec la latence

    Event evtOSC = Event(&oscIn);
    // from remote UDP
    IPAddress UDPIP;
    int UDPPort;
    UDP* udp = nullptr;

    /** @brief Initialize NetworkOSC with the supplied configuration and bindings. */
    NetworkOSC(String name, int _OSC_localPort, String _OSC_IPaddress,int _OSC_remotePort, String echoCommand, bool _configurable=false, String group = GROUP_INTERFACE_OSC):NetworkDevice(name, group ,_OSC_localPort,_OSC_remotePort,echoCommand, _configurable){
      icon = "g";        

      if (!destination.ip.fromString(_OSC_IPaddress)) destination.ip = IPAddress(0,0,0,0);

      registerDataSetAddress(&addressPing);         //       
    
      evtOSC.type = EVENT_OSC_MESSAGE;              // thta's the only event type this interface is raising

    
    }

    virtual String getDefaultHint() const override { return "hint.sysosc"; }

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~NetworkOSC(){
    }

    /** @brief Start this interface using its current configuration. */
    virtual bool begin(){
      DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
      return true;
    }

    /** @brief End the interface session and release its active connection. */
    virtual bool end(){
      DEBUG_INFO("%s::end() calling stop()",interfaceName.c_str());            
      return stop();
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(JsonArray& properties) override{
      NetworkDevice::serializeProperties(properties);
      
      buf = (char*)malloc((strlen(ADDRESS_PING)+destination.echoCommand.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_PING,destination.echoCommand);
      properties.add(String(buf)); 
      free(buf);              
    }

    /** @brief Restore one property string and report whether its value changed. */
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
    #define OSC_MESSAGE_PROCESSED_AT_ONCE 3

   /** @brief Perform one service-specific connection and data-processing pass. */
   virtual bool monitorService() override {
      dataProcessed = false;

      // read a burst of incoming OSC message as data arrived
      for (uint8_t i = 0; i < OSC_MESSAGE_PROCESSED_AT_ONCE; ++i) {
        int8_t dataAvailable = readOSC();
        if (dataAvailable <= 0) break;  // UDP stream is empty, we exit

        dataProcessed = true; // we received some data        

        if (state.code != IF_READY) setStateConnected();  // in case we were not yet in the proper state, we set it
        connectAttemptCount = 0;        // we got some data so reset the counter

        process();        // method to process the incoming data       
        
      }
           
      if (!dataProcessed) {
        uint32_t now = millis();

        if (now - destination.lastRxTime > OSC_PING_DELAY &&
            now - destination.lastPingTx > OSC_PING_DELAY) {    // we only ping if we didn't receive data within the delay

          if (connectAttemptCount >= 2) {
            if (state.label != IF_STATE_PENDING) setStateQueryingDevice();
            connectAttemptCount = 0;
          } else {
            if (pulseCheck()) connectAttemptCount++;
            else {
              if (handleConnectService()){
                setStateQueryingDevice();                                 // we are not yet connected to the OSC device, we will try by sending a ping
                destination.lastPingTx = 0;                               // with the valid UDP we just got so we force ping to be triggered immediately                
              }else{
                setStateError("Can't connect (UDP failed)");
              }
            }
          }
        }
      }

      // sending
      OSCMessage msgOut;
      if (queue.dequeue(msgOut, destination.latency)) {   // here we dequeue only one message at once, we could try to dequeue 2 using a for loop
        dataProcessed |= _sendOSCMessage(&msgOut);        
      }

      return dataProcessed;
    }

    
    /**
     * @brief Queue an OSC command, or transmit it immediately when requested.
     *
     * Normal commands use the paced outgoing queue; presence checks can bypass it.
     * @return Whether queue insertion or immediate transmission succeeded.
     */
    bool send(OSCMessage& msg, bool immediate = false){
      
      if (immediate) {
        return _sendOSCMessage(&msg);
      } else {
        return queue.enqueue(msg);
      }
    }

    /**
     * @brief Dispatch the current incoming OSC message to registered listeners.
     *
     * evtOSC borrows oscIn. Listeners must consume the message during this call or
     * copy what they need before the next received packet replaces oscIn.
     * Concrete mixer backends override this method to route protocol addresses.
     */
    virtual void process(){
      //by default, we simply propagate the data to whoever has register as event listener
      // the event OSC which is throw hold a pointer to the OPSCin data which has just arrived
      fireEvent(evtOSC);  
    }    

    char addr[64];    
    
    /** @brief Read and decode incoming UDP OSC traffic. */
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
              oscIn.getAddress(addr, 0);

              destination.lastRxTime = millis();

              #ifdef DEBUG
                dump(oscIn, oscDump, sizeof(oscDump));              
                DEBUG_INFO("<<(%s,%i) %s %s",destination.ip.toString().c_str(),remotePort, addr, oscDump);              
              #endif

              // see if it's the reply to a _target (/ping) command => we call internal method to trigger latency calculation and user processing
              if (destination.echoCommand == addr) pulseBeat(&oscIn);
             
            }              
            // ✔ message valide
            // retour : au moins 1, même si aucun argument
            int argCount = oscIn.size();
            return (argCount == 0 ? 1 : argCount);
        }
    
        return 0;
    }


    /** @brief Record the remote reply used for reachability monitoring. */
    virtual void echo(void* data) override{
      UDPIP = udp->remoteIP();
      UDPPort = udp->remotePort();      
    }

    #ifdef DEBUG
      char oscDump[256];
    #endif

    /** @brief Print message contents for diagnostics. */
    char* dump(OSCMessage &msg, char* buffer, size_t bufferSize) {
      if (bufferSize == 0) return buffer;
      buffer[0] = '\0';

      size_t offset = 0;
      for (int i = 0; i < msg.size(); i++) {
        if (offset >= bufferSize) break;

        if (msg.isInt(i)) {
          offset += snprintf(buffer + offset, bufferSize - offset, "%d ", msg.getInt(i));
        } else if (msg.isFloat(i)) {
          offset += snprintf(buffer + offset, bufferSize - offset, "%f ", msg.getFloat(i));
        } else if (msg.isString(i)) {
          char txt[128];
          if (msg.getString(i, txt, sizeof(txt)) > 0) {
            offset += snprintf(buffer + offset, bufferSize - offset, "%s ", txt);
          }
        } else if (msg.isBoolean(i)) {
          offset += snprintf(buffer + offset, bufferSize - offset, "%s ", msg.getBoolean(i) ? "true" : "false");
        } else if (msg.isDouble(i)) {
          offset += snprintf(buffer + offset, bufferSize - offset, "%lf ", msg.getDouble(i));
        } else if (msg.isBlob(i)) {
          offset += snprintf(buffer + offset, bufferSize - offset, "[blob:%d] ", msg.getBlobLength(i));
        } else {
          offset += snprintf(buffer + offset, bufferSize - offset, "[type:%c] ", msg.getType(i));
        }
      }

      return buffer;
    }

    /** @brief Stop the active operation or connection. */
    virtual bool stop() override{          // disconnect the network for network interface
      DEBUG_INFO("%s::stop()",interfaceName.c_str());
      return handleDisconnectService();
    };  

    /**
     * @brief Allow this OSC interface to be enabled or disabled by the user.
     *
     * @return Always true.
     */
    virtual bool isSwitchable(){
      return true;
    }    

    /** @brief Start this service on the available network connection. */
    virtual bool handleConnectService() override{
      DEBUG_INFO("%s::handleConnectService()",interfaceName.c_str());
      // network connection is established so we now try to connect the device to its protocol
      if (udpID == -1) {
        udpID = hwm.network->requestUDPslotID(localPort);   // the network will create or reuse a udp slot and give its ID to us
      }
      if (udpID == -1) return false;                            // can't get any UDP connectivity => try again until next queryTime expired
      
      DEBUG_INFO("udp(%i) got UDP slot #%i",localPort,udpID);      
      return true;
    }

    /** @brief Close this service and release its transport resources. */
    virtual bool handleDisconnectService() override{
      DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
      destination.lastPingTx = 0;                                // force immediate ping attempt
      if (udpID != -1) hwm.network->releaseUDPSlot(&udpID);          // release any valid UDP slot we would have acquired 
      queue.clear();      
      return true;
    }    
  
    /** @brief Set the interface status to querying the device. */
    void setStateQueryingDevice(){
      //DEBUG_PRINT("\n%s: setStateQueryingDevice()",interfaceName.c_str());
      // network is connected but not the OSC device is not answering
      DeviceInterface::setStatePending();
      state.subState = destination.ip.toString();      
      state.progress = -1;      
    }

    /** @brief Request a presence check for the remote device. */
    virtual bool ping() override {
      OSCMessage msg(destination.echoCommand.c_str());      
      destination.lastPingTx = millis();
      return _sendOSCMessage(&msg);                     // no enqueing , direct send for the ping
    }    


    /** @brief Update the quality-of-service indicator used by the display. */
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

    /** @brief Encode and transmit one OSC message to the configured endpoint. */
    bool _sendOSCMessage(OSCMessage *msg){           
      if (msg == nullptr) {
        DEBUG_ERROR(" **** FAIL (msg null) ****");
        return false;
      }

      if (msg->hasError()) {
        DEBUG_ERROR(" **** FAIL (OSC msg invalid) ****");
        return false;
      }

      bool success = false;

      #ifdef DEBUG
        msg->getAddress(addr, 0);      
        dump(*msg, oscDump, sizeof(oscDump));              
        DEBUG_INFO(">>(%s,%i) %s %s",destination.ip.toString().c_str(),remotePort, addr, oscDump);            
      #endif
    

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



};

#endif
