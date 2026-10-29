/**
 * @file HWComponents.h
 * @brief Hardware inputs, battery monitoring and network connections.
 */

#ifndef HWComponents_h
#define HWComponents_h

#include "Interval.h"

#include "iconMap.h"

#include <Ethernet.h>       // apply following fix: https://github.com/arduino-libraries/Ethernet/pull/107/files
#include <EthernetUdp.h>
#include <WiFiGeneric.cpp>  // for tcpipinit
#include <ArduinoOTA.h>     // for WIFI_MEDIA program upload 
#include <ESP32Ping.h>      // for WiFi

#include <HttpClient.h> // amcewen/HttpClient@^2.2.0 as opposed to the std lib #include <HTTPClient.h> which only handle WiFIclient
#include <WiFiClientSecure.h>
#include "BluetoothSerial.h"   //required for OSC
#include <ESPmDNS.h>          // for WIFI registration
#include <EthernetBonjour.h>  //for Eth registration

#include "utility/w5100.h"    // for readSnSR

#include <WebSocketClient.hpp>    // https://github.com/skaarj1989/mWebSockets#meta     // use a local repository with SOCKETBUFFER SIZE CHANGED one because of https://github.com/skaarj1989/mWebSockets/issues/72
//#include <WebSocket.hpp>
using namespace net;
using WiFiWebSocketClient = WebSocketClient<WiFiClient>;
using EthernetWebSocketClient = WebSocketClient<EthernetClient>;

/**
 * @brief Socket event kind and associated text.
 */
struct SocketData{
  uint16_t length;
  const char* message="";
  int code;
  unsigned long receivedAt = 0;
};

///////////////////////////////////////////////////////////
/** @brief Return a readable Wi-Fi connection status for diagnostics. */
const char* wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_NO_SHIELD:        return "No shield";
    case WL_IDLE_STATUS:      return "Idle";
    case WL_NO_SSID_AVAIL:    return "No SSID available";
    case WL_SCAN_COMPLETED:   return "Scan completed";
    case WL_CONNECTED:        return "Connected";
    case WL_CONNECT_FAILED:   return "Connect failed";
    case WL_CONNECTION_LOST:  return "Connection lost";
    case WL_DISCONNECTED:     return "Disconnected";
    default:                  return "Unknown";
  }
}
/////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
void onEthernetBonjourServiceFoundCallback(const char* serviceType,
                           MDNSServiceProtocol_t proto,
                           const char* serviceName,
                           const byte ipAddr[4],
                           unsigned short port,
                           const char* txt);


///////////////////////////////////////////////////////////////////
//                MIDI LAYER OBJECTS DECLARATION            ///////
///////////////////////////////////////////////////////////////////
#include "interfaces/appleMidiHelpers.h"
#define MIDI_CHANNEL_OMNI 0

#define APPLE_MIDI_PORT DEFAULT_CONTROL_PORT
#define MDNS_ANNOUNCED_APPLEMIDI_SERVICE "apple-midi"   // standard name for apple midi. Do not change

using namespace APPLEMIDI_NAMESPACE;



/**
 * @brief Run one reachability check, delete its parameters and end the worker task.
 */
void pingTask(void*);
/**
 * @brief Run a configured HTTP request under the network mutex.
 *
 * Build the API reply from HTTP status/body and invoke the request callback.
 * The operation runs in a task so callers need not wait in app.refresh().
 */
void connectHTTPTask(void*);
/**
 * @brief Hardware component with initialization, periodic refresh and event dispatch.
 */

class HWComponent: public GenericComponent, public EventDispatcher{
  public:
    uint8_t CSPin;
  
    /** @brief Return the name used to identify this event source in logs. */
    virtual const char* getDispatcherName() const override{
      return id.c_str();
    }

    /** @brief Initialize HWComponent with the supplied configuration and bindings. */
    HWComponent(uint8_t pin, boolean d=true):GenericComponent(d){
      CSPin = pin;
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void stop(){
      //DEBUG_PRINT("\n%s stop()",id.c_str());
    }
};

/**
 * @brief Hardware component exposing a two-state value.
 */
class HWComponent2State : public HWComponent{
  public: 
    bool state;
    bool previousState;

    /** @brief Initialize HWComponent2State with the supplied configuration and bindings. */
    HWComponent2State(uint8_t pin, bool s = false, boolean d=false):HWComponent(pin,d){      
      state = s;
      previousState = state;
    }

    /** @brief Refresh this component and report whether its displayed state changed. */
    bool refresh(bool force=false){
      refreshed = false;  // reset previous state
      if (previousState != state || force){
        previousState = state;
        if (debug){
          DEBUG_INFO("%s changed state to: %i",id.c_str(),state);
        }
        refreshed = true;
      }
      return refreshed;
    }

};

/**
 * @brief Drives a digital output pin.
 */
class DigitalOut : public HWComponent2State{
  public:

    /** @brief Initialize DigitalOut with the supplied configuration and bindings. */
    DigitalOut(uint8_t p,bool s = HIGH): HWComponent2State(p,s){
      pinMode(CSPin, OUTPUT);
      id = concatStrings("DigitalOut~", p);
      refresh(true);  //force the state
    }

    /** @brief Refresh this component and report whether its displayed state changed. */
    bool refresh(bool force=false){
      HWComponent2State::refresh(force);
      if (refreshed){
        digitalWrite(CSPin, state);
      }
      return refreshed;
    }

    /** @brief Prepare this object for use with its current configuration. */
    virtual void init(){
      HWComponent2State::init();
    }

};

/**
 * @brief Samples an analog input and tracks its measured value.
 */
class AnalogIn : public HWComponent{
  private:
    uint16_t refreshPercentageThreshold = 1;  ///< if value change of 1% it will be considered as change
  
  public:
    uint16_t value=0;

    /** @brief Initialize AnalogIn with the supplied configuration and bindings. */
    AnalogIn(uint8_t p, uint16_t t=0, boolean d=true): HWComponent(p,d){
      refreshPercentageThreshold = t;
      pinMode(CSPin, INPUT);
      id = concatStrings("AnalogIn~", p);
    }

    /** @brief Refresh this component and report whether its displayed state changed. */
    bool refresh(bool force=false){
      refreshed = force;
      uint16_t v = analogRead(CSPin);

      uint16_t deltaPercentage = 0;
      if (value!=0) deltaPercentage = abs(1.0-((float)v/(float)value))*100;
      else deltaPercentage = abs(1.0-((float)v/1.0))*100; // avoid divide per zero
      
      if (deltaPercentage >= refreshPercentageThreshold || force){
        if (debug){
          //DEBUG_PRINT("\n%s (%d->%d)",id.c_str(),value,v);
        }
        value = v;        
        if (debug){
          //DEBUG_PRINT("\tchanged to: %u (%d>%d)",value,deltaPercentage,refreshPercentageThreshold);
        }
        refreshed = true;
        
      }
      
      return refreshed;
    }

    /** @brief Prepare this object for use with its current configuration. */
    virtual void init(){
      HWComponent::init();
      refresh();
    }

};

#define VBAT_CHANGE_THRESHOLD 0.1

#define VBAT_CRIT 3.2     // selon chat gpt => tension minimal. En dessous de 3.0v , dommages irreversibles
#define VBAT_MIN 3.3      // selon chat gpt => tension a surveiller, seuil de pré-alarme
#define VBAT_WARNING VBAT_MIN-VBAT_CHANGE_THRESHOLD
#define VBAT_MAX 4.2      // selon chat gpt => tension maximale
#define VBAT_USB 4.6      // 

#define VBAT_OFFSET 0.0 // compensate voltage drop (positive or negative) from the diode or other component

#define BAT_REFRESH_RATE 1000 // refresh every second
#define BAT_NUM_READINGS 5


enum{
  BAT_CRITICAL_LEVEL,
  BAT_LOW_LEVEL,
  BAT_MID_LEVEL,
  BAT_FULL_LEVEL,
  BAT_MAX_LEVEL
};

enum BatteryState {
    BAT_CHECKING,  
    BAT_USB_PLUGGED,
    BAT_CHARGING,
    BAT_DISCHARGING,
};

/**
 * @brief Averages battery measurements and reports charge/power status.
 */
class BatteryMonitor : public HWComponent {
  // See:
  // https://forum.seeedstudio.com/t/xiao-ble-sense-mbed-2-7-2-battery-charge-and-voltage-monitor-analogread-p0-31-does-not-work/266438

public:
  Event evt;

  uint8_t indicator = BAT_FULL_LEVEL;
  float vBat = VBAT_MAX;
  float previousVBat = VBAT_MAX;
  BatteryState batteryState = BAT_CHECKING;

  /** @brief Initialize BatteryMonitor with the supplied configuration and bindings. */
  BatteryMonitor(uint8_t p, bool d = true) : HWComponent(p, d) {
    pinMode(CSPin, INPUT);
    id = concatStrings("BatteryMonitor~", p);
    evt.type = EVENT_BATTERY_REFRESHED;
  }

  /** @brief Prepare this object for use with its current configuration. */
  virtual void init() override {
    HWComponent::init();

    readIndex = 0;
    readSum = 0;
    initialized = false;
    lastReadMillis = 0;

    // Remplit le buffer une première fois pour stabiliser la moyenne
    uint32_t firstReading = analogReadMilliVolts(CSPin);
    for (int i = 0; i < BAT_NUM_READINGS; i++) {
      readings[i] = firstReading;
      readSum += firstReading;
    }

    float initialVBat = convertMilliVoltsToVBat(firstReading);
    vBat = initialVBat;
    previousVBat = initialVBat;
    indicator = computeIndicator(vBat);
    batteryState = determineBatteryState(vBat, previousVBat, VBAT_CHANGE_THRESHOLD / 2.0f);

    initialized = true;

    refreshed = true;
    fireEvent(evt);
  }

  /** @brief Refresh this component and report whether its displayed state changed. */
  virtual bool refresh(bool force = false) override {
    refreshed = false;

    if (!initialized) return false;

    unsigned long now = millis();
    if (!force && (now - lastReadMillis < BAT_REFRESH_RATE)) {
      return false;
    }
    lastReadMillis = now;

    // Retire l’ancienne valeur du buffer circulaire
    readSum -= readings[readIndex];

    // Lit la nouvelle valeur
    uint32_t rawReading = analogReadMilliVolts(CSPin);
    readings[readIndex] = rawReading;
    readSum += rawReading;

    // Passe à la case suivante
    readIndex = (readIndex + 1) % BAT_NUM_READINGS;

    // Moyenne glissante
    uint32_t averageReading = readSum / BAT_NUM_READINGS;
    float newVBat = convertMilliVoltsToVBat(averageReading);

    bool voltageChanged = false;
    bool indicatorChanged = false;
    bool stateChanged = false;

    // Sauvegarde l’ancienne tension avant mise à jour
    float oldVBat = vBat;
    uint8_t oldIndicator = indicator;
    BatteryState oldBatteryState = batteryState;

    float delta = fabs(newVBat - oldVBat);

    // Mise à jour tension seulement si variation suffisante ou force
    if (force || delta >= VBAT_CHANGE_THRESHOLD) {
      previousVBat = oldVBat;
      vBat = newVBat;
      voltageChanged = true;
    }

    // Même si la variation est faible, on peut utiliser la valeur récente
    // pour l'état si on veut une vision plus actuelle.
    float stateReferenceVBat = voltageChanged ? vBat : newVBat;
    float statePreviousVBat  = voltageChanged ? previousVBat : oldVBat;

    indicator = computeIndicator(stateReferenceVBat);
    if (indicator != oldIndicator) {
      indicatorChanged = true;
    }

    batteryState = determineBatteryState(stateReferenceVBat, statePreviousVBat, VBAT_CHANGE_THRESHOLD / 2.0f);
    if (batteryState != oldBatteryState) {
      stateChanged = true;
    }

    refreshed = force || voltageChanged || indicatorChanged || stateChanged;

    if (debug && refreshed) {
      //DEBUG_PRINT(
      //  "\n%s refreshed raw:%lu avg:%lu vBat:%.3f prev:%.3f delta:%.3f ind:%u state:%d",
      //  id.c_str(),
      //  rawReading,
      //  averageReading,
      //  vBat,
      //  previousVBat,
      //  delta,
      //  indicator,
      //  batteryState
      //);
    }

    if (refreshed) {
      fireEvent(evt);
    }

    return refreshed;
  }

private:
  uint32_t readings[BAT_NUM_READINGS] = {0};
  int readIndex = 0;
  uint32_t readSum = 0;
  unsigned long lastReadMillis = 0;
  bool initialized = false;

