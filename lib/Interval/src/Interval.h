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

#ifdef DEBUG  //ets_printf(__VA_ARGS__) //
  SemaphoreHandle_t serialMutex;
  #define DEBUG_PRINT(...)   if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(500))) { Serial.printf(__VA_ARGS__); Serial.flush(); xSemaphoreGive(serialMutex);} else (void)0
#else
  #define DEBUG_PRINT(...) 
#endif

#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_LITTLEFS

typedef void (*VoidCallback)();
typedef void (*VoidIntCallback)(int);
typedef void (*VoidDoubleIntCallback)(int,int);
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
    
    return;   

    uint8_t buffer[MAX_PACKET_SIZE];
    if (udp != nullptr) {    
      uint16_t len = snprintf((char *)buffer, MAX_PACKET_SIZE, "<%d> %s [%s]%s", PRI_INFO, "esp32", "footswitch", message);
      len = (len > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : len; // handle truncation if message too long
      udp->beginPacket("82.65.4.93", 514);
      udp->write(buffer, len);
      udp->endPacket();    
      //DEBUG_PRINT("\nlog:%s",buffer);
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
        //DEBUG_PRINT("\n~%s()",id.c_str());
      }
    }

    virtual bool refresh(bool force=false) = 0;  
    
    virtual void init(){
      //if (debug) DEBUG_PRINT("\n%s.init()",id.c_str()); 
    }

};

class EventDispatcher {

  public:

      struct Event{
        uint8_t type;
        void* data;
      };

      using EventCallback = std::function<void(Event*)>;

      enum EventType {
        EVENT_ANALOG_BUTTON,     
        EVENT_DIGITAL_BUTTON,     
        EVENT_NETWORK_CHANGED_STATE,
        EVENT_NETWORK,
        EVENT_INTERNET_PING,
        EVENT_APPLICATION
      };



    void registerCallback(EventCallback cb) {
      eventCallBack = cb;
    }

    void fireEvent(Event* event) {
      if (eventCallBack != nullptr) eventCallBack(event);
    }

  protected:
    EventCallback eventCallBack = nullptr;

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
    //DEBUG_PRINT("\nUnable to create output file, halting");
    return 0;
  }
  LZPacker::setProgressCallBack( gzCallback );
  size_t len = strlen(data);
  size_t outputSize = LZPacker::compress( (uint8_t*)data, len, &fout );
  //size_t fileSize = fout.position();
  //DEBUG_PRINT("\nCompressed %d input bytes to %u raw bytes in file%s\n", len, outputSize, filename.c_str());
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
  DEBUG_PRINT("\nCompressed %d input bytes to %u raw bytes\n", len, outputSize);
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
    //DEBUG_PRINT("\nErreur : Fichier non trouvé: %s", filename.c_str());
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
    //DEBUG_PRINT("\nErreur : Décompression échouée");
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
        //DEBUG_PRINT("\t => %s",error.c_str());
        addValue("void","value");
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

///////////////////// JSON SERIALIZATION AND UTILITIES ////////////////////////

class JsonListFile : public JsonList{
  public:
      String filename="voidconf.conf";

      JsonListFile():JsonList(){};

      bool load(String f,String defaultValues=""){

        // We first clean up existing values
        /*while (keyValues.size()>0){

          KeyValue* kv = keyValues.shift();

          delete(kv);
        }*/
        bool success = false;

        if (defaultValues != "") deserialize(defaultValues);

        filename = f;
        // we try to allocate maximum memory and keep 20K free for security
        //DynamicJsonDocument doc(getMaxAlloc());
        JsonDocument doc;

        if (!LittleFS.exists(filename)){

        }else{
          File file = LittleFS.open(filename);
          DeserializationError error = deserializeJson(doc, file);      
          if (!error){
            jsonToKeyValues(&doc);
            success = true;
          }else{

          }
          doc.shrinkToFit();   

        }
        return success;      
      }

      bool save(String f=""){
        bool success = true;
        if (f !="") filename = f;
        String s;
        serialize(&s);

        File file = LittleFS.open(filename, FILE_WRITE);
        if(!file){

            success = false;
        }
        if(!file.print(s)){

            success = false;
        }else{

        }
        file.close();  
        return success;
      }
};

