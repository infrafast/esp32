#ifndef Interval_h
#define Interval_h


#include <Arduino.h>
#include <LinkedList.h>
#include <ArduinoJson.h>
#include <vector>
#include <FS.h>
#include <LittleFS.h>
#define DEST_FS_USES_LITTLEFS // filesystem object will be available as "tarGzFS"
#include <ESP32-targz.h>

//#include <miniz.h>    ==> works partially. produce gzip file but inflate seems not to be gzip format
//#include <zlib.h> ==> incompatible with arm

#ifdef DEBUG  
  #define DEBUG_PRINT(...)   Serial.printf(__VA_ARGS__); Serial.flush() //ets_printf(__VA_ARGS__) //
#else
  #define DEBUG_PRINT(...) 
#endif

#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_LITTLEFS

typedef void (*VoidCallback)();
typedef void (*VoidIntCallback)(int);
typedef String (*ReturnStringCallback)();
typedef std::function<void(bool*)> StdFunctionCallbackVoidBool; 

//typedef void (BaseManager::*GUIScreenBuilderCallBack)(uint8_t);
#define MAX_PACKET_SIZE 500
#define PRI_DEBUG 15 // 8 + 7
#define PRI_INFO  14 // 8 + 6
#define PRI_WARNING 12 // 8 + 4
#define PRI_ERROR 11 // 8 + 3
class LogUDP {
public:
  // Méthode statique pour envoyer un message à un serveur de journalisation UDP
  static void sendLog(const char* message, UDP* udp) {    
    uint8_t buffer[MAX_PACKET_SIZE];
    if (udp != nullptr) {    
      uint16_t len = snprintf((char *)buffer, MAX_PACKET_SIZE, "<%d> %s [%s]%s", PRI_INFO, "esp32", "footswitch", message);
      len = (len > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : len; // handle truncation if message too long
      udp->beginPacket("82.65.4.93", 514);
      udp->write(buffer, len);
      udp->endPacket();    
      DEBUG_PRINT("\nlog:%s",buffer);
    }
  }
};


class GenericComponent{
  protected:
    bool debug = true;      
 
  public:
    String id="GenericComponent";
    bool refreshed = false; 

    GenericComponent(bool d=false){
      debug=d;    
    }

    virtual ~GenericComponent(){
      if (debug){
        DEBUG_PRINT("\n~%s()",id.c_str());
      }
    }

    virtual bool refresh(bool force=false) = 0;  
    
    virtual void init(){
      if (debug) DEBUG_PRINT("\n%s.init()",id.c_str());
    }

};


class EventDispatcher {

  public:

      struct Event{
        uint8_t type;
        void* data;
      };

      using eventCallback = std::function<void(Event*)>;

      enum EventType {
        EVENT_ANALOG_BUTTON,     
        EVENT_DIGITAL_BUTTON,     
        EVENT_NETWORK_CHANGED_STATE,
        EVENT_NETWORK,
        EVENT_INTERNET_PING,
        EVENT_APPLICATION
      };



    void registerCallback(eventCallback cb) {
      callbacks.push_back(cb);
    }

    void fireEvent(Event* event) {
      for (auto& callback : callbacks) {
          callback(event);
      }
    }

  protected:
    std::function<void(Event*)> callback;
    std::vector<eventCallback> callbacks;

};


///////////////////// API MANAGEMENT //////////////////////

#define API_ORIGIN "origin"
#define API_STATUS "status"
  #define API_OK 1
  #define API_ERROR 0
#define API_BODY   "body"

// Structure pour stocker les données décompressées dans un buffer
struct ByteBuffer : public Stream {
  // Attributs
  uint8_t* buffer = nullptr;  // Pointeur vers le buffer
  size_t size = 0;            // Taille actuelle des données dans le buffer
  size_t capacity = 0;        // Capacité maximale allouée au buffer
  size_t readIndex = 0;       // Position actuelle de lecture

  // Constructeur
  ByteBuffer(size_t initialCapacity = 64) { // Capacité initiale par défaut
    capacity = initialCapacity;
    buffer = (uint8_t*)malloc(capacity);
  }

  // Méthode pour écrire des données dans le buffer
  size_t write(const uint8_t* data, size_t len) override {
    if (size + len > capacity) {
      // Reallocation de mémoire si le buffer est plein
      capacity = (size + len) * 2;  // Double la capacité
      buffer = (uint8_t*)realloc(buffer, capacity);
    }
    memcpy(buffer + size, data, len); // Copier les données
    size += len;
    return len; // Retourne le nombre d'octets écrits
  }

  // Méthode d'écriture d'un seul octet
  size_t write(uint8_t data) override {
    return write(&data, 1);
  }

  // Méthode pour vérifier la disponibilité des données (taille restante non lue)
  int available() override {
    return size - readIndex; // Données non lues restantes
  }

  // Méthode pour lire un octet
  int read() override {
    if (readIndex < size) {              // Vérifie s'il reste des données à lire
      return buffer[readIndex++];       // Retourne l'octet et avance l'index
    }
    return -1; // Retourne -1 si aucune donnée disponible
  }

  // Méthode pour lire plusieurs octets dans un buffer
  size_t readBytes(char* dest, size_t len) {
    size_t bytesRead = 0;
    while (bytesRead < len && available()) {
      dest[bytesRead++] = read(); // Lit octet par octet
    }
    return bytesRead;
  }

  // Méthode pour lire sans avancer l'index
  int peek() override {
    if (readIndex < size) {
      return buffer[readIndex]; // Retourne l'octet sans avancer l'index
    }
    return -1;
  }

  // Méthode pour vider le buffer
  void flush() override {
    readIndex = 0; // Réinitialise l'index de lecture
    size = 0;      // Réinitialise la taille
    shrink();
  }

  void shrink() {
      if (size == 0) {
          capacity = 64; // Reset to initial size
          buffer = (uint8_t*)realloc(buffer, capacity);
      }
  }  