  /** @brief Convert the measured ADC millivolts to battery voltage. */
  float convertMilliVoltsToVBat(uint32_t milliVolts) {
    float val = 2.0f * milliVolts / 1000.0f;
    val += VBAT_OFFSET;
    return val;
  }

  /** @brief Map the battery voltage to the displayed charge indicator. */
  uint8_t computeIndicator(float voltage) {
    int level = mapFloatToInt(voltage, VBAT_MIN, VBAT_MAX, 0, BAT_MAX_LEVEL - 1);

    if (level < 0) level = 0;
    if (level > BAT_MAX_LEVEL - 1) level = BAT_MAX_LEVEL - 1;

    return (uint8_t)level;
  }

  /** @brief Choose the battery state from supply and voltage measurements. */
  BatteryState determineBatteryState(float currentVBat, float previousVBat, float threshold) {
    if (currentVBat >= VBAT_USB) {
      return BAT_USB_PLUGGED;
    }

    float delta = currentVBat - previousVBat;

    if (delta > threshold) {
      return BAT_CHARGING;
    }

    if (delta < -threshold) {
      return BAT_DISCHARGING;
    }

    // zone neutre : on conserve un état cohérent
    if (batteryState == BAT_USB_PLUGGED && currentVBat < VBAT_USB) {
      return BAT_DISCHARGING;
    }

    if (batteryState == BAT_CHECKING) {
      return BAT_DISCHARGING;
    }

    return batteryState;
  }
};


#define MIN_PUSH_DELAY    350   // must correspond to webapp Reactivity range
#define MAX_PUSH_DELAY    900   //

#define DEFAULT_PUSH_DELAY 500
#define DEFAULT_DCLIC_DELAY 250

// Generic debounce interval shared by button-related strip logic. The
// startup workflow has its own explicitly named delay in HWManager.h.
constexpr uint32_t BUTTON_DEBOUNCE_MS = 20;



#define DCLIC_DETECTION_PUSHDOWNMAXTIME LONG_PUSH_DELAY-1       // pour detecter un double clic, le premier clic doit etre court   200

// Structure pour les événements complexes
/**
 * @brief Button gesture, logical button id and associated duration.
 */
struct ButtonData {
  uint8_t id;         ///< type de clic : SIMPLE, DOUBLE, HOLD...
  uint8_t buttonId;   ///< identifiant logique du bouton
  unsigned long time; // durée / temps associé
};

/**
 * @brief Readable button gesture names for debugging.
 */
class ButtonEventNames {    // pour le debug
public:
  /**
   * @brief Return the debug label for a recognized button gesture id.
   */
  static const char* toString(uint8_t id) {
    switch (id) {
      case 1: return "BUTTON_SIMPLE_CLIC";
      case 2: return "BUTTON_DOUBLE_CLIC";
      case 3: return "BUTTON_LONG_CLIC";
      case 4: return "BUTTON_HOLD";
      case 5: return "BUTTON_RELEASE_HOLD";
      default: return "BUTTON_UNKNOWN";
    }
  }
};

/**
 * @brief Event carrying button gesture data.
 */
class ButtonEvent : public Event {
public:

  #ifdef DEBUG
  /** @brief Export this object using its JSON representation. */
  const char* serialize() const override {
    snprintf(_buffer, sizeof(_buffer),
             "buttonType=%s(%u), buttonId=%u, time=%lu",
             ButtonEventNames::toString(((ButtonData*)data)->id),
             ((ButtonData*)data)->id,
             ((ButtonData*)data)->buttonId,
             ((ButtonData*)data)->time);
    return _buffer;
  }
  #endif

private:

  mutable char _buffer[96] = {0};
};

enum EventID {
  BUTTON_SIMPLE_CLIC = 1,
  BUTTON_DOUBLE_CLIC,
  BUTTON_LONG_CLIC,
  BUTTON_ENTER_HOLD,  
  BUTTON_HOLD,
  BUTTON_RELEASE_HOLD,
};

/**
 * @brief Recognizes clicks, double clicks and holds from a sampled input.
 *
 * A shared task samples registered buttons every 5 ms instead of polling from
 * the main loop. The gesture state machine distinguishes debounce, delayed single
 * clicks, double clicks, hold entry/repeats and release. Timing values use milliseconds.
 */
class Button : public HWComponent2State {
public:
  ButtonEvent evtBtn;

  unsigned long pushedTime = 0;

  // Latched release information consumed by startup workflows such as
  // HardwareManager::wakeUp(). The shared button task may process the release
  // between two calls made by the application, so the transition and its
  // duration must survive until the consumer explicitly acknowledges it.
  volatile bool releasePending = false;
  volatile unsigned long releasedDuration = 0;

  // Accessed by the button sampling task and by the shutdown workflow.
  // Volatile makes the shutdown request visible across ESP32 cores.
  volatile bool usesLed = true;
  uint16_t push_reactivity = DEFAULT_PUSH_DELAY;

  uint16_t maxClickDuration = 180;
  uint16_t doubleClickGap = DEFAULT_DCLIC_DELAY;  

  ButtonData btData = {};

  /** @brief Initialize Button with the supplied configuration and bindings. */
  Button(uint8_t p,
         int e = LOW,
         boolean d = true,
         uint16_t push = DEFAULT_PUSH_DELAY,
         uint16_t dClic = DEFAULT_DCLIC_DELAY)
    : HWComponent2State(p, !e, d) {
    edge = e;
    push_reactivity = push;
    doubleClickGap = dClic;
    evtBtn.data = &btData;
  }

  /** @brief Prepare this object for use with its current configuration. */
  virtual void init() {
    HWComponent2State::init();
    processRefresh(true);
    registerButton();
    startGlobalRefreshTask();
  }

  /**
   * @brief Leave normal-loop polling idle; the shared task samples buttons.
   *
   * @return Always false in this implementation.
   */
  bool refresh(bool force = false) {
    (void)force;
    return false;
  }

  /**
   * @brief Consume the next debounced release transition, if any.
   *
   * Button sampling runs in a dedicated FreeRTOS task. This latch bridges that
   * asynchronous producer with synchronous workflows without making callers
   * duplicate GPIO reads or debounce logic.
   */
  bool consumeRelease(unsigned long& duration) {
    bool pending = false;
    portENTER_CRITICAL(&transitionMux);
    if (releasePending) {
      duration = releasedDuration;
      releasePending = false;
      pending = true;
    }
    portEXIT_CRITICAL(&transitionMux);
    return pending;
  }

  /** @brief Stop the active operation or connection. */
  virtual void stop() override {
    unregisterButton();
    stopGlobalRefreshTaskIfUnused();
    HWComponent2State::stop();
  }

  /** @brief Return the logical button identifier used in gesture events. */
  virtual int getID() = 0;
  /** @brief Assign the logical button identifier used in gesture events. */
  virtual void setID(uint8_t id) = 0;

protected:

  /** @brief Fill and dispatch the gesture event for this logical button. */
  void emitButtonEvent(uint8_t eventId, unsigned long eventTime) {
    btData.id = eventId;
    btData.buttonId = getID();
    btData.time = eventTime;
    fireEvent(evtBtn);   // remplace par fireEvent(&evt) si ton code n'a pas encore migré en référence
  }

private:
  // BUTTON_SAMPLE_MS = fréquence d’échantillonnage (polling)
  // BUTTON_DEBOUNCE_MS = temps de stabilité requis
  // => ici ~2 samples requis pour valider un changement
  static constexpr uint32_t BUTTON_SAMPLE_MS = 5;
  static constexpr uint32_t BUTTON_PRESS_DEBOUNCE_MS = 5; 
  static constexpr uint32_t BUTTON_RELEASE_DEBOUNCE_MS = 15;      //10 gagne en reactivité en restant tolerant au bounce


  int edge;
  bool holdEntered = false;

  unsigned long pendingClickDuration = 0;
  unsigned long firstClickReleasedAt = 0;
  bool waitingSecondClick = false;
  bool secondClickInProgress = false;

  unsigned long startPushTime = 0;
  
  static constexpr uint32_t HOLD_REPEAT_MS = 100;      ///< throttle the hold event to avoid too much events emission when holding the button. This value is independent of push_reactivity, and should be lower than it to avoid missing events when holding the button for a long time. Adjust according to your needs.
  unsigned long lastHoldEventTime = 0;
 
  static constexpr uint8_t MAX_BUTTONS = 5;

  portMUX_TYPE transitionMux = portMUX_INITIALIZER_UNLOCKED;

  static Button* registeredButtons[MAX_BUTTONS];
  static uint8_t registeredButtonCount;
  static TaskHandle_t globalRefreshTaskHandle;

  //debounce management
  bool rawState = false;
  bool lastRawState = false;
  bool debouncedState = false;
  unsigned long rawStateChangedAt = 0;  

  bool registered = false;

  /** @brief Add this button to the shared sampling registry if a slot is available. */
  void registerButton() {
    if (registered) return;

    if (registeredButtonCount >= MAX_BUTTONS) {
      DEBUG_ERROR("%s button registry full", id.c_str());
      return;
    }

    registeredButtons[registeredButtonCount++] = this;
    registered = true;
  }

  /** @brief Remove this button from the shared sampling registry. */
  void unregisterButton() {
    if (!registered) return;

    for (uint8_t i = 0; i < registeredButtonCount; i++) {
      if (registeredButtons[i] == this) {
        for (uint8_t j = i; j < registeredButtonCount - 1; j++) {
          registeredButtons[j] = registeredButtons[j + 1];
        }

        registeredButtons[registeredButtonCount - 1] = nullptr;
        registeredButtonCount--;
        registered = false;
        return;
      }
    }
  }

  /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
  static void globalRefreshTaskEntry(void* pvParameters) {
    (void)pvParameters;

    while (true) {
      for (uint8_t i = 0; i < registeredButtonCount; i++) {
        Button* btn = registeredButtons[i];

        if (btn != nullptr) {
          btn->processRefresh(false);
        }
      }

      vTaskDelay(pdMS_TO_TICKS(BUTTON_SAMPLE_MS));
    }
  }

  /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
  static void startGlobalRefreshTask() {
    if (globalRefreshTaskHandle != nullptr) {
      return;
    }

    BaseType_t ok = xTaskCreatePinnedToCore(
      &Button::globalRefreshTaskEntry,
      "btnRefreshAll",
      4096,
      nullptr,
      3,
      &globalRefreshTaskHandle,
      1
    );

    if (ok != pdPASS) {
      globalRefreshTaskHandle = nullptr;
      DEBUG_ERROR("global button refresh task creation failed");
    }
  }

  /** @brief Delete the shared sampler when no buttons remain registered. */
  static void stopGlobalRefreshTaskIfUnused() {
    if (registeredButtonCount > 0) return;

    if (globalRefreshTaskHandle != nullptr) {
      vTaskDelete(globalRefreshTaskHandle);
      globalRefreshTaskHandle = nullptr;
    }
  }

