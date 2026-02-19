#ifndef HWComponents_h
#define HWComponents_h

#include "Interval.h"
#include <Ethernet.h>       // apply following fix: https://github.com/arduino-libraries/Ethernet/pull/107/files
#include <EthernetUdp.h>
#include <WiFiGeneric.cpp>  // for tipipinit
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
using WSclientOnMessageCallback = std::function<void(const char*)>;
// using WSclientOnMessageCallback = std::function<void(IWebSocket &ws, const WebSocketDataType dataType,const char *message, uint16_t length)>;

////////////////////////////////////////////////////////////////////
///                MIDI LAYER OBJECTS DECLARATION            ///////
////////////////////////////////////////////////////////////////////
#include "interfaces/appleMidiHelpers.h"
#define MIDI_CHANNEL_OMNI 0
/*#define APPLE_MIDIETHCLIENT_NAME  "interval"    
#define APPLE_MIDIWIFICLIENT_NAME  "interval"    */
#define MDNS_ANNOUNCED_APPLEMIDI_SERVICE "apple-midi"   // standard name for apple midi. Do not change
//APPLEMIDI_CREATE_INSTANCE(EthernetUDP,MIDI_RTP_ETH,"Interval-footswitch-Ethernet",DEFAULT_CONTROL_PORT);
//APPLEMIDI_CREATE_INSTANCE(WiFiUDP,MIDI_RTP_WIFI,"Interval-footswitch-WiFi",DEFAULT_CONTROL_PORT);
//APPLEMIDI_CREATE_DEFAULTSESSION_INSTANCE();
using namespace APPLEMIDI_NAMESPACE;
/*
AppleMIDISession<WiFiUDP> *appleMidiSession;
MidiInterface<AppleMIDISession<WiFiUDP>,AppleMIDISettings> *myMidi;
void OnAppleMidiException(const ssrc_t& ssrc, const Exception& e, const int32_t value );
*/
////////////////////////////////////////////////////////////////////
///                CLIENT SOCKET CALLBACKS                  ///////
////////////////////////////////////////////////////////////////////
void defaultWSclientOnMessage(const char* message){
  //DEBUG_PRINT("\nWebsocketclient Message reçu : %s",message);
}

WSclientOnMessageCallback userWSclientOnMessage = defaultWSclientOnMessage;

void socketClientOnOpenCallback(IWebSocket &ws) {
  //DEBUG_PRINT("\nConnecté au serveur WebSocket !");
  /*const char message[]{ "Hello from Arduino client!" };
  ws.send(WebSocketDataType::TEXT, message, strlen(message));  */
}

void onEthernetBonjourServiceFoundCallback(const char* serviceType,
                           MDNSServiceProtocol_t proto,
                           const char* serviceName,
                           const byte ipAddr[4],
                           unsigned short port,
                           const char* txt);

void socketClientOnMessageCallback(IWebSocket &ws, const WebSocketDataType dataType,const char *message, uint16_t length){
  switch (dataType) {
    case net::WebSocketDataType::TEXT:
      userWSclientOnMessage(message);
      break;
    case net::WebSocketDataType::BINARY:
      //DEBUG_PRINT("\nDonnées binaires reçues");
      break;
  }                     
}

void socketClientOnErrorCallback(WebSocketError error) {
  //DEBUG_PRINT("\nErreur WebSocket : %d",static_cast<int>(error));
}

void socketClientOnClose(IWebSocket &ws, WebSocketCloseCode code, const char *reason,uint16_t length){
  //DEBUG_PRINT("\nWebSocket closed: %d %s",static_cast<int>(code),reason);
}

////////////////////////////////////////////////////////////////////////////////////////////
void pingTask(void*);
void connectHTTPTask(void*);
////////////////////////////////////////////////////////////////////////////////////////////

class HWComponent: public GenericComponent, public EventDispatcher{
  public:
    uint8_t CSPin;
    //Event evt;  // in EventDispatcher

    HWComponent(uint8_t pin, boolean d=true):GenericComponent(d){
      CSPin = pin;
    }

    virtual void stop(){
      //DEBUG_PRINT("\n%s stop()",id.c_str());
    }
};

class HWComponent2State : public HWComponent{
  public: 
    bool state;
    bool previousState;

    HWComponent2State(uint8_t pin, bool s = false, boolean d=false):HWComponent(pin,d){      
      state = s;
      previousState = state;
    }

    bool refresh(bool force=false){
      refreshed = false;  // reset previous state
      if (previousState != state || force){
        previousState = state;
        if (debug){
          DEBUG_PRINT("\n%s changed state to: %i",id.c_str(),state);
        }
        refreshed = true;
      }
      return refreshed;
    }

};

class DigitalOut : public HWComponent2State{
  public:

    DigitalOut(uint8_t p,bool s = HIGH): HWComponent2State(p,s){
      pinMode(CSPin, OUTPUT);
      id = concatStrings("DigitalOut~", p);
      refresh(true);  //force the state
    }

    bool refresh(bool force=false){
      HWComponent2State::refresh(force);
      if (refreshed){
        digitalWrite(CSPin, state);
      }
      return refreshed;
    }

    virtual void init(){
      HWComponent2State::init();
    }

};

class AnalogIn : public HWComponent{
  private:
    uint16_t refreshPercentageThreshold = 1;  // if value change of 1% it will be considered as change
  
  public:
    uint16_t value=0;

    AnalogIn(uint8_t p, uint16_t t=0, boolean d=true): HWComponent(p,d){
      refreshPercentageThreshold = t;
      pinMode(CSPin, INPUT);
      id = concatStrings("AnalogIn~", p);
    }
    
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

    virtual void init(){
      HWComponent::init();
      refresh();
    }

};

#define VBAT_CHANGE_THRESHOLD 0.06

#define VBAT_CRIT 3.2     // selon chat gpt => tension minimal. En dessous de 3.0v , dommages irreversibles
#define VBAT_MIN 3.3      // selon chat gpt => tension a surveiller, seuil de pré-alarme
#define VBAT_WARNING VBAT_MIN-VBAT_CHANGE_THRESHOLD
#define VBAT_MAX 4.2      // selon chat gpt => tension maximale
#define VBAT_USB 4.6      // 

#define VBAT_OFFSET 0.0 // compensate voltage drop (positive or negative) from the diode or other component

#define BAT_REFRESH_RATE 500 // refresh every 500ms
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
    BAT_DISCHARGING,
};

class BatteryMonitor : public HWComponent{
  // read https://forum.seeedstudio.com/t/xiao-ble-sense-mbed-2-7-2-battery-charge-and-voltage-monitor-analogread-p0-31-does-not-work/266438

  public:
    uint8_t indicator = BAT_FULL_LEVEL;
    float vBat = VBAT_MAX;
    // Global variables for tracking battery state
    float previousVBat = 0.0;
    BatteryState batteryState = BAT_CHECKING;


    BatteryMonitor(uint8_t p, boolean d=true): HWComponent(p,d){
      pinMode(CSPin, INPUT);
      id = concatStrings("BatteryMonitor~", p);
    };

    virtual void init(){
      HWComponent::init();
      for (readIndex = 0; readIndex < BAT_NUM_READINGS-1; readIndex++) read();
      refresh(true);  // force first reading
    }

    void read(){
      readings[readIndex] = analogReadMilliVolts(CSPin);     
      readSum = readSum + readings[readIndex];      
      //if (debug) DEBUG_PRINT("\nreadings[%i]:%i",readIndex,readings[readIndex]);
    }

    bool refresh(bool force=false){
      refreshed = force;      

      if (millis()-lastReadMillis<BAT_REFRESH_RATE && !force) return refreshed;
      lastReadMillis = millis();

      readSum = readSum - readings[readIndex];
      read();
      readIndex = (readIndex + 1) % BAT_NUM_READINGS;

      uint32_t averageReading = readSum / BAT_NUM_READINGS;  

      float val = 2 * averageReading / 1000.0; 
      val += VBAT_OFFSET;

      // rounding to 2 digit after decimal place
      float delta = abs(val-vBat);          
      /*int c = (int)(delta * 1000 + .5);
      float roundedDelta = c/1000.0;             */

      if (delta>=VBAT_CHANGE_THRESHOLD || force){    // 0.06 we only consider change of 0.6mv
        vBat = val;
        
        previousVBat = vBat;
        if (debug){
          //DEBUG_PRINT("\n%s changed to: %.3f readSum:%i averageReading:%i delta:%.3f",id.c_str(),vBat,readSum, averageReading,/*roundedDelta*/delta);          
        }        
        
        indicator = mapFloatToInt(vBat,VBAT_MIN,VBAT_MAX,0,BAT_MAX_LEVEL-1);
        if (indicator > BAT_MAX_LEVEL-1) indicator = 0;          
       
        refreshed = true;
      }     

      if (readIndex == BAT_NUM_READINGS-1/* && batteryState == BAT_CHECKING*/){ 
        // we have enough readings to get a good average 
        batteryState = determineBatteryState(vBat, previousVBat,VBAT_CHANGE_THRESHOLD/2);

        if (debug){
          //DEBUG_PRINT("\n %ldms battery.status:%d %.2fv",millis(),batteryState,vBat);
        }
        refreshed = true;
      }

      return refreshed;
    }

  private:
    uint32_t readings[BAT_NUM_READINGS];      // the readings from the analog input
    int readIndex = 0;              // the index of the current reading
    uint32_t readSum = 0;                   // the running readSum
    unsigned long lastReadMillis = 0;

    BatteryState determineBatteryState(float currentVBat, float previousVBat, float threshold) {
      if (vBat>VBAT_USB) return BAT_USB_PLUGGED;
      /*float delta = currentVBat - previousVBat;
      if (delta > threshold) {
          return BAT_USB_PLUGGED;
      } else {
          return BAT_DISCHARGING;
      }*/
      return BAT_DISCHARGING;
    }    
};


#define DEBOUCE_TIME 25

#define DCLIC_DETECTION_PUSHDOWNMAXTIME LONG_PUSH_DELAY-1       // pour detecter un double clic, le premier clic doit etre court   200

// Structure pour les événements complexes
struct ButtonEvent {
    uint8_t id;
    uint8_t buttonId;
    unsigned long time; // en ms
};


#define DEFAULT_PUSH_DELAY 800
#define DEFAULT_DCLIC_DELAY 250

namespace Interval {
class Button : public HWComponent2State{
  public:

    enum EventID {
      BUTTON_SIMPLE_CLIC=1,
      BUTTON_DOUBLE_CLIC,
      BUTTON_LONG_CLIC,
      BUTTON_HOLD,
      BUTTON_RELEASE_HOLD,     
    };

    unsigned long pushedTime = 0;
    bool doubleClic = false;
    bool usesLed = true;    
    uint16_t push_reactivity = DEFAULT_PUSH_DELAY, dClic_reactivity = DEFAULT_DCLIC_DELAY;    

    ButtonEvent btEvent;

    Button(uint8_t p, int e=LOW, boolean d=true, uint16_t push=DEFAULT_PUSH_DELAY, uint16_t dClic=DEFAULT_DCLIC_DELAY): HWComponent2State(p,!e,d){
       // we set the initial state to the opposite of the state we want to detect so it doesnt trigger the event at startup
      edge = e; //do we want to detect high or low
      push_reactivity = push;
      dClic_reactivity = dClic;
      evt.data = &btEvent;

      registerCallback([this](Event* event) {
          this->default_buttonCallBack(event);
      });

    }

    virtual void init(){
      HWComponent2State::init();
      /*DEBUG_PRINT("\n\t%s.push_reactivity=%d",id.c_str(),push_reactivity);      
      DEBUG_PRINT("\n\t%s.dClic_reactivity=%d",id.c_str(),dClic_reactivity);      */
      refresh();
    }

    void default_buttonCallBack(Event* event) {
        /*ButtonEvent* btnEvt = (ButtonEvent*) event->data;
        DEBUG_PRINT("\ndefault_buttonCallBack - %d => %d: %dms\n", btnEvt->id, btnEvt->buttonId, btnEvt->time);*/
    }

    bool refresh(bool force=false) {
      refreshed = force;
      if(millis()-startDebouceTime<DEBOUCE_TIME){
        //non blocking debouncer
        //DEBUG_PRINT("."); 
        return refreshed;
      }

      read();

      HWComponent2State::refresh(force);
      if (refreshed){         
        btEvent.buttonId = getID();                           
        startDebouceTime = millis();
        if (state != edge){ //released
          if (pushedTime<dClic_reactivity && !pendingDoubleClicDetection){            
            pendingDoubleClicDetection = true;    
            startTimeBetween2Clics = millis();                                                       
          }else{
            pendingDoubleClicDetection = false;
            if (!doubleClic){
              if (pushedTime>push_reactivity){                
                btEvent.time = pushedTime-push_reactivity;
                btEvent.id = BUTTON_RELEASE_HOLD;
              }else if (pushedTime>push_reactivity/3){
                btEvent.time = pushedTime;
                btEvent.id = BUTTON_LONG_CLIC;
              }else{
                btEvent.time = pushedTime;
                btEvent.id = BUTTON_SIMPLE_CLIC;
              }           
              fireEvent(&evt);
            }
          }
          
          if (usesLed) {
            ledcWrite(0,255);
          }

        }else { //pressed        
          doubleClic = false;
          startPushTime = millis();
          if (pendingDoubleClicDetection){
            doubleClic = true;
            btEvent.time = millis()-startTimeBetween2Clics;
            btEvent.id = BUTTON_DOUBLE_CLIC;
            fireEvent(&evt); 
            /*if (usesLed) {
              ledcWrite(0,0);   // if we want to light the double clic
            }*/           
          }
        }        
      }else{
        if (state == edge){
          pushedTime = millis()-startPushTime;        
          if (pushedTime>push_reactivity){
            btEvent.time = pushedTime-push_reactivity;
            btEvent.id = BUTTON_HOLD;
            fireEvent(&evt);
            if (usesLed){
              ledcWrite(0,255);
            }            
          }else if (!pendingDoubleClicDetection && usesLed && pushedTime<push_reactivity){
            uint8_t dutyCycle = map(pushedTime,0,push_reactivity,0,255);
            // Attacher le canal LEDC à la broche LED
            ledcWrite(0, dutyCycle);
          }
        }else{ 
          pushedTime = 0;
          if (millis()-startTimeBetween2Clics>dClic_reactivity && pendingDoubleClicDetection){
            pendingDoubleClicDetection = false;          
            if (!doubleClic){
              btEvent.time = pushedTime;
              btEvent.id = BUTTON_SIMPLE_CLIC;
              fireEvent(&evt);
            }         
          }  
        }      
      }
      return refreshed;
    }    
  


    virtual int getID() = 0;    
    virtual void setID(uint8_t id) = 0;    

  private:
    int edge; 
    bool pendingDoubleClicDetection = false;
    unsigned long startTimeBetween2Clics = 0,startPushTime = 0, startDebouceTime = 0;
    virtual void read() = 0;    
};
}

enum{
  NO_MEDIA,
  CABLE_MEDIA,
  WIFI_MEDIA
};

enum{
  DISCONNECTED = false,
  CONNECTED = true,
};

class DigitalButton: public Interval::Button{
  public:
  
    DigitalButton(uint8_t p, uint8_t mode=INPUT_PULLUP, int e=LOW, boolean d=false, uint16_t push = DEFAULT_PUSH_DELAY, uint16_t dClic = DEFAULT_DCLIC_DELAY):Button(p, e,d, push, dClic){      
      pinMode(p, mode); //
      id = concatStrings("DigitalButton~", getID());
      evt.type = EVENT_DIGITAL_BUTTON;
    }

    virtual int getID(){
      //DEBUG_PRINT("\ndigitalgetID(): %d",CSPin);
      return CSPin;
    }    

    void setID(uint8_t id){
      CSPin = id;
    }

  private:

    virtual void read(){
      //two consecutive reading to avoid false reading
      boolean input1 = digitalRead(CSPin);
      delay(1);
      boolean input2 = digitalRead(CSPin);
      if (input1 == input2) state = input1;      
    }

};

class LadderButton: public Interval::Button{
  public:
    static uint8_t bCount;
    uint8_t virtualID;

    LadderButton(AnalogIn* i, int e=HIGH,boolean d=true, uint16_t push = DEFAULT_PUSH_DELAY, uint16_t dClic = DEFAULT_DCLIC_DELAY):Button(i->CSPin, e,d,push, dClic){      
      input = i;
      pinMode(input->CSPin, INPUT); //analog
      virtualID = bCount;      
      id = concatStrings("LadderButton~", getID());
      bCount++;
      evt.type = EVENT_ANALOG_BUTTON;
      //input = new AnalogIn(p,d);
    }

    virtual int getID(){
      //DEBUG_PRINT("\nladdergetID(): %d",virtualID);
      return virtualID;
    }    

    void setID(uint8_t id){
      virtualID = id;
    }

  private:        
    AnalogIn* input;

    #define RESOLUTION 4095
    //https://www.ignorantofthings.com/2018/07/the-perfect-multi-button-input-resistor.html
    // https://docs.google.com/spreadsheets/d/1G6wTsAxZDnVvzIDBkg5LrhhY1T7q4p_pDSZ6owvBCMA/edit?usp=sharing
    // for 3 button with 1st resistor 47Kohm, other resistance should be: 24k, 68k


    virtual void read() {

      //input->refresh();
      float avg = RESOLUTION / float(bCount);
      if ((input->value > (bCount - 0.5) * avg) ){
        state = LOW;
      }else{
        //DEBUG_PRINT("\n%s val:%i",id.c_str(),input->value);

        int lowLimit = 0;
        int highLimit;

        /* measured values
          LEFT RANGE DETECTION: 0-683       ==> measured : 0
          CENTER RANGE DETECTION: 683-2048  ==> measured : 1326-1570
          RIGHT RANGE DETECTION: 2048-3413  ==> measured : 2660-2778

        */

        for (int i = 0; i < bCount; i++) {          
          if (i>0) lowLimit = round(((i-1) + 0.5) * avg);
          highLimit = round((i + 0.5) * avg);
          //DEBUG_PRINT("\t (%i,%i)",lowLimit,highLimit);
          if (input->value >= lowLimit && input->value <highLimit) { 
            if (i == virtualID){
              //DEBUG_PRINT("X");
              state = HIGH;
            } 
          }
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

//typedef std::function<void()> StdFunctionCallbackVoid; 
typedef std::function<void(const char*)> StdFunctionCallbackStringPointer;

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

struct UdpSlot {
  UDP* udp = nullptr;
  int port = -1;
  bool active = false;
  uint8_t media = CABLE_MEDIA;
  uint8_t reservations = 0;

  void reset() {
    udp = nullptr;
    port = -1;
    active = false;
    media = NO_MEDIA;
    reservations = 0;
  }  
};

class Network : public HWComponent2State{

  public:
    WebSocketClient<WiFiClient> *wsWiFiClient{ nullptr };
    WebSocketClient<EthernetClient> *wsEthClient{ nullptr };

    float networkConnectAttemptProgress = 0.0;

    SemaphoreHandle_t networkBusy;

    StdFunctionCallbackVoidPingEcho internetCallback = nullptr;    
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
    PingEcho internet = PingEcho("8.8.8.8"); 
    uint16_t LANlatency = 38;

    bool ethEnabled = true;
    bool WiFiEnabled = true;
    String SSID="undef",lastConnectedSSID="";
    bool AP = false;

    Network(uint8_t pin, bool debug):HWComponent2State(pin,DISCONNECTED,debug){
      id = concatStrings("Network~", pin);
      networkBusy = xSemaphoreCreateMutex();
    }

    virtual UdpSlot* useUDPslot(int i) = 0;
    virtual int requestUDPslotID(int port) = 0;
    virtual UDP* newUDP() = 0;
    virtual void stopAllUDP() = 0;
    virtual UdpSlot* stopUDPslot(int i) = 0;
    virtual void releaseUDPSlot(int* i)=0;
    virtual void setLink(EthernetLinkStatus linkStatus) = 0;    
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path)=0;
    virtual void clientSocketSend(const char *message)=0;
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE)=0;
    virtual WebSocketReadyState clientSocketState()=0;
    virtual EthernetLinkStatus getLink() = 0;
    virtual String getConnectedDeviceIP() = 0;
    virtual void asyncPing(PingEcho* echo, PingCallback callBack)=0;
    virtual bool asyncConnect(AsyncHttpClient *clientParam)=0;
    virtual String dumpW5500Sockets()=0;
    
    virtual void init(){
      //if (debug) DEBUG_PRINT("\n%s.init()",id.c_str());
      WiFi.setAutoConnect(false); 
      WiFi.setAutoReconnect(false);      
    }

    virtual void setSSID(String ssid, String pwd){
      SSID = ssid;
      wiFiList->addValue(SSID,pwd);         
    }
    

    virtual void setWiFi(JsonList* list, String wiFiID){
      wiFiList = list;
      wiFiIndex = wiFiList->getIndex(wiFiID);
      SSID = wiFiID;
      lastConnectedSSID = SSID;
    }



    void setWifiPower(){
      //#define WIFI_POWER WIFI_POWER_MINUS_1dBm // Set WiFi RF power output to lowest level      
      //#define WIFI_POWER WIFI_POWER_11dBm // Set WiFi RF power output to middle level
      //#define WIFI_POWER WIFI_POWER_19_5dBm // Set WiFi RF power output to highest level        
      #define WIFI_POWER WIFI_POWER_8_5dBm    // Set WiFi RF power output to standard level        

      //WiFi.setTxPower(WIFI_POWER); // maximum power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/ 
    }

    uint16_t wiFiTimeout = WIFI_CHECK_TIMEOUT_MIN;

    void shiftConnectTimeout() {
        wiFiTimeout += 3000;
        if (wiFiTimeout > WIFI_CHECK_TIMEOUT_MAX) {
            wiFiTimeout = WIFI_CHECK_TIMEOUT_MIN;
        }
    }

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

    void stopWiFi(){
      //DEBUG_PRINT("\n---------> stopWiFi() : WiFiEnabled = false <------------");
      WiFiEnabled = false;
      disconnectWifi();
    }

    void unpauseWiFi(){
      //DEBUG_PRINT("\n\t\t\t\t---------> unpauseWiFi() : WiFiEnabled = true <------------");
      WiFiEnabled = true ;      
    }

 
  protected:
    JsonList* wiFiList;
    uint8_t wiFiIndex;
    //String oldSSID = "";

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

class WiFiEth : public Network{
  public:
    EthernetLinkStatus cable = Unknown, oldCable = Unknown;
    unsigned long checkEthCableTime=0;

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

    virtual void init(){
      Network::init();
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
          if (debug) DEBUG_PRINT("\n%s !!! THERE IS NO ETHERNET HW !!!",id.c_str());
        }else{
          if (debug) DEBUG_PRINT("\n%s Using WiFi & Ethernet",id.c_str());
          DEBUG_PRINT("\nregistering onEthernetBonjourServiceFoundCallback");
          EthernetBonjour.setServiceFoundCallback(onEthernetBonjourServiceFoundCallback);         
          setLink(Ethernet.linkStatus());   // check immediately if ethernet is connected so it has priority on WiFI connection    => done in the thread
        }
      }else{
        //if (debug) DEBUG_PRINT("\n%s Using WiFi only",id.c_str());
      }
     
      if (!ethEnabled){
        // power off the ethernet
        stopEthernet();
      }
      wipeWifiConfig();

      // we start with Ethernet priority so reset WiFi counter timer so we let time to ethernet to startup
      lastWiFiAttempt = millis();      

    }    

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

    void reset(){
      digitalWrite(rstPin, LOW);  // Maintenez le W5500 en état de reset
      delay(1);  // Maintenir le RESET bas pendant au moins 500 ns (nous utilisons ici 1 ms pour être sûr)
      digitalWrite(rstPin, HIGH);  // Libérer le RESET du W5500
      delay(200);  // Attendez au moins 200 ms pour que le W5500 termine son initialisation interne
    }

    #include <utility/w5100.h>
    /*
    Bonjour / mDNS	    1 UDP
    Serveur HTTP	      2 = 1 LISTEN + 1 ESTABLISHED par client
    WebSocket	          1 TCP
    Behringer OSC       1 UDP       
    OSC natif           1 UDP
    Mixing station OSC  1 UDP
    Mixing station sock 1 TCP
    AppleMIDI           1 UDP
    Artnet              1 UDP
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
              "\t\t\t\t\t | FREE=%d TCP=%d UDP=%d",
              freeCount,
              tcpCount,
              udpCount);

      return String(buffer);
    }

    #define MAX_UDP_SLOT 4
    UdpSlot udpSlots[MAX_UDP_SLOT];   // max 4 objets

    bool isValidUDPslotIndex(int i) const {
      return i >= 0 && i < MAX_UDP_SLOT;
    }

    void destroyUDPslot(int i) {
      if (!isValidUDPslotIndex(i)) return;
      if (udpSlots[i].udp != nullptr) {
        udpSlots[i].udp->stop();
        delete udpSlots[i].udp;
      }
      udpSlots[i].reset();
    }

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
    

    virtual UDP* newUDP(){
      // Créer l'objet UDP du bon type
      if (media == CABLE_MEDIA)
         return new EthernetUDP;
      else
        return new WiFiUDP;      
    }

    virtual int requestUDPslotID(int port) {

      if (port <= 0 || port > 65535) {
        DEBUG_PRINT("\n[requestUDPslotID] invalid port %d", port);
        return -1;
      }

      // 1) Si un slot existe déjà pour ce port, on le réutilise ou on le remplace selon media
      for (int i = 0; i < MAX_UDP_SLOT; i++) {
        if (udpSlots[i].port == port && udpSlots[i].reservations > 0) {
          if (!ensureUDPslotMedia(i)) {
            DEBUG_PRINT("\n[UDP-%d/%d] ensureUDPslotMedia failed", i, port);
            return -1;
          }
          udpSlots[i].reservations++;
          DEBUG_PRINT("\n[UDP-%d/%d] ID reclaimed (refs=%d)", i, port, udpSlots[i].reservations);
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
            DEBUG_PRINT("\n[UDP] newUDP() failed");
            return -1;
          }

          DEBUG_PRINT("\n[UDP-%d/%d] ID created", i, port);
          return i;
        }
      }

      DEBUG_PRINT("\n[UDP] Reached max!");
      return -1;
    }

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
          DEBUG_PRINT("\n[UDP-%d/%d] begin()", i, udpSlots[i].port);                     
        }
      }
      return &udpSlots[i];
    }

    virtual void releaseUDPSlot(int* i){
      if (i == nullptr) return;
      if (!isValidUDPslotIndex(*i)) {
        *i = -1;
        return;
      }

      UdpSlot& slot = udpSlots[*i];
      if (slot.reservations > 0) slot.reservations--;

      if (slot.reservations == 0) {
        DEBUG_PRINT("\n[UDP-%d/%d] released (destroy)", *i, slot.port);
        destroyUDPslot(*i);
      } else {
        DEBUG_PRINT("\n[UDP-%d/%d] released (refs=%d)", *i, slot.port, slot.reservations);
      }

      *i = -1;   // invalidate slot ID
    }

    virtual UdpSlot* stopUDPslot(int i){
      
      if (!isValidUDPslotIndex(i)){
        DEBUG_PRINT("\nstopUDP(%i) out of range", i);
        return nullptr;
      }

      if (udpSlots[i].udp != nullptr){
        udpSlots[i].udp->stop();
        udpSlots[i].active = false;
        DEBUG_PRINT("\n[UDP-%d/%d] stopped", i, udpSlots[i].port);
        return &udpSlots[i];
      }else{
        DEBUG_PRINT("\n[UDP-%d/%d] stopped failed", i, udpSlots[i].port);        
      }

      return nullptr;

    }

    virtual void stopAllUDP() {
      for (int i = 0; i < MAX_UDP_SLOT; i++) {
        if (udpSlots[i].udp != nullptr && udpSlots[i].reservations > 0){
          stopUDPslot(i);
        }
      }
    }

    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path){
      DEBUG_PRINT("\nclientSocketOpen(%s,%d,%s)",host,port,path);
      return false;
     
      if (state != CONNECTED){
        DEBUG_PRINT("\t failed, network not connected!");
        return false;
      } 
      if (media == CABLE_MEDIA){
        if (wsEthClient!= nullptr){
          if (!wsEthClient->open(host, port, path)){
            DEBUG_PRINT("\t open wsEthClient failed!");
            return false;
          }
        }
      }else{
        if (wsWiFiClient!= nullptr){
          if (!wsWiFiClient->open(host, port, path)){
            DEBUG_PRINT("\t open wsWiFiClient failed!");
            return false;
          }
        }
      }
      //DEBUG_PRINT("\t success");
      return true;
    }

    virtual void clientSocketSend(const char *message){
      if (media == CABLE_MEDIA){
        if (wsEthClient!= nullptr){      
          if (wsEthClient->getReadyState() == WebSocketReadyState::OPEN){
            wsEthClient->send(WebSocketDataType::TEXT, message, strlen(message));
            //DEBUG_PRINT("\nwsEthClientSend(%s)",message);
          }
        }
      }else{
        if (wsWiFiClient!= nullptr){      
          wsWiFiClient->send(WebSocketDataType::TEXT, message, strlen(message));
          //DEBUG_PRINT("\nwsWiFiClientSend(%s)",message);
        }
      }      
    }

    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE){
      const char reason[]={"application close"};
      if (media == CABLE_MEDIA){
        if (wsEthClient != nullptr){
          wsEthClient->close(code,true,reason,strlen(reason));
        }
      }else{
        if (wsWiFiClient != nullptr){
          wsWiFiClient->close(code,true,reason,strlen(reason));
        }        
      }
    }
    
    
    virtual WebSocketReadyState clientSocketState(){
      //DEBUG_PRINT("\nWebSocketReadyState()");
      if (media == CABLE_MEDIA){
        if (wsEthClient!= nullptr) return wsEthClient->getReadyState();
      }else{
        if (wsWiFiClient!= nullptr) return wsWiFiClient->getReadyState();
      }
      //DEBUG_PRINT(": CLOSED");
      return WebSocketReadyState::CLOSED;      
    }


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

    virtual void setLink(EthernetLinkStatus linkStatus){
      cable = linkStatus;
      checkEthCableTime = millis();
    }

    virtual EthernetLinkStatus getLink(){
      return cable;
    }

    void startWifi(String ssid, String pwd){
      /*WiFi.disconnect(true);
      delay(500);
      WiFi.setSleep(false); // Désactiver le mode veille Wi-Fi
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reconfigurer DHCP    */

      DEBUG_PRINT("\nstartWifi(%s,%s), wifiEnabled=%d, status: %d, MAC:%s",SSID.c_str(),pwd.c_str(),WiFiEnabled,WiFi.status(),WiFi.macAddress().c_str());                
      // Hostname avant begin
      //WiFi.setHostname("esp32-client");
      // Pas d’économie d’énergie pendant les tests
      //esp_wifi_set_ps(WIFI_PS_NONE);      
      //WiFi.disconnect(false,true);      
      WiFi.begin(ssid,pwd);
      lastWiFiAttempt = millis();      
      //_newWifiAttempt = false;      
    }

    void stopEthernet(){
      Network::stop();
      //if (debug) DEBUG_PRINT("\teth CSPin input");
      pinMode(CSPin, INPUT);
      // 
      pinMode(CSPin, OUTPUT);
      digitalWrite(CSPin, LOW);
    }

    virtual String getConnectedDeviceIP(){
      return "GW-IP";   
    }

    virtual void stop(){
      stopWiFi();     
      stopEthernet();
    }

    bool refresh(bool force=false) {
      
      if (millis()-checkEthCableTime > ETH_CABLE_CHECK_FREQUENCY && ethEnabled){   // only if we use the full network stack and not only AP mode
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
                IP = Ethernet.localIP();      
                subnet = Ethernet.subnetMask();   
                DEBUG_PRINT("\nEthernet.begin() => got IP:%s",IP.toString().c_str());  
                if (client == NULL) client = new EthernetClient();
                if (server == NULL) server = new EthernetServer(80); // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
                //if (udp == NULL) udp = new EthernetUDP;
                if (wsEthClient == nullptr){
                  wsEthClient = new EthernetWebSocketClient{}; 
                  wsEthClient->onOpen(socketClientOnOpenCallback);
                  wsEthClient->onMessage(socketClientOnMessageCallback);
                  wsEthClient->onError(socketClientOnErrorCallback);
                  wsEthClient->onClose(socketClientOnClose);                        
                }
                if (appleMidiClient == nullptr) appleMidiClient = new AppleMidiWithInterfaceWrapper<EthernetUDP>(/*APPLE_MIDIETHCLIENT_NAME*/systemCfg.getString(S_AP_NAME)->c_str(),DEFAULT_CONTROL_PORT);
                String midiName = concatStrings(appleMidiClient->getName(), "._", MDNS_ANNOUNCED_APPLEMIDI_SERVICE);
                String httpName = concatStrings(appleMidiClient->getName(), "._http");
                if (EthernetBonjour.begin(/*APPLE_MIDIETHCLIENT_NAME*/systemCfg.getString(S_AP_NAME)->c_str())){
                  EthernetBonjour.addServiceRecord(midiName.c_str(), appleMidiClient->getPort(),MDNSServiceUDP);
                  EthernetBonjour.addServiceRecord(httpName.c_str(), 80, MDNSServiceTCP);
                }else{
                  DEBUG_PRINT("\nEthernetBonjour registration <%s> failed", systemCfg.getString(S_AP_NAME)->c_str());
                }   
                state = CONNECTED;
                media = CABLE_MEDIA;                
                DEBUG_PRINT("\t...all Eth object instantiated\n");                  
                xSemaphoreGive(networkBusy);
              }else{
                DEBUG_PRINT("\nETH CONNECT: networkBusy\n"); 
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
              IP = WiFi.localIP();
              subnet = WiFi.subnetMask();
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
                wsWiFiClient->onOpen(socketClientOnOpenCallback);
                wsWiFiClient->onMessage(socketClientOnMessageCallback);
                wsWiFiClient->onError(socketClientOnErrorCallback);
                wsWiFiClient->onClose(socketClientOnClose);
              }
              if (appleMidiClient == nullptr) appleMidiClient = new AppleMidiWithInterfaceWrapper<WiFiUDP>(/*APPLE_MIDIWIFICLIENT_NAME*/systemCfg.getString(S_AP_NAME)->c_str(),DEFAULT_CONTROL_PORT);                     
              if (MDNS.begin(systemCfg.getString(S_AP_NAME)->c_str())){
                MDNS.addService(MDNS_ANNOUNCED_APPLEMIDI_SERVICE, "udp", appleMidiClient->getPort());
                MDNS.addService("http", "tcp", 80);                                
              }else{
                DEBUG_PRINT("\nMDNS registration <%s> failed",appleMidiClient->getName());
              }
              state = CONNECTED;
              media = WIFI_MEDIA;
              DEBUG_PRINT("\t...all WiFi object instantiated\n");                                
              xSemaphoreGive(networkBusy);
            }else{
              DEBUG_PRINT("\nWIFI CONNECT: networkBusy"); 
            }

          }
        }
      }else if (state == CONNECTED) {
        //ethernet has priority over wifi, hence if we are connected by WiFi but the cable is plugged, we disconnect WiFi
        if (media == WIFI_MEDIA && cable == LinkON){
          evt.type = EVENT_NETWORK_CHANGED_MEDIA; // inform immediately the webapp to terminate
          fireEvent(&evt);        
          disconnectWifi();
          state = DISCONNECTED; // cable will try reconnect at next refresh
          lastEthAttempt = 0; // with no delay
        }else{
          // *********************************************************************************
          // we are connected so do whatever is required to refresh common network object here
          // *********************************************************************************          
                          
          // then read incoming midi packets
          //if (appleMidiClient != nullptr) appleMidiClient->read();   // we prefer to put it into the AppleMIDIInterface monitor() as we will save some time in case interface is offline
          
          // finally do whatever is required to refresh specialized network object here
          if (media == CABLE_MEDIA){
            if (cable == LinkON){
              // do whatever is required when we are connected by cable
              if (wsEthClient != nullptr) wsEthClient->listen();
              EthernetBonjour.run();
            }else{
              state = DISCONNECTED; 
              media = NO_MEDIA;
            }
          }else{
            if (WiFi.status() == WL_CONNECTED){
              // do whatever is required when we are connected by WiFI
              // this code is executed in loop
              //ArduinoOTA.handle();     
              if (wsWiFiClient != nullptr) wsWiFiClient->listen();
            }else{
              // this code is executed in loop
              state = DISCONNECTED; 
              media = NO_MEDIA;     
            }
          }
        }
      }
      Network::refresh(force);
      if (cable != oldCable){
        oldCable = cable;
        refreshed = true;   // this refresh has to be flagged after NetworkRefresh to avoid being erased
      }

      if (refreshed){
        
        DEBUG_PRINT("\n%s changed state => cable:%i, media:%i",id.c_str(),cable,media);
        
        if (state == CONNECTED){
          // reset the progress indicator
          networkConnectAttemptProgress = -1;
          // initialiser of common object to both Wifi and Ethernet goes here      
          if (media == WIFI_MEDIA){
            SSID = WiFi.SSID();
            gateway = WiFi.gatewayIP();
            lastConnectedSSID = SSID;
            //DEBUG_PRINT("\n         + - + - saving lastConnectedSSID=SSID=%s",lastConnectedSSID.c_str());                          
            unpauseWiFi();       
          }else{
            // ETHERNET        
            gateway = Ethernet.gatewayIP();                                      
           }
          lastActiveMedia = media;
          checkInternet();
          DEBUG_PRINT("\n...NETWORK CONNECTED");
        }else if (state == DISCONNECTED){
          if (xSemaphoreTake(networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){          
            DEBUG_PRINT("\n...NETWORK DISCONNECTED");
            // common destruction
            internet.alive = false;
            internet.latency = 0;

            //if (udp != NULL){udp->stop(); delete udp; udp = NULL;}           
            if (client != NULL){client->stop(); delete client; client = NULL;} 
            if (server != NULL){delete server; server  = NULL;} 
            if (wsWiFiClient != nullptr){delete wsWiFiClient; wsWiFiClient = nullptr;}
            if (wsEthClient != nullptr){ delete wsEthClient; wsEthClient = nullptr;}
            if (appleMidiClient != nullptr){ appleMidiClient->endSession(); delete appleMidiClient; appleMidiClient = nullptr;}
            
            // specific destruction to WIFI or ETH
            if (lastActiveMedia == CABLE_MEDIA){
              // eth
              EthernetBonjour.stop();
            }else if (lastActiveMedia == WIFI_MEDIA){
              // wifi
              MDNS.end();
            }
            lastActiveMedia = NO_MEDIA;
            DEBUG_PRINT("\n...ALL NETWORK OBJECTED CLEANED");
            xSemaphoreGive(networkBusy);              
          }else{
            DEBUG_PRINT("\nnetwork DISCONNECTED: networkBusy"); 
          }
          restoreWiFiSSID();
        }
      }
      return refreshed;
    }

    void onInternetPing(PingEcho* echo) {
        if (internetCallback !=nullptr) internetCallback(echo);
        DEBUG_PRINT("\n\n\n\t\t\t * * * * onInternetPing %s latency:%i * * * *",echo->alive?"OK":"fail",echo->latency);        
    }

    // Fonction pour démarrer la tâche de ping non bloquante
    virtual void asyncPing(PingEcho* echo, PingCallback callBack) {
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
      DEBUG_PRINT("\nasyncPing(%s) task created",echo->host.c_str());
    }

    virtual bool asyncConnect(AsyncHttpClient *clientParam){      
      DEBUG_PRINT("\n\t\t~.~.~.~.~.~.~.~.~.~. asyncConnect(%s,%i)",clientParam->host.toString().c_str(),clientParam->port);
      if (clientParam->host[3] == 255){
        DEBUG_PRINT("\t=>broadcast, stop.");  
        return false;
      }

      if (clientParam->URL == ""){
        DEBUG_PRINT("\t=>empty URL invalid, stop.");  
        return false;
      }

      // Si nous sommes déjà en train de tenter une connexion, on ne relance pas la tâche
      if (clientParam->isClientConnecting ) {
        DEBUG_PRINT("\tconnection pending, no more attempt allowed, stop.");
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

    void checkInternet(){
      DEBUG_PRINT("\n\n\tcheckInternet => ");
      // Utiliser des lambdas pour passer les méthodes membres comme callbacks
      asyncPing(
        &internet,
        [&](PingEcho* echo) { onInternetPing(echo); }
      );
      // latency
      //if (LAN) 
      //LANlatency = 20;
      //else 
    }

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
    int mixerPort = 10023;
    IPAddress mixerIP={192,168,0,255};
    bool  webAppOn = false;
    unsigned long lastWiFiAttempt = 0, lastEthAttempt = 0;
    uint8_t rstPin;
};

class  NoNetwork: public Network{
    
  public:

    NoNetwork(bool debug):Network(0,debug){ 
      id = "NoNetwork~0";
      ethEnabled = false;
      WiFiEnabled = false;
    }

    bool refresh(bool force=false) {  
      return false;
    }

    virtual void setLink(EthernetLinkStatus linkStatus){}    
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path){return false;}
    virtual void clientSocketSend(const char *message){}
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE){}
    virtual WebSocketReadyState clientSocketState(){return WebSocketReadyState::CLOSED;}
    virtual EthernetLinkStatus getLink(){return LinkOFF;}
    virtual String getConnectedDeviceIP(){return "";}
    virtual void asyncPing(PingEcho* echo, PingCallback callBack){}
    virtual bool asyncConnect(AsyncHttpClient *clientParam){return false;}
    virtual UDP* newUDP(){return nullptr;}
    virtual String dumpW5500Sockets(){return "";}
    virtual UdpSlot* stopUDPslot(int i){return nullptr;}
    virtual void stopAllUDP(){}
    virtual void releaseUDPSlot(int* i){}

};

/* debug wifi AP:
https://github.com/espressif/esp-idf/issues/6289
https://github.com/espressif/esp-idf/issues/6108
https://stackoverflow.com/questions/56078329/is-there-a-way-to-get-the-ip-address-that-assigned-to-the-new-sta-that-connected
https://community.platformio.org/t/timer-interrupt-and-wifi-events-dont-work-together/12833
https://www.esp32.com/viewtopic.php?t=6292

*/


class AccessPoint : public Network{
  public:

    AccessPoint(const String* ssid, bool debug):Network(0,debug){ 
      id = "AccessPoint";
      media = WIFI_MEDIA;     
      SSID = concatStrings(*ssid, random(99)); // we create a different SSID each time to force the client (laptop) to see it as a new network and avoid asking to renew the DHCP lease, which oculd lead to failing getting an IP address.
      //setWifiPower();
      AP = true;
    }

    ~AccessPoint(){
      //if(debug) DEBUG_PRINT("\n~AccessPoint()");
      WiFi.mode(WIFI_STA);        
      if (server != NULL){
        //DEBUG_PRINT("\nTRACE 22");
        delete server;
        //DEBUG_PRINT("\nTRACE 23");
      }
    }

    virtual UdpSlot* useUDPslot(int i){return nullptr;}
    virtual int requestUDPslotID(int port){return 0;}
    virtual void stopAllUDP(){}
    virtual UdpSlot* stopUDPslot(int i){return nullptr;}
    virtual void releaseUDPSlot(int* i){}
    virtual void asyncPing(PingEcho* echo, PingCallback callBack){}
    virtual bool asyncConnect(AsyncHttpClient *clientParam){ return false;}
    virtual bool clientSocketOpen(const char *host, uint16_t port, const char *path){return false;}
    virtual void clientSocketSend(const char *message){};
    virtual void clientSocketClose(WebSocketCloseCode code=WebSocketCloseCode::NORMAL_CLOSURE){}
    virtual WebSocketReadyState clientSocketState(){return WebSocketReadyState::CLOSED;}
    virtual UDP* newUDP(){return nullptr;}
    virtual String dumpW5500Sockets(){};

    virtual void init(){
      #define VISIBLE 0
      #define MAX_AP_CONNECTION 1
      #define WIFI_CHANNEL 2
      Network::init();

      //pauseWifi();       // cut all previous connection but keep trying connecting
      stopWiFi();

      #ifdef USE_NETWORK
        WiFi.mode(WIFI_AP_STA);        
        //if (debug) DEBUG_PRINT("\n WiFi.mode(WIFI_AP_STA)");        
      #else
        WiFi.mode(WIFI_AP);
        //if (debug) DEBUG_PRINT("\n WiFi.mode(WIFI_AP)");
      #endif     
      
      WiFi.mode(WIFI_AP_STA);
      //WiFi.setTxPower(WIFI_POWER_19_5dBm);  // remets la puissance maximum
      WiFi.setMinSecurity(WIFI_AUTH_OPEN); // accepte tous les réseaux ouverts et sécurisés


      WiFi.softAP(SSID,"",WIFI_CHANNEL,VISIBLE,MAX_AP_CONNECTION);         
      //WiFi.softAP(SSID,"");     // doesnt fix the AP stability issue    
      //setWifiPower();      


     IP = WiFi.softAPIP();
      WiFi.enableLongRange(true);
      DEBUG_PRINT("\nAccessPoint.init (%s) %s",SSID.c_str(),IP.toString().c_str());        
    }

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
          //if (debug) DEBUG_PRINT("\nCient connected to AP");
          if (server == NULL) server = new WiFiServer(80,1);         
          server->begin();
        }else{
          //if (debug) DEBUG_PRINT("\nCient disconnected from AP");
          if (server != NULL){if (debug) DEBUG_PRINT("\nDELETING SERVER");delete server; server  = NULL;} 
        }                
      }
      return refreshed;
    }

    virtual String getConnectedDeviceIP(){    // see https://www.esp32.com/viewtopic.php?t=6292
        wifi_sta_list_t wifi_sta_list;
        tcpip_adapter_sta_list_t adapter_sta_list;
        esp_wifi_ap_get_sta_list(&wifi_sta_list);
        tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[0];
        ip4_addr_t ip;
        ip.addr = station.ip.addr;
        return ip4addr_ntoa(&ip);
    }

    virtual void setLink(EthernetLinkStatus linkStatus){}
    virtual EthernetLinkStatus getLink(){return LinkOFF;}      // access point has no physicalLink

    virtual void stop(){
      // do stuff to stop AP
      //WiFi.setTxPower(WIFI_POWER_MINUS_1dBm); // power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/
    }

  private:

};

#endif