  // Libérer la mémoire allouée
  ~ByteBuffer() {
    if (buffer) {
      free(buffer);
    }
  }
};




// buffer pour stocker les données décompressées
ByteBuffer* uncompressedByteBuffer;

// Fonction de callback pour écrire les données
bool streamWriterCallback(unsigned char* buff, size_t buffsize) {
  uncompressedByteBuffer->write(buff, buffsize);
  return true; // Retourner true pour indiquer que l'écriture est réussie
}

void gzCallback(size_t bytes_read, size_t total_bytes){
  //DEBUG_PRINT("\n ungzip %d bytes ",bytes_read);
  yield();
}

size_t writeBufferToGzFile(String filename, const char* data){
  //DEBUG_PRINT("\nwriteBufferToGzFile->%s",data);  
  File fout = LittleFS.open(filename, "w");
  if(!fout)
  {
    DEBUG_PRINT("\nUnable to create output file, halting");
    return 0;
  }
  LZPacker::setProgressCallBack( gzCallback );
  size_t len = strlen(data);
  size_t outputSize = LZPacker::compress( (uint8_t*)data, len, &fout );
  //size_t fileSize = fout.position();
  DEBUG_PRINT("\nCompressed %d input bytes to %u raw bytes in file%s\n", len, outputSize, filename.c_str());
  fout.close();
  return outputSize;
}



size_t bufferToGz(String* buf, ByteBuffer* dest){
  // using miniz (does not work)
  const char* data = buf->c_str();
  size_t len = strlen(data);
  // deflate!
  size_t outputSize = LZPacker::compress( (uint8_t*)data, len, dest );
  //size_t fileSize = fout.position();
  Serial.printf("\nCompressed %d input bytes to %u raw bytes\n", len, outputSize);
  return outputSize;
}


/*
size_t writeBufferToGzFile(String filename, String* buf){
  // using miniz (does not work)
  const char* data = buf->c_str();
  size_t dataLength = strlen(data);

  // Buffer pour les données compressées
  size_t bufferSize = compressBound(dataLength) + 18; // Ajout de la taille pour l'en-tête et le footer GZIP
  uint8_t* compressedData = (uint8_t*)malloc(bufferSize);
  if (!compressedData) {
      Serial.println("Erreur d'allocation mémoire !");
      return 0;
  }

  // Création de l'en-tête GZIP
  uint8_t gzipHeader[10] = {
      0x1f, 0x8b, // Signature GZIP
      0x08,       // Compression DEFLATE
      0x00,       // Flags
      0x00, 0x00, 0x00, 0x00, // Timestamp
      0x00,       // Extra flags
      0x03        // OS (Unix)
  };

  // Ajout de l'en-tête au tampon
  memcpy(compressedData, gzipHeader, sizeof(gzipHeader));
  size_t offset = sizeof(gzipHeader);

  // Compression avec zlib
  uLongf compressedSize = bufferSize - offset - 8; // Enlever la taille de l'en-tête et du footer
  int result = compress2(&compressedData[offset], &compressedSize, (const uint8_t*)data, dataLength, Z_BEST_COMPRESSION);

  if (result != Z_OK) {
      Serial.printf("Erreur de compression : %d\n", result);
      free(compressedData);
      return 0;
  }

  // Mise à jour de l'offset
  offset += compressedSize;

  // Ajout du footer GZIP (CRC32 + taille non compressée)
  uint32_t crc = crc32(0L, Z_NULL, 0); // Initialisation du CRC32
  crc = crc32(crc, (const uint8_t*)data, dataLength); // Calcul du CRC32
  memcpy(&compressedData[offset], &crc, 4); // CRC32
  offset += 4;
  memcpy(&compressedData[offset], &dataLength, 4); // Taille originale
  offset += 4;

  // Écriture des données compressées dans LittleFS
  File file = LittleFS.open("/data.gz", "w");
  if (!file) {
      Serial.println("Impossible d'ouvrir le fichier !");
      free(compressedData);
      return 0;
  }

  file.write(compressedData, offset);
  file.close();

  Serial.printf("Données compressées et sauvegardées au format GZIP (%d octets) !\n", offset);

  // Nettoyage
  free(compressedData);

  return 1;
}
*/

void targzNullLoggerCallback(const char* format, ...){}


// Fonction pour lire un fichier gz et remplir le buffer de bytes décompressés
size_t readGzFileToBuffer(String filename, ByteBuffer* _buffer) {
  // Ouvrir le fichier compressé
  uncompressedByteBuffer = _buffer;

  File gzFile = LittleFS.open(filename, "r");
  if (!gzFile) {
    DEBUG_PRINT("\nErreur : Fichier non trouvé: %s", filename.c_str());
    return 0;
  }

  // Créer un décompresseur
  GzUnpacker gzUnpacker;
  
  //GZUnpacker->haltOnError( true ); // stop on fail (manual restart/reset required)
  gzUnpacker.setupFSCallbacks( targzTotalBytesFn, targzFreeBytesFn ); // prevent the partition from exploding, recommended
  gzUnpacker.setGzProgressCallback( BaseUnpacker::targzNullProgressCallback ); // targzNullProgressCallback or defaultProgressCallback
  gzUnpacker.setLoggerCallback(targzNullLoggerCallback);    // gz log verbosity BaseUnpacker::targzPrintLoggerCallback
  /*
  if( !gzUnpacker.gzExpander(tarGzFS, filename.c_str(), tarGzFS, "/tmp.unzip") ) {
    DEBUG_PRINT("\ngzExpander failed with return code #%d", GZUnpacker->tarGzGetError() );
  }*/


  // Définir une méthode de callback pour écrire les données décompressées
  gzUnpacker.setStreamWriter(streamWriterCallback);

  // Décompresser le fichier
  if( !gzUnpacker.gzStreamExpander(&gzFile, gzFile.size())){
  //if (!gzExpander.expand(&gzUnpacker, gzFile)) {
    DEBUG_PRINT("\nErreur : Décompression échouée");
    gzFile.close();
    return 0;
  }

  gzFile.close();

  // Retourner un pointeur vers le buffer contenant les données
  return uncompressedByteBuffer->size;
}

int mapFloatToInt(float x, float in_min, float in_max, int out_min, int out_max) {
  if (x>in_max) x = in_max;
  else if (x<in_min) x = in_min;
  int v = (int)((x - in_min) * (float)(out_max - out_min) / (in_max - in_min) + out_min + 0.5);
  return v;
}

float scaleValue(float input, float in_min, float in_max, float out_min, float out_max) {
    return out_min + ((input - in_min) / (in_max - in_min)) * (out_max - out_min);
}

// general purpose utility functions
uint8_t countChar(String s, char c){
    uint8_t res = 0;
    for (int i=0;i<s.length();i++) if (s.charAt(i) == c) res++;
    return res;
}

uint8_t countWord(String str, String sub) {
    uint8_t count = 0;
    int index = 0;
    
    // Iterate through the string
    while ((index = str.indexOf(sub, index)) != -1) {
        // Increment count and move the index past this occurrence
        count++;
        index += sub.length();
    }
    
    return count;
}

String removePattern(const char* input, const char* pattern) {
    char result[strlen(input)]; // Buffer pour stocker la sous-chaîne extraite
    // Utilisation de sscanf pour extraire la partie après "/con/n"
    if (sscanf(input, pattern, result) == 1) {
        return String(result);
    } else {
        return String(input);
    }
}


float truncateToOneDecimal(float num) {
    return ((int)(num * 10.0)) / 10.0;
}

float roundHalf(float nombre) {
    return round(nombre * 2) / 2.0;
}

void removeJsonQuote(String* buffer){
  buffer->replace("\"{","{");   
  buffer->replace(":\"{",":{");  
  buffer->replace("}\"}","}}");      
  buffer->replace("}\"","}");   
  buffer->replace("\\","");
  buffer->replace(":\"[",":[");       
  buffer->replace("]\",","],");
  buffer->replace("]\"}","]}");           
}

int32_t getMaxAlloc(){
  uint32_t freeHeap = ESP.getMaxAllocHeap();
  int32_t maxDocMemAlloc ; //= freeHeap-50000;
  if (freeHeap<20000) maxDocMemAlloc = 5000;  
  else maxDocMemAlloc = 20000;
  return maxDocMemAlloc;
}

typedef struct  {
  String key;
  String value;
} KeyValue;
#define KV_UNIT_SIZE 100 // according to arduinojson assistant its 32

class JsonList{