  /**
   * @brief Debounce sampled input and emit click/hold gestures.
   *
   * Use separate press/release stability delays. A short release waits through the
   * double-click gap before emitting a single click; a second short press confirms
   * a double click. A sustained press emits hold-entry once, repeats hold events
   * every HOLD_REPEAT_MS, then emits hold-release. Durations are in milliseconds.
   */
  bool processRefresh(bool force = false) {
    refreshed = force;

    unsigned long now = millis();

    read();
    rawState = state;

    if (force) {
      lastRawState = rawState;
      debouncedState = rawState;
      rawStateChangedAt = now;
      state = debouncedState;
      previousState = state;
      refreshed = true;
    } else {
      if (rawState != lastRawState) {
        lastRawState = rawState;
        rawStateChangedAt = now;
      }

      uint32_t debounceDelay = (rawState == edge)
      ? BUTTON_PRESS_DEBOUNCE_MS
      : BUTTON_RELEASE_DEBOUNCE_MS;

      if ((now - rawStateChangedAt) >= debounceDelay && debouncedState != rawState) {
        debouncedState = rawState;
        state = debouncedState;
        HWComponent2State::refresh(false);
      } else {
        state = debouncedState;
        refreshed = false;
      }
    }

    if (refreshed) {
      btData.buttonId = getID();

      if (state != edge) { // released
        unsigned long clickDuration = millis() - startPushTime;
        pushedTime = clickDuration;

        // Preserve a real debounced release before the normal click/hold
        // state machine clears transient timing data. The forced initial
        // sample in init() is not a user release and must not be latched.
        if (!force) {
          portENTER_CRITICAL(&transitionMux);
          releasedDuration = clickDuration;
          releasePending = true;
          portEXIT_CRITICAL(&transitionMux);
        }

        if (holdEntered) {
          emitButtonEvent(BUTTON_RELEASE_HOLD, pushedTime - push_reactivity);
          waitingSecondClick = false;
          secondClickInProgress = false;
        }
        else if (secondClickInProgress) {
          secondClickInProgress = false;

          if (clickDuration <= maxClickDuration) {
            emitButtonEvent(BUTTON_DOUBLE_CLIC, millis() - firstClickReleasedAt);
            waitingSecondClick = false;
          } else {
            // 2e appui trop long : on valide finalement le 1er clic simple
            emitButtonEvent(BUTTON_SIMPLE_CLIC, pendingClickDuration);
            waitingSecondClick = false;
          }
        }
        else if (clickDuration <= maxClickDuration) {
          pendingClickDuration = clickDuration;
          firstClickReleasedAt = millis();
          waitingSecondClick = true;
        }
        else if (clickDuration > push_reactivity / 3) {
          emitButtonEvent(BUTTON_LONG_CLIC, clickDuration);
          waitingSecondClick = false;
        }
        else {
          emitButtonEvent(BUTTON_SIMPLE_CLIC, clickDuration);
          waitingSecondClick = false;
        }

        if (usesLed) {
          ledcWrite(0, 255);
        }

      } else { // pressed
        holdEntered = false;
        lastHoldEventTime = 0;
        startPushTime = millis();

        if (waitingSecondClick && (millis() - firstClickReleasedAt <= doubleClickGap)) {
          secondClickInProgress = true;
        } else {
          waitingSecondClick = false;
          secondClickInProgress = false;
        }
      }

    } else {
      if (state == edge) { // still pressed
        pushedTime = millis() - startPushTime;
        const bool holdThresholdReached = (pushedTime >= push_reactivity);

        if (!waitingSecondClick && usesLed) {
          if (holdThresholdReached) {
            ledcWrite(0, 255);
          } else {
            uint8_t dutyCycle = map(pushedTime, 0, push_reactivity, 0, 255);
            ledcWrite(0, dutyCycle);
          }
        }

        if (holdThresholdReached) {
          if (!holdEntered) {
            holdEntered = true;
            emitButtonEvent(BUTTON_ENTER_HOLD, pushedTime - push_reactivity);
          }

          if (millis() - lastHoldEventTime >= HOLD_REPEAT_MS) {
            lastHoldEventTime = millis();
            emitButtonEvent(BUTTON_HOLD, pushedTime - push_reactivity);
          }
        }

      } else { // still released
        pushedTime = 0;

        if (waitingSecondClick && (millis() - firstClickReleasedAt > doubleClickGap)) {
          waitingSecondClick = false;
          emitButtonEvent(BUTTON_SIMPLE_CLIC, pendingClickDuration);
        }
      }
    }


    return refreshed;
  }

  /** @brief Read pending input from this transport or hardware source. */
  virtual void read() = 0;
};

Button* Button::registeredButtons[Button::MAX_BUTTONS] = { nullptr };
uint8_t Button::registeredButtonCount = 0;
TaskHandle_t Button::globalRefreshTaskHandle = nullptr;

/**
 * @brief Reads a button connected to a digital pin.
 */
class DigitalButton : public Button {
public:
  /** @brief Initialize DigitalButton with the supplied configuration and bindings. */
  DigitalButton(uint8_t p,
                uint8_t mode = INPUT_PULLUP,
                int e = LOW,
                boolean d = false,
                uint16_t push = DEFAULT_PUSH_DELAY,
                uint16_t dClic = DEFAULT_DCLIC_DELAY)
    : Button(p, e, d, push, dClic)
      {
    pinMode(p, mode);
    evtBtn.type = EVENT_DIGITAL_BUTTON;
    id = concatStrings("DigitalButton~", getID());
  }

  /** @brief Return the logical button identifier used in gesture events. */
  virtual int getID() override {
    return CSPin;
  }

  /** @brief Assign the logical button identifier used in gesture events. */
  void setID(uint8_t id) override {
    CSPin = id;
  }


private:

  /** @brief Read pending input from this transport or hardware source. */
  virtual void read() override {
    bool input1 = digitalRead(CSPin);
    delay(1);
    bool input2 = digitalRead(CSPin);

    if (input1 == input2) {
      state = input1;
    }
  }
};

/**
 * @brief Reads a button selected by an analog resistor ladder.
 */
class LadderButton : public Button {
public:
  static uint8_t bCount;
  uint8_t virtualID;

  /** @brief Initialize LadderButton with the supplied configuration and bindings. */
  LadderButton(AnalogIn* i,
               int e = HIGH,
               boolean d = true,
               uint16_t push = DEFAULT_PUSH_DELAY,
               uint16_t dClic = DEFAULT_DCLIC_DELAY)
    : Button(i->CSPin, e, d, push, dClic)
      {
    input = i;
    evtBtn.type = EVENT_ANALOG_BUTTON;
    pinMode(input->CSPin, INPUT);
    virtualID = bCount;
    id = concatStrings("LadderButton~", getID());
    bCount++;
  }

  /** @brief Return the logical button identifier used in gesture events. */
  virtual int getID() override {
    return virtualID;
  }

  /** @brief Assign the logical button identifier used in gesture events. */
  void setID(uint8_t id) override {
    virtualID = id;
  }

private:
  AnalogIn* input = nullptr;

  #define RESOLUTION 4095
    //https://www.ignorantofthings.com/2018/07/the-perfect-multi-button-input-resistor.html
    // https://docs.google.com/spreadsheets/d/1G6wTsAxZDnVvzIDBkg5LrhhY1T7q4p_pDSZ6owvBCMA/edit?usp=sharing
    // for 3 button with 1st resistor 47Kohm, other resistance should be: 24k, 68k  

  /** @brief Read a resistor-ladder button using the retained earlier algorithm. */
  virtual void OLDread() {
    float avg = RESOLUTION / float(bCount);

    if (input->value > (bCount - 0.5f) * avg) {
      state = LOW;
    } else {
      int lowLimit = 0;
      int highLimit = 0;

      for (int i = 0; i < bCount; i++) {
        if (i > 0) {
          lowLimit = round(((i - 1) + 0.5f) * avg);
        }

        highLimit = round((i + 0.5f) * avg);
        /* measured values
          LEFT RANGE DETECTION: 0-683       ==> measured : 0
          CENTER RANGE DETECTION: 683-2048  ==> measured : 1326-1570
          RIGHT RANGE DETECTION: 2048-3413  ==> measured : 2660-2778

        */


        if (input->value >= lowLimit && input->value < highLimit) {
          if (i == virtualID) {
            state = HIGH;
          }
        }
      }
    }
  }

  /** @brief Read pending input from this transport or hardware source. */
  virtual void read() override {
    uint16_t value = analogRead(input->CSPin);
    float avg = RESOLUTION / float(bCount);

    state = LOW; // état relâché par défaut pour edge = HIGH

    if (value > (bCount - 0.5f) * avg) {
      return;
    }

    for (int i = 0; i < bCount; i++) {
      int lowLimit = 0;

      if (i > 0) {
        lowLimit = round(((i - 1) + 0.5f) * avg);
      }

      int highLimit = round((i + 0.5f) * avg);
          /* measured values
            LEFT RANGE DETECTION: 0-683       ==> measured : 0
            CENTER RANGE DETECTION: 683-2048  ==> measured : 1326-1570
            RIGHT RANGE DETECTION: 2048-3413  ==> measured : 2660-2778

          */

      if (value >= lowLimit && value < highLimit) {
        state = (i == virtualID) ? HIGH : LOW;
        return;
      }
    }
  }


};


uint8_t LadderButton::bCount = 0;

// littleFS : see https://github.com/espressif/arduino-esp32/tree/master/libraries/LittleFS/examples/LITTLEFS_PlatformIO
// ping : see https://github.com/pbecchi/ESP32_ping/tree/master    https://techoverflow.net/2021/06/19/how-to-ping-gateway-in-esp32/
// network : https://github.com/Xinyuan-LilyGO/LilyGo-W5500-Lite   https://mischianti.org/esp32-ethernet-w5500-with-plain-http-and-ssl-https/#google_vignette
// power W5500: https://github.com/pauldeng/arduino-leonardo-eth-w5500-powerdown-example/blob/arduino-default-ether-lib/src/main.cpp
#ifdef USE_HSPI_PORT
  #warning "USE_HSPI_PORT ACTIVATED"
#endif


enum{
  NO_MEDIA,
  CABLE_MEDIA,
  WIFI_MEDIA
};

enum{
  DISCONNECTED = false,
  CONNECTED = true,
};

//typedef std::function<void()> StdFunctionCallbackVoid; 
typedef std::function<void(const char*)> StdFunctionCallbackStringPointer;

/**
 * @brief Parameters and state used to start an HTTP request asynchronously.
 */
struct AsyncHttpClient{  
    IPAddress host;
    uint16_t port;
    Client* client;
    StdFunctionCallbackStringPointer callback;
    //StdFunctionCallbackVoid failConnectCallback;
    String method = HTTP_METHOD_GET;
    String URL;
    String post="";    
    bool isClientConnecting = false; // Indicateur pour éviter de lancer plusieurs tâches
    uint32_t timeOut = 1000;
};

// Prototype pour la tâche de connexion
/* 
void connectHTTPTask(void *parameter) {
    bool success = false;
    
    AsyncHttpClient *clientObj = static_cast<AsyncHttpClient *>(parameter);
    // Si la connexion est établie, appeler la fonction callback    
    Client* client = clientObj->client;
    
    String url = clientObj->URL;
    IPAddress serverIP = clientObj->host;
    uint16_t port = clientObj->port;
    uint32_t timeOut = clientObj->timeOut;
    String method = clientObj->method;  
    if (timeOut<1000) timeOut = 1000;
    String post = "";
    if (method == HTTP_METHOD_POST) post = clientObj->post;
    
    DEBUG_PRINT("\nconnectHTTPTask(%s,%s,%i,<%s>)",serverIP.toString().c_str(),url.c_str(),port,post.c_str());
    
    JsonDocument doc;      
    doc[API_ORIGIN] = url;

    String payload="";

    client = NULL;
    if (client != NULL){
      HttpClient httpClient(*client);      
      //int httpResponseCode = httpClient->get(serverIP.toString().c_str(),url.c_str());
      httpClient.beginRequest();
      httpClient.setHttpResponseTimeout(timeOut);
      int httpResponseCode = httpClient.startRequest(serverIP.toString().c_str(),port,url.c_str(),method.c_str(),"ESP32HttpClient/1.0");     

      if (httpResponseCode == 0) {
        //DEBUG_PRINT("\nRequête %s lancée",method.c_str());

        if (method == HTTP_METHOD_POST ){
          // Ajouter un en-tête Content-Type pour indiquer JSON
          httpClient.sendHeader("Content-Type", "application/json");
          httpClient.sendHeader("Content-Length", String(post.length()).c_str());
          //DEBUG_PRINT("\nPOST write: %s",post.c_str());
          // Envoyer le corps de la requête
          httpClient.write((const uint8_t*)post.c_str(), post.length());               
        }

        int httpStatusCode = httpClient.responseStatusCode();
        //DEBUG_PRINT("\nStatut HTTP : %d\n", httpStatusCode);

        if (httpStatusCode == 200) {
          httpClient.skipResponseHeaders();    // we want to read the body only
          //payload = httpClient.readString();

          while (httpClient.available()) { // Tant qu'il y a des données disponibles
            char c = httpClient.read();    // Lire un caractère
            payload += c;                   // Ajouter le caractère au payload
            yield();
          }

          //DEBUG_PRINT("\nRéponse %s reçue :<%s>",method.c_str(),payload.c_str());
          JsonDocument jo;
          deserializeJson(jo,payload);
          doc[API_STATUS] = HTTP_REST_OK;
          doc[API_BODY] = jo;
        } else {           
          doc[API_STATUS] = API_ERROR;
          doc[API_BODY] = concatStrings("httpStatusCode: ", httpStatusCode);
        }
      } else {
        // ça veut dire pas de serveur a cette adresse
        doc[API_STATUS] = API_ERROR;
        doc[API_BODY] = concatStrings("Bad request: ", httpResponseCode);
      }  
      httpClient.endRequest();
      httpClient.stop();
    }else{
        // ça veut dire pas de serveur a cette adresse
        doc[API_STATUS] = API_ERROR;
        doc[API_BODY] = "NULL client";
    }
    // envoie la reponse
    String result;        
    serializeJson(doc,result);
    clientObj->callback(result); 
    //clientObj->callback("{\"origin\":\"/app/state\",\"status\":1,\"data\":{\"progress\": 0,\"state\": \"connected\"}}");
    //DEBUG_PRINT("\nRESULT=>%s",result.c_str());
    // Fin de la tentative de connexion, réinitialiser l'indicateur
    clientObj->isClientConnecting = false;
    doc.clear();   
    
    // Tâche terminée, suppression    
    //DEBUG_PRINT("\nWaiting semaphore release in connectTask");
    if (xSemaphoreTake(clientObj->mutex, portMAX_DELAY)) { // Prendre le mutex
      //DEBUG_PRINT("\nconnectHTTPTask finished");

      //if (clientObj->taskHandle != NULL) {
        //clientObj->taskHandle = NULL;       // Mettre à jour le pointeur
      //}

      xSemaphoreGive(clientObj->mutex); // Relâcher le mutex
      vTaskDelete(NULL); // Supprimer la tâche    
    }
}*/
String httpTaskResult;
ByteBuffer readHttpBuffer;

/**
 * @brief Tracks an allocated UDP transport slot.
 */
struct UdpSlot {
  UDP* udp = nullptr;
  int port = -1;
  bool active = false;
  uint8_t media = CABLE_MEDIA;
  uint8_t reservations = 0;

