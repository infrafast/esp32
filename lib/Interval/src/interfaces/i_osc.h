#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"
#include "OSCMessage.h"

// ============================================================
// Config OSC queue
// ============================================================

#define OSC_QUEUE_SIZE 10                                       // can send virtually 10 maximum message at once, which is the amount for a full config when  a strip refresh
#define OSC_QUEUE_SLOT_SIZE 512
#define OSC_MESSAGE_PROCESSED_AT_ONCE 3                        // 3 

#if OSC_MESSAGE_PROCESSED_AT_ONCE > OSC_QUEUE_SIZE
  //#error "OSC_MESSAGE_PROCESSED_AT_ONCE must not exceed OSC_QUEUE_SIZE"
#endif

#define OSC_PING_DELAY 3000

#define OSC_RX_TASK_STACK_SIZE 4096
#define OSC_RX_TASK_PRIORITY 5                                  // 5 is good
#define OSC_RX_TASK_CORE 0

#define OSC_QUEUE_WARN_THRESHOLD_PERCENT 75
size_t highWatermark = (OSC_QUEUE_SIZE * OSC_QUEUE_WARN_THRESHOLD_PERCENT) / 100;
bool warnedHighWatermark = false;

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

// ============================================================
// Helpers de sérialisation OSC
// ============================================================

class MemoryPrint : public Print {
public:
  MemoryPrint(uint8_t* buf, size_t maxLen)
    : _buf(buf), _maxLen(maxLen), _len(0), _overflow(false) {}

  size_t write(uint8_t b) override {
    if (_len >= _maxLen) {
      _overflow = true;
      return 0;
    }

    _buf[_len++] = b;
    return 1;
  }

  size_t length() const {
    return _len;
  }

  bool overflow() const {
    return _overflow;
  }

private:
  uint8_t* _buf;
  size_t _maxLen;
  size_t _len;
  bool _overflow;
};

class CountingPrint : public Print {
public:
  CountingPrint() : _len(0) {}

  size_t write(uint8_t) override {
    ++_len;
    return 1;
  }

  size_t length() const {
    return _len;
  }

private:
  size_t _len;
};

// ============================================================
// Ring buffer OSC statique, thread-safe, sans malloc
// ============================================================

class OscMessageQueue {
public:
  OscMessageQueue()
    : head(0),
      tail(0),
      count(0),
      droppedCount(0),
      tooLargeCount(0),
      lastSendTime(0) {
    mutex = xSemaphoreCreateMutexStatic(&mutexBuffer);
  }

  bool enqueue(OSCMessage& msg, bool rejectDuplicate = true, bool dropOldestIfFull = true) {
    if (msg.hasError()) return false;

    CountingPrint counter;
    msg.send(counter);

    size_t serializedLen = counter.length();

    if (serializedLen == 0) return false;

    if (serializedLen > OSC_QUEUE_SLOT_SIZE) {
      ++tooLargeCount;
      return false;
    }

    uint8_t temp[OSC_QUEUE_SLOT_SIZE];

    MemoryPrint mp(temp, OSC_QUEUE_SLOT_SIZE);
    msg.send(mp);

    if (mp.overflow() || mp.length() != serializedLen) {
      ++tooLargeCount;
      return false;
    }

    if (!lock()) return false;

    if (rejectDuplicate) {
      for (uint8_t n = 0; n < count; ++n) {
        uint8_t idx = (head + n) % OSC_QUEUE_SIZE;

        if (len[idx] == serializedLen &&
            memcmp(data[idx], temp, serializedLen) == 0) {
          unlock();
          return false;
        }
      }
    }

    if (count >= OSC_QUEUE_SIZE) {
      if (!dropOldestIfFull) {
        ++droppedCount;
        unlock();
        return false;
      }

      head = (head + 1) % OSC_QUEUE_SIZE;
      --count;
      ++droppedCount;
    }

    memcpy(data[tail], temp, serializedLen);
    len[tail] = serializedLen;

    tail = (tail + 1) % OSC_QUEUE_SIZE;
    ++count;

    if (count >= highWatermark && !warnedHighWatermark) {
      warnedHighWatermark = true;
      DEBUG_WARN("OSC queue near overflow (%d/%d)", count, OSC_QUEUE_SIZE);
    }    

    if (count >= OSC_QUEUE_SIZE) {
      ++droppedCount;
      DEBUG_ERROR("OSC queue overflow → dropping oldest (dropped=%d)", droppedCount);
    }

    unlock();
    return true;
  }