  public:

    JsonList(){}

    int size(){
      return keyValues.size();
    }

    void serialize(String* buffer){
      //int32_t docSize = buffer->length() + KV_UNIT_SIZE * keyValues.size();
      //DynamicJsonDocument doc(getMaxAlloc());
      JsonDocument doc;
      for (uint8_t i=0;i<keyValues.size();i++){
        KeyValue* kv = keyValues.get(i);        
        doc[kv->key] = kv->value;        
        //DEBUG_PRINT("\ndoc[%s] = %s",kv->key.c_str(),kv->value.c_str());
      }
      serializeJson(doc, *buffer);
      //DEBUG_PRINT("\n\nBEFORE\t\t%s",buffer->c_str());
      removeJsonQuote(buffer);
      //DEBUG_PRINT("\n\nSERIALIZE JsonList AFTER\t\t%s",buffer->c_str());
    }

    void deserialize(String kvPair){
      JsonDocument doc;
      DeserializationError error;
      error = deserializeJson(doc,kvPair );
      if (!error) jsonToKeyValues(&doc);
      else{
        DEBUG_PRINT("\t => %s",error.c_str());
      }  
    }

    virtual const String* getString(const String name, const String defaultValue="") {
      int8_t index;
      index = getIndex(name);
      if (index < 0) {
        noToken = defaultValue;
        return &noToken;
      } else {
        KeyValue* kv = keyValues.get(index);
        return &kv->value;
      }
    };

    bool setValue(String name, String value) {
      bool ret = false;
      int8_t index;
      index = getIndex(name);
      if (index>-1){
        //DEBUG_PRINT("\nsetValue=>");
        ret = updateValue(index,value);
      }else{
        ret = addValue(name,value);
      }
      return ret;
    };

    bool addValue(String name, String value) {        
        bool ret = false;
        int8_t index = getIndex(name);
        if (index == -1){  // if not existing
          KeyValue* kv = new KeyValue;
          kv->key = name;
          kv->value = value;
          if (keyValues.add(kv)){
            //DEBUG_PRINT("\n\taddValue(<%s>,<%s>)",name.c_str(),value.c_str());
            ret = true;
          }
        }else{  // value exists, we update it
          ret = updateValue(index,value);
        }
        return ret;
    };
    
    bool deleteValue(String name){
      int8_t index = getIndex(name);
      //DEBUG_PRINT("\nsize before delete: %i",keyValues.size());
      if (index >= 0) {  
        delete(keyValues.remove(index));
        /*DEBUG_PRINT("\n\t{deleteValue(%s)}",name.c_str());
        DEBUG_PRINT("\nsize after delete: %i",keyValues.size());*/
        return true;
      }else return false;
    }

    int16_t getIndex(String name) {
      int16_t i = 0;
        for(; i < keyValues.size(); i++){
          KeyValue* kv = keyValues.get(i);
          if (kv->key == name) return i;
        }
      return -1;
    }    

    String getName(uint8_t index){
      if (index>keyValues.size()) return "";
      else{
        KeyValue* kv = keyValues.get(index);
        return kv->key;
      }
    }    
    
  protected:  
    LinkedList<KeyValue*> keyValues = LinkedList<KeyValue*>(); 