  /** @brief Reset this object to its initial state. */
  void reset() {
    udp = nullptr;
    port = -1;
    active = false;
    media = NO_MEDIA;
    reservations = 0;
  }  
};

/**
 * @brief Event carrying a socket notification.
 */
class SocketEvent : public Event {
public:

  /**
   * @brief Initialize SocketEvent and its configured member bindings.
   */
  SocketEvent(void* data):Event(data){}

  #ifdef DEBUG
  /** @brief Export this object using its JSON representation. */
  const char* serialize() const override {
    if (((SocketData*)data)->length > 0) return ((SocketData*)data)->message;
    else return "";
  }
  #endif
};

/**
 * @brief Common connection state and transport resources for networking.
 */
class Network : public HWComponent2State{

  public:
    WebSocketClient<WiFiClient> *wsWiFiClient{ nullptr };
    WebSocketClient<EthernetClient> *wsEthClient{ nullptr };

    SocketData socketData;
    SocketEvent evtSock = SocketEvent(&socketData);    
    
    Event evtNet;

    
    float networkConnectAttemptProgress = 0.0;

    String networkConnectivityIcon = ICON_MIXETHWIFI;

    SemaphoreHandle_t networkBusy;

    StdFunctionCallbackVoidTargetEcho internetCallback = nullptr;    
    // Déclarer un pointeur générique pour le client
    //IWebSocket* wsClient = nullptr;  //     wsClient = wifiClient;   

    //IWebSocketServer *wss{ nullptr };
    //UDP* udp = NULL;
    Server* server=NULL;

    MidiClient* appleMidiClient = nullptr;

    uint8_t media = NO_MEDIA;
    uint8_t lastActiveMedia = NO_MEDIA;
    IPAddress IP, gateway,subnet;
    //HttpClient* http = NULL;    
    Client* client = NULL;   
    TargetEcho internet = TargetEcho("8.8.8.8"); 
    uint16_t LANlatency = 38;

    bool ethEnabled = true; ///< Ethernet is enabled by configuration.
    /**
     * @brief Whether a W5500 was detected during hardware initialization.
     *
     * This is deliberately separate from @c ethEnabled: Ethernet can be
     * enabled in software while the physical controller is absent or
     * undetectable. GUI and connection-attempt decisions must use this flag
     * when they depend on actual Ethernet hardware availability.
     */
    bool ethHardwareAvailable = false;
    bool WiFiEnabled = true;
    String SSID="undef",lastConnectedSSID="";
    bool AP = false;

    /** @brief Initialize Network with the supplied configuration and bindings. */
    Network(uint8_t pin, bool debug):HWComponent2State(pin,DISCONNECTED,debug){
      id = concatStrings("Network~", pin);
      networkBusy = xSemaphoreCreateMutex();
      state = DISCONNECTED; previousState = state; // if we set the initial state to the opposite of the state we want to detect it trigger the event DISCONNECT otherwise if previousState = state, it goes straight 
    }

    /** @brief Open the requested UDP slot and associate it with the service. */
    virtual UdpSlot* useUDPslot(int i) = 0;
    /** @brief Find an available UDP slot for a service. */
    virtual int requestUDPslotID(int port) = 0;
    /** @brief Create a UDP transport for the selected network media. */
    virtual UDP* newUDP() = 0;
    /** @brief Stop UDP sockets before changing connections or serving Ethernet traffic. */
    virtual void stopAllUDP() = 0;
    /** @brief Stop the socket associated with a UDP slot. */
    virtual UdpSlot* stopUDPslot(int i) = 0;
    /** @brief Release a previously reserved UDP slot. */
    virtual void releaseUDPSlot(int* i)=0;
    /** @brief Set the connection link state. */
    virtual void setLink(EthernetLinkStatus linkStatus) = 0;    
    /** @brief Start opening the configured WebSocket connection. */
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path)=0;
    /** @brief Send text through the current WebSocket connection. */
    virtual void clientSocketSend(const char *message)=0;
    /** @brief Close the current WebSocket connection. */
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE)=0;
    /** @brief Report the current WebSocket connection state. */
    virtual WebSocketReadyState clientSocketState()=0;
    /** @brief Return whether the network link is available. */
    virtual EthernetLinkStatus getLink() = 0;
    /** @brief Return the connected peer address. */
    virtual String getConnectedDeviceIP() = 0;
    /** @brief Start a reachability check and report its result through a callback. */
    virtual void asyncPing(TargetEcho* echo, PingCallback callBack)=0;
    /** @brief Start a connection attempt without waiting in the application loop. */
    virtual bool asyncConnect(AsyncHttpClient *clientParam)=0;
    /** @brief Describe the W5500 socket allocations for diagnostics. */
    virtual String dumpW5500Sockets()=0;
    /** @brief Return the address of this network connection. */
    virtual String getIP() = 0;
    
    /** @brief Prepare this object for use with its current configuration. */
    virtual void init(){
      if (debug) DEBUG_INFO("Network::init()");
      WiFi.setAutoConnect(false); 
      WiFi.setAutoReconnect(false);      
    }

    /** @brief Select the Wi-Fi network name and associated credentials. */
    virtual void setSSID(String ssid, String pwd){
      SSID = ssid;
      wiFiList->addValue(SSID,pwd);         
    }
    

    /** @brief Provide the saved network choices and preferred SSID. */
    virtual void setWiFi(JsonList* list, String wiFiID){
      wiFiList = list;
      wiFiIndex = wiFiList->getIndex(wiFiID);
      SSID = wiFiID;
      lastConnectedSSID = SSID;
    }



    /** @brief Set Wi-Fi transmit power. */
    void setWifiPower(){
      //#define WIFI_POWER WIFI_POWER_MINUS_1dBm // Set WiFi RF power output to lowest level      
      //#define WIFI_POWER WIFI_POWER_11dBm // Set WiFi RF power output to middle level
      //#define WIFI_POWER WIFI_POWER_19_5dBm // Set WiFi RF power output to highest level        
      #define WIFI_POWER WIFI_POWER_8_5dBm    // Set WiFi RF power output to standard level        

      //WiFi.setTxPower(WIFI_POWER); // maximum power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/ 
    }

    uint16_t wiFiTimeout = WIFI_CHECK_TIMEOUT_MIN;

    /** @brief Advance the timeout used by the connection attempt. */
    void shiftConnectTimeout() {
        wiFiTimeout += 3000;
        if (wiFiTimeout > WIFI_CHECK_TIMEOUT_MAX) {
            wiFiTimeout = WIFI_CHECK_TIMEOUT_MIN;
        }
    }

    /** @brief Export this object using its JSON representation. */
    String serialize(){
      //DEBUG_PRINT("\nserialize %s=>",id.c_str());
      //DynamicJsonDocument doc(500);   
      JsonDocument doc;
      #ifdef USE_NETWORK      
        doc["useNetwork"] = true;
        doc["cable"] = (getLink() == LinkON ? true:false);
        doc["localIP"] = IP.toString();
        doc["gatewayIP"] = gateway.toString();
        doc["networkState"] = (state == CONNECTED ? true:false);
        doc["lastSSID"] = lastConnectedSSID;
      #else
        doc["useNetwork"] = false;
      #endif
      String data;
      serializeJson(doc, data);
      //DEBUG_PRINT("\n%s->serialize: %s",id.c_str(),data.c_str());
      return data;
    }

    /** @brief Pause or stop Wi-Fi connection attempts. */
    void stopWiFi(){
      //DEBUG_PRINT("\n---------> stopWiFi() : WiFiEnabled = false <------------");
      WiFiEnabled = false;
      disconnectWifi();
    }

    /** @brief Allow Wi-Fi connection attempts to resume. */
    void unpauseWiFi(){
      //DEBUG_PRINT("\n\t\t\t\t---------> unpauseWiFi() : WiFiEnabled = true <------------");
      WiFiEnabled = true ;      
    }


  protected:
    JsonList* wiFiList;
    uint8_t wiFiIndex;
    //String oldSSID = "";

    /** @brief Disconnect the current Wi-Fi session. */
    void disconnectWifi(){
      HWComponent2State::stop();
     
      if (WiFi.getMode() == WIFI_AP_STA){
        //if (debug) DEBUG_PRINT("\tStopping STA, AP only"); 
        WiFi.mode(WIFI_AP);  // not required f we consider disconnectWifi is about shuting down STA, so no need to 
      }else if (WiFi.getMode() == WIFI_STA){
        //if (debug) DEBUG_PRINT("\tWiFi off");
        if (WiFi.isConnected()) {
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
        }
      }else{
        // if we are in AP mode only, we do nothing
         //if (debug) DEBUG_PRINT("\tNo Change on WiFI, already turned OFF (AP only)");
      }
      delay(100);
      //esp_wifi_stop();
    }        

    TaskHandle_t pingTaskHandle = NULL;

  private:
    bool oldAppConnected = false;

};

//constexpr uint16_t kBufferMaxSize{256};  //https://github.com/skaarj1989/mWebSockets/issues/72#issuecomment-2095599695

/**
 * @brief Chooses and monitors Wi-Fi or Ethernet connectivity.
 *
 * Coordinates Ethernet preference and Wi-Fi fallback. Services borrow shared UDP
 * slots by local port; the four-slot table tracks reservations and recreates
 * transports when media changes. The AP configuration connection is separate.
 */
class WiFiEth : public Network{
  public:
    EthernetLinkStatus cable = Unknown;///< , oldCable = cable;
    unsigned long checkEthCableTime=0;

    /** @brief Initialize WiFiEth with the supplied configuration and bindings. */
    WiFiEth(uint8_t pin, uint8_t rstP, bool enableEth = true, bool debug = true): Network(pin, debug){      
      id = concatStrings("WiFiEth~", pin);
      rstPin = rstP;
      ethEnabled = enableEth;
      String forgedEthMac = WiFi.macAddress();

      sscanf(forgedEthMac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &ethMac[0], &ethMac[1], &ethMac[2], &ethMac[3], &ethMac[4], &ethMac[5]);      
      /* we forge the eth mac address based on the wifi but we set the last digit to 99 to avoid conflicting eth and wifi mac adress when
      / AP is connected...

      sscanf(forgedEthMac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx", &ethMac[0], &ethMac[1], &ethMac[2], &ethMac[3], &ethMac[4]);      
      ethMac[5]=0x99;*/

      /*pinMode(rstPin, OUTPUT);
      digitalWrite(rstPin,LOW);     // this has to be done before anything, including init and powerring on
      */


    }

    /** @brief Release resources held by this object. */
    ~WiFiEth(){
      stopSocketOpenTask();
      if (wsWiFiClient != nullptr){ delete wsWiFiClient; wsWiFiClient = nullptr; }
      if (wsEthClient != nullptr){ delete wsEthClient; wsEthClient = nullptr; }
      if (instance == this) instance = nullptr;
    }