class Config : public JsonListFile{
  public:
    Config():JsonListFile(){}  

    const int getInt(String name) {
      return getString(name,"0")->toInt();
    };
    const float getFloat(String name) {
      return getString(name,"0")->toFloat();
    };
    const boolean getBool(String name) {
      return (*getString(name,"0") != "0");
    };

};

class I18n : public JsonListFile{
  public:
    I18n():JsonListFile(){}

    virtual const String* getString(const String name, const String defaultValue="") {
      int8_t index;
      index = getIndex(String(name));
      if (index < 0) {

        addValue(name,name);  // if it doesnt exist we create it in the list
        return getString(name);
      } else {
        
        KeyValue* kv = keyValues.get(index);  

        return &kv->value;
      }
    };
};




unsigned long onOffPushedStartTime = 0;

bool pushBuffer = false, clearBuffer = false;

void goDeepSleep();

void xTaskBackground(void *pvParameters);
//void xTaskEth(void *pvParameters);

// persistants keyValues ID
#define PERSISTANT_SCREEN_STATE   "screenState"
#define PERSISTANT_SCREEN_ID      "screenID"
#define PERSISTANT_STRIP_ID       "stripID"
#define PERSISTANT_TFT_ROTATION   "rotation"
#define PERSISTANT_BRIGHTNESS     "brightness"

#include "HWManager.h"
HardwareManager hwm;

#include "GUIManager.h"
TFT_eSPI* GUI::tft = &hwm.tft;
GUIManager gui(false);
OpenFontRender* Label::ofr = &gui.labelOfr;
OpenFontRender* Icon::ofr = &gui.iconOfr;

#include "DataManagement.h"

#include "APPManager.h"

ApplicationManager app(&hwm,&gui);


void goDeepSleep(){
    // SHUTDOWN
  // One thing to keep in mind is that when you disable the power to the sensor by turning off the transistor 
  // you would also want to be sure t1o set the pins used for SCL and SDA as INPUT's (high-z state)
  // so that the sensor doesn't try to draw power through some odd internal wiring that might find either of those lines > 0V.     
  // Watch out for floating inputs, they consume power.
  // high side switch
  // High Side transistor switch (PNP BJT or P-Channel MOSFET)    
  // best one : https://forum.arduino.cc/t/mosfet-as-a-power-switch-for-low-power-consumption/882239/6
  // and https://arduino.stackexchange.com/questions/43986/is-there-a-way-to-power-off-external-devices-during-deep-sleep
  // open question: reset the devices ? when back, it will take time to power them on again, how to sync it with the class creation, etc... ?
  //DEBUG_PRINT("\ngoDeepSleep");
  hwm.setPowerRail(OFF);    

  while (hwm.onOff.state == ON_OFF_TRIGGER) hwm.onOff.refresh(); //wait button to be released

  #ifndef PROD
    DEBUG_PRINT("\ngoDeepSleep()");
    while(1);
  #else
    //DEBUG_PRINT(" SHUTTING DOWN");
  #endif

  esp_sleep_enable_ext0_wakeup(ONOFF_PIN,ON_OFF_TRIGGER); //1 = High, 0 = Low  s3    
  esp_deep_sleep_start();  
}