    bool updateValue(uint8_t index, String value){
      KeyValue* kv = keyValues.get(index);
      kv->value = value;
      //DEBUG_PRINT("\n\tupdateValue<%d>(%s,%s)",index,getName(index).c_str(),value.c_str());
      return true;
    }

    void jsonToKeyValues(JsonDocument* doc){
      JsonObject root = doc->as<JsonObject>();
      for (JsonPair kv : root){
        addValue(kv.key().c_str(),kv.value().as<String>());
      }
    }        
  
  private:
    String noToken = "";    
};



unsigned long onOffPushedStartTime = 0;

bool pushBuffer = false, clearBuffer = false;

void goDeepSleep();

void xTaskBackground(void *pvParameters);
//void xTaskEth(void *pvParameters);

// persistants keyValues ID
#define PERSISTANT_SCREEN_STATE   "screenState"
#define PERSISTANT_SCREEN_ID      "screenID"
#define PERSISTANT_TFT_ROTATION   "rotation"

#include "HWManager.h"
#include "GUIManager.h"
#include "APPManager.h"

HardwareManager hwm;
TFT_eSPI* GUI::tft = &hwm.tft;
GUIManager gui(false);
OpenFontRender* Label::ofr = &gui.labelOfr;
OpenFontRender* Icon::ofr = &gui.iconOfr;
ApplicationManager app(&hwm,&gui);
DataRepository* StripSkeleton::dataRepo = &app.dataRepository;

void goDeepSleep(){
    // SHUTDOWN
  // One thing to keep in mind is that when you disable the power to the sensor by turning off the transistor 
  // you would also want to be sure to set the pins used for SCL and SDA as INPUT's (high-z state)
  // so that the sensor doesn't try to draw power through some odd internal wiring that might find either of those lines > 0V.     
  // Watch out for floating inputs, they consume power.
  // high side switch
  // High Side transistor switch (PNP BJT or P-Channel MOSFET)    
  // best one : https://forum.arduino.cc/t/mosfet-as-a-power-switch-for-low-power-consumption/882239/6
  // and https://arduino.stackexchange.com/questions/43986/is-there-a-way-to-power-off-external-devices-during-deep-sleep
  // open question: reset the devices ? when back, it will take time to power them on again, how to sync it with the class creation, etc... ?
  DEBUG_PRINT("\ngoDeepSleep");
  hwm.setPowerRail(OFF);    

  while (hwm.onOff.state == ON_OFF_TRIGGER) hwm.onOff.refresh(); //wait button to be released

  #ifndef PROD
    DEBUG_PRINT(" LOOP");
    while(1);
  #else
    DEBUG_PRINT(" SHUTTING DOWN");
  #endif

  esp_sleep_enable_ext0_wakeup(ONOFF_PIN,ON_OFF_TRIGGER); //1 = High, 0 = Low  s3    
  esp_deep_sleep_start();  
}

void xTaskBackground(void *pvParameters){
  //static unsigned long checkEthCableTime = 0;
  while (true){
  
    vTaskDelay(2);  //added for S3 to avoid trigerring the wathcdog


    // network monitoring
    /*if (millis()-checkEthCableTime > 1000 && hwm.network->ethEnabled){   // only if we use the full network stack and not only AP mode
      hwm.network->setLink(Ethernet.linkStatus());
      checkEthCableTime = millis();
    }*/
    
    // watch the switch off button only if it was pushed

    if (onOffPushedStartTime !=0 && digitalRead(ONOFF_PIN)==ON_OFF_TRIGGER){
      long push = millis()-onOffPushedStartTime;
      //ets_printf("\n\t\t%u",push);
      if (abs(push)>SHUTDOWN_PUSH_DELAY && hwm.startupState != RESET_STARTUP){ // we do not shutdown if there is a reset in progress
        //ets_printf("\nthresold");
        screen.id = SCREEN_BYE; screen.state = STATE_BYE_MODAL_MESSAGE;
        app.menuTopText = emptyString; app.menuBottomLabel = emptyString;        
        app.byeIcon = &app.powerIcon; app.byeIconColor = &GUI::white;         
        // It's ok, if the program is stuck it won't black screen but at least it 
        // wait the switch to change state, the loop program continues normally
        //ets_printf("\n\t\ton/off pushed for %u ms",push);
        unsigned long t = millis();
        while (digitalRead(ONOFF_PIN)==ON_OFF_TRIGGER && millis()-t<4000){
          yield();
          //ets_printf(".");
        }
        goDeepSleep();
      }
    }else{
      onOffPushedStartTime = millis();
    }

    //GUI tasks
    if (pushBuffer){
      //For some reason DMA requires that a MISO pin is defined. If you do not have a spare then you can allocate it to be the same as the TFT_CS line and it will work, e.g. in your case add the following to the setup file:
      //#define TFT_MISO 17
      //https://github.com/Bodmer/TFT_eSPI/discussions/2233
      //ets_printf("\npushBuffer");
      gui.screenBuffer.pushSprite(0,0);    
      pushBuffer = false;
    }else if (clearBuffer){
      //ets_printf("\nclearBuffer");
      gui.screenBuffer.fillSprite(TFT_BGCOLOR);        
      clearBuffer = false;
    }

  }
}




void commonFileServerWrapper(Request &req, Response &res) {
  app.commonFileServer(req,res);
}

/*
void gzFileServer(Request &req, Response &res) {
    const char * path = req.path();
    res.set("Content-Encoding", "gzip");
    String fullPath = "/webapp"+String(path)+".gz";  

    uint32_t free = ESP.getFreeHeap();

    //DEBUG_PRINT("\nserving <gz> [%d] ->%s",free, fullPath.c_str());
    if (LittleFS.exists(fullPath)){
      File file = LittleFS.open(fullPath);
      //while (file.available()) res.write(file.read());
      app.sendFileInChunks(&file,&res);
      res.end();
    }else{
      res.sendStatus(404);   //send(404, "text/plain", "Not found");    
      res.set("Content-Type", "text/html");
      res.print(fullPath.c_str());
      res.end();          
    }      
}*/

void scanNetworksTaskWrapper(void * parameter){
  app.scanNetworksTask(parameter);
  // Delete the task itself
  vTaskDelete(NULL);
}

void webServerTaskWrapper(void * parameter){
  while(true){
    vTaskDelay(1);
    app.handleWeb(parameter);
  }
}

// Fonction de gestion de la connexion client
/*void handleWebConnection(void *parameter) {
  // Récupérer l'objet EthernetClient à partir du paramètre
  EthernetClient client = *((EthernetClient*)parameter);

  // Gestion de la connexion et des données
            while (client.connected()) {
              DEBUG_PRINT("\nWebapp connected eth");
              app.webServer->process(&client);
            } 
                          client.stop();

  // Terminer la connexion avec le client
  // Supprimer la tâche une fois le traitement terminé
  vTaskDelete(NULL);
}*/


void OnWiFiEvent(WiFiEvent_t event){
  app.evt.type = EventDispatcher::EVENT_NETWORK;
  app.evt.data = &event;
  app.fireEvent(&app.evt);
  // below line is to avoid that a call to start wifi cut a pending connection DIRTY CODE
  if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED){    
    hwm.network->wiFiTimeout = 10000; // we let 10s to get the IP and setup the wifi link
  }
}