    /** @brief Prepare this object for use with its current configuration. */
    virtual void init(){
      Network::init();

      instance = this; //for sockets and other callbacks

      //if (debug) DEBUG_PRINT("\n%s.init()",id.c_str());
      tcpipInit();  // so ethernet can benefit from getting mac address and other low level function even if WiFi is not up      
      // if enabled is true it means wde want to use the ethernet shield, so we try to initialize if
      
/*      WiFi.setSleep(false); // Désactiver le mode veille Wi-Fi
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reconfigurer DHCP      */
      
      if (ethEnabled){
        //ESP.eraseConfig(); // https://github.com/esp8266/Arduino/issues/1624
         //WiFi.setAutoConnect(false);

        //SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
        //reset();

        Ethernet.init(CSPin);   // I've tested to move it in the constructor but it crashes, this has to be called in the setup() 
        // test if ethernet module is present and instantiate class accordingly
        Ethernet.begin(ethMac,100,100);
        if (Ethernet.hardwareStatus() == EthernetNoHardware){
          ethHardwareAvailable = false;
          networkConnectivityIcon = ICON_WIFI_STRENGTH_3;
          if (debug) DEBUG_WARN("%s !!! THERE IS NO ETHERNET HW !!!",id.c_str());
        }else{
          ethHardwareAvailable = true;
          if (debug) DEBUG_INFO("%s Using WiFi & Ethernet",id.c_str());
          //EthernetBonjour.setServiceFoundCallback(onEthernetBonjourServiceFoundCallback);         
          setLink(Ethernet.linkStatus());   // check immediately if ethernet is connected so it has priority on WiFI connection    => done in the thread
        }
      }else{
        //if (debug) DEBUG_PRINT("\n%s Using WiFi only",id.c_str());
        networkConnectivityIcon = ICON_WIFI_STRENGTH_3;
      }
     
      if (!ethEnabled){
        // power off the ethernet
        stopEthernet();
      }
      wipeWifiConfig();

      // we start with Ethernet priority so reset WiFi counter timer so we let time to ethernet to startup
      lastWiFiAttempt = millis();      

    }    

    /** @brief Clear the Wi-Fi configuration retained by the network stack. */
    void wipeWifiConfig() {
      // Do the same as Erase Flash
      // 1) Couper le Wi-Fi et effacer les creds Arduino dans la NVS
      //    (2e paramètre = erase NVS des creds)
      WiFi.disconnect(true /*wifioff*/, true /*erase*/);
      // 2) Rétablir la config Wi-Fi par défaut côté IDF (efface aussi les profils)
      esp_wifi_restore();
      // 3) Éviter de réécrire automatiquement plus tard
      WiFi.persistent(false);
    }    

    /**
     * @brief Pulse the Ethernet reset line, then wait for W5500 startup.
     *
     * Drive reset low for 1 ms, release it and wait 200 ms. The board shares reset
     * with the TFT, so initialization order matters.
     */
    void reset(){
      digitalWrite(rstPin, LOW);  // Maintenez le W5500 en état de reset
      delay(1);  // Maintenir le RESET bas pendant au moins 500 ns (nous utilisons ici 1 ms pour être sûr)
      digitalWrite(rstPin, HIGH);  // Libérer le RESET du W5500
      delay(200);  // Attendez au moins 200 ms pour que le W5500 termine son initialisation interne
    }

    #include <utility/w5100.h>
    /**
     * @brief Return a readable label for a W5500 socket status register value.
     */

    static const char* socketStatusToString(uint8_t st) {
      switch (st) {
        case 0x00: return "CLOSED";
        case 0x13: return "INIT";
        case 0x14: return "TCP_LISTEN";
        case 0x15: return "TCP_SYNSENT";
        case 0x16: return "TCP_SYNRECV";
        case 0x17: return "TCP_EST";
        case 0x18: return "TCP_FIN";
        case 0x1A: return "TCP_CLOSING";
        case 0x1B: return "TCP_TIMEWAIT";
        case 0x1C: return "TCP_CLOSEWAIT";
        case 0x1D: return "TCP_LASTACK";
        case 0x22: return "UDP";
        case 0x32: return "IPRAW";
        case 0x42: return "MACRAW";
        case 0x5F: return "PPPOE";
        default:   return "UNK";
      }
    }

    /** @brief Describe the W5500 socket allocations for diagnostics. */
    String dumpW5500Sockets() {

      char buffer[512];
      size_t offset = 0;

      uint8_t freeCount = 0;
      uint8_t tcpCount  = 0;
      uint8_t udpCount  = 0;

      for (int s = 0; s < MAX_SOCK_NUM; s++) {

        uint8_t st = W5100.readSnSR(s);

        if (st == 0x00) freeCount++;
        if (st == 0x22) udpCount++;
        if (st >= 0x13 && st <= 0x1D) tcpCount++;

        offset += snprintf(buffer + offset,
                          sizeof(buffer) - offset,
                          "S%d=%s(%02X)\t",
                          s,
                          socketStatusToString(st),
                          st);

        if (offset >= sizeof(buffer)) break;
      }

      snprintf(buffer + offset,
              sizeof(buffer) - offset,
              "\t| FREE=%d TCP=%d UDP=%d",
              freeCount,
              tcpCount,
              udpCount);      
      return String(buffer);
    }

    #define MAX_UDP_SLOT 4
    UdpSlot udpSlots[MAX_UDP_SLOT];   ///< max 4 objets

    /** @brief Check that a UDP slot index is within its fixed bounds. */
    bool isValidUDPslotIndex(int i) const {
      return i >= 0 && i < MAX_UDP_SLOT;
    }

    /** @brief Release the transport object held by a UDP slot. */
    void destroyUDPslot(int i) {
      if (!isValidUDPslotIndex(i)) return;
      if (udpSlots[i].udp != nullptr) {
        udpSlots[i].udp->stop();
        delete udpSlots[i].udp;
      }
      udpSlots[i].reset();
    }

    /** @brief Recreate a UDP slot when its transport differs from the selected network media. */
    bool ensureUDPslotMedia(int i) {
      if (!isValidUDPslotIndex(i)) return false;
      UdpSlot& slot = udpSlots[i];

      if (slot.port < 0 || slot.reservations == 0) return false;

      if (slot.udp != nullptr && slot.media == media) return true;

      if (slot.udp != nullptr) {
        slot.udp->stop();
        delete slot.udp;
        slot.udp = nullptr;
      }

      slot.udp = newUDP();
      if (slot.udp == nullptr) {
        slot.active = false;
        slot.media = NO_MEDIA;
        return false;
      }

      slot.active = false;
      slot.media = media;
      return true;
    }
    

    /** @brief Create a UDP transport for the selected network media. */
    virtual UDP* newUDP(){
      // Créer l'objet UDP du bon type
      if (media == CABLE_MEDIA)
         return new EthernetUDP;
      else
        return new WiFiUDP;      
    }

    /**
     * @brief Reserve one of four UDP slots, sharing an existing slot for the same port.
     *
     * Validate the port, reuse a matching slot and increment its reservation count,
     * or create a transport in a free slot. Media changes recreate the transport.
     * @param port Local UDP port from 1 to 65535.
     * @return Slot index, or -1 for invalid input, exhaustion or setup failure.
     * @note Pair each successful reservation with releaseUDPSlot().
     */
    virtual int requestUDPslotID(int port) {

      if (port <= 0 || port > 65535) {
        DEBUG_ERROR("[requestUDPslotID] invalid port %d", port);
        return -1;
      }

      // 1) Si un slot existe déjà pour ce port, on le réutilise ou on le remplace selon media
      for (int i = 0; i < MAX_UDP_SLOT; i++) {
        if (udpSlots[i].port == port && udpSlots[i].reservations > 0) {
          if (!ensureUDPslotMedia(i)) {
            DEBUG_WARN("[UDP-%d/%d] ensureUDPslotMedia failed", i, port);
            return -1;
          }
          udpSlots[i].reservations++;
          DEBUG_INFO("[UDP-%d/%d] ID reclaimed (refs=%d)", i, port, udpSlots[i].reservations);
          return i;
        }
      }

      // 2) Sinon, créer dans un slot libre
      for (int i = 0; i < MAX_UDP_SLOT; i++) {
        if (udpSlots[i].reservations == 0) {

          destroyUDPslot(i);

          udpSlots[i].port = port;
          udpSlots[i].reservations = 1;

          if (!ensureUDPslotMedia(i)) {
            destroyUDPslot(i);
            DEBUG_WARN("[UDP] newUDP() failed");
            return -1;
          }

          DEBUG_INFO("[UDP-%d/%d] ID created", i, port);
          return i;
        }
      }

      DEBUG_WARN("[UDP] Reached max!");
      return -1;
    }

    /** @brief Open the requested UDP slot and associate it with the service. */
    virtual UdpSlot* useUDPslot(int i) {

      if (!isValidUDPslotIndex(i) || udpSlots[i].reservations == 0 || udpSlots[i].port < 0){
        //DEBUG_PRINT("\ngetUDPslot(%d) out of range or null",i);
        return nullptr;
      }

      if (!ensureUDPslotMedia(i)) {
        return nullptr;
      }
       
      // Restart socket
      if (!udpSlots[i].active){
        udpSlots[i].active = udpSlots[i].udp->begin(udpSlots[i].port);
        if (!udpSlots[i].active){
          //DEBUG_PRINT("\n[UDP-%d/%d] begin() failed", i, udpSlots[i].port);
          return nullptr;
        }else{
          //DEBUG_PRINT("\n[UDP-%d/%d] begin()", i, udpSlots[i].port);                     
        }
      }
      return &udpSlots[i];
    }

    /**
     * @brief Release one UDP reservation and invalidate the caller slot id.
     *
     * Destroy the transport only when no reservations remain; other services sharing
     * the local port keep their slot. A pointer to the slot id is required.
     */
    virtual void releaseUDPSlot(int* i){
      if (i == nullptr) return;
      if (!isValidUDPslotIndex(*i)) {
        *i = -1;
        return;
      }

      UdpSlot& slot = udpSlots[*i];
      if (slot.reservations > 0) slot.reservations--;

      if (slot.reservations == 0) {
        DEBUG_INFO("[UDP-%d/%d] released (destroy)", *i, slot.port);
        destroyUDPslot(*i);
      } else {
        DEBUG_INFO("[UDP-%d/%d] released (refs=%d)", *i, slot.port, slot.reservations);
      }

      *i = -1;   // invalidate slot ID
    }

    /** @brief Stop the socket associated with a UDP slot. */
    virtual UdpSlot* stopUDPslot(int i){
      
      if (!isValidUDPslotIndex(i)){
        DEBUG_ERROR("\nstopUDP(%i) out of range", i);
        return nullptr;
      }

      if (udpSlots[i].udp != nullptr){
        udpSlots[i].udp->stop();
        udpSlots[i].active = false;
        DEBUG_INFO("[UDP-%d/%d] stopped", i, udpSlots[i].port);
        return &udpSlots[i];
      }else{
        DEBUG_ERROR("[UDP-%d/%d] stopped failed", i, udpSlots[i].port);        
      }

      return nullptr;

    }

    /** @brief Stop UDP sockets before changing connections or serving Ethernet traffic. */
    virtual void stopAllUDP() {
      for (int i = 0; i < MAX_UDP_SLOT; i++) {
        if (udpSlots[i].udp != nullptr && udpSlots[i].reservations > 0){
          stopUDPslot(i);
        }
      }
    }

    /*

    SOCKETS MANAGEMENT
    
    */

    enum SocketOpenJobState : uint8_t {
      SOCKET_JOB_IDLE,
      SOCKET_JOB_RUNNING,
      SOCKET_JOB_FAILED
    };

    volatile SocketOpenJobState socketOpenJobState = SOCKET_JOB_IDLE;

    TaskHandle_t socketOpenTaskHandle = nullptr;

    // Paramètres copiés pour la task
    String pendingHost;
    String pendingPath;
    uint16_t pendingPort = 0;    