  bool dequeueNow(OSCMessage& outMsg) {
    uint8_t temp[OSC_QUEUE_SLOT_SIZE];
    size_t tempLen = 0;

    if (!lock()) return false;

    if (count == 0) {
      unlock();
      return false;
    }

    tempLen = len[head];
    memcpy(temp, data[head], tempLen);

    head = (head + 1) % OSC_QUEUE_SIZE;
    --count;

    if (count < highWatermark) {
      warnedHighWatermark = false;
  }    

    unlock();

    outMsg.empty();

    for (size_t i = 0; i < tempLen; ++i) {
      outMsg.fill(temp[i]);
    }

    bool ok = !outMsg.hasError();

    if (!ok) outMsg.empty();

    return ok;
  }

  bool dequeue(OSCMessage& outMsg, int delayBetweenTwoSend) {
    unsigned long now = millis();

    if ((unsigned long)(now - lastSendTime) < (unsigned long)delayBetweenTwoSend) {
      return false;
    }

    if (!dequeueNow(outMsg)) return false;

    lastSendTime = now;
    return true;
  }

  bool isEmpty() const {
    if (!lock()) return true;

    bool empty = (count == 0);

    unlock();
    return empty;
  }

  size_t size() const {
    if (!lock()) return 0;

    size_t s = count;

    unlock();
    return s;
  }

  size_t capacity() const {
    return OSC_QUEUE_SIZE;
  }

  size_t freeSlots() const {
    if (!lock()) return 0;

    size_t freeCount = OSC_QUEUE_SIZE - count;

    unlock();
    return freeCount;
  }

  size_t dropped() const {
    return droppedCount;
  }

  size_t tooLarge() const {
    return tooLargeCount;
  }

  void clear() {
    if (!lock()) return;

    head = 0;
    tail = 0;
    count = 0;

    unlock();
  }

private:
  uint8_t data[OSC_QUEUE_SIZE][OSC_QUEUE_SLOT_SIZE];
  size_t len[OSC_QUEUE_SIZE];

  uint8_t head;
  uint8_t tail;
  uint8_t count;

  volatile size_t droppedCount;
  volatile size_t tooLargeCount;

  unsigned long lastSendTime;

  StaticSemaphore_t mutexBuffer;
  SemaphoreHandle_t mutex = nullptr;

  bool lock() const {
    if (mutex == nullptr) return false;
    return xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE;
  }

  void unlock() const {
    if (mutex != nullptr) xSemaphoreGive(mutex);
  }
};

#define GROUP_INTERFACE_OSC "OSC"

class NetworkOSC: public NetworkDevice {

public:
  OSCMessage oscIn;
  OSCMessage oscOut;

  OscMessageQueue txQueue;
  OscMessageQueue rxQueue;

  Event evtOSC = Event(&oscIn);

  IPAddress UDPIP;
  int UDPPort;
  UDP* udp = nullptr;

  NetworkOSC(
    String name,
    int _OSC_localPort,
    String _OSC_IPaddress,
    int _OSC_remotePort,
    String echoCommand,
    bool _configurable = false,
    String group = GROUP_INTERFACE_OSC
  ) : NetworkDevice(
        name,
        group,
        _OSC_localPort,
        _OSC_remotePort,
        echoCommand,
        _configurable
      ) {
    icon = "g";

    if (!destination.ip.fromString(_OSC_IPaddress)) {
      destination.ip = IPAddress(0, 0, 0, 0);
    }

    registerDataSetAddress(&addressPing);

    evtOSC.type = EVENT_OSC_MESSAGE;
  }

  ~NetworkOSC() {
    stopRxTask();
  }

  virtual bool begin() {
    DEBUG_INFO("%s::begin() => nothing to do", interfaceName.c_str());
    return true;
  }

  virtual bool end() {
    DEBUG_INFO("%s::end() calling stop()", interfaceName.c_str());
    return stop();
  }