/*
String minSec,testString="jtestSTRING-123g",connectionState="",vBat="",milli3,milli4;
String longText="The Game of Life, also known simply as Life, is a cellular automaton devised by the British mathematician John Horton Connway in 1970.";
int16_t midVertical = 120, tenPixels = 35, topScreen = 10, leftScreen = 10, bottomScreen = 220, x1=40, x2=100, x3=130, x4=200;
int16_t m2 = tenPixels + 32;
int16_t m3 = tenPixels + 64;
bool onOff = true;

int16_t xa = 20, xb=90, xc=220;
float negative = -5.5, positive = 5; 
String db="dB",ms="ms";*/

//////////////////////////////////////////////////////////////
////                MENU ACTION ITEM CALLBACKS         ///////
//////////////////////////////////////////////////////////////

void back_menu_item_selected(MenuComponent* p_menu_component) {
  DEBUG_PRINT("\nback_menu_item_selected: ");
  if (!app.ms.back()) app.restoreScreen();    
  else app.menuSystemFullRefresh();
}

void shutdown_menu_item_selected(MenuComponent* p_menu_component) {
  goDeepSleep();
}

void save_profile_item_selected(MenuComponent* p_menu_component) {
  DEBUG_PRINT("\nsave_profile_item_selected ");
}

void tft_rotation_item_selected(MenuComponent* p_menu_component) {
  app.swapButton = true;
  if (app.persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO) == INVERTED_SCREEN_YES) app.persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO);
  else app.persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_YES);
  app.setScreenAndButtonRotation();
  back_menu_item_selected(p_menu_component);
}

void reset_item_selected(MenuComponent* p_menu_component){
  app.goToResetScreenAndSleep("CLEAR");
}

void info_item_selected(MenuComponent* p_menu_component) {  
  DEBUG_PRINT("\ninfo item selected: id:%i",p_menu_component->get_name());
  screen.id = SCREEN_UTILS;
  screen.state = STATE_UTILS_INFO;
}

void handleConfigConnection(){
  DEBUG_PRINT("\nhandleConfigConnection");
  // socket is opened between footswitch and browser, hence we indicate the connection is made
  if (app.webAppConnected){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_WEBAPPCONNECTED;    
  }else{
  // socket is not connected so we launch the webapp
    if (app.startWebApp()){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTWEBAPP;    
      /*app.setWidgetText(MENU_ICON_MIDDLE].widget->setFcolor(&GUI::green);
      app.menuTopText = *app.lang.getString("Open browser http://")+hwm.network->mainIP.toString();
      app.setWidgetText(CONFIG_TEXTAREA_MESSAGE].widget->setText(&app.menuTopText);*/
    }else{
      // There is no valid connection, so we launch AP mode    
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTCONFIGMEDIA;
      hwm.startAP(app.system.getString(S_AP_NAME));
      app.setAPandNetIcons();
      //https://deepbluembedded.com/esp32-wifi-library-examples-tutorial-arduino/#esp32-wifi-modes
      /*app.setWidgetText(MENU_ICON_MIDDLE].widget->setFcolor(&GUI::red);
      app.setWidgetText(CONFIG_TEXTAREA_MESSAGE].widget->setText(app.lang.getString("Error"));*/
    }  
  }
}

void config_menu_item_selected(MenuComponent* p_menu_component){  
  handleConfigConnection();
}

void loadLangFromMenu_item_selected(MenuComponent* p_menu_component){
  const char* menuString = p_menu_component->get_name();
  String langName = app.getNameFromString(menuString);
  // load should return a boolean which indicate if the lanugage file has been loaded correctly to 
  // process the semantic return and update the menu title accordingly if it was loaded or not
  if (app.lang.load("/"+langName+".lang")){
    app.system.setValue(S_LANG,langName);
    app.menuActiveIconColor = TFT_GREEN;
    GUI::glowingGoldColor.value = &GUI::unUsedColor;
    app.system.save();
    app.restoreScreen();    // return to the caller
    //app.setWidgetText(MENU_ICON_MIDDLE].widget->setFcolor(&GUI::green);
  }else{
    //app.setWidgetText(MENU_ICON_MIDDLE].widget->setFcolor(&GUI::red);
    app.menuActiveIconColor = TFT_RED;
    GUI::glowingGoldColor.value = &GUI::unUsedColor;
    app.menuTopText = *app.lang.getString("File Error");
  }  
}

void switchToBank_item_selected(MenuComponent* p_menu_component){
  DEBUG_PRINT("\n%s",p_menu_component->get_name());
  screen.id = SCREEN_BANK;
  screen.state = 0;
  for (; screen.state < app.banks.size(); screen.state++){
    if (app.banks.get(screen.state)->name == p_menu_component->get_name()) break;
  }
}