    /** @brief Start opening the configured WebSocket connection. */
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path) {
      DEBUG_INFO("%s::clientSocketOpen(%s,%d,%s)", id.c_str(), host, port, path);

      if (state != CONNECTED) {
        DEBUG_PRINT("\t❗failed, network not connected!");
        return false;
      }

      if (socketOpenJobState == SOCKET_JOB_RUNNING) {
        DEBUG_PRINT("\t❗already opening");
        return true;
      }

      WebSocketReadyState wsState = clientSocketState();
      if (wsState == WebSocketReadyState::OPEN) {
        DEBUG_PRINT("\t❗already opened");
        return true;
      }

      if (wsState == WebSocketReadyState::CONNECTING) {
        DEBUG_PRINT("\t❗already connecting");
        return true;
      }

      pendingHost = host ? host : "";
      pendingPath = path ? path : "";
      pendingPort = port;

      socketOpenJobState = SOCKET_JOB_RUNNING;

      BaseType_t ok = xTaskCreatePinnedToCore(
        &WiFiEth::socketOpenTaskEntry,
        "wsOpenTask",
        6144,
        this,
        1,
        &socketOpenTaskHandle,
        1
      );

      if (ok != pdPASS) {
        DEBUG_ERROR("\t task creation failed!");
        socketOpenJobState = SOCKET_JOB_FAILED;
        socketOpenTaskHandle = nullptr;
        return false;
      }

      DEBUG_PRINT("\t opening started asynchronously");
      return true;
    }

    /** @brief Send text through the current WebSocket connection. */
    virtual void clientSocketSend(const char *message){
      if (media == CABLE_MEDIA){
        if (wsEthClient!= nullptr){      
          if (wsEthClient->getReadyState() == WebSocketReadyState::OPEN){
            wsEthClient->send(WebSocketDataType::TEXT, message, strlen(message));
            DEBUG_INFO("🌐→→→ %s",message);

          }
        }
      }else{
        if (wsWiFiClient!= nullptr){      
          wsWiFiClient->send(WebSocketDataType::TEXT, message, strlen(message));
          DEBUG_INFO("🌐→→→ %s",message);
        }
      }      
    }

    /** @brief Close the current WebSocket connection. */
    virtual void clientSocketClose(WebSocketCloseCode code = WebSocketCloseCode::NORMAL_CLOSURE) {
      DEBUG_INFO("%s::clientSocketClose()", id.c_str());
      const char reason[] = {"application close"};
      if (state != CONNECTED || media == NO_MEDIA) {
        DEBUG_INFO("%s::clientSocketClose() skipped: network not connected (state:%d media:%d)", id.c_str(), state, media);
        socketOpenJobState = SOCKET_JOB_IDLE;
        return;
      }

      if (media == CABLE_MEDIA) {
        if (wsEthClient != nullptr && wsEthClient->getReadyState() == WebSocketReadyState::OPEN) {
          wsEthClient->close(code, true, reason, strlen(reason));
        }
      } else {
        if (wsWiFiClient != nullptr && wsWiFiClient->getReadyState() == WebSocketReadyState::OPEN) {
          wsWiFiClient->close(code, true, reason, strlen(reason));
        }
      }

      socketOpenJobState = SOCKET_JOB_IDLE;
    }
    
    
    /** @brief Report the current WebSocket connection state. */
    virtual WebSocketReadyState clientSocketState() {
      if (socketOpenJobState == SOCKET_JOB_RUNNING) {
        return WebSocketReadyState::CONNECTING;
      }

      if (media == CABLE_MEDIA) {
        if (wsEthClient != nullptr) return wsEthClient->getReadyState();
      } else {
        if (wsWiFiClient != nullptr) return wsWiFiClient->getReadyState();
      }

      return WebSocketReadyState::CLOSED;
    }


    /** @brief Select the Wi-Fi network name and associated credentials. */
    virtual void setSSID(String ssid, String pwd){
      Network::setSSID(ssid,pwd);
      //DEBUG_PRINT("\nsetSSID(%s,%s)",ssid,pwd);                 
      if (media != CABLE_MEDIA){  // priority on Ethernet
        //DEBUG_PRINT("\n    !!!! Disconnecting WiFI !!!!",ssid,pwd);     
        stopWiFi();
        state = DISCONNECTED;       
        startWifi(ssid, pwd);
      }
    }

    /** @brief Set the connection link state. */
    virtual void setLink(EthernetLinkStatus linkStatus){
      cable = linkStatus;
      checkEthCableTime = millis();
    }

    /** @brief Return whether the network link is available. */
    virtual EthernetLinkStatus getLink(){
      return cable;
    }

    /** @brief Begin a Wi-Fi connection attempt using the selected credentials. */
    void startWifi(String ssid, String pwd){
      /*WiFi.disconnect(true);
      delay(500);
      WiFi.setSleep(false); // Désactiver le mode veille Wi-Fi
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reconfigurer DHCP    */

      DEBUG_INFO("startWifi(%s,%s), wifiEnabled=%d, status: %s, MAC:%s",SSID.c_str(),pwd.c_str(),WiFiEnabled,wifiStatusToString(WiFi.status()),WiFi.macAddress().c_str());                
      // Hostname avant begin
      //WiFi.setHostname("esp32-client");
      // Pas d’économie d’énergie pendant les tests
      //esp_wifi_set_ps(WIFI_PS_NONE);      
      //WiFi.disconnect(false,true);      
      WiFi.begin(ssid,pwd);
      lastWiFiAttempt = millis();      
      //_newWifiAttempt = false;      
    }

    /** @brief Stop the Ethernet connection. */
    void stopEthernet(){
      Network::stop();
      //if (debug) DEBUG_PRINT("\teth CSPin input");
      pinMode(CSPin, INPUT);
      // 
      pinMode(CSPin, OUTPUT);
      digitalWrite(CSPin, LOW);
    }

    /** @brief Return the connected peer address. */
    virtual String getConnectedDeviceIP(){
      return "GW-IP";   
    }

    /** @brief Stop the active operation or connection. */
    virtual void stop(){
      stopWiFi();     
      stopEthernet();
    }

    //////////////////// Static callback methods /////////////////////////////////////

    /** @brief Forward a socket error to the active Wi-Fi/Ethernet instance. */
    void static socketClientOnErrorCallbackStatic(WebSocketError error) {
      if (instance) instance->socketClientOnErrorCallback(error);
    }

    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    void static socketClientOnCloseStatic(IWebSocket &ws, WebSocketCloseCode code, const char *reason,uint16_t length){
      if (instance) instance->socketClientOnClose(code,reason,length);
    }     

    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    void static socketClientOnOpenCallbackStatic(IWebSocket &ws) {
      if (instance) instance->socketClientOnOpenCallback();
    }

    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    void static socketClientOnMessageCallbackStatic(IWebSocket &ws, const WebSocketDataType dataType,const char *message, uint16_t length){
      if (instance) instance->socketClientOnMessageCallback(dataType,message,length);               
    }

    //////////////////// Instance callback methods /////////////////////////////////////

    /** @brief Report a WebSocket error through the network socket event. */
    void socketClientOnErrorCallback(WebSocketError error) {
      evtSock.type = EVENT_SOCKET_ERROR;
      socketData.code = (int)error;
      fireEvent(evtSock);
    }

    /** @brief Forward the socket connection or message callback as a network event. */
    void socketClientOnClose(WebSocketCloseCode code, const char *reason,uint16_t length){
      evtSock.type = EVENT_SOCKET_CLOSE;
      socketData.code = (int)code;
      socketData.message = reason;
      socketData.length = length;
      fireEvent(evtSock);
    }    

    /** @brief Forward the socket connection or message callback as a network event. */
    void socketClientOnOpenCallback() {
      evtSock.type = EVENT_SOCKET_OPEN;
      fireEvent(evtSock);
    }

    /** @brief Forward the socket connection or message callback as a network event. */
    void socketClientOnMessageCallback(const WebSocketDataType dataType,const char *message, uint16_t length){
      switch (dataType) {
        case net::WebSocketDataType::TEXT:{
          evtSock.type = EVENT_SOCKET_MESSAGE;
          socketData.message = message;
          socketData.length = length;
          socketData.receivedAt = millis();
          fireEvent(evtSock);
        }
        break;
        case net::WebSocketDataType::BINARY:
          //ignored
        break;
      }                     
    }
    
    ////////////////////////////////////////////////////////
    /** @brief Reset socket callback flags before another connection attempt. */
    void resetWSstate(){
      socketOpenJobState = SOCKET_JOB_IDLE;
      socketOpenTaskHandle = nullptr;
      pendingHost = "";
      pendingPath = "";
      pendingPort = 0;      
      DEBUG_INFO("WebSocket state reset");
    }

    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    void stopSocketOpenTask(){
      if (socketOpenTaskHandle != nullptr && socketOpenTaskHandle != xTaskGetCurrentTaskHandle()) {
        DEBUG_WARN("stopSocketOpenTask() called while task is running, waiting for completion...");        
        TickType_t start = xTaskGetTickCount();
        while (socketOpenJobState == SOCKET_JOB_RUNNING && (xTaskGetTickCount() - start) < pdMS_TO_TICKS(500)) {
          vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (socketOpenJobState == SOCKET_JOB_RUNNING) {
          DEBUG_WARN("socketOpenTask still running, forcing task delete");
          vTaskDelete(socketOpenTaskHandle);
        }
      }
      DEBUG_INFO("socketOpenTask stopped");
      resetWSstate();
    }


    /**
     * @brief Monitor link changes and advance Ethernet/Wi-Fi connection attempts.
     *
     * Prefer Ethernet when its cable/link becomes available. Wi-Fi retries use the
     * saved network list and connection timeouts; changes notify the application
     * through network events. This is connection orchestration, not device-protocol
     * processing, which remains in the corresponding interfaces.
     */
    bool refresh(bool force=false) {
    
      if (millis()-checkEthCableTime > ETH_CABLE_CHECK_FREQUENCY &&
          ethEnabled && ethHardwareAvailable){   // only poll a detected W5500
        setLink(Ethernet.linkStatus());        
      }
      // the loop ensure that we always have one connection at the same time, if wifi is connected, ethernet will not connect and vice versa
      // ethernet has the priority on WiFi
      //cable = Ethernet.linkStatus();      
      //DEBUG_PRINT("\n:cable:%d",cable);
      if (state == DISCONNECTED){        
        // first try to connect eth
        if (cable == LinkON){       // ethernet is enabled  
          #define ETH_BEGIN_RETRY_DELAY 1000
          unsigned long delta = millis()-lastEthAttempt;                    
          if (delta >= ETH_BEGIN_RETRY_DELAY){            
            lastEthAttempt = millis();
            // toggle the progress indicator
            if (networkConnectAttemptProgress == 1.0) networkConnectAttemptProgress = 0.0;
            else networkConnectAttemptProgress = 1.0;
            
            bool success;
            /* IF CONFIG = STATIC IP {
            IP = config.getIP();
              SSID = concatStrings("Static IP: ", IP);
              success = Ethernet.begin(mac, IP, gateway, gateway, subnet);
            }
            else
            */
            SSID = "Connecting DHCP";
            success = Ethernet.begin(ethMac,100,100);

            if (success){  // as it take 100ms, we only do it every ETH_BEGIN_RETRY_DELAY otherwise it really slows down the loop
              
              if (xSemaphoreTake(networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){
                // save power    
                disconnectWifi();
                // we just connected ETH so we do everything related to ETH specialized object instanciation
                if (client == NULL) client = new EthernetClient();
                if (server == NULL) server = new EthernetServer(80); // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
                //if (udp == NULL) udp = new EthernetUDP;
                if (wsEthClient == nullptr){
                  wsEthClient = new EthernetWebSocketClient{};   
                  wsEthClient->onOpen(WiFiEth::socketClientOnOpenCallbackStatic);
                  wsEthClient->onMessage(WiFiEth::socketClientOnMessageCallbackStatic);
                  wsEthClient->onError(WiFiEth::socketClientOnErrorCallbackStatic);
                  wsEthClient->onClose(WiFiEth::socketClientOnCloseStatic);                      
                }
                if (appleMidiClient == nullptr) appleMidiClient = new AppleMidiWithInterfaceWrapper<EthernetUDP>(/*APPLE_MIDIETHCLIENT_NAME*/systemCfg.getString(S_AP_NAME)->c_str(),APPLE_MIDI_PORT);
                state = CONNECTED;
                media = CABLE_MEDIA;                
                DEBUG_INFO("\t...all Eth object instantiated\n");                  
                networkConnectivityIcon = ICON_ETH;
                xSemaphoreGive(networkBusy);
              }else{
                DEBUG_WARN("ETH CONNECT: networkBusy\n"); 
              }
            }else{
              DEBUG_PRINT(".");
            }
          }
        }else{
          if (WiFi.status() != WL_CONNECTED){
            if (WiFiEnabled){                   
              unsigned long delta = millis()-lastWiFiAttempt;
              networkConnectAttemptProgress = scaleValue(delta,0,wiFiTimeout,0.0,1.0);

              if (networkConnectAttemptProgress >= 1.0){

                //WiFi.begin("infrafast","Mf9k8iie2");
                
                   /* IF CONFIG = STATIC IP {
                    IP = config.getIP();
                    // Configurer l'adresse IP statique
                    if (!WiFi.config(IP, gateway, subnet)) {
                      DEBUG_PRINT("\nStatic WifI IP failed");
                    }
                  }*/
                  // DHCP
                WiFi.config(INADDR_ANY, INADDR_ANY, INADDR_ANY);

                String pass = *wiFiList->getString(SSID);    // getting the password for the given ssid
                startWifi(SSID,pass);

                //next  credential to attempt
                wiFiIndex = (wiFiIndex + 1) % wiFiList->size();
                if (wiFiIndex == 0) shiftConnectTimeout(); 
                SSID = *wiFiList->getName(wiFiIndex);              
              }            

            }
            //if (debug) DEBUG_PRINT("~"); // I put that in OnWiFiEvent => SYSTEM_EVENT_STA_DISCONNECTED to avoid flooding the serial out
          }else{
            // we just connected WIFI_MEDIA so we do everything related to WIFI_MEDIA specialized object instanciation
            if (xSemaphoreTake(networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){
              DEBUG_PRINT("\t => got IP:%s",IP.toString().c_str()); 
              //if (debug) DEBUG_PRINT("\nWifi.begin() => got IP: %s, %idb",IP.toString().c_str(),WiFi.RSSI());
              if (client == NULL) client = new WiFiClient();
              if (server == NULL){ 
                server = new WiFiServer(80,3);
                server->begin(); // <-- for some reason this begin cause the webserver not react with ethernet if it is called. But is required for WiFi                              
              }
              //if (udp == NULL) udp = new WiFiUDP; 
              if (wsWiFiClient == nullptr){
                wsWiFiClient = new WiFiWebSocketClient{};
                wsWiFiClient->onOpen(WiFiEth::socketClientOnOpenCallbackStatic);
                wsWiFiClient->onMessage(WiFiEth::socketClientOnMessageCallbackStatic);
                wsWiFiClient->onError(WiFiEth::socketClientOnErrorCallbackStatic);
                wsWiFiClient->onClose(WiFiEth::socketClientOnCloseStatic);                  
              }
              if (appleMidiClient == nullptr) appleMidiClient = new AppleMidiWithInterfaceWrapper<WiFiUDP>(systemCfg.getString(S_AP_NAME)->c_str(),APPLE_MIDI_PORT);                                 
              state = CONNECTED;
              media = WIFI_MEDIA;
              DEBUG_PRINT("\t...all WiFi object instantiated\n");                                
              xSemaphoreGive(networkBusy);
            }else{
              DEBUG_WARN("WIFI CONNECT: networkBusy"); 
            }
          }
        }
      }else if (state == CONNECTED) {
        //ethernet has priority over wifi, hence if we are connected by WiFi but the cable is plugged, we disconnect WiFi
        if (media == WIFI_MEDIA && cable == LinkON){
          DEBUG_INFO("+--------------------------*---------------+");
          DEBUG_INFO("| NETWORK PLUGGED ETH WHILE WIFI CONNECTED |");
          DEBUG_INFO("+------------------------------------------+");           
          evtNet.type = EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED;
          fireEvent(evtNet);        
          disconnectWifi();
          state = DISCONNECTED; // cable will try reconnect at next refresh
          lastEthAttempt = 0; // with no delay
        }else{
          // *********************************************************************************
          // we are connected so do whatever is required to refresh common network object here
          // *********************************************************************************          
                          
          // then read incoming midi packets
          //if (appleMidiClient != nullptr) appleMidiClient->read();   // we prefer to put it into the AppleMIDIInterface monitor(bool enabled) as we will save some time in case interface is offline
          
          // finally do whatever is required to refresh specialized network object here
          if (media == CABLE_MEDIA){
            if (cable == LinkON){
              // do whatever is required when we are connected by cable
              if (wsEthClient != nullptr) wsEthClient->listen();
            }else{
              state = DISCONNECTED; 
              media = NO_MEDIA;
            }
          }else{
            if (WiFi.status() == WL_CONNECTED){
              // do whatever is required when we are connected by WiFI
              // this code is executed in loop
              int8_t signalStrength = abs(WiFi.RSSI());

              if (signalStrength >=90) networkConnectivityIcon = ICON_WIFI_STRENGTH_0;
              else if (signalStrength >=75) networkConnectivityIcon = ICON_WIFI_STRENGTH_1;
              else if (signalStrength >=45) networkConnectivityIcon = ICON_WIFI_STRENGTH_2;
              else networkConnectivityIcon = ICON_WIFI_STRENGTH_3;                

              //ArduinoOTA.handle();     
              if (wsWiFiClient != nullptr) wsWiFiClient->listen();
            }else{
              // this code is executed once
              state = DISCONNECTED; 
              media = NO_MEDIA;     

            }
          }
        }
      }

      Network::refresh(force);
      /*if (cable != oldCable){
        oldCable = cable;
        refreshed = true;   // this refresh has to be flagged after NetworkRefresh to avoid being erased
      }*/

      if (refreshed){
        DEBUG_INFO("+------------------------------------------------+");
        DEBUG_INFO("| %s changed state => cable:%i, media:%i    |",id.c_str(),cable,media);
        DEBUG_INFO("+------------------------------------------------+\n");         
       
        if (state == CONNECTED){
          // reset the progress indicator
          networkConnectAttemptProgress = -1;
          // initialiser of common object to both Wifi and Ethernet goes here      
          getIP();
          DEBUG_INFO(" => got IP:%s",getIP().c_str());                                

          if (media == WIFI_MEDIA){
            SSID = WiFi.SSID();
            lastConnectedSSID = SSID;
            //DEBUG_PRINT("\n         + - + - saving lastConnectedSSID=SSID=%s",lastConnectedSSID.c_str());                          
            unpauseWiFi();       
          }else{
            // ETHERNET        
                                     
           }

          lastActiveMedia = media;
          checkInternet();
          DEBUG_INFO("+-------------------+");
          DEBUG_INFO("| NETWORK CONNECTED |");
          DEBUG_INFO("+-------------------+");
          

          ///////// ONCE ALL IS CORRECTLY PREPARED, WE FIRE THE EVENT
          evtNet.type = EVENT_NETWORK_CONNECTED;
          fireEvent(evtNet);  

        }else if (state == DISCONNECTED){
          if (xSemaphoreTake(networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){          
            DEBUG_INFO("+----------------------+");
            DEBUG_INFO("| NETWORK DISCONNECTED |");
            DEBUG_INFO("+----------------------+"); 
            // common destruction
            internet.alive = false;
            internet.latency = 0;

            //if (udp != NULL){udp->stop(); delete udp; udp = NULL;}           
            if (client != NULL){client->stop(); delete client; client = NULL;} 
            if (server != NULL){delete server; server  = NULL;} 
            stopSocketOpenTask();
            // Do not close/delete websocket clients here: on abrupt network loss,
            // close() can attempt a write on an invalid socket and crash.
            if (appleMidiClient != nullptr){ appleMidiClient->endSession(); delete appleMidiClient; appleMidiClient = nullptr;}
            
            lastActiveMedia = NO_MEDIA;
            if (!ethEnabled || !ethHardwareAvailable) networkConnectivityIcon = ICON_WIFI_STRENGTH_3;
            else networkConnectivityIcon = ICON_MIXETHWIFI; 
            DEBUG_INFO("+------------------------------+");           
            DEBUG_INFO("| ALL NETWORK OBJECTED CLEANED |");
            DEBUG_INFO("+------------------------------+");

            ///////// ONCE ALL IS CORRECTLY PREPARED, WE FIRE THE EVENT
            evtNet.type = EVENT_NETWORK_DISCONNECTED;
            fireEvent(evtNet);  

            xSemaphoreGive(networkBusy);              
          }else{
            DEBUG_WARN("network DISCONNECTED: networkBusy"); 
          }
          restoreWiFiSSID();
        }

        evtNet.type = EVENT_NETWORK_CONNECTION_CHANGED;
        fireEvent(evtNet);    

      }
      return refreshed;
    }

    /** @brief Return the address of this network connection. */
    String getIP() override{
      if (media == CABLE_MEDIA){
        IP = Ethernet.localIP();      
        subnet = Ethernet.subnetMask();    
        gateway = Ethernet.gatewayIP();
      }else{
        IP = WiFi.localIP();
        subnet = WiFi.subnetMask();
        gateway = WiFi.gatewayIP();      
      }
      return IP.toString();
    }

    /** @brief Apply the result of an internet reachability check. */
    void onInternetPing(TargetEcho* echo) {
        if (internetCallback !=nullptr) internetCallback(echo);
        //DEBUG_PRINT("\n\n\n\t\t\t * * * * onInternetPing %s latency:%i * * * *",echo->alive?"OK":"fail",echo->latency);        
    }

    // Fonction pour démarrer la tâche de ping non bloquante
    /** @brief Start a reachability check and report its result through a callback. */
    virtual void asyncPing(TargetEcho* echo, PingCallback callBack) {
      PingParameters* params = new PingParameters;
      params->echo = echo;
      params->callback = callBack;
      xTaskCreate(
        pingTask,         // Tâche à exécuter
        "PingTask",       // Nom de la tâche
        4096*2,             // Taille de la pile de la tâche
        params,           // Paramètres de la tâche (adresse IP et callbacks)
        2,                // Priorité
        &pingTaskHandle   // Handle de la tâche
      );
      //DEBUG_PRINT("\nasyncPing(%s) task created",echo->host.c_str());
    }

    /** @brief Start a connection attempt without waiting in the application loop. */
    virtual bool asyncConnect(AsyncHttpClient *clientParam){      
      DEBUG_INFO("\t\t~.~.~.~.~.~.~.~.~.~. asyncConnect(%s,%i)",clientParam->host.toString().c_str(),clientParam->port);
      if (clientParam->host[3] == 255){
        DEBUG_WARN("\t=>broadcast, stop.");  
        return false;
      }

      if (clientParam->URL == ""){
        DEBUG_WARN("\t=>empty URL invalid, stop.");  
        return false;
      }

      // Si nous sommes déjà en train de tenter une connexion, on ne relance pas la tâche
      if (clientParam->isClientConnecting ) {
        DEBUG_WARN("\tconnection pending, no more attempt allowed, stop.");
        //clientParam->client->stop();
        //vTaskDelete(httpTaskHandle);
        //clientParam->taskHandle = nullptr;
        return false;
      }
      
      clientParam->client = client;
      clientParam->isClientConnecting = true;
      // Démarrer la tâche FreeRTOS pour la connexion
      xTaskCreatePinnedToCore(
          connectHTTPTask,     // Tâche à exécuter
          "ConnectTask",         // Nom de la tâche
          4096*2,                  // Taille de la pile (en octets)
          clientParam,           // Paramètre passé à la tâche (ici, l'objet courant)
          3,                     // Priorité de la tâche élevée
          NULL, //&clientParam->taskHandle,       // Pointeur vers l'identifiant de la tâche (inutile ici)
          0                      // Utilisation du 1er cœur de l'ESP32, qui est different de la boucle principale afin de ne pas perturber l'affichage
      );
      
      return true;
    }

    /** @brief Schedule an internet reachability check when required. */
    void checkInternet(){
      //DEBUG_PRINT("\n\n\tcheckInternet => ");
      // Utiliser des lambdas pour passer les méthodes membres comme callbacks
      asyncPing(
        &internet,
        [&](TargetEcho* echo) { onInternetPing(echo); }
      );
      // latency
      //if (LAN) 
      //LANlatency = 20;
      //else 
    }

    /** @brief Restore the preferred Wi-Fi network from saved settings. */
    void restoreWiFiSSID(){
      // next WiFi attempt will start with the last one that was connected
      //_newWifiAttempt = true; // reset flag for immediate reconnection at the deco.
      SSID = lastConnectedSSID;          
      //DEBUG_PRINT("\n         + - + - + restoring SSID=%s",SSID.c_str());
    }
/*    void test(){
      if (client->connect("httpbin.org", 80)) {
        DEBUG_PRINT("\nclient Connected!");
        // Make a HTTP request:
        client->println("GET /get HTTP/1.1");
        client->println("Host: httpbin.org");
        client->println("Connection: close");
        client->println();
      } else {
        // if you didn't get a connection to the server:
        DEBUG_PRINT("\nclient connection failed");
      }      
    }
*/
   
   /* void scanWifi(){
      Serial.println("Scan start");
  
      // WiFi.scanNetworks will return the number of networks found.
      int n = WiFi.scanNetworks();
      Serial.println("Scan done");
      if (n == 0) {
          Serial.println("no networks found");
      } else {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.printf("%2d",i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4d", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2d", WiFi.channel(i));
            Serial.print(" | ");
            switch (WiFi.encryptionType(i))
            {
            case WIFI_AUTH_OPEN:
                Serial.print("open");
                break;
            case WIFI_AUTH_WEP:
                Serial.print("WEP");
                break;
            case WIFI_AUTH_WPA_PSK:
                Serial.print("WPA");
                break;
            case WIFI_AUTH_WPA2_PSK:
                Serial.print("WPA2");
                break;
            case WIFI_AUTH_WPA_WPA2_PSK:
                Serial.print("WPA+WPA2");
                break;
            case WIFI_AUTH_WPA2_ENTERPRISE:
                Serial.print("WPA2-EAP");
                break;
            case WIFI_AUTH_WPA3_PSK:
                Serial.print("WPA3");
                break;
            case WIFI_AUTH_WPA2_WPA3_PSK:
                Serial.print("WPA2+WPA3");
                break;
            case WIFI_AUTH_WAPI_PSK:
                Serial.print("WAPI");
                break;
            default:
                Serial.print("unknown");
            }
            Serial.println();
            delay(10);
        }
      }
      Serial.println("");
  
      // Delete the scan result to free memory for code below.
      WiFi.scanDelete();
    }*/

  private:
    byte ethMac[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //bool _newWifiAttempt = false;    // set to false to allow ether connection first to start
    bool  webAppOn = false;
    unsigned long lastWiFiAttempt = 0, lastEthAttempt = 0;
    uint8_t rstPin;
    static WiFiEth* instance ;

    /* 
      THREAD MANAGING SOCKET CONNECTION
    */
    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    static void socketOpenTaskEntry(void *param) {
      WiFiEth *self = static_cast<WiFiEth*>(param);
      if (self) {
        self->socketOpenTask();
      }
      vTaskDelete(nullptr);
    }

    /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
    void socketOpenTask() {
      DEBUG_INFO("%s::socketOpenTask()", id.c_str());

      bool success = false;

      if (xSemaphoreTake(networkBusy, pdMS_TO_TICKS(2000)) == pdTRUE) {
        if (media == CABLE_MEDIA) {
          if (wsEthClient != nullptr) {
            success = wsEthClient->open(
              pendingHost.c_str(),
              pendingPort,
              pendingPath.c_str()
            );
          }
        } else {
          if (wsWiFiClient != nullptr) {
            success = wsWiFiClient->open(
              pendingHost.c_str(),
              pendingPort,
              pendingPath.c_str()
            );
          }
        }

        xSemaphoreGive(networkBusy);
      } else {
        DEBUG_WARN("socketOpenTask: networkBusy timeout");
      }

      if (!success) {
        socketOpenJobState = SOCKET_JOB_FAILED;
      } else {
        socketOpenJobState = SOCKET_JOB_IDLE;
      }

      socketOpenTaskHandle = nullptr;
    }
};

WiFiEth* WiFiEth::instance = nullptr;

/**
 * @brief Fallback network component when networking is disabled.
 */
class  NoNetwork: public Network{
    
  public:

    /** @brief Initialize NoNetwork with the supplied configuration and bindings. */
    NoNetwork(bool debug):Network(0,debug){ 
      id = "NoNetwork~0";
      ethEnabled = false;
      WiFiEnabled = false;
    }

    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    bool refresh(bool force=false) {  
      return false;
    }

    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void setLink(EthernetLinkStatus linkStatus){}    
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path){return false;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void clientSocketSend(const char *message){}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE){}
    /**
     * @brief Report the current WebSocket connection state.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual WebSocketReadyState clientSocketState(){return WebSocketReadyState::CLOSED;}
    /**
     * @brief Return whether the network link is available.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual EthernetLinkStatus getLink(){return LinkOFF;}
    /**
     * @brief Return the connected peer address.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual String getConnectedDeviceIP(){return "";}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void asyncPing(TargetEcho* echo, PingCallback callBack){}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual bool asyncConnect(AsyncHttpClient *clientParam){return false;}
    /**
     * @brief Create a UDP transport for the selected network media.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual UDP* newUDP(){return nullptr;}
    /**
     * @brief Describe the W5500 socket allocations for diagnostics.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual String dumpW5500Sockets(){return "";}
    /**
     * @brief Stop the socket associated with a UDP slot.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual UdpSlot* stopUDPslot(int i){return nullptr;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void stopAllUDP(){}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual void releaseUDPSlot(int* i){}
    /**
     * @brief Return the address of this network connection.
     *
     * @note Networking is disabled; this implementation supplies a fallback/no-op result.
     */
    virtual String getIP() override{return "";}

};

/* debug wifi AP:
https://github.com/espressif/esp-idf/issues/6289
https://github.com/espressif/esp-idf/issues/6108
https://stackoverflow.com/questions/56078329/is-there-a-way-to-get-the-ip-address-that-assigned-to-the-new-sta-that-connected
https://community.platformio.org/t/timer-interrupt-and-wifi-events-dont-work-together/12833
https://www.esp32.com/viewtopic.php?t=6292

*/


/**
 * @brief Wi-Fi access point used for device configuration.
 */
class AccessPoint : public Network{
  public:

    /** @brief Initialize AccessPoint with the supplied configuration and bindings. */
    AccessPoint(const String* ssid, bool debug):Network(0,debug){ 
      id = "AccessPoint";
      media = WIFI_MEDIA;     
      SSID = concatStrings(*ssid, random(99)); // we create a different SSID each time to force the client (laptop) to see it as a new network and avoid asking to renew the DHCP lease, which oculd lead to failing getting an IP address.
      //setWifiPower();
      AP = true;
    }

    /** @brief Release resources held by this object. */
    ~AccessPoint(){
      //if(debug) DEBUG_PRINT("\n~AccessPoint()");
      WiFi.mode(WIFI_STA);        
      if (server != NULL){
        //DEBUG_PRINT("\nTRACE 22");
        delete server;
        //DEBUG_PRINT("\nTRACE 23");
      }
    }

    /** @brief Open the requested UDP slot and associate it with the service. */
    virtual UdpSlot* useUDPslot(int i){return nullptr;}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual int requestUDPslotID(int port){return 0;}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void stopAllUDP(){}
    /** @brief Stop the socket associated with a UDP slot. */
    virtual UdpSlot* stopUDPslot(int i){return nullptr;}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void releaseUDPSlot(int* i){}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void asyncPing(TargetEcho* echo, PingCallback callBack){}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     */
    virtual bool asyncConnect(AsyncHttpClient *clientParam){ return false;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always false.
     */
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path){return false;}
    /** @brief Send text through the current WebSocket connection. */
    virtual void clientSocketSend(const char *message){};
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE){}
    /** @brief Report the current WebSocket connection state. */
    virtual WebSocketReadyState clientSocketState(){return WebSocketReadyState::CLOSED;}
    /** @brief Create a UDP transport for the selected network media. */
    virtual UDP* newUDP(){return nullptr;}
    /** @brief Describe the W5500 socket allocations for diagnostics. */
    virtual String dumpW5500Sockets(){return emptyString;}

    /** @brief Prepare this object for use with its current configuration. */
    virtual void init(){
      #define VISIBLE 0
      #define MAX_AP_CONNECTION 1
      #define WIFI_CHANNEL 2
      Network::init();

      //pauseWifi();       // cut all previous connection but keep trying connecting
      stopWiFi();

      // Configuration mode is deliberately AP-only. The normal STA object is
      // paused by HardwareManager::startAP(), so keeping STA enabled here can
      // make the radio continue retrying stale credentials while the browser
      // is trying to associate with the configuration network.
      WiFi.mode(WIFI_AP);
      //WiFi.setTxPower(WIFI_POWER_19_5dBm);  // remets la puissance maximum
      WiFi.setMinSecurity(WIFI_AUTH_OPEN); // accepte tous les réseaux ouverts et sécurisés


      if (!WiFi.softAP(SSID,"",WIFI_CHANNEL,VISIBLE,MAX_AP_CONNECTION)) {
        DEBUG_ERROR("AccessPoint.softAP() failed for %s", SSID.c_str());
      }
      //WiFi.softAP(SSID,"");     // doesnt fix the AP stability issue    
      //setWifiPower();      


      getIP();
      WiFi.enableLongRange(true);
      DEBUG_INFO("%s.init (%s) %s",SSID.c_str(),id.c_str(),IP.toString().c_str());        
    }

    /** @brief Return the address of this network connection. */
    String getIP(){
      IP = WiFi.softAPIP();
      return IP.toString();
    }

    /** @brief Refresh this component and report whether its displayed state changed. */
    bool refresh(bool force=false) {      
      // the state is modified by onEvent()
      if (WiFi.softAPgetStationNum()>0){
        state = CONNECTED;      
      }else{
        state = DISCONNECTED;
      }
      Network::refresh(force);
      if (refreshed){
        if (state == CONNECTED){
          // Network::refresh() only sets refreshed on a state transition, so
          // this diagnostic is emitted once per association, not on every loop.
          String clientIP = getConnectedDeviceIP();
          String clientMAC = getConnectedDeviceMAC();
          DEBUG_INFO("AccessPoint client connected: MAC=%s IP=%s stations=%u",
                     clientMAC.length() ? clientMAC.c_str() : "unknown",
                     clientIP.length() ? clientIP.c_str() : "unknown",
                     WiFi.softAPgetStationNum());
          if (server == NULL) server = new WiFiServer(80,1);
          server->begin();
          evtNet.type = EVENT_NETWORK_CONNECTED;
          fireEvent(evtNet);
        }else{
          DEBUG_INFO("AccessPoint client disconnected; stations=%u",
                     WiFi.softAPgetStationNum());
          if (server != NULL){if (debug) DEBUG_INFO("DELETING SERVER");delete server; server  = NULL;}
          evtNet.type = EVENT_NETWORK_DISCONNECTED;
          fireEvent(evtNet);
        }
      }
      return refreshed;
    }

    /** @brief Return the MAC address of the first connected station. */
    virtual String getConnectedDeviceMAC(){
        wifi_sta_list_t wifi_sta_list = {};
        if (esp_wifi_ap_get_sta_list(&wifi_sta_list) != ESP_OK || wifi_sta_list.num == 0) {
            return emptyString;
        }

        char mac[18] = {};
        snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                 wifi_sta_list.sta[0].mac[0], wifi_sta_list.sta[0].mac[1],
                 wifi_sta_list.sta[0].mac[2], wifi_sta_list.sta[0].mac[3],
                 wifi_sta_list.sta[0].mac[4], wifi_sta_list.sta[0].mac[5]);
        return String(mac);
    }

    /** @brief Return the IP address of the first connected station. */
    virtual String getConnectedDeviceIP(){
        wifi_sta_list_t wifi_sta_list = {};
        tcpip_adapter_sta_list_t adapter_sta_list = {};
        if (esp_wifi_ap_get_sta_list(&wifi_sta_list) != ESP_OK || wifi_sta_list.num == 0) {
            return emptyString;
        }
        if (tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list) != ESP_OK ||
            adapter_sta_list.num == 0) {
            return emptyString;
        }

        ip4_addr_t ip;
        ip.addr = adapter_sta_list.sta[0].ip.addr;
        const char* address = ip4addr_ntoa(&ip);
        return address ? String(address) : emptyString;
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void setLink(EthernetLinkStatus linkStatus){}
    /** @brief Return whether the network link is available. */
    virtual EthernetLinkStatus getLink(){return LinkOFF;}      // access point has no physicalLink

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void stop(){
      // do stuff to stop AP
      //WiFi.setTxPower(WIFI_POWER_MINUS_1dBm); // power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/
    }

  private:

};

#endif