void xTaskBackground(void *pvParameters){  
  while (true){
  
    vTaskDelay(2);  //added for S3 to avoid trigerring the wathcdog


    // network monitoring
    /*static unsigned long checkEthCableTime = 0;
    if (millis()-checkEthCableTime > 1000 && hwm.network->ethEnabled){   // only if we use the full network stack and not only AP mode
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

void unpackProgressCallback( uint8_t progress ) {
  // app.progress = progress;
}

void tarStatusProgressCallback( const char* name, size_t size, size_t total_unpacked ){
  // app.unpackFIleName = name;
} 


void commonFileServerWrapper(Request &req, Response &res) {
  app.commonFileServer(req,res);
}

void commonFileServerWrapperPost(Request &req, Response &res) {
  app.httpPost(req,res);
}


// Tâche pour gérer un client dans son propre thread
/*void handleWebEthClientTask(void* parameter) {
  DEBUG_PRINT("\nhandleWebEthClientTask()");
  EthernetClient* client = (EthernetClient*)parameter;

  if (client && client->connected()) {
    app.webServer->process(client);  // Thread-safe car chaque client est isolé
  }

  client->stop();
  delete client; // important : éviter les fuites mémoire
  app.activeClients--;
  vTaskDelete(NULL); // terminer la tâche
}*/



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

// Tâche de ping non bloquante
void pingTask(void* parameter) {
  // Récupérer les paramètres passés à la tâche
  PingTaskParameters* params = static_cast<PingTaskParameters*>(parameter);
  DEBUG_PRINT("\npingTaskthread running=>%s",params->ipAddress);

  if (Ping.ping(params->ipAddress.c_str())) {
    params->callback(true);  // Appel du callback en cas de succès
  } else {
    params->callback(false);  // Appel du callback en cas d'échec
  }

  // Libérer la mémoire allouée pour les paramètres
  delete params;

  // Suspendre la tâche après chaque ping (exécution ponctuelle)
  vTaskDelete(NULL);
}

// Prototype pour la tâche de connexion
void connectHTTPTask(void *parameter) {

  if (xSemaphoreTake(hwm.network->networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){ 

    AsyncHttpClient *clientObj = static_cast<AsyncHttpClient *>(parameter);
    DEBUG_PRINT("\nconnectHTTPTask(%s,%s,%i,<%s>)",clientObj->host.toString().c_str(),clientObj->URL.c_str(),clientObj->port,clientObj->post.c_str());
     
    Client* client = clientObj->client;  

    if (clientObj->method == HTTP_METHOD_GET) clientObj->post = ""; 

    //String result="{\""+String(API_ORIGIN)+"\":\""+clientObj->URL+"\",";

    JsonDocument doc;        
    doc[API_ORIGIN] = clientObj->URL;

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
          doc[API_STATUS] = API_OK;
          doc[API_BODY] = payload;

          //DEBUG_PRINT("\nRéponse %s reçue : %s",clientObj->method.c_str(),payload.c_str());
        } else {           
          doc[API_STATUS] = API_ERROR;
          doc[API_BODY] = String(httpStatusCode);
        }
      } else {
        doc[API_BODY] = String(httpResponseCode);
        doc[API_STATUS] = API_ERROR;

      }
      httpClient.stop();    
    }else{
        doc[API_BODY] = "\"Null client\"";
        doc[API_STATUS] = API_ERROR;
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

    clientObj->isClientConnecting = false;

    xSemaphoreGive(hwm.network->networkBusy);

  }else{
    DEBUG_PRINT("\nconnectHTTPTask: networkbusy");
  }
  vTaskDelay(1);
  vTaskDelete(NULL);
}


/*void webServerTaskWrapper(void * parameter){
  while(true){
    vTaskDelay(1);
    app.handleWeb(parameter);
  }
}*/

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

//////////////////////////////////////////////////////////////
////                MENU ACTION ITEM CALLBACKS         ///////
//////////////////////////////////////////////////////////////

void back_menu_item_selected(MenuComponent* p_menu_component) {
  //DEBUG_PRINT("\nback_menu_item_selected: ");
  if (!app.ms.back()) app.restoreScreen();    
  else app.menuSystemFullRefresh();
}

void shutdown_menu_item_selected(MenuComponent* p_menu_component) {
  goDeepSleep();
}

void brightness_menu_item_selected(MenuComponent* p_menu_component) {
  /*if (GUI::screenBrightness == 1.0) GUI::screenBrightness = 0.5;
  else GUI::screenBrightness = 1.0;
  app.triggerRedraw(); // force redraw*/
  screen.id = SCREEN_UTILS;
  screen.state = STATE_UTILS_BRIGHTNESS;
}

void save_profile_item_selected(MenuComponent* p_menu_component) {
  //DEBUG_PRINT("\nsave_profile_item_selected ");
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
  //DEBUG_PRINT("\ninfo item selected: id:%i",p_menu_component->get_name());
  screen.id = SCREEN_UTILS;
  screen.state = STATE_UTILS_INFO;
}

void handleConfigConnection(){
  //DEBUG_PRINT("\nhandleConfigConnection");
  // socket is opened between footswitch and browser, hence we indicate the connection is made
  if (app.webAppConnected){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_WEBAPPCONNECTED;    
  }else{
  // socket is not connected so we launch the webapp
    if (app.startWebApp()){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTWEBAPP;    
    }else{
      // There is no valid connection, so we launch AP mode    
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTCONFIGMEDIA;
      hwm.startAP(app.system.getString(S_AP_NAME));
      app.setAPandNetIcons();
      //https://deepbluembedded.com/esp32-wifi-library-examples-tutorial-arduino/#esp32-wifi-modes
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
    /*app.menuActiveIconColor = TFT_GREEN;
    GUI::glowingGoldColor.value = &GUI::unUsedColor;*/
    app.system.save();
    app.restoreScreen();    // return to the caller
  }else{
    app.menuActiveIconColor = TFT_RED;
    GUI::glowingGoldColor.value = &GUI::unUsedColor;
    app.menuTopText = *app.lang.getString("File Error");
  }  
}

void switchToBank_item_selected(MenuComponent* p_menu_component){
  //DEBUG_PRINT("\n%s",p_menu_component->get_name());
  app.jumpToBank(p_menu_component->get_name());
}

void menu_selected(MenuComponent* p_menu_component){
  //DEBUG_PRINT("\n\t\t ====> MENU %s SELECTED <====",p_menu_component->get_name());
  
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

void voidOverlay(uint8_t id){
//  DEBUG_PRINT("\nvoidOverlay()");
}


void baseOverLayBuilder(uint8_t id){  
  /*GUI::tft->fillRect(0,app.overlayIconY,TFT_WIDTH,OVERLAY_ICON_SIZE,GUI::overlayIconBackgroundColor);
  GUI::tft->drawFastHLine(0,app.overlayIconY-1,TFT_WIDTH,TFT_SILVER);*/
  gui.add(new Icon(&app.networkIcon,Align::Left,&GUI::zero, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.networkIconColorIdle,&GUI::defaultBcolor));             
  //gui.add(new AnimatedIcon(app.iconRolling,TWOFRAMEICON,Align::Center,&app.iconRow2, &app.overlayIconY,OVERLAY_ICON_SIZE,ANIMATE_REPEAT,250,&GUI::blue,&GUI::defaultBcolor));              
  gui.add(new Icon(&app.apIcon,Align::Left,&app.apIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.apIconColor,&GUI::defaultBcolor));                  
  gui.add(new Icon(&app.batIcon,Align::Left,&app.batIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.batIconColor,&GUI::defaultBcolor));          
  //gui.add (new PendingBar(&app.pendingBarWidth /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &GUI::zero /*y*/, TFT_WIDTH /*w*/, 1 /*h*/, /*pingpong*/ false, 0 /*offset*/, 0 /*radius*/, &GUI::green /*fc*/ , &GUI::defaultBcolor/*bc*/, 30, 4));                    
  app.rollingIcon(false);
}

float testV = 76;

void bankOverlayBuilder(uint8_t BankIndex){
  
  baseOverLayBuilder(BankIndex);
  
  //we do not display the bankNavigator if user deactivate the bank screen display
  if (app.disabledBankRendering == true) return;

  Interval::Bank* bank = app.banks.get(BankIndex);
  if (bank != nullptr){
    //DEBUG_PRINT("\n\n\n--------------------------------- bankOverlayBuilder %s ---------------------------------\n\n ",bank->name.c_str());
    BankNavigator* navBar = new BankNavigator(bank,true);   
    gui.add(navBar);
  }else{
    //DEBUG_PRINT("\nbankOverlayBuilder NULL BANK");
  }
}



//////////////////////////////////////////////////////////////
////                SCREENBUILDERS CALLBACKS           ///////
//////////////////////////////////////////////////////////////
#define MODAL_SCREEN_ICON_SIZE (ICON_MAX_SIZE/1.7)
#define MODAL_SCREEN_SCROLLBAR_HEIGHT 8
#define MODAL_SCREEN_TEXTAREA_WIDTH (TFT_WIDTH/2)*0.9

int16_t topTextY = GUI::y1+TOP2WIDGET_SIZE;
int16_t iconY = topTextY + CONFIG_TEXTAREA_MESSAGE_SIZE;
int16_t iconX = 0;
int16_t textAreaX = (TFT_WIDTH/2) + (MODAL_SCREEN_TEXTAREA_WIDTH/2);
int16_t bottomTextY = iconY + MODAL_SCREEN_ICON_SIZE;
int16_t scrollBarY = TFT_HEIGHT - NAV_HEIGHT*1.2 - MODAL_SCREEN_SCROLLBAR_HEIGHT;

String iconInterfaceConnect = ICON_FOOTSWITCH;
uint16_t iconInterfaceColor = GUI::red;

void modalScreen(const String* topLabel, const String* midText, const String* bottomLabel, String* icon){
  gui.add(new Label(topLabel, Align::Center,&app.midScreenHoriz, &topTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::gold,&GUI::defaultBcolor));    

  // if we have empty textfield, we don't display it and center the icon
  iconX = TFT_WIDTH/2;
  if (*midText != emptyString){
    iconX -= MODAL_SCREEN_ICON_SIZE;
    gui.add(new TextArea(midText, MODAL_SCREEN_TEXTAREA_WIDTH,Align::Center,Align::Center,&textAreaX,&iconY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));     
  }

  gui.add(new Icon(icon,Align::Center,&iconX, &iconY,MODAL_SCREEN_ICON_SIZE,&iconInterfaceColor,&GUI::defaultBcolor));   
  gui.add(new Label(bottomLabel, Align::Center,&app.midScreenHoriz, &bottomTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::gold,&GUI::defaultBcolor));    
}

void bankScreenBuilder(uint8_t subScreen){  
  app.userScreenBuilder(SCREEN_BANK,subScreen);

  if (app.disabledBankRendering == true) return;

  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = TFT_FCOLOR;
  GUI::cls();

  /*app.,SCREEN_BANK);
  app.persistants.putUChar(PERSISTANT_SCREEN_STATE,subScreen);*/
    
  Interval::Bank* bank = app.banks.get(subScreen);
  //DEBUG_PRINT("\n\n\n---------------------------------bankScreenBuilder ");

  if (bank != nullptr){

    uint8_t layout = bank->currentLayout;

    Strip* strip = app.focusedStrip;

    if (strip != nullptr){
      StripState* stripState = &strip->state;
      // check if the strip is online
      switch (stripState->code){
        case STRIP_DISCONNECTED:
          //DEBUG_PRINT("\nStrip %s is disconnected",strip->type.c_str());
          modalScreen(&hwm.network->SSID,app.lang.getString("Attempt"),app.lang.getString("offline"),&app.networkIcon);
          //gui.add (new ProgressBarH(&hwm.network->wiFiConnectAttemptProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                      
          //gui.add (new PendingBar(&oneHundedFloat /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                                    
        break;
        case STRIP_PENDING:
          //DEBUG_PRINT("\nStrip %s is searching",strip->type.c_str());          
          modalScreen(stripState->stateLabel,stripState->subStateLabel,app.lang.getString("please wait"),&iconInterfaceConnect);  
          //gui.add (new PendingBar(&oneHundedFloat /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                                              
        break;
        case STRIP_CONNECTING:
          //DEBUG_PRINT("\nStrip %s is syncing",strip->type.c_str());          
          modalScreen(stripState->stateLabel,stripState->subStateLabel,app.lang.getString("please wait"),&iconInterfaceConnect);  
          //gui.add (new ProgressBarH(&stripState->progress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                      
        break;
        case STRIP_ONLINE:
          //DEBUG_PRINT("\nStrip %s is online",strip->type.c_str());
          //DEBUG_PRINT("%s window: %d, strip: %d, layout: %d",bank->name.c_str(),strip->win, bank->selector,layout);
          if (layout == LAYOUT_ZOOM){
            strip->pushToGUI(/*true,*/LAYOUT_ZOOM);    // affiche uniquement un strip qui est celui selectionné avec focus dessus
          }else{
            for (const auto& winstrip : bank->strips) {
              // affiche tous les strips de la fenetre courante
              if (winstrip->win == strip->win){            
                winstrip->pushToGUI(/**/);
              }
            }
          } 
        break;    
        default:
          //DEBUG_PRINT("\nStrip %s status code unmanaged(%i)",strip->type.c_str(),stripState->code);
        break;
      }
      
      app.stripHeader = new StripHeaderWidget(strip,&app.lang);      
      gui.add(app.stripHeader);      

      app.focus(strip,bank); // give focus to the strip
      
      
    }else{
      //DEBUG_PRINT("\nNO STRIP TO BUILD, EMPTY BANK");
      gui.add(new TextArea(app.lang.getString("No Strip"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
    }
  }else{
      //DEBUG_PRINT("\nNO BANK");
      gui.add(new TextArea(app.lang.getString("No Bank"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  }
  
  //DEBUG_PRINT("\n--------------bankScreenBuilder exit-----------------");
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
  //DEBUG_PRINT("\nIN-----------------nmenuScreenBuilder()--------------");    
  setMenuColors();
  addMenuWidgets();
  //LogUDP::sendLog("menuScreenBuilder",hwm.network->udp);
  //DEBUG_PRINT("\nOUT-----------------nmenuScreenBuilder()--------------");    
}

void bankSwitchScreenBuilder(uint8_t id){
  //DEBUG_PRINT("\nbankSwitchScreenBuilder()");  
  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = GUI::gold;
  addMenuWidgets();
}

void defaultWizardScreenBuilder(){
    //DEBUG_PRINT("\ndefaultWizardScreenBuilder");
    GUI::cls();
    app.menuBottomLabel = "DEFAULT WIZARD";
    app.menuTopText = "PRESS A BUTTON";
    addTopandBottomMenuLabel();
}

void defaultWizardStepFlow(){
  //DEBUG_PRINT("\ndefaultWizardStepFlow(%i)");
  if (app.getButtonState(MIDDLEB)){
    app.persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
    app.restoreScreen();
  } 
}

void defaultBootScreen(){
  if (LittleFS.exists(CUSTOM_LOGO_FILE))
    gui.drawJpg(0, 0, CUSTOM_LOGO_FILE);
  else
    gui.drawJpg(0, 0, "/logo.jpg");
}

void wizardScreenBuilder(uint8_t subScreen){
  //DEBUG_PRINT("\nwizardScreenBuilder(%d)",subScreen);
  switch(subScreen){
    case STATE_WIZARD_LANG_STEP1:
      // nothin specific => select lang
      //DEBUG_PRINT("\tVOID GUI",subScreen);
    break;
    case STATE_WIZARD_CONNECT_CONFIG_STEP2:
      // nothing specific => connect config is done by utility screen set within handleConfigConnection() 
      //DEBUG_PRINT("\tVOID GUI",subScreen);
    break;      
    case STATE_WIZARD_CONNECT_NETWORK_STEP3:
      GUI::cls();
      gui.add(new Label(app.lang.getString("Waiting network"), Align::Center,&app.midScreenHoriz, &app.midScreenVertical,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));   
    break;    
    case STATE_WIZARD_USER_STEP4:
      app.userScreenBuilder(SCREEN_WIZARD, STATE_WIZARD_USER_STEP4);
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
    //DEBUG_PRINT("addLeftRightIcons()");
    GUI::cls(GUI::defaultBcolor);  
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

int16_t brightYpos = app.menuIconYpos*1.2;

void utilsScreenBuilder(uint8_t state){
  //DEBUG_PRINT("\nutilsScreenBuilder(%d)",state);
  setMenuColors();  // in case it's called directly from the wizard@startup
  switch (state) {
    case STATE_UTILS_BRIGHTNESS:{
      //DEBUG_PRINT("\nSTATE_UTILS_BRIGHTNESS");
      GUI::cls();
      //gui.add(new ProgressBarH(&GUI::screenBrightness /*v*/, BRIGHTNESS_MIN /*min*/,BRIGHTNESS_MAX /*max*/, &GUI::zero /*x*/, &app.menuIconYpos /*y*/, TFT_WIDTH /*w*/, TFT_HEIGHT/6 /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::gold /*fc*/ , &GUI::blue/*bc*/, &GUI::fullBrightness));
      gui.add(new Label(app.lang.getString("BRIGHTNESS"), Align::Center,&app.midScreenHoriz, &GUI::three,MENU_TITLE_SIZE*0.8,&GUI::defaultFcolor,&GUI::defaultBcolor));            
      gui.add(new Arc(&GUI::screenBrightness, BRIGHTNESS_MIN,BRIGHTNESS_MAX , &app.midScreenHoriz, &app.midScreenVertical, 90, &GUI::gold, &GUI::transparentColor,30));              
      gui.add(new Icon(&app.brightIcon, Align::Center,&app.midScreenHoriz,&brightYpos,80,&GUI::menuActiveIconColor,&GUI::defaultBcolor));
      gui.add(new NumericLabel(&GUI::screenBrightness, 0, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor,&GUI::fullBrightness,"%%",100));            // alter = 100%
    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_SEARCHING_CONNECTION:{

    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_INFO:
      //DEBUG_PRINT("\n\nCPU freq:%i",getCpuFrequencyMhz());
      GUI::cls(GUI::menuBackGroundColor);

      gui.add(new TextArea(app.lang.getString("!\"#$%%&'()*+,-./ 0123456789 :;<=>?@ ABCCDEFGHIJKLMNNOPQRSTUVWXYZ [\\]^_` abcdefghijklmnopqrstuvwxyz {|}~çèéà"), TFT_WIDTH*0.80,Align::Center,Align::Center,&app.midScreenHoriz,&app.midScreenVertical,25,&GUI::defaultFcolor,&GUI::defaultBcolor));

      gui.add(new Label(&app.vBatLabel, Align::Center,&app.midScreenHoriz, &app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));      
    break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_CONNECTCONFIGMEDIA:
      buildNetworkScreen();
      //DEBUG_PRINT("\nSTATE_UTILS_CONNECTCONFIGMEDIA: %s",app.menuTopText.c_str());
    break;
    case STATE_UTILS_CONNECTWEBAPP:
      app.configIconGlowingRedColor.value = &GUI::unUsedColor;
      app.leftIconColor = GUI::menuActiveIconColor;
      addLeftRightIcons();  
      app.iconPending[0] = ICON_TIMER;
      app.iconPending[1] = " ";
      //gui.add(new AnimatedIcon(app.iconPending,TWOFRAMEICON,Align::Center,&app.midScreenHoriz, &app.midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));                    
      gui.add(new TextArea(app.lang.getString("Open browser"), TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(&hwm.configURL, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));      

      app.leftIcon = ICON_LAPTOP;
      app.rightIcon = ICON_FOOTSWITCH;
      DEBUG_PRINT("\nSTATE_UTILS_CONNECTWEBAPP");          
    break;
    case STATE_UTILS_WEBAPPCONNECTED:         
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
      //DEBUG_PRINT("\nSTATE_UTILS_WEBAPPCONNECTED: %s",app.menuTopText.c_str());      
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
      //DEBUG_PRINT("\nReceived binary");
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
  DEBUG_PRINT("\nwebAppConnected = true!");
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
  // app.broadCast(app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivity\"}"));  // inform webclient               could be done via ws.send as well  
  // it should only be short message as the socketbuffer out is quite small, hence the fullCOnfig is requested by Post instead
  /*String fullConfig = app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"fullConfig\"}"); 
  ws.send(WebSocketDataType::TEXT, fullConfig.c_str(), fullConfig.length());*/
}

//////////////////////////////////////////////////////////////
////                DEFAULT STRIPS CONSTRUCTOR         ///////
//////////////////////////////////////////////////////////////
Strip* textStripConstructorCallback(){
  //DEBUG_PRINT("\n\ttextStripConstructorCallback()"/*,values.c_str()*/);
  return new TextStrip(stripTextSkel);
}

Strip* midiStripConstructorCallback(){
  return new MidiStrip(stripMidiSkel);
}

Strip* sysexStripConstructorCallback(){
  return new SysexStrip(stripSysexSkel);
}

#endif