void menu_selected(MenuComponent* p_menu_component){
  DEBUG_PRINT("\n\t\t ====> MENU %s SELECTED <====",p_menu_component->get_name());
  
  if (strcmp(p_menu_component->get_name(),app.langMenuString) == 0){
    // this doesn't work as the app.ms selected scrool the item above LANG and not LANG itself since the menuSelected action is not complete yet (we are inside)
    /*for (int i = 0; i < app.langMenu->get_num_components(); ++i) {
      MenuComponent const* cp_m_comp = app.langMenu->get_menu_component(i);
      String country = String(cp_m_comp->get_name()).substring(2,String(cp_m_comp->get_name()).length());
      DEBUG_PRINT("\n%s <-> %s",cp_m_comp->get_name(),app.system.getString(S_LANG)->c_str());              
      if (strcmp(country.c_str(),app.system.getString(S_LANG)->c_str()) == 0) break;
      DEBUG_PRINT("\t[%s]",app.langMenu->get_current_component()->get_name());              
      app.ms.next(true);
      DEBUG_PRINT("\t[%s]",app.langMenu->get_current_component()->get_name());              
    } */  
  }
}

//////////////////////////////////////////////////////////////
////                OVERLAYS CALLBACKS                 ///////
//////////////////////////////////////////////////////////////

void voidOverlay(uint8_t id){DEBUG_PRINT("\nvoidOverlay()");}


void baseOverLayBuilder(uint8_t id){
  /*GUI::tft->fillRect(0,app.overlayIconY,TFT_WIDTH,OVERLAY_ICON_SIZE,GUI::overlayIconBackgroundColor);
  GUI::tft->drawFastHLine(0,app.overlayIconY-1,TFT_WIDTH,TFT_SILVER);*/
  gui.add(new Icon(&app.networkIcon,Align::Left,&GUI::zero, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.networkIconColorIdle,&GUI::defaultBcolor));             
  gui.add(new AnimatedIcon(app.iconRolling,TWOFRAMEICON,Align::Center,&app.iconRow2, &app.overlayIconY,OVERLAY_ICON_SIZE,ANIMATE_REPEAT,250,&GUI::blue,&GUI::defaultBcolor));              
  gui.add(new Icon(&app.apIcon,Align::Left,&app.apIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.apIconColor,&GUI::defaultBcolor));                  
  gui.add(new Icon(&app.batIcon,Align::Left,&app.batIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.batIconColor,&GUI::defaultBcolor));          
  gui.add (new PendingBar(&app.pendingBarWidth /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &GUI::zero /*y*/, TFT_WIDTH /*w*/, 1 /*h*/, /*pingpong*/ false, 0 /*offset*/, 0 /*radius*/, &GUI::green /*fc*/ , &GUI::defaultBcolor/*bc*/, 30, 4));                    
  app.rollingIcon(false);
}

float testV = 76;

void bankOverlayBuilder(uint8_t BankIndex){
  
  baseOverLayBuilder(BankIndex);
  return;
  #define PROGRESSBAR_HEIGHT 5
  //GUI::tft->fillRect(0,TFT_HEIGHT-OVERLAY_ICON_SIZE-PROGRESSBAR_HEIGHT-1,TFT_WIDTH,PROGRESSBAR_HEIGHT,TFT_CYAN);
  /*Arc* arc = new Arc(&testV, 0.0,100.0, &app.midScreenHoriz,&app.midScreenVertical, TFT_HEIGHT/2-10,&GUI::gold,&GUI::transparentColor);
  gui.add(arc);*/

  Bank* bank = app.banks.get(BankIndex);
  if (bank != nullptr){
    //DEBUG_PRINT("\n\n\n---------------------------------\n\n bankOverlayBuilder %s\n\n ---------------------------------",bank->name.c_str());
    BankNavigator* navBar = new BankNavigator(bank,true);   
    gui.add(navBar);
  }else{
    DEBUG_PRINT("\nbankOverlayBuilder NULL BANK");
  }
}

//////////////////////////////////////////////////////////////
////                SCREENBUILDERS CALLBACKS           ///////
//////////////////////////////////////////////////////////////