  virtual void serializeProperties(JsonArray& properties) override {
    NetworkDevice::serializeProperties(properties);

    buf = (char*)malloc((strlen(ADDRESS_PING) + destination.echoCommand.length()) * sizeof(char*) + 1);
    sprintf(buf, ADDRESS_PING, destination.echoCommand);
    properties.add(String(buf));
    free(buf);
  }

  virtual bool restoreProperty(const char* property) override {
    bool changed = false;

    #define MAXLENGHTINPUT 32
    char inputStr[MAXLENGHTINPUT];

    if (sscanf(property, ADDRESS_PING, &inputStr)) {
      if (destination.echoCommand != inputStr) changed = true;
      destination.echoCommand = inputStr;
    }

    return changed | NetworkDevice::restoreProperty(property);
  }

  void startRxTask() {
    if (rxTaskHandle != nullptr) return;

    rxTaskRunning = true;

    BaseType_t result = xTaskCreatePinnedToCore(
      NetworkOSC::rxTaskThunk,
      "OSC_RX",
      OSC_RX_TASK_STACK_SIZE,
      this,
      OSC_RX_TASK_PRIORITY,
      &rxTaskHandle,
      OSC_RX_TASK_CORE
    );

    if (result != pdPASS) {
      rxTaskHandle = nullptr;
      rxTaskRunning = false;
      DEBUG_ERROR("OSC_RX task creation failed");
    }
  }

  void stopRxTask() {
    rxTaskRunning = false;

    if (rxTaskHandle != nullptr) {
      TaskHandle_t task = rxTaskHandle;
      rxTaskHandle = nullptr;
      vTaskDelete(task);
    }
  }

  static void rxTaskThunk(void* param) {
    NetworkOSC* self = static_cast<NetworkOSC*>(param);

    if (self != nullptr) {
      self->rxTaskLoop();
    }

    vTaskDelete(nullptr);
  }

  void rxTaskLoop() {
    OSCMessage rxMsg;

    while (rxTaskRunning) {
      if (udpID == -1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }

      int8_t r = readOSC(rxMsg);

      if (r > 0) {
        rxQueue.enqueue(rxMsg, false, true);
        rxMsg.empty();

        if (state.code != IF_READY) {
          setStateConnected();
        }

        connectAttemptCount = 0;
      } else {
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    }
  }

  virtual bool monitorService() override {
    dataProcessed = false;

    // queue is fed by the rxTask, so we just need to dequeue and process messages here
    uint8_t nFrames = 0;
    for (; nFrames < OSC_MESSAGE_PROCESSED_AT_ONCE; ++nFrames) {
      if (!rxQueue.dequeueNow(oscIn)) break;

      dataProcessed = true;

      // did we receive a response to a ping command?
      oscIn.getAddress(addr, 0);
      if (destination.echoCommand == addr) {
        pulseBeat(&oscIn);
      }

      process();
    }

    if (nFrames > 0) {
      DEBUG_INFO("%s::monitorService() processed %d OSC message(s)", interfaceName.c_str(), nFrames);
    }
  
    if (!dataProcessed) {
      uint32_t now = millis();

      if (now - destination.lastRxTime > OSC_PING_DELAY &&
          now - destination.lastPingTx > OSC_PING_DELAY) {

        if (connectAttemptCount >= 2) {
          if (state.label != IF_STATE_PENDING) setStateQueryingDevice();
          connectAttemptCount = 0;
        } else {
          if (pulseCheck()) {
            connectAttemptCount++;
          } else {
            if (handleConnectService()) {
              setStateQueryingDevice();
              destination.lastPingTx = 0;
            } else {
              setStateError("Can't connect (UDP failed)");
            }
          }
        }
      }
    }

    OSCMessage msgOut;

    for (uint8_t i = 0; i < OSC_MESSAGE_PROCESSED_AT_ONCE; ++i) {
      if (!txQueue.dequeue(msgOut, destination.latency / 2)) break;

      dataProcessed |= _sendOSCMessage(&msgOut);
      msgOut.empty();
    }

    return dataProcessed;
  }

  bool send(OSCMessage& msg, bool immediate = false) {
    if (immediate) return _sendOSCMessage(&msg);
    return txQueue.enqueue(msg, true, false);
  }

  virtual void process() {
    fireEvent(evtOSC);
  }

  char addr[64];

  int readOSC(OSCMessage& targetMsg) {
    UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);
    udp = nullptr;

    if (udpSlot == NULL) return -1;

    udp = udpSlot->udp;
    if (udp == nullptr) return -1;

    int size = udp->parsePacket();

    if (size > 0) {
      targetMsg.empty();

      while (size--) {
        targetMsg.fill(udp->read());
        delay(0);
      }

      if (targetMsg.hasError()) {
        DEBUG_INFO("readOSC: %i\n", targetMsg.getError());
        targetMsg.empty();
        return 0;
      }

      char localAddr[64];
      targetMsg.getAddress(localAddr, 0);

      destination.lastRxTime = millis();

      UDPIP = udp->remoteIP();
      UDPPort = udp->remotePort();

      #ifdef DEBUG
        dump(targetMsg, oscDump, sizeof(oscDump));
        DEBUG_INFO("<<(%s,%i) %s %s",
                   destination.ip.toString().c_str(),
                   remotePort,
                   localAddr,
                   oscDump);
      #endif

      int argCount = targetMsg.size();
      return (argCount == 0 ? 1 : argCount);
    }

    return 0;
  }

  virtual void echo(void* data) override {
    // Les infos remote sont maintenant capturées dans readOSC()
    // pour éviter de dépendre du pointeur udp depuis un autre contexte.
  }

  #ifdef DEBUG
    char oscDump[256];
  #endif

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

  virtual bool stop() override {
    DEBUG_INFO("%s::stop()", interfaceName.c_str());
    return handleDisconnectService();
  }

  virtual bool isSwitchable() {
    return true;
  }

  virtual bool handleConnectService() override {
    DEBUG_INFO("%s::handleConnectService()", interfaceName.c_str());

    if (udpID == -1) {
      udpID = hwm.network->requestUDPslotID(localPort);
    }

    if (udpID == -1) return false;

    DEBUG_INFO("udp(%i) got UDP slot #%i", localPort, udpID);

    startRxTask();

    return true;
  }

  virtual bool handleDisconnectService() override {
    DEBUG_INFO("%s::handleDisconnectService()", interfaceName.c_str());

    stopRxTask();

    destination.lastPingTx = 0;

    if (udpID != -1) {
      hwm.network->releaseUDPSlot(&udpID);
    }

    txQueue.clear();
    rxQueue.clear();

    return true;
  }

  void setStateQueryingDevice() {
    DeviceInterface::setStatePending();
    state.subState = destination.ip.toString();
    state.progress = -1;
  }

  virtual bool ping() override {
    OSCMessage msg(destination.echoCommand.c_str());
    destination.lastPingTx = millis();

    return _sendOSCMessage(&msg);
  }

  virtual void determineQoS() override {
    if (state.code != IF_READY) {
      state.QoSlevel = QoS::UNDETERMINED;
    } else if (destination.latency <= 50 && destination.jitter <= 10) {
      state.QoSlevel = QoS::GOOD;
    } else if (destination.latency <= 100 && destination.jitter <= 20) {
      state.QoSlevel = QoS::MEDIUM;
    } else if (destination.latency <= 200 && destination.jitter <= 40) {
      state.QoSlevel = QoS::POOR;
    } else {
      state.QoSlevel = QoS::CRITICAL;
    }
  }

  bool _sendOSCMessage(OSCMessage *msg) {
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
      DEBUG_INFO(">>(%s,%i) %s %s",
                 destination.ip.toString().c_str(),
                 remotePort,
                 addr,
                 oscDump);
    #endif

    UdpSlot* udpSlot = hwm.network->useUDPslot(udpID);

    if (udpSlot == nullptr) {
      DEBUG_ERROR(" **** FAIL (UdpSlot null) ****");
      return false;
    }

    UDP* udp = udpSlot->udp;

    if (udp != nullptr) {
      udp->beginPacket(destination.ip, remotePort);
      msg->send(*udp);
      udp->endPacket();
      success = true;
    } else {
      DEBUG_ERROR(" **** FAIL (udp null) ****");
    }

    return success;
  }

private:
  uint8_t connectAttemptCount = 0;
  bool udpConnectionLost = false;
  int udpID = -1;

  TaskHandle_t rxTaskHandle = nullptr;
  volatile bool rxTaskRunning = false;
};

#endif