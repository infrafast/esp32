#ifndef HWComponents_h
#define HWComponents_h

#include <Ethernet.h>
#include <EthernetUdp.h>
#include <WiFiGeneric.cpp>   // for tipipinit
#include <ArduinoOTA.h>           // for WIFI_MEDIA program upload 
#include <ESP32Ping.h>
#include <HttpClient.h> // amcewen/HttpClient@^2.2.0 as opposed to the std lib #include <HTTPClient.h> which only handle WiFIclient
#include <WiFiClientSecure.h>
#include "BluetoothSerial.h"   //required for OSC
// https://github.com/andrew-susanto/Arduino-Ethernet-Icmp.git	 => platform IO: works but corrupt the WiFi
//#include <EthernetICMP.h>

class HWComponent: public GenericComponent{
  public:
    uint8_t CSPin;

    HWComponent(uint8_t pin, boolean d=true):GenericComponent(d){
      CSPin = pin;
    }

    virtual void stop(){
      DEBUG_PRINT("\n%s stop()",id.c_str());
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
      id = "DigitalOut~"+String(p);
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
      id = "AnalogIn~"+String(p);
    }
    
    bool refresh(bool force=false){
      refreshed = force;
      uint16_t v = analogRead(CSPin);

      uint16_t deltaPercentage = 0;
      if (value!=0) deltaPercentage = abs(1.0-((float)v/(float)value))*100;
      else deltaPercentage = abs(1.0-((float)v/1.0))*100; // avoid divide per zero
      
      if (deltaPercentage >= refreshPercentageThreshold || force){
        if (debug){
          DEBUG_PRINT("\n%s (%d->%d)",id.c_str(),value,v);
        }
        value = v;        
        if (debug){
          DEBUG_PRINT("\tchanged to: %u (%d>%d)",value,deltaPercentage,refreshPercentageThreshold);
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

#define VBAT_CRIT 3.2
#define VBAT_MIN 3.3
#define VBAT_WARNING VBAT_MIN-VBAT_CHANGE_THRESHOLD
#define VBAT_MAX 4.2
#define VBAT_USB 4.6

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
    BAT_CHARGING,
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
      id = "BatteryMonitor~"+String(p);      
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
          DEBUG_PRINT("\n%s changed to: %.3f readSum:%i averageReading:%i delta:%.3f",id.c_str(),vBat,readSum, averageReading,/*roundedDelta*/delta);          
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
      if (vBat>VBAT_USB) return BAT_CHARGING;
      /*float delta = currentVBat - previousVBat;
      if (delta > threshold) {
          return BAT_CHARGING;
      } else {
          return BAT_DISCHARGING;
      }*/
      return BAT_DISCHARGING;
    }    
};


#define DEBOUCE_TIME 25

#define DCLIC_DETECTION_PUSHDOWNMAXTIME LONG_PUSH_DELAY-1       // pour detecter un double clic, le premier clic doit etre court   200

typedef void (*ButtonEventCallBack)(uint8_t , unsigned long );

enum{
  SIMPLE_CLIC,
  DOUBLE_CLIC,
  LONG_CLIC,
  HOLD,
  RELEASE_HOLD,
  MAX_CLIC_DEF
};

void default_simpleClicCallBack(uint8_t buttonID, unsigned long time){
  //DEBUG_PRINT("\n%i SIMPLE %u",buttonID,time );
}

void default_doubleClicCallBack(uint8_t buttonID, unsigned long time){
  //DEBUG_PRINT("\n%i DOUBLE %u",buttonID,time );
}

void default_longClicCallBack(uint8_t buttonID, unsigned long time){
  //DEBUG_PRINT("\n%i LONG %u",buttonID,time );
}

void default_holdCallback(uint8_t buttonID, unsigned long time){
  //DEBUG_PRINT("\n%i HOLD %u",buttonID,time );
}

void default_releaseHoldCallback(uint8_t buttonID, unsigned long time){
  //DEBUG_PRINT("\n%i RELEASED %u",buttonID,time );
}

void empty_button_callback(uint8_t buttonID, unsigned long time){
  // DEBUG_PRINT("\n%i RELEASED %u",buttonID,time );
}

#define DEFAULT_PUSH_DELAY 800
#define DEFAULT_DCLIC_DELAY 250

class Button : public HWComponent2State{
  public:
    unsigned long pushedTime = 0;
    bool doubleClic = false;
    bool usesLed = true;    
    uint16_t push_reactivity = DEFAULT_PUSH_DELAY, dClic_reactivity = DEFAULT_DCLIC_DELAY;    

    Button(uint8_t p, int e=LOW, boolean d=true, uint16_t push=DEFAULT_PUSH_DELAY, uint16_t dClic=DEFAULT_DCLIC_DELAY): HWComponent2State(p,!e,d){
       // we set the initial state to the opposite of the state we want to detect so it doesnt trigger the event at startup
      edge = e; //do we want to detect high or low
      push_reactivity = push;
      dClic_reactivity = dClic;
      clicCallBack[SIMPLE_CLIC] = default_simpleClicCallBack;
      clicCallBack[DOUBLE_CLIC] = default_doubleClicCallBack;
      clicCallBack[LONG_CLIC] = default_longClicCallBack;
      clicCallBack[HOLD] = default_holdCallback;
      clicCallBack[RELEASE_HOLD] = default_releaseHoldCallback;
    }

    virtual void init(){
      HWComponent2State::init();
      /*DEBUG_PRINT("\n\t%s.push_reactivity=%d",id.c_str(),push_reactivity);      
      DEBUG_PRINT("\n\t%s.dClic_reactivity=%d",id.c_str(),dClic_reactivity);      */
      refresh();
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
      
        startDebouceTime = millis();

        if (state != edge){ //released
          if (pushedTime<dClic_reactivity && !pendingDoubleClicDetection){            
            pendingDoubleClicDetection = true;    
            startTimeBetween2Clics = millis();                                                       
          }else{
            pendingDoubleClicDetection = false;
            if (!doubleClic){
              if (pushedTime>push_reactivity){
                 this->clicCallBack[RELEASE_HOLD](getID(),pushedTime-push_reactivity);                                       
              }else if (pushedTime>push_reactivity/3){
                 this->clicCallBack[LONG_CLIC](getID(),pushedTime);
              }else{
                this->clicCallBack[SIMPLE_CLIC](getID(),pushedTime);                
              }                                        
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
            this->clicCallBack[DOUBLE_CLIC](getID(),millis()-startTimeBetween2Clics);
            /*if (usesLed) {
              ledcWrite(0,0);   // if we want to light the double clic
            }*/           
          }
        }        
      }else{
        if (state == edge){
          pushedTime = millis()-startPushTime;        
          if (pushedTime>push_reactivity){
            this->clicCallBack[HOLD](getID(),pushedTime-push_reactivity); // for the callback, the time duration starts at 0 that why we dededuct HOLD_PUSH_DELAY * (*push_reactivity)
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
              //DEBUG_PRINT("\n\t%s simpleClic: %ums",id.c_str(),millis()-startTimeBetween2Clics);                  
              this->clicCallBack[SIMPLE_CLIC](getID(),pushedTime);      
            }         
          }  
        }      
      }
      return refreshed;
    }    
  
    void setCallBack(uint8_t button_event, ButtonEventCallBack button_function ){
      if (button_event>MAX_CLIC_DEF) return;
      clicCallBack[button_event] = button_function;
    }

    virtual int getID() = 0;    
    virtual void setID(uint8_t id) = 0;    

  private:
    int edge; 
    bool pendingDoubleClicDetection = false;
    unsigned long startTimeBetween2Clics = 0,startPushTime = 0, startDebouceTime = 0;
    ButtonEventCallBack clicCallBack[MAX_CLIC_DEF];
    virtual void read() = 0;    
};

enum{
  NO_MEDIA,
  CABLE_MEDIA,
  WIFI_MEDIA
};

enum{
  DISCONNECTED = false,
  CONNECTED = true,
};

class DigitalButton: public Button{
  public:
  
    DigitalButton(uint8_t p, uint8_t mode=INPUT_PULLUP, int e=LOW, boolean d=false, uint16_t push = DEFAULT_PUSH_DELAY, uint16_t dClic = DEFAULT_DCLIC_DELAY):Button(p, e,d, push, dClic){      
      pinMode(p, mode); //
      id = "DigitalButton~"+String(getID());
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

class LadderButton: public Button{
  public:
    static uint8_t bCount;
    uint8_t virtualID;

    LadderButton(AnalogIn* i, int e=HIGH,boolean d=true, uint16_t push = DEFAULT_PUSH_DELAY, uint16_t dClic = DEFAULT_DCLIC_DELAY):Button(i->CSPin, e,d,push, dClic){      
      input = i;
      pinMode(input->CSPin, INPUT); //analog
      virtualID = bCount;      
      id = "LadderButton~"+String(getID());
      bCount++;
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

typedef std::function<void(bool)> PingCallback; // Utilisation de std::function pour plus de flexibilité

struct PingTaskParameters {
  const char* ipAddress;
  PingCallback callback;
};

// Tâche de ping non bloquante
void pingTask(void* parameter) {
  // Récupérer les paramètres passés à la tâche
  PingTaskParameters* params = static_cast<PingTaskParameters*>(parameter);
  //DEBUG_PRINT("pingTask=>%s",params->ipAddress);
  if (Ping.ping(params->ipAddress)) {
    params->callback(true);  // Appel du callback en cas de succès
  } else {
    params->callback(false);  // Appel du callback en cas d'échec
  }

  // Libérer la mémoire allouée pour les paramètres
  delete params;

  // Suspendre la tâche après chaque ping (exécution ponctuelle)
  vTaskDelete(NULL);
}

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
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
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
    doc[HTTP_REST_ORIGIN] = url;

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
          doc[HTTP_REST_STATUS] = HTTP_REST_OK;
          doc[HTTP_REST_DATA] = jo;
        } else {           
          doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;
          doc[HTTP_REST_DATA] = "httpStatusCode: "+String(httpStatusCode);
        }
      } else {
        // ça veut dire pas de serveur a cette adresse
        doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;
        doc[HTTP_REST_DATA] = "Bad request: "+String(httpResponseCode);
      }  
      httpClient.endRequest();
      httpClient.stop();
    }else{
        // ça veut dire pas de serveur a cette adresse
        doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;
        doc[HTTP_REST_DATA] = "NULL client";
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

// Prototype pour la tâche de connexion
void connectHTTPTask(void *parameter) {
    AsyncHttpClient *clientObj = static_cast<AsyncHttpClient *>(parameter);
    DEBUG_PRINT("\nconnectHTTPTask(%s,%s,%i,<%s>)",clientObj->host.toString().c_str(),clientObj->URL.c_str(),clientObj->port,clientObj->post.c_str());
     
    Client* client = clientObj->client;  

    if (clientObj->method == HTTP_METHOD_GET) clientObj->post = ""; 

    //String result="{\""+String(HTTP_REST_ORIGIN)+"\":\""+clientObj->URL+"\",";

    JsonDocument doc;        
    doc[HTTP_REST_ORIGIN] = clientObj->URL;

    if (client != NULL){
      HttpClient httpClient(*client);      
      //int httpResponseCode = httpClient->get(serverIP.toString().c_str(),url.c_str());
      httpClient.beginRequest();
      httpClient.setHttpResponseTimeout(clientObj->timeOut);
      int httpResponseCode = httpClient.startRequest(clientObj->host.toString().c_str(),clientObj->port,clientObj->URL.c_str(),clientObj->method.c_str(),"ESP32HttpClient/1.0");     

      if (httpResponseCode == 0) {
        //DEBUG_PRINT("\nRequête %s lancée",method.c_str());

        if (clientObj->method == HTTP_METHOD_POST ){
          // Ajouter un en-tête Content-Type pour indiquer JSON
          httpClient.sendHeader("Content-Type", "application/json");
          httpClient.sendHeader("Content-Length", String(clientObj->post.length()).c_str());
          //DEBUG_PRINT("\nPOST write: %s",post.c_str());
          // Envoyer le corps de la requête
          httpClient.write((const uint8_t*)clientObj->post.c_str(), clientObj->post.length());               
        }

        // Terminer la requête
        httpClient.endRequest();

        int httpStatusCode = httpClient.responseStatusCode();
        DEBUG_PRINT("\nStatut HTTP : %d\n", httpStatusCode);

        if (httpStatusCode == 200) {
          httpClient.skipResponseHeaders();    // we want to read the body only

          readHttpBuffer.flush();
          while (httpClient.available()) { // Tant qu'il y a des données disponibles
            readHttpBuffer.write(httpClient.read());    // Lire un caractère
            yield();
          }

          JsonDocument payload;
          //deserializeJson(payload,httpClient);
          deserializeJson(payload,readHttpBuffer.buffer);
          doc[HTTP_REST_STATUS] = HTTP_REST_OK;
          doc[HTTP_REST_DATA] = payload;

          //DEBUG_PRINT("\nRéponse %s reçue : %s",clientObj->method.c_str(),payload.c_str());
        } else {           
          doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;
          doc[HTTP_REST_DATA] = String(httpStatusCode);
        }
      } else {
        doc[HTTP_REST_DATA] = String(httpResponseCode);
        doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;

      }
      httpClient.stop();    
    }else{
        doc[HTTP_REST_DATA] = "\"Null client\"";
        doc[HTTP_REST_STATUS] = HTTP_REST_ERROR;
    }

    //result += "\"status\":"+String(status)+",\"data\":"+String(payload)+"}";
    //result ="{\"origin\":\"/app/state\",\"status\":1,\"data\":{\"progress\": 0,\"state\": \"connected\"}}";

    //DEBUG_PRINT("\nsend => %s",result.c_str());


    // envoie la reponse
    serializeJson(doc,httpTaskResult);
    //clientObj->callback("{\"origin\":\"/app/state\",\"status\":1,\"data\":{\"progress\": 0,\"state\": \"connected\"}}");
    clientObj->callback(httpTaskResult.c_str());
    //DEBUG_PRINT("\nRESULT=>%s",result.c_str());
    // Fin de la tentative de connexion, réinitialiser l'indicateur
    //doc.clear();
    // Tâche terminée, suppression    
    //DEBUG_PRINT("\nWaiting semaphore release in connectTask");

    doc.clear();

    if (xSemaphoreTake(clientObj->mutex, portMAX_DELAY)) { // Prendre le mutex
      //DEBUG_PRINT("\nconnectHTTPTask finished");

      //if (clientObj->taskHandle != NULL) {
      //  clientObj->taskHandle = NULL;       // Mettre à jour le pointeur
      //}
      clientObj->isClientConnecting = false;
      xSemaphoreGive(clientObj->mutex); // Relâcher le mutex
      vTaskDelete(NULL); // Supprimer la tâche    
    }
}


class Network : public HWComponent2State{

  public:
    
    //IWebSocketServer *wss{ nullptr };
    UDP* udp = NULL;
    Server* server=NULL;
    uint8_t media = NO_MEDIA;
    IPAddress IP, gateway,subnet;
    //HttpClient* http = NULL;    
    Client* client = NULL;   
    bool internet = false; 
    uint16_t WANlatency = 250, LANlatency = 38;

    bool ethEnabled = true;
    bool WiFiEnabled = true;
    String SSID="undef",lastConnectedSSID="";
    bool AP = false;

    Network(uint8_t pin, bool debug):HWComponent2State(pin,DISCONNECTED,debug){
      id = "Network~"+String(pin);
    }

    virtual void setLink(EthernetLinkStatus linkStatus) = 0;
    virtual EthernetLinkStatus getLink() = 0;
    virtual String getConnectedDeviceIP() = 0;
    virtual void setUserInternetCallback(StdFunctionCallbackVoidBool uic)=0;
    virtual void setSSID(String ssid, String pwd){
      SSID = ssid;
      wiFiList->addValue(SSID,pwd);         
    }
    
    virtual void asyncPing(const char* ipAddress, PingCallback callback)=0;
    // Méthode pour initier la connexion non bloquante, appelée dans la boucle
    virtual bool asyncConnect(AsyncHttpClient *clientParam)=0;

    virtual void setWiFi(JsonList* list, String wiFiID){
      wiFiList = list;
      wiFiIndex = wiFiList->getIndex(wiFiID);
      SSID = wiFiID;
      lastConnectedSSID = SSID;
    }

    uint16_t getConnectTimeout(){
      return 7000;  //ms
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
      WiFiEnabled = false;
      disconnectWifi();
    }

    void unpauseWiFi(){
      //DEBUG_PRINT("\n\t\t\t\t---------> unpauseWiFi() <------------");
      WiFiEnabled = true ;      
    }

 
  protected:
    JsonList* wiFiList;
    uint8_t wiFiIndex;
    //String oldSSID = "";

    void disconnectWifi(){
      HWComponent2State::stop();
     
      if (WiFi.getMode() == WIFI_AP_STA){
        if (debug) DEBUG_PRINT("\tStopping STA, AP only"); 
        WiFi.mode(WIFI_AP);  // not required f we consider disconnectWifi is about shuting down STA, so no need to 
      }else if (WiFi.getMode() == WIFI_STA){
        if (debug) DEBUG_PRINT("\tWiFi off");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
      }else{
        // if we are in AP mode only, we do nothing
         if (debug) DEBUG_PRINT("\tNo Change on WiFI, already turned OFF (AP only)");
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
    StdFunctionCallbackVoidBool userInternetCallback = nullptr;

    WiFiEth(uint8_t pin, uint8_t rstP, bool enableEth = true, bool debug = true): Network(pin, debug){      
      id = "WiFiEth~"+String(pin);
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

    void init(){
      if (debug) DEBUG_PRINT("\n%s.init()",id.c_str());
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
          ethEnabled = false;
        }else{
          if (debug) DEBUG_PRINT("\n%s Using WiFi & Ethernet",id.c_str());
        }
      }else{
        if (debug) DEBUG_PRINT("\n%s Using WiFi only",id.c_str());
      }
      if (!ethEnabled){
        // power off the ethernet
        stopEthernet();
      }
    }    

    void reset(){
      digitalWrite(rstPin, LOW);  // Maintenez le W5500 en état de reset
      delay(1);  // Maintenir le RESET bas pendant au moins 500 ns (nous utilisons ici 1 ms pour être sûr)
      digitalWrite(rstPin, HIGH);  // Libérer le RESET du W5500
      delay(200);  // Attendez au moins 200 ms pour que le W5500 termine son initialisation interne
    }


    virtual void setSSID(String ssid, String pwd){
      Network::setSSID(ssid,pwd);
      DEBUG_PRINT("\nsetSSID(%s,%s)",ssid,pwd);                 
      if (media != CABLE_MEDIA){  // priority on Ethernet
        DEBUG_PRINT("\n    !!!! Disconnecting WiFI !!!!",ssid,pwd);     
        stopWiFi();
        state = DISCONNECTED;       
        startWifi(ssid, pwd);
      }
    }

    virtual void setLink(EthernetLinkStatus linkStatus){
      cable = linkStatus;
    }

    virtual EthernetLinkStatus getLink(){
      return cable;
    }

    void startWifi(String ssid, String pwd){
      /*WiFi.disconnect(true);
      delay(500);
      WiFi.setSleep(false); // Désactiver le mode veille Wi-Fi
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE); // Reconfigurer DHCP    */

      if (debug) DEBUG_PRINT("\nstartWifi(%s,%s)",SSID.c_str(),pwd.c_str());                
      WiFi.begin(ssid,pwd);
      lastWiFiAttempt = millis();      
      _newWifiAttempt = false;      
    }

    void stopEthernet(){
      Network::stop();
      if (debug) DEBUG_PRINT("\teth CSPin input");
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
      
      if (millis()-checkEthCableTime > 1000 && ethEnabled){   // only if we use the full network stack and not only AP mode
        setLink(Ethernet.linkStatus());
        checkEthCableTime = millis();
      }
      // the loop ensure that we always have one connection at the same time, if wifi is connected, ethernet will not connect and vice versa
      // ethernet has the priority on WiFi
      //cable = Ethernet.linkStatus();      
      //DEBUG_PRINT("\n:cable:%d",cable);
      if (state == DISCONNECTED){        
        // first try to connect eth

        if (cable == LinkON){       // ethernet is enabled  

          #define ETH_BEGIN_RETRY_DELAY 1000
          if (millis()-lastEthAttempt>ETH_BEGIN_RETRY_DELAY){

              lastEthAttempt = millis();

              bool success;
              
                /* IF CONFIG = STATIC IP {
                  IP = config.getIP();
                  success = Ethernet.begin(mac, IP, gateway, gateway, subnet);
                  }
                  else
                  */
              success = Ethernet.begin(ethMac,100,100);

              if (success){  // as it take 100ms, we only do it every ETH_BEGIN_RETRY_DELAY otherwise it really slows down the loop
                state = CONNECTED;
                media = CABLE_MEDIA;
                // save power            
                disconnectWifi();
                // we just connected ETH so we do everything related to ETH specialized object instanciation
                IP = Ethernet.localIP();         
                if (debug) DEBUG_PRINT("\nEthernet.begin() => got IP:%s",IP.toString().c_str());  
                if (client == NULL) client = new EthernetClient();
                if (server == NULL) server = new EthernetServer(80); // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
                if (udp == NULL) udp = new EthernetUDP;              
              }else{
                if (debug) DEBUG_PRINT(".");
              }
          }
        }else{
          if (WiFi.status() != WL_CONNECTED){
            if (_newWifiAttempt){
              //WiFi.begin("infrafast","Mf9k8iie2");
              if (WiFiEnabled){                   

                /* IF CONFIG = STATIC IP {
                  IP = config.getIP();
                  // Configurer l'adresse IP statique
                  if (!WiFi.config(IP, gateway, subnet)) {
                    DEBUG_PRINT("\nStatic WifI IP failed");
                  }
                }*/

                String pass = *wiFiList->getString(SSID);    // getting the password for the given ssid
                startWifi(SSID,pass);
              }              
            }
            if (millis()-lastWiFiAttempt>getConnectTimeout()){
              _newWifiAttempt = true;
              //next  credential to attempt
              wiFiIndex = (wiFiIndex + 1) % wiFiList->size();
              SSID = wiFiList->getName(wiFiIndex);              
            }            
            //if (debug) DEBUG_PRINT("~"); // I put that in OnWiFiEvent => SYSTEM_EVENT_STA_DISCONNECTED to avoid flooding the serial out
          }else{
            // we just connected WIFI_MEDIA so we do everything related to WIFI_MEDIA specialized object instanciation
            state = CONNECTED;
            media = WIFI_MEDIA;
            // save power and disconnect ethernet driver
            // TBC 
            IP = WiFi.localIP();
            if (debug) DEBUG_PRINT("\nWifi.begin() => got IP: %s, %idb",IP.toString().c_str(),WiFi.RSSI());
            if (client == NULL) client = new WiFiClient();
            if (server == NULL) server = new WiFiServer(80,1);
            if (udp == NULL) udp = new WiFiUDP;
          }
        }
      }else if (state == CONNECTED) {
        //ethernet has priority over wifi, hence if we are connected by WiFi but the cable is plugged, we disconnect WiFi
        if (media == WIFI_MEDIA && cable == LinkON){
          disconnectWifi();
          state = DISCONNECTED; // cable will try reconnect at next refresh
          lastEthAttempt = 0; // with no delay
        }else{
          // we are connected so do whatever is required to refresh common network object here
          ;           
          // while ( (http->connected() || http->available()) && ((millis() - timeoutStart) < 30*1000) ){
          //and monitoring the connexion + do whatever is required to refresh specialized network object here
          if (media == CABLE_MEDIA){
            if (cable == LinkON){
              // do whatever is required when we are connected by cable              
            }else{
              state = DISCONNECTED; 
              media = NO_MEDIA;
            }
          }else{
            if (WiFi.status() == WL_CONNECTED){
              // do whatever is required when we are connected by WiFI
              // this code is executed in loop
              ArduinoOTA.handle();     
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
        if (debug) DEBUG_PRINT("\n%s changed state => cable:%i, media:%i",id.c_str(),cable,media);
        // common instantiation to both Wifi and Ethernet goes here
        if (state == CONNECTED){
          // client has been instantiated before
          //if (http == NULL) http = new HttpClient(*client);
          udp->begin(8888);   //this is to receive packets on port 8888
          if (media == WIFI_MEDIA){
            server->begin(); // <-- for some reason this begin cause the webserver not react with ethernet if it is called. But is required for WiFi
            SSID = WiFi.SSID();
            gateway = WiFi.gatewayIP();
            lastConnectedSSID = SSID;
            DEBUG_PRINT("\n         + - + - saving lastConnectedSSID=SSID=%s",lastConnectedSSID.c_str());                          
            unpauseWiFi();
          }else{
            // ETHERNET
            gateway = Ethernet.gatewayIP();
          }
          checkInternet();
          // doesnt change anything works without, with one or with both
        }else if (state == DISCONNECTED){
          internet = false;
          if (udp != NULL){udp->stop(); delete udp; udp = NULL;}           
          if (client != NULL){client->stop(); delete client; client = NULL;} 
          if (server != NULL){delete server; server  = NULL;} 
          //if (http != NULL){http->stop(); delete http; http = NULL;}                    
          restoreWiFiSSID();
          //DEBUG_PRINT("\nall done");
        }
      }

      return refreshed;
    }

    virtual void setUserInternetCallback(StdFunctionCallbackVoidBool uic){
      userInternetCallback = uic;    
    }

    void onInternetPing(bool online) {
        internet = online;
        DEBUG_PRINT("\n\n\n\t\t\t * * * * PING %s * * * *",internet?"OK":"fail");
        if (userInternetCallback !=nullptr) userInternetCallback(internet);
    }

    // Fonction pour démarrer la tâche de ping non bloquante
    virtual void asyncPing(const char* ipAddress, PingCallback callBack) {
      // Allouer la mémoire pour stocker les paramètres de la tâche
      PingTaskParameters* params = new PingTaskParameters;
      params->ipAddress = ipAddress;
      params->callback = callBack;
      // Créer la tâche FreeRTOS pour le ping
      xTaskCreate(
        pingTask,         // Tâche à exécuter
        "PingTask",       // Nom de la tâche
        4096*2,             // Taille de la pile de la tâche
        params,           // Paramètres de la tâche (adresse IP et callbacks)
        1,                // Priorité
        &pingTaskHandle   // Handle de la tâche
      );
    }

    virtual bool asyncConnect(AsyncHttpClient *clientParam){        
      // Si nous sommes déjà en train de tenter une connexion, on ne relance pas la tâche
      if (clientParam->isClientConnecting ) {
        //DEBUG_PRINT("\n\t client connection pending, no more attempt allowed");
        //clientParam->client->stop();
        //vTaskDelete(httpTaskHandle);
        //clientParam->taskHandle = nullptr;
        return false;
      }else{
        DEBUG_PRINT("\n\t\t~.~.~.~.~.~.~.~.~.~. asyncConnect(%s,%i)",clientParam->host.toString().c_str(),clientParam->port);
        clientParam->client = client;
        clientParam->isClientConnecting = true;
        // Démarrer la tâche FreeRTOS pour la connexion
        xTaskCreatePinnedToCore(
            connectHTTPTask,     // Tâche à exécuter
            "ConnectTask",         // Nom de la tâche
            4096*2,                  // Taille de la pile (en octets)
            clientParam,           // Paramètre passé à la tâche (ici, l'objet courant)
            1,                     // Priorité de la tâche
            NULL, //&clientParam->taskHandle,       // Pointeur vers l'identifiant de la tâche (inutile ici)
            0                      // Utilisation du 1er cœur de l'ESP32, qui est different de la boucle principale afin de ne pas perturber l'affichage
        );
      }
      return true;
    }

    void checkInternet(){
      //DEBUG_PRINT("\n\n\n\t\t\tcheckInternet => ");
      // Utiliser des lambdas pour passer les méthodes membres comme callbacks
      asyncPing("8.8.8.8", 
        [&](bool online) { onInternetPing(online); }
      );
      // latency
      //if (LAN) 
      //LANlatency = 20;
      //else 
        WANlatency = 150;

    }

    void restoreWiFiSSID(){
      // next WiFi attempt will start with the last one that was connected
      _newWifiAttempt = true; // reset flag for immediate reconnection at the deco.
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
    bool _newWifiAttempt = true;    // 
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
    virtual EthernetLinkStatus getLink(){return LinkOFF;}      // access point has no physicalLink
    virtual void stop(){}    
    virtual String getConnectedDeviceIP(){return "";}
    virtual bool asyncConnect(AsyncHttpClient *clientParam){return false;}
    virtual void asyncPing(const char* ipAddress, PingCallback callback){};
    virtual void setUserInternetCallback(StdFunctionCallbackVoidBool uic){};
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
      SSID = *ssid+String(random(99)); // we create a different SSID each time to force the client (laptop) to see it as a new network and avoid asking to renew the DHCP lease, which oculd lead to failing getting an IP address.      
      WiFi.setTxPower(WIFI_POWER_19_5dBm);       // highest power outpu
      AP = true;
    }

    ~AccessPoint(){
      if(debug) DEBUG_PRINT("\n~AccessPoint()");
      WiFi.mode(WIFI_STA);        
      if (server != NULL){
        DEBUG_PRINT("\nTRACE 22");
        delete server;
        DEBUG_PRINT("\nTRACE 23");
      }
    }

    virtual void asyncPing(const char* ipAddress, PingCallback callback){};
    virtual bool asyncConnect(AsyncHttpClient *clientParam){ return false;}
    virtual void setUserInternetCallback(StdFunctionCallbackVoidBool uic){};

    void init(){
      #define VISIBLE 0
      #define MAX_AP_CONNECTION 1
      #define WIFI_CHANNEL 2
      Network::init();

      //pauseWifi();       // cut all previous connection but keep trying connecting
      stopWiFi();

      #ifdef USE_NETWORK
        WiFi.mode(WIFI_AP_STA);        
        if (debug) DEBUG_PRINT("\n WiFi.mode(WIFI_AP_STA)");        
      #else
        WiFi.mode(WIFI_AP);
        if (debug) DEBUG_PRINT("\n WiFi.mode(WIFI_AP)");
      #endif     
      WiFi.setAutoConnect(false); WiFi.setAutoReconnect(false);

      
      WiFi.softAP(SSID,"",WIFI_CHANNEL,VISIBLE,MAX_AP_CONNECTION);         
      //WiFi.softAP(SSID,"");     // doesnt fix the AP stability issue    
      WiFi.setTxPower(WIFI_POWER_19_5dBm); // maximum power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/      
      IP = WiFi.softAPIP();
      if (debug) DEBUG_PRINT("\nAccessPoint.init() softAPIP(%s) => %s",SSID.c_str(),IP.toString().c_str());        
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
          if (debug) DEBUG_PRINT("\nCient connected to AP");
          if (server == NULL) server = new WiFiServer(80,1);         
          server->begin();
        }else{
          if (debug) DEBUG_PRINT("\nCient disconnected from AP");
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
      WiFi.setTxPower(WIFI_POWER_MINUS_1dBm); // power => https://github.com/G6EJD/ESP32-8266-Adjust-WiFi-RF-Power-Output/
    }

  private:

};

#endif