void bankScreenBuilder(uint8_t subScreen){
  GUI::cls();
  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = TFT_FCOLOR;
  app.userBankScreenBuilder(subScreen);
  if (app.disabledBankRendering == true) return;

  app.persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
  app.persistants.putUChar(PERSISTANT_SCREEN_STATE,subScreen);
    
  Bank* bank = app.banks.get(subScreen);
  DEBUG_PRINT("\n\n\n---------------------------------bankScreenBuilder ");

  if (bank != nullptr){

    uint8_t layout = bank->currentLayout;

    Strip* strip = bank->getStrip(bank->selector);

    if (strip != nullptr){
      DEBUG_PRINT("%s window: %d, strip: %d, layout: %d",bank->name.c_str(),strip->win, bank->selector,layout);
      if (layout == LAYOUT_ZOOM){        
        strip->refresh();
        strip->buildWidgets(true,LAYOUT_ZOOM);    // affiche uniquement un strip qui est celui selectionné et focus true
      }else{
        for (const auto& winstrip : bank->strips) {
          // affiche tous les strips de la fenetre courante
          if (winstrip->win == strip->win){            
            winstrip->refresh();
            bool focus = (winstrip->index == strip->index)?true:false;
            winstrip->buildWidgets(focus);
          }
        }
      } 
      strip->buildHeaderAndFooter();                     

    }else{
      DEBUG_PRINT("\nNO STRIP TO BUILD, EMPTY BANK");
      gui.add(new TextArea(app.lang.getString("No Strip"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
    }
  }else{
      DEBUG_PRINT("\nNO BANK");
      gui.add(new TextArea(app.lang.getString("No Bank"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  }
  
  DEBUG_PRINT("\n--------------X-----------------");
}

void addTopandBottomMenuLabel(){
  gui.add(new TextArea(&app.menuTopText, TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  gui.add(new Label(&app.menuBottomLabel, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
}

void addMenuWidgets(){
  GUI::cls();

  addTopandBottomMenuLabel();

  gui.add(new Icon(&app.menuItemIcon[1], Align::Center,&app.midScreenHoriz, &app.menuIconYpos,MENU_ICON_SIZE,&app.menuActiveIconColor,&GUI::defaultBcolor));
  gui.add(new Icon(&app.menuItemIcon[0], Align::Center,&app.leftMenuIconXpos, &app.menuIconYpos,MENU_ICON_SIZE,&GUI::menuInactiveIconColor,&GUI::defaultBcolor));  
  gui.add(new Icon(&app.menuItemIcon[2], Align::Center,&app.rightMenuIconXpos, &app.menuIconYpos,MENU_ICON_SIZE,&GUI::menuInactiveIconColor,&GUI::defaultBcolor));    
}

void setMenuColors(){
  GUI::defaultBcolor = GUI::menuBackGroundColor;
  GUI::defaultFcolor = GUI::menuActiveIconColor;  
}

void menuScreenBuilder(uint8_t subScreen){
  DEBUG_PRINT("\nmenuScreenBuilder()");  
  setMenuColors();
  addMenuWidgets();
  //LogUDP::sendLog("menuScreenBuilder",hwm.network->udp);
}

void bankSwitchScreenBuilder(uint8_t id){
  DEBUG_PRINT("\nbankSwitchScreenBuilder()");  
  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = GUI::gold;
  addMenuWidgets();
}

void defaultWizardScreenBuilder(){
    DEBUG_PRINT("\ndefaultWizardScreenBuilder");
    GUI::cls();
    app.menuBottomLabel = "DEFAULT WIZARD";
    app.menuTopText = "PRESS A BUTTON";
    addTopandBottomMenuLabel();
}

void defaultWizardStepFlow(){
  DEBUG_PRINT("\ndefaultWizardStepFlow(%i)");
  if (app.getButtonState(MIDDLEB)) app.gotoWizardEnd();  
}

void defaultBootScreen(){
  if (LittleFS.exists(CUSTOM_LOGO_FILE))
    gui.drawJpg(0, 0, CUSTOM_LOGO_FILE);
  else
    gui.drawJpg(0, 0, "/logo.jpg");
}

void wizardScreenBuilder(uint8_t subScreen){
  DEBUG_PRINT("\nwizardScreenBuilder(%d)",subScreen);
  switch(subScreen){
    case STATE_WIZARD_LANG_STEP1:
      // nothin specific => select lang
      DEBUG_PRINT("\tVOID GUI",subScreen);
    break;
    case STATE_WIZARD_CONNECT_CONFIG_STEP2:
      // nothing specific => connect config is done by utility screen set within handleConfigConnection() 
      DEBUG_PRINT("\tVOID GUI",subScreen);
    break;      
    case STATE_WIZARD_CONNECT_NETWORK_STEP3:
      GUI::cls();
      gui.add(new Label(app.lang.getString("Waiting network"), Align::Center,&app.midScreenHoriz, &app.midScreenVertical,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));   
    break;    
    case STATE_WIZARD_USER_STEP4:
      app.userWizardScreenBuilder();
    break;
    case STATE_WIZARD_END_STEP5:
      
    break;
  }
}

void byeScreenBuilder(uint8_t subScreen){
  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = TFT_FCOLOR;
  GUI::cls();
  addTopandBottomMenuLabel();
  //gui.add(new Icon(&app.byeIcon,Align::Center,&app.midScreenHoriz, &app.bigWidgetMidY,WIDGET_BIG_WIDTH,&app.byeIconColor,&GUI::defaultBcolor));                     
  gui.add(new Icon(app.byeIcon,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,app.byeIconColor,&GUI::defaultBcolor));                     
}

void startupScreenBuilder(uint8_t id){
  gui.drawJpg(0,0,"/webapp/logo.jpg");
}

void promptScreenBuiler(uint8_t id){}


void addLeftRightIcons(){
    DEBUG_PRINT("addLeftRightIcons()");
    GUI::tft->fillScreen(GUI::defaultBcolor);    
    gui.add(new Icon(&app.leftIcon, Align::Right,&app.hquarter2xPos,&app.menuIconYpos,MENU_ICON_SIZE,&app.leftIconColor,&GUI::defaultBcolor));
    gui.add(new Icon(&app.rightIcon, Align::Left,&app.hquarter3xPos,&app.menuIconYpos,MENU_ICON_SIZE,&GUI::menuActiveIconColor,&GUI::defaultBcolor));
    addTopandBottomMenuLabel();    
}

void setAPIconAndText(){
  gui.add(new TextArea(app.lang.getString("Connect WiFi Access Point"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  app.leftIcon = ICON_AP;
}

void buildNetworkScreen(){
  GUI::cls();

  app.rightIcon = ICON_LAPTOP;
  app.configIconGlowingRedColor.value = &app.leftIconColor;  
  //gui.add(new AnimatedIcon(app.iconRolling,2,Align::Center,&app.midScreenHoriz, &app.midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));              
  #ifdef USE_NETWORK
    if (!hwm.network->ethEnabled){
      //if ethernet bank is absent, we only offer to connect the AP
      setAPIconAndText();                    
    }else{
      gui.add(new TextArea(app.lang.getString("Connect"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      app.leftIcon = ICON_MIXETHWIFI;
    }
  #else
    setAPIconAndText();
  #endif

  gui.add(new Icon(&app.leftIcon, Align::Right,&app.hquarter2xPos,&app.menuIconYpos,MENU_ICON_SIZE,&app.leftIconColor,&GUI::defaultBcolor));
  gui.add(new Icon(&app.rightIcon, Align::Left,&app.hquarter3xPos,&app.menuIconYpos,MENU_ICON_SIZE,&GUI::menuActiveIconColor,&GUI::defaultBcolor));

  gui.add(new Label(&hwm.configMedia->SSID, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
}


void utilsScreenBuilder(uint8_t state){
  DEBUG_PRINT("\nutilsScreenBuilder(%d)",state);
  switch (state) {
    case STATE_UTILS_SEARCHING_CONNECTION:{

    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_INFO:
      setMenuColors();  // in case it's called directly from the wizard@startup
      DEBUG_PRINT("\n\nCPU freq:%i",getCpuFrequencyMhz());
      GUI::tft->fillScreen(GUI::menuBackGroundColor);
      app.menuTopText = *app.lang.getString("Info item");
      gui.add(new Label(&app.menuTopText, Align::Center,&app.midScreenHoriz, &GUI::zero,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));
      gui.add(new Label(&app.vBatLabel, Align::Center,&app.midScreenHoriz, &app.midScreenHoriz,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));      
    break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_CONNECTCONFIGMEDIA:
      setMenuColors();  // in case it's called directly from the wizard@startup
      buildNetworkScreen();
      DEBUG_PRINT("\nSTATE_UTILS_CONNECTCONFIGMEDIA: %s",app.menuTopText.c_str());
    break;
    case STATE_UTILS_CONNECTWEBAPP:
      setMenuColors();  // in case it's called directly from the wizard@startup
      app.configIconGlowingRedColor.value = &GUI::unUsedColor;
      app.leftIconColor = GUI::menuActiveIconColor;
      addLeftRightIcons();  
      app.iconPending[0] = ICON_TIMER;
      app.iconPending[1] = " ";
      gui.add(new AnimatedIcon(app.iconPending,TWOFRAMEICON,Align::Center,&app.midScreenHoriz, &app.midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));                    

      gui.add(new TextArea(app.lang.getString("Open browser"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(&hwm.configIP, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));      

      app.leftIcon = ICON_LAPTOP;
      app.rightIcon = ICON_FOOTSWITCH;
      DEBUG_PRINT("\nSTATE_UTILS_CONNECTWEBAPP: %s",app.menuTopText.c_str());
    break;
    case STATE_UTILS_WEBAPPCONNECTED:   
      setMenuColors();  // in case it's called directly from the wizard@startup
      addLeftRightIcons();
      //gui.add(new Label(&app.configClientIP, Align::Left,&app.quarter1xPos, &app.belowMenuIcon,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));
      //gui.add(new Label(&hwm.configIP, Align::Right,&app.quarter4xPos, &app.belowMenuIcon,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));      
      app.iconPending[0] = ICON_LEFTDOT;
      app.iconPending[1] = ICON_RIGHTDOT;      
      gui.add(new AnimatedIcon(app.iconPending,TWOFRAMEICON,Align::Center,&app.midScreenHoriz, &app.midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));                    

      gui.add(new TextArea(app.lang.getString("Press button"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(app.lang.getString("Connected"), Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));      

      app.leftIcon = ICON_LAPTOP;
      app.rightIcon = ICON_FOOTSWITCH;
      DEBUG_PRINT("\nSTATE_UTILS_WEBAPPCONNECTED: %s",app.menuTopText.c_str());      
    break;
    ////////////////////////////////////////////////////////////////////////////////
  }
}

  //////////////////////////////////////////////////////////////////////
 ////                SERVER SOCKET CALLBACKS                   ///////
////////////////////////////////////////////////////////////////////

void socketMessageCallback(IWebSocket &ws, const WebSocketDataType dataType, const char *message, uint16_t length){
  String response;
  switch (dataType) {
    case net::WebSocketDataType::TEXT:
      response = app.parseJsonInput(message);
      //DEBUG_PRINT("\n\t\t socketsend=><%s>",response.c_str());                 
    break;
    case net::WebSocketDataType::BINARY:
      DEBUG_PRINT("\nReceived binary");
    break;
  }
  ws.send(dataType, response.c_str(), response.length());
}

void socketCloseCallback(IWebSocket &ws, WebSocketCloseCode code, const char *reason, uint16_t length) {
  DEBUG_PRINT("\n\t!!! socketCloseCallback=>code:%i length:%i\n",code,length);
  app.webAppConnected = false;
  hwm.configMedia->refreshed = true;    // this is to trigger app manager to refresh the screen showing how to connect.
}


void socketConnectionCallback(IWebSocket &ws) {

  app.webAppConnected = true;

  const auto protocol = ws.getProtocol();
  if (protocol) {
    DEBUG_PRINT("\nProtocol: %i",protocol);
  }

  ws.onMessage(socketMessageCallback);
  ws.onClose(socketCloseCallback);

  app.configClientIP = ws.getRemoteIP().toString();
  DEBUG_PRINT("\nNew websocket client IP: %s",app.configClientIP.c_str());

  // any message to be send straigth after connexion was established would be put here.
  //app.broadCast(app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivityConfig\"}"));  // inform webclient               could be done via ws.send as well  
  // it should only be short message as the socketbuffer out is quite small, hence the fullCOnfig is requested by Post instead

  /*String fullConfig = app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"fullConfig\"}"); 
  ws.send(WebSocketDataType::TEXT, fullConfig.c_str(), fullConfig.length());*/

  String connectivityConfig = app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivityConfig\"}");
  ws.send(WebSocketDataType::TEXT, connectivityConfig.c_str(), connectivityConfig.length());

}

//////////////////////////////////////////////////////////////
////                DEFAULT STRIPS CONSTRUCTOR         ///////
//////////////////////////////////////////////////////////////
Strip* textStripConstructorCallback(){
  //DEBUG_PRINT("\n\ttextStripConstructorCallback()"/*,values.c_str()*/);
  return new TextStrip(stripTextSkel,&gui);
}

#endif