/*
0.13 - Interval - Footswitch project
0.14 - Auto update SW from .bin.gz file on LittleFS root
0.15 - Fix SW update process + auto restart after update
0.16 - launch fw update modal from web interface
0.17 - updates implementation finished 
0.18 - full file update implementation with web app sync and error handling
0.19 - ping class refactor + progress during fw update + interface state broadcast management + revamp of datasets + iMidi hardening
0.20 - compatibility with code server
0.21 - udp connectivity hardening
0.22 - interfaces refactor + better state management
0.23 - event and data handing, implementation of interfaces
0.24 - starting sync data implementation
0.xx - mixer value based on dedicated shared struct
*/


#ifndef Interval_h
#define Interval_h

#ifndef _V
  #define _V "0.01"
#endif


#include <Arduino.h>

#include <LinkedList.h>
#include <ArduinoJson.h>
#include <vector>
#include <FS.h>
#include <LittleFS.h>

#define DEST_FS_USES_LITTLEFS // filesystem object will be available as "tarGzFS"
#include <ESP32-targz.h>

#include <array>
#include <utility>

//#define USE_FTP

#if defined(USE_FTP) && defined(USE_NETWORK)
  #include "ESP-FTP-Server-Lib.h"
  #include "FTPFilesystem.h"
  FTPServer ftp;
#endif



#define INTERVAL_PRODUCT_NAME "footswitch"
#define INTERVAL_MANUFACTURER "interval"
#define INTERVAL_DEFAULT_HOSTNAME "infrafast.fr"    //"82.65.4.93"
#define SERVER_SW_UPDATE_PATH "/fwupdate/"

#define WEBAPP_PATH "/webapp"

#define FW_UPDATE_URL "firmware"

bool useUSBMidi = false;

#define FW_EXTENSION ".tgz"

#ifdef DEBUG  //ets_printf(__VA_ARGS__) //
  #define debugMon Serial  //  
  SemaphoreHandle_t serialMutex = xSemaphoreCreateMutex();
  #define DEBUG_PRINT(...) if (!useUSBMidi) if (xSemaphoreTake(serialMutex, pdMS_TO_TICKS(500))) { debugMon.printf(__VA_ARGS__); debugMon.flush(); xSemaphoreGive(serialMutex);} else (void)0
  
  // Macros publiques
  #define DEBUG_INFO(...)  DEBUG_PRINT("\nℹ\t" __VA_ARGS__)    //✅
  #define DEBUG_WARN(...)  DEBUG_PRINT("\n⚠️\t" __VA_ARGS__)
  #define DEBUG_ERROR(...) DEBUG_PRINT("\n❌\t" __VA_ARGS__)
  

#else
  #define DEBUG_PRINT(...) 
  #define DEBUG_INFO(...) 
  #define DEBUG_WARN(...) 
  #define DEBUG_ERROR(...)
  #define debugMon 0
#endif

#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_LITTLEFS
 


// le timeout des check Wifi doit etre superieur a celui du check ethernet cable de sorte de lui laisser la priorité en etant au minimum de 3000
#define ETH_CABLE_CHECK_FREQUENCY 3000

#define WIFI_CHECK_TIMEOUT_MIN  ((ETH_CABLE_CHECK_FREQUENCY >= 3000) ? (ETH_CABLE_CHECK_FREQUENCY*1.5) : 3000)
#define WIFI_CHECK_TIMEOUT_MAX  9000


class TargetEcho{
  public:
    bool alive = false;
    unsigned long lastRxTime ;    
    uint16_t latency = 30;      // normal latency on TCP/IP lan
    uint16_t jitter = 5;
    String host;
    IPAddress ip;  
    TargetEcho(String h):host(h){} 
    String echoCommand = "";

    unsigned long lastPingSentAt = 0;
    bool pingPending = false;    

};

typedef std::function<void(TargetEcho*)> PingCallback; // Utilisation de std::function pour plus de flexibilité

class PingParameters {
public:
  PingParameters(){
    //DEBUG_PRINT("\nPingParameters created");
  }
  ~PingParameters(){
    //DEBUG_PRINT("\nPingParameters deleted");
  }
  TargetEcho* echo;
  PingCallback callback;
};


enum{
    STOP_UDP,
    DELETE_UDP
};

typedef void (*VoidCallback)();
typedef void (*VoidIntCallback)(int);
typedef void (*VoidDoubleIntCallback)(int,int);
typedef String (*ReturnStringCallback)();
typedef std::function<void(TargetEcho*)> StdFunctionCallbackVoidTargetEcho; 

//////////////////////////////////////////////////////////////////////////
/// @brief // Callback to report progress during unpacking
/// @param percent Completed percent (0.0 - 100.0)
String updateProgressLabel = "";
float subProgressPercent = 0.0;
float overallProgressPercent = 0.0;
static uint8_t lastFileProgress = 0;
uint8_t progressPercentInt = 0;   // 0..100
size_t proportionalSize = 0;   // 0..100
size_t processedSize = 0;
size_t totalGzSize = 0;


String updateSWerrorString = "";

enum{
  SYSTEM_OFF,
  BATTERY_STARTUP,
  NORMAL_STARTUP,
  WIZARD_STARTUP,
  SYSTEM_RESTART,
  RESTART_REQUEST_AS_SUB_CDC,
  USB_CDC_AND_FWUPDATE_STARTUP,    
  RESET_STARTUP,  
  NO_FS_STARTUP,  
};

/*
void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_STARTED_EVENT:
                debugMon.println("USB PLUGGED");
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                debugMon.println("USB UNPLUGGED");
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                debugMon.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en);
                break;
            case ARDUINO_USB_RESUME_EVENT:
                debugMon.println("USB RESUMED");
                break;

            default:
                break;
        }
    }
    else if (event_base == ARDUINO_USB_CDC_EVENTS) {
        arduino_usb_cdc_event_data_t *data = (arduino_usb_cdc_event_data_t *)event_data;
        switch (event_id) {
            case ARDUINO_USB_CDC_CONNECTED_EVENT:
                debugMon.println("CDC CONNECTED");
                break;
            case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
                debugMon.println("CDC DISCONNECTED");
                break;
            case ARDUINO_USB_CDC_LINE_STATE_EVENT:
                debugMon.printf("CDC LINE STATE: dtr: %u, rts: %u\n", data->line_state.dtr, data->line_state.rts);
                break;
            case ARDUINO_USB_CDC_LINE_CODING_EVENT:
                debugMon.printf("CDC LINE CODING: bit_rate: %lu, data_bits: %u, stop_bits: %u, parity: %u\n", data->line_coding.bit_rate,
                              data->line_coding.data_bits, data->line_coding.stop_bits, data->line_coding.parity);
                break;
            case ARDUINO_USB_CDC_RX_EVENT:
                debugMon.printf("CDC RX [%u]:", data->rx.len);
                {
                    uint8_t buf[data->rx.len];
                    size_t len = debugMon.read(buf, data->rx.len);
                    debugMon.write(buf, len);
                }
                debugMon.println();
                break;
            case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
                debugMon.printf("CDC RX Overflow of %d bytes", data->rx_overflow.dropped_bytes);
                break;

            default:
                break;
        }
    }
    else if (event_base == ARDUINO_USB_MIDI_EVENTS) {
        midiEventPacket_t *data = (midiEventPacket_t *)event_data;
        switch (event_id) {
            default:
                debugMon.printf("MIDI EVENT:  ID=%d, DATA=%d\r\n", event_id, (uint32_t)data);
                break;
        }
    }
}
*/

template<typename... Args>
inline void concatString(String& destination, Args&&... args) {
  std::array<String, sizeof...(Args)> parts{String(std::forward<Args>(args))...};
  size_t totalLength = destination.length();
  for (const auto& part : parts) {
    totalLength += part.length();
  }
  destination.reserve(totalLength);
  for (const auto& part : parts) {
    destination += part;
  }
}

template<typename... Args>
inline String concatStrings(Args&&... args) {
  String result;
  concatString(result, std::forward<Args>(args)...);
  return result;
}


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

    }

};

enum EventType : uint8_t {
  EVENT_WEBAPP_STARTED,
  EVENT_WEBAPP_STOPPED,
  EVENT_SOCKET_OPEN,
  EVENT_SOCKET_CLOSE,
  EVENT_SOCKET_MESSAGE,
  EVENT_SOCKET_ERROR,
  EVENT_BATTERY_REFRESHED,
  EVENT_DIGITAL_BUTTON,
  EVENT_ANALOG_BUTTON,
  EVENT_NETWORK,
  EVENT_NETWORK_DISCONNECTED,
  EVENT_NETWORK_CONNECTED,
  EVENT_NETWORK_CONNECTION_CHANGED,
  EVENT_INTERNET_PING,
  EVENT_REQUEST_DIRECTORY_SERVICE_REFRESH,
  EVENT_DIRECTORY_SERVICE_REFRESHED,
  EVENT_DIRECTORY_NOT_READY,
  EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED,  
  EVENT_INTERFACE_DATA_READY,
  EVENT_OSC_MESSAGE,
  EVENT_WEBSOCKET_CLOSED,
  EVENT_TEXT_MESSAGE,
  EVENT_INTERFACE_MESSAGE,
  EVENT_COUNT
};

/* Table de noms des événements */
#ifdef DEBUG
static const char* const EVENT_NAMES[EVENT_COUNT] = {
  "EVENT_WEBAPP_STARTED",
  "EVENT_WEBAPP_STOPPED",
  "EVENT_SOCKET_OPEN",
  "EVENT_SOCKET_CLOSE",
  "EVENT_SOCKET_MESSAGE",
  "EVENT_SOCKET_ERROR",
  "EVENT_BATTERY_REFRESHED",
  "EVENT_DIGITAL_BUTTON",
  "EVENT_ANALOG_BUTTON",
  "EVENT_NETWORK",
  "EVENT_NETWORK_DISCONNECTED",
  "EVENT_NETWORK_CONNECTED",
  "EVENT_NETWORK_CONNECTION_CHANGED",
  "EVENT_INTERNET_PING",
  "EVENT_REQUEST_DIRECTORY_SERVICE_REFRESH",
  "EVENT_DIRECTORY_SERVICE_REFRESHED",
  "EVENT_DIRECTORY_NOT_READY",
  "EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED",
  "EVENT_INTERFACE_DATA_READY",
  "EVENT_OSC_MESSAGE",
  "EVENT_WEBSOCKET_CLOSED",
  "EVENT_TEXT_MESSAGE",
  "EVENT_INTERFACE_MESSAGE"
};
#endif

class Event {
public:
  virtual ~Event() = default;

  Event(void* _data=nullptr):data(_data){};

  EventType type;

  void* data;

  void attachData(void* _data){
    data=_data;
  }

  #ifdef DEBUG
  virtual const char* name() const {
    return getEventName(type);
  }

  virtual const char* serialize() const {
    return "";
  }

  static const char* getEventName(EventType id) {
    if (id < EVENT_COUNT) {
      return EVENT_NAMES[id];
    }
    return "UNKNOWN_EVENT";
  }
  #endif


  static bool isEventDumpEnabled(EventType id) {
    if (id >= EVENT_COUNT) return false;
    return (eventDumpMask & (1UL << id)) != 0;
  }

  static void enableEventDump(EventType id) {
    if (id < EVENT_COUNT) {
      eventDumpMask |= (1UL << id);
    }
  }

  static void disableEventDump(EventType id) {
    if (id < EVENT_COUNT) {
      eventDumpMask &= ~(1UL << id);
    }
  }

  static void setEventDumpMask(uint32_t mask) {
    eventDumpMask = mask;
  }

  static uint32_t getEventDumpMask() {
    return eventDumpMask;
  }

  static void enableAllEventDumps() {
    if (EVENT_COUNT >= 32) {
      eventDumpMask = 0xFFFFFFFFUL;
    } else {
      eventDumpMask = (1UL << EVENT_COUNT) - 1;
    }
  }

  static void disableAllEventDumps() {
    eventDumpMask = 0;
  }

  void dump(const char* direction, const char* caller) const {
    if (!isEventDumpEnabled(type)) return;

    DEBUG_INFO("+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+");
    DEBUG_INFO("| %s %s %s %s", caller, direction, name(),serialize());
    DEBUG_INFO("+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+");
  }

  static uint32_t eventDumpMask;
};

uint32_t Event::eventDumpMask =
  (EVENT_COUNT >= 32) ? 0xFFFFFFFFUL : ((1UL << EVENT_COUNT) - 1);


class EventListener {
public:
  virtual ~EventListener() = default;

  virtual void onEvent(const Event& evt) = 0;

  void _onEvent(const Event& evt) {
#ifdef DEBUG
    evt.dump("listen", getListenerName());
#endif
    onEvent(evt);
  }

  static void eventTrampoline(void* context, const Event& evt) {
    if (context) {
      static_cast<EventListener*>(context)->_onEvent(evt);
    }
  }

  virtual const char* getListenerName()  = 0;
};

class EventDispatcher {
public:
  static constexpr uint8_t DEFAULT_MAX_CALLBACKS = 10;
  static constexpr uint8_t ABSOLUTE_MAX_CALLBACKS = 64;

  virtual const char* getDispatcherName() const = 0;

  using CallbackFn = void (*)(void*, const Event&);

  struct HandlerSlot {
    void* context = nullptr;
    CallbackFn fn = nullptr;
  };

  EventDispatcher(uint8_t maxCallbacks = DEFAULT_MAX_CALLBACKS) {
    if (maxCallbacks == 0)
      maxCallbacks = DEFAULT_MAX_CALLBACKS;

    if (maxCallbacks > ABSOLUTE_MAX_CALLBACKS)
      maxCallbacks = ABSOLUTE_MAX_CALLBACKS;

    maxHandlers = maxCallbacks;
    eventHandlers = new HandlerSlot[maxHandlers];
  }

  virtual ~EventDispatcher() {
    delete[] eventHandlers;
  }

  bool registerEventListener(EventListener* listener) {
    if (!listener) return false;
    
    // duplicate?
    for (uint8_t i = 0; i < maxHandlers; i++) {
      if (eventHandlers[i].context == listener &&
          eventHandlers[i].fn == EventListener::eventTrampoline) {
        DEBUG_WARN("EventDispatcher: listener already registered");
        return false;
      }
    }

    // place libre ?
    for (uint8_t i = 0; i < maxHandlers; i++) {
      if (!eventHandlers[i].fn) {
        eventHandlers[i].context = listener;
        eventHandlers[i].fn = EventListener::eventTrampoline;
        return true;
      }
    }

    DEBUG_ERROR("!!! EventDispatcher IS FULL !!!");
    return false;
  }

  bool unregisterEventListener(EventListener* listener) {
    if (!listener) return false;

    for (uint8_t i = 0; i < maxHandlers; i++) {
      if (eventHandlers[i].context == listener &&
          eventHandlers[i].fn == EventListener::eventTrampoline) {
        eventHandlers[i].context = nullptr;
        eventHandlers[i].fn = nullptr;
        return true;
      }
    }
    return false;
  }

  uint8_t countRegisteredListeners() const {
    uint8_t count = 0;

    for (uint8_t i = 0; i < maxHandlers; ++i) {
      if (eventHandlers[i].fn != nullptr) {
        count++;
      }
    }

    return count;
  }  

  void fireEvent(const Event& evt) {
    #ifdef DEBUG
      evt.dump("🔥fire", getDispatcherName());
    #endif

    for (uint8_t i = 0; i < maxHandlers; ++i) {
      if (eventHandlers[i].fn) {
        eventHandlers[i].fn(eventHandlers[i].context, evt);
      }
    }
  }

private:
  HandlerSlot* eventHandlers = nullptr;
  uint8_t maxHandlers = DEFAULT_MAX_CALLBACKS;
};

///////////////////// API MANAGEMENT //////////////////////

#define API_ORIGIN "origin"
#define API_STATUS "status"
  #define API_OK 1
  #define API_ERROR 0
#define API_BODY   "body"


////////////////////// UTILITIES ///////////////////////////
void populateFileList(String extension, String folder, std::vector<String> *list, String prefix="", bool withExtension=true) {
  //we first clean all items from the list

  list->clear();

  File root = LittleFS.open(folder);
  DEBUG_INFO("Listing: 📝.%s in %S",extension.c_str(),folder.c_str());
  File file = root.openNextFile();
  while (file){
    if (!file.isDirectory()){
      String fullFileName = file.name();
      if (fullFileName.endsWith(extension) || extension == "*") {
        String filename = prefix;
        // if we don't want the extension, we remmove it
        if (!withExtension) filename = concatStrings(filename,fullFileName.substring(0, fullFileName.lastIndexOf('.')));
        else filename = concatStrings(filename,fullFileName); // k is the icon ID
        list->push_back(filename);
        DEBUG_PRINT("\n\t %s",filename.c_str());
      }
    }
    file = root.openNextFile();
    yield();
  }
}

template<typename T>
bool saveStruct(const char* filename, const T& data){

    File f = LittleFS.open(filename, FILE_WRITE);
    if (!f) {
        Serial.println("Erreur ouverture fichier en écriture");
        return false;
    }

    size_t written = f.write(reinterpret_cast<const uint8_t*>(&data), sizeof(T));
    f.close();

    if (written != sizeof(T)) {
        Serial.println("Erreur ecriture structure");
        return false;
    }

    return true;
}
/*
technique utilisée dans beaucoup de firmware ESP32 qui permet de sauvegarder et charger 
automatiquement n'importe quelle struct dans SPIFFS/NVS avec une seule fonction template :
*/
template<typename T>
bool loadStruct(const char* filename, T& data){
    File f = LittleFS.open(filename, FILE_READ);
    if (!f) {
        DEBUG_ERROR("can't open file %s",filename);
        return false;
    }

    if (f.size() != sizeof(T)) {
        DEBUG_ERROR("invalid file size %s : %i/%i",filename,f.size(),sizeof(T));
        f.close();
        return false;
    }

    size_t read = f.read(reinterpret_cast<uint8_t*>(&data), sizeof(T));
    f.close();

    if (read != sizeof(T)) {
        DEBUG_ERROR("error reading %s : %i/%i",filename);
        return false;
    }

    return true;
}

bool checkAndDeleteFile(const char* filename) {
    if (LittleFS.exists(filename)) {
        DEBUG_INFO("Fichier trouvé: %s → suppression...\n", filename);

        if (LittleFS.remove(filename)) {
            DEBUG_INFO("Fichier supprimé avec succès");
            return true;
        } else {
            DEBUG_ERROR("Échec de la suppression");
            return false;
        }
    }

    //DEBUG_PRINT("ℹ️ Aucun fichier .bin.gz à la racine");
    return false;
}

typedef struct LittleFSInfo{
  size_t total;
  size_t used;
  size_t freeSpace;
} ;

void getLittleFSInfo(LittleFSInfo &info) {
  info.total     = LittleFS.totalBytes();
  info.used      = LittleFS.usedBytes();
  info.freeSpace = info.total - info.used;
}


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
  //DEBUG_PRINT("\nCompressed %d input bytes to %u raw bytes\n", len, outputSize);
  return outputSize;
}

void targzNullLoggerCallback(const char* format, ...){}

#define SAFE_SCANF_SPACE '^'
// Remplace tous les caractères 'from' par 'to' dans la chaîne donnée
String replaceAllChars(String str, char from, char to) {
    for (char &c : str) {
        if (c == from) {
            c = to;
        }
    }
    return str;
}


// Convertit une chaîne hex en octets.
// - Ignore espaces, tab, ':', '-', '_'.
// - Accepte "0x" / "0X" au début ou avant chaque octet.
// - N'écrit pas plus de outMax octets.
// Retourne true si conversion complète et paire ; false sinon.
// En cas d'échec, outLen = 0.
size_t hexStringToBytes(const String& in, uint8_t* out, size_t outMax) {
  if (!out || outMax == 0) return 0;

  auto nibble = [](char c) -> int8_t {
    if (c >= '0' && c <= '9') return (int8_t)(c - '0');
    if (c >= 'a' && c <= 'f') return (int8_t)(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return (int8_t)(c - 'A' + 10);
    return -1;
  };

  const char* p   = in.c_str();
  const char* end = p + in.length();

  bool haveHi = false;
  uint8_t hi  = 0;
  size_t outLen = 0;

  while (p < end) {
    char c = *p++;

    // Ignorer les séparateurs usuels
    if (c == ' ' || c == '\t' || c == ':' || c == '-' || c == '_' || c == ',') continue;

    // Gérer préfixe 0x/0X en frontière d’octet
    if (!haveHi && c == '0' && p < end && (*p == 'x' || *p == 'X')) { ++p; continue; }

    int8_t n = nibble(c);
    if (n < 0) return 0;  // caractère non hex

    if (!haveHi) {
      hi = (uint8_t)n;
      haveHi = true;
    } else {
      if (outLen >= outMax) return 0;  // débordement
      out[outLen++] = (uint8_t)((hi << 4) | (uint8_t)n);
      haveHi = false;
    }
  }

  // Nibble orphelin → erreur
  if (haveHi) return 0;

  return outLen;
}






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

  // Avoid division by zero or degenerate range
  if (in_max == in_min) {
    return out_min;     // or out_max, or 0 — mais jamais UB
  }

  // Clamp
  if (x < in_min) x = in_min;
  else if (x > in_max) x = in_max;

  // Safe computation (double avoids precision explosions)
  double ratio = (double)(x - in_min) / (double)(in_max - in_min);
  double out   = (double)out_min + ratio * (double)(out_max - out_min);

  // Safe rounding
  if (out > INT32_MAX) out = INT32_MAX;
  else if (out < INT32_MIN) out = INT32_MIN;

  return (int)(out + (out >= 0 ? 0.5 : -0.5));
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
        addValue("deserialize","error");
      }  
    }

    virtual const String* getString(const String name, const String defaultValue="") {
      int8_t index;
      index = getIndex(name);
      //DEBUG_PRINT("\ngetString(%s,%s)=>",name.c_str(),defaultValue.c_str());        
      if (index < 0) {
        noToken = defaultValue;
        //DEBUG_PRINT("\tno token");
        return &noToken;
      } else {        
        KeyValue* kv = keyValues.get(index);
        //DEBUG_PRINT("%s",kv->value.c_str());
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

    String* getName(uint8_t index){
      if (index>keyValues.size()) return &noToken;
      else{
        KeyValue* kv = keyValues.get(index);
        return &kv->key;
      }
    }    
    
  protected:  
    LinkedList<KeyValue*> keyValues = LinkedList<KeyValue*>(); 

    bool updateValue(uint8_t index, String value){
      KeyValue* kv = keyValues.get(index);
      kv->value = value;
      //DEBUG_PRINT("\n\tupdateValue<%d>(%s,%s)",index,getName(index)->c_str(),value.c_str());
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

      void clean(){
        // We first clean up existing values
        while (keyValues.size()>0){
          KeyValue* kv = keyValues.shift();
          delete(kv);
        }
      }

      bool load(String f,String defaultValues=""){
        bool success = true;
        
        filename = f;
        
        if (!LittleFS.exists(filename)){
          success = false;     
          //DEBUG_PRINT("\nJsonListFile::file %s doesnt exit, use default values\n",filename.c_str());
          if (defaultValues != "") deserialize(defaultValues);          
        }else{

          JsonDocument doc;
          DeserializationError error;

          if (filename.endsWith(".gz")){
            // compressed file
            ByteBuffer decompressedJson;
            size_t dataSize = 0;

            dataSize = readGzFileToBuffer(filename.c_str(),&decompressedJson);

            if (dataSize == 0){ // uncompress failed
              DEBUG_PRINT("\nJsonListFile::readGzFileToBuffer(%s) failed",filename.c_str());
              success = false;
            }else{
              // Désérialiser les données JSON
              error = deserializeJson(doc, decompressedJson.buffer);
              if (error) success = false;
            }    

          }else{
            // regular file
            File file = LittleFS.open(filename);
            error = deserializeJson(doc, file);    
            if (error) success = false;
          }

          if (success){
              doc.shrinkToFit();   
              jsonToKeyValues(&doc);
              //DEBUG_PRINT("\nJsonListFile::load(%s) ok\n",filename.c_str());
          }else{
            DEBUG_PRINT("\nJsonListFile::load(%s) failed\n",filename.c_str());
          }

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
    const boolean getBool(String name, bool defaultValue=true) {
      return (*getString(name,defaultValue?"true":"false") == "true");
    };

};

Config systemCfg = Config();
Config userCfg = Config();

///////////////////// SYSTEM PARAMETERS ////////////////////////
#define S_PUSHDELAY         "pushDelay"
#define S_DCLICREACTIVITY   "dClicReactivity"
#define S_FADERSPEED        "faderSpeed"
#define S_AP_NAME           "apName"
#define S_LANG              "lang"
#define S_PROFILE           "profile"
#define S_SSID              "ssid"
#define S_LAST_USED_SSID    "lastSSID"
#define S_BUTTON_ASSIGN     "buttonsAssignements"
#define S_SHORTCUTS         "shortcuts"
#define S_HOSTNAME          "hostname"
#define S_SWUPDATE_PROMPT   "startupUpdate"


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


I18n lang = I18n();
I18n userLang = I18n();

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
#define PERSISTANT_STARTUP        "startup"

#define MAX_RTP_DIR_ENDPOINTS 3
#define EP_DISCOVERY_TIME_OUT 6000
// ====== Liste simple des endpoints découverts ======
struct RtpmidiEndpoint {
  char name[64];        // nom d'instance (ex: "Mon MacBook Pro")
  IPAddress ip;
  uint16_t port;
  uint32_t lastSeenMs;
};

class RtpDirectory {
public:
  RtpmidiEndpoint ep[MAX_RTP_DIR_ENDPOINTS];
  int epCount;


  RtpDirectory() : epCount(0) {}

  void clear() { epCount = 0; }

  int count() const { return epCount; }

  const RtpmidiEndpoint& get(int idx) const { return ep[idx]; }

  // Upsert robuste: fusion par nom + règles IP/port
  void upsertEndpoint(const char* instanceName, const byte ipBytes[4], unsigned short port) {
    if (!ipBytes) return;

    IPAddress ip(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
    const char* name = instanceName ? instanceName : "?";
    const unsigned long now = millis();

    // 1) Déjà présent exactement (IP + port) -> refresh + nom
    for (int i = 0; i < epCount; i++) {
      if (ep[i].ip == ip && ep[i].port == port) {
        safeCopyName(ep[i].name, sizeof(ep[i].name), name);
        ep[i].lastSeenMs = now;
        return;
      }
    }

    // 2) Fusion par nom (instanceName)
    for (int i = 0; i < epCount; i++) {
      if (sameName(ep[i].name, name)) {
        // Port : on accepte la mise à jour (utile quand SRV/infos arrivent sur une autre interface)
        if (ep[i].port != port && port != 0) {
          ep[i].port = port;
        }

        // IP : anti-écrasement par link-local 169.254.x.x
        const bool oldLL = isLinkLocal169(ep[i].ip);
        const bool newLL = isLinkLocal169(ip);

        if (ep[i].ip != ip) {
          if (!newLL && oldLL) {
            // link-local -> LAN : on remplace (bon)
            ep[i].ip = ip;
          } else if (!newLL && !oldLL) {
            // Deux IP LAN possibles (Wi-Fi/Ethernet). Par défaut: on garde la première (stable).
            // Si tu préfères la dernière, décommente:
            // ep[i].ip = ip;
          } else if (newLL && !oldLL) {
            // Nouvelle link-local, ancienne LAN: on ignore l'IP reçue
          } else {
            // Deux link-local: on peut remplacer
            ep[i].ip = ip;
          }
        }

        ep[i].lastSeenMs = now;
        return;
      }
    }

    // 3) Sinon, nouvelle entrée
    if (epCount >= MAX_RTP_DIR_ENDPOINTS) return;

    RtpmidiEndpoint &e = ep[epCount++];
    safeCopyName(e.name, sizeof(e.name), name);
    e.ip = ip;
    e.port = port;
    e.lastSeenMs = now;
  }

  // Optionnel: petit "nettoyage" des entrées trop vieilles (si tu veux)
  void pruneOlderThanMs(unsigned long maxAgeMs) {
    const unsigned long now = millis();
    int w = 0;
    for (int r = 0; r < epCount; r++) {
      if ((now - ep[r].lastSeenMs) <= maxAgeMs) {
        if (w != r) ep[w] = ep[r];
        w++;
      }
    }
    epCount = w;
  }

  private:

  static bool isLinkLocal169(const IPAddress& ip) {
    return ip[0] == 169 && ip[1] == 254;
  }

  static void safeCopyName(char* dst, size_t dstSize, const char* src) {
    if (!dst || dstSize == 0) return;
    if (!src) src = "?";
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
  }

  // Comparaison "safe" en tenant compte de tailles
  static bool sameName(const char* a, const char* b) {
    if (!a || !b) return false;
    // Comparer au plus sur la longueur stockée
    return strncmp(a, b, 63) == 0; // 63 car name[64] ci-dessus
  }
};



#include "HWManager.h"
HardwareManager hwm;

enum{
  IF_NOT_READY,
  IF_READY
};

enum class QoS : int8_t {
  NOTAPPLICABLE = -1,
  UNDETERMINED,  
  CRITICAL,
  POOR,
  MEDIUM,
  GOOD
};

uint16_t QoStoColor(QoS qos) {
  switch (qos) {
      case QoS::GOOD:           return TFT_GREEN;    
      case QoS::MEDIUM:         return TFT_YELLOW;   
      case QoS::POOR:           return TFT_ORANGE;   
      case QoS::CRITICAL:       return TFT_RED;      
      case QoS::NOTAPPLICABLE:  return TFT_WHITE;
      case QoS::UNDETERMINED:   return 0x5AEB;        // = TFT_GREY
      default:                  return TFT_WHITE;    // fallback
  }
}

#include "interfaces/i_states.h"

struct State{ // used by Strip and Interface
  uint8_t code = IF_NOT_READY;
  String label = IF_STATE_NOTIMPLEMENTED;
  String subState = "subState";
  String details = "details";
  String led = "black";
  bool enabled = false;
  bool switchable = true;
  float progress = -1;

  String previousStateLabel = "";
  String previousLed = "";
  uint8_t previousStateCode = 0;
  bool previousEnabled = false;
  bool previousSwitchable = switchable;
  float previousProgress = -1;

  QoS QoSlevel = QoS::NOTAPPLICABLE;

  bool changed = false;               // this is used by the strip to determine if they have to refresh
  bool broadcastState = false;
  bool broadcastConfig = false;
};

#include "GUIManager.h"

TFT_eSPI* GUI::tft = &hwm.tft;
GUIManager gui(false);
OpenFontRender* Label::ofr = &gui.labelOfr;
OpenFontRender* Icon::ofr = &gui.iconOfr;

#include "DataManagement.h"

class ApplicationManager;

class Log {
  public:
    // Initialiser la classe avec une instance d'ApplicationManager
    static void init(ApplicationManager* manager) {
        am = manager;
    }

    // Déposer un message
    static void drop(const char* originator, const char* message);

private:
    // Pointeur vers l'instance globale
    static ApplicationManager* am;
};

//--- utilitaires URL -----------------------------------------------------------
struct UrlParts {
  String proto, host, path;
  uint16_t port = 80;
  bool ok = false;
};

static UrlParts parseUrl(const String& url) {
  UrlParts u;
  int p = url.indexOf("://");
  if (p < 0) return u;
  u.proto = url.substring(0, p);
  String rest = url.substring(p + 3);
  int slash = rest.indexOf('/');
  if (slash >= 0) {
    u.host = rest.substring(0, slash);
    u.path = rest.substring(slash);
  } else {
    u.host = rest;
    u.path = "/";
  }
  int colon = u.host.indexOf(':');
  if (colon >= 0) {
    u.port = (uint16_t)u.host.substring(colon + 1).toInt();
    u.host = u.host.substring(0, colon);
  } else {
    u.port = (u.proto == "https") ? 443 : 80;
  }
  u.ok = (u.host.length() > 0);
  return u;
}

struct StreamParams {
  Stream* stream;
  bool ownsStream = false;
  StreamParams(Stream* s,bool o):stream(s),ownsStream(o){}
};

#define UPDATE_FOLDER "/"
#define FIRMWARE_BIN_FILENAME UPDATE_FOLDER"firmware.bin.gz"
#define IMMEDIATE true

#define PRESS_TIME_TO_POWEROFF 4000     // 4s to shutdown

ApplicationManager* Log::am = nullptr;

#include "APPManager.h"

void Log::drop(const char* originator, const char* message) {
  if (am) {
      am->log(originator, message);
  }
}

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
  unsigned long start = millis();

  hwm.persistants.putUChar(PERSISTANT_STARTUP,SYSTEM_OFF);        

  while (hwm.onOff.state == ON_OFF_TRIGGER && millis()-start < 4000) hwm.onOff.refresh(); //wait button to be released
  
  hwm.setPowerRail(OFF);    

  delay(2000);

  esp_sleep_enable_ext0_wakeup(ONOFF_PIN,ON_OFF_TRIGGER); //1 = High, 0 = Low  s3    
  esp_deep_sleep_start();      
}

void xTaskBackground(void *pvParameters){  
  while (true){
  
    vTaskDelay(2);  //added for S3 to avoid trigerring the watchdog

    // network monitoring
    /*static unsigned long checkEthCableTime = 0;
    if (millis()-checkEthCableTime > ETH_CABLE_CHECK_FREQUENCY && hwm.network->ethEnabled){   // only if we use the full network stack and not only AP mode
      hwm.network->setLink(Ethernet.linkStatus());
    }*/
  
    // watch the switch off button only if it was pushed
    if (onOffPushedStartTime !=0 && digitalRead(ONOFF_PIN) == ON_OFF_TRIGGER){
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
        while (digitalRead(ONOFF_PIN)==ON_OFF_TRIGGER && millis()-t<PRESS_TIME_TO_POWEROFF){
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

/*void commonFileServerWrapperPost(Request &req, Response &res) {
  app.httpPost(req,res);
}*/

static TaskHandle_t    guiRefreshTaskHandle = nullptr;

#define STEP_MS 2
// ---------- Tâche d’animation ----------
void guiRefreshTask(void *param_type) {
  //DEBUG_PRINT("\nstartGUIThread()"); 
   
  for (int i=0;;) {
    // Vérifie si on nous demande d’arrêter (notification)
    // 0 ms de délai = on ne bloque pas; on anime puis on revérifie à chaque tour
    if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
      // Signale qu’on n’a plus de tâche active
      guiRefreshTaskHandle = nullptr;
      vTaskDelete(nullptr); // termine la tâche
    }
    vTaskDelay(pdMS_TO_TICKS(STEP_MS));
    gui.refresh();
  }
}


void startGUIThread(uint8_t screen, uint8_t state) {
  // 
  gui.purge();
  app.buildGUI(screen,state);  

  if (guiRefreshTaskHandle == nullptr) {
    xTaskCreatePinnedToCore(
      guiRefreshTask,
      "guiRefreshTask",
      4096,        // stack
      nullptr,
      2,           // priorité moyenne
      &guiRefreshTaskHandle,
      0            // core 0
    );
  }
}

// ---------- Arrêter l’animation ----------
void stopGUIThread() {
  if (guiRefreshTaskHandle) {
    // Envoie une notification; la tâche se nettoie puis s’auto-supprime
    xTaskNotifyGive(guiRefreshTaskHandle);
    // Optionnel: attendre qu’elle se termine vraiment
    // (petit spin-wait simple; vous pouvez raffiner avec un EventGroup)
    const uint32_t t0 = millis();
    app.triggerRedraw();            // we call for screen redraw, it will recall the previous registered screen
    while (guiRefreshTaskHandle && (millis() - t0 < 500)) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  //DEBUG_PRINT("\nstopGUIThread()");  
}


void scanNetworksTaskWrapper(void * parameter){
  app.scanNetworksTask(parameter);
  // Delete the task itself
  vTaskDelete(NULL);
}

#include <EthernetICMP.h>  // fixed by  https://github.com/arduino-libraries/Ethernet/pull/107/files

// Choix du socket ICMP (0–7 sur W5x00) + identifiant ICMP
void ping(PingParameters* params) {

  // NB: host resolution doesnt work
  DEBUG_INFO("[ping] %s",params->echo->host.c_str());
  if (WiFi.hostByName(params->echo->host.c_str(), params->echo->ip) == 1){

    //DEBUG_PRINT("\npingTask running...");  
    if (xSemaphoreTake(hwm.network->networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){
      if (hwm.network->media == WIFI_MEDIA){    
        if (Ping.ping(params->echo->host.c_str())) {
          params->echo->alive = true;
          params->echo->latency = Ping.averageTime();
        } else {
          params->echo->alive = false;
        }    

      }else{
        // IMPLEMENT PING FOR ETHERNET
        EthernetICMPPing ping(7, 1);
        //EthernetICMPPing::setTimeout(3000); // ms
        
        uint32_t t0 = millis();
        // Lance le ping avec 3 retries
        EthernetICMPEchoReply reply = ping(params->echo->ip, 3);
        uint32_t t1 = millis();
        if (reply.status == SUCCESS) {
          params->echo->alive = true;
          params->echo->latency = t1 - t0;   // ✅ latence en ms
        } else {
          params->echo->alive = false;
        }
      }      
      params->callback(params->echo);  // Appel du callback en cas de succès
      xSemaphoreGive(hwm.network->networkBusy);      
    }else {
        DEBUG_PRINT("\n\n\[ping] %s :: sempahore networkBusy",params->echo->host.c_str());
    }
  }else {
    DEBUG_PRINT("\n\n\[ping] %s :: host no resolvable",params->echo->host.c_str());
  }
}

void pingTask(void* params){
  ping(static_cast<PingParameters*>(params));
  // Libérer la mémoire allouée pour les paramètres
  delete (PingParameters*)params;
  // Suspendre la tâche après chaque ping (exécution ponctuelle)
  vTaskDelete(NULL);  
}


// Prototype pour la tâche de connexion
void connectHTTPTask(void *parameter) {

  if (xSemaphoreTake(hwm.network->networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){ 

    AsyncHttpClient *clientObj = static_cast<AsyncHttpClient *>(parameter);
    DEBUG_INFO("connectHTTPTask(%s,%s,%i,<%s>)",clientObj->host.toString().c_str(),clientObj->URL.c_str(),clientObj->port,clientObj->post.c_str());
     
    Client* client = clientObj->client;  

    if (clientObj->method == HTTP_METHOD_GET) clientObj->post = ""; 

    //String result = concatStrings("{\"", String(API_ORIGIN), "\":\"", clientObj->URL, "\",");

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
        DEBUG_INFO("Statut HTTP : %d\n", httpStatusCode);

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

    //result += concatStrings("\"status\":", String(status), ",\"data\":", String(payload), "}");
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
    DEBUG_WARN("connectHTTPTask: networkbusy");
  }
  vTaskDelay(1);
  vTaskDelete(NULL);
}
////////////////////////////////////////////////////////////////////////
///////////////////////                              ///////////////////
///////////////////////  FIRMWARE UPDATE FUNCTIONS   ///////////////////
///////////////////////                              ///////////////////
////////////////////////////////////////////////////////////////////////

// tar --format=ustar --exclude='.DS*' -czvf 0.13.tgz -C ./tgztest .

static String makeAbsoluteUrl(const String& baseUrl, const String& href) {
  if (href.startsWith("http://") || href.startsWith("https://")) return href;
  bool baseHasSlash = baseUrl.endsWith("/");
  if (href.startsWith("/")) {
    // base proto+host
    int p = baseUrl.indexOf("://");
    if (p >= 0) {
      int hostEnd = baseUrl.indexOf('/', p + 3);
      if (hostEnd >= 0) return baseUrl.substring(0, hostEnd) + href;
      return baseUrl + href;
    }
  }
  return baseUrl + (baseHasSlash ? "" : "/") + href;
}

//--- parsing HTML très simple: récupère hrefs terminant par .tgz ----------------
static void findFszLinksInHtml(const String& html, std::vector<String>& outLinks) {
  int pos = 0;
  while (true) {
    int idx = html.indexOf(FW_EXTENSION, pos);
    if (idx < 0) break;

    int L = -1;
    for (int i = idx; i >= 0; --i) {
      if (html[i] == '"' || html[i] == '\'') { L = i; break; }
      if (html[i] == '>') break;
    }
    int R = -1;
    for (size_t i = idx; i < html.length(); ++i) {
      if (html[i] == '"' || html[i] == '\'') { R = (int)i; break; }
      if (html[i] == '<') break;
    }
    if (L >= 0 && R > L) {
      String href = html.substring(L + 1, R);
      href.trim();
      if (href.endsWith(FW_EXTENSION)) outLinks.push_back(href);
    }
    pos = idx + 4;
  }
}

//--- extrait la version float depuis un nom "0.00.tgz" -------------------------
static bool extractFloatVersionFromFilename(const String& filename, double& outVer) {
  if (!filename.endsWith(FW_EXTENSION)) return false;
  // isole le nom de fichier sans chemin
  int slash = filename.lastIndexOf('/');
  String base = (slash >= 0) ? filename.substring(slash + 1) : filename;
  // retire l'extension
  String verStr = base.substring(0, base.length() - 4); // enlève FW_EXTENSION
  verStr.trim();
  // contrôle rapide du format (chiffres + un point, ex: 0.00)
  bool hasDot = false;
  for (size_t i = 0; i < verStr.length(); ++i) {
    char c = verStr[i];
    if (c == '.') {
      if (hasDot) return false; // un seul point attendu
      hasDot = true;
    } else if (!isDigit(c)) {
      return false;
    }
  }
  if (!hasDot) return false;

  outVer = verStr.toFloat(); // double-compatible
  return true;
}

/**
 * Recherche la version distante maximale au format "float" (ex: 0.00.tgz),
 * compare avec currentVersion ("0.00"), et si >, ouvre le flux HTTP sur ce fichier.
 *
 * @param baseUrl           URL listant les fichiers (index HTML)
 * @param currentVersion    version locale (String, ex "1.25")
 * @param outHttp           HttpClient (amcewen) — retour: flux ouvert sur le fichier si maj
 * @param outRemoteVersion  version distante retenue (String)
 * @param outFileUrl        URL absolue du fichier choisi
 * @return true si une nouvelle version est trouvée ET que le flux fichier est ouvert
 */

int8_t findNewerRemoteFWUpdate(const String& baseUrl,
                               const String& currentVersion,
                               HttpClient& outHttp,
                               String& outRemoteVersion,
                               String& outFileUrl)
{
  DEBUG_INFO("[FWUPDATE]\t\t(v%s) Listing: %s", _V, baseUrl.c_str());

  // 1) GET du listing
  UrlParts u = parseUrl(baseUrl);
  if (!u.ok) return -1;

  //DEBUG_PRINT("\t host: %s, port:%d, path: %s", u.host.c_str(), u.port, u.path.c_str());  

  outHttp.stop();
  int err = outHttp.startRequest(u.host.c_str(), u.port, u.path.c_str(), "GET", "ESP32HttpClient/1.0");
  if (err != 0) {
    DEBUG_PRINT("/!\\startRequest err=%d\n", err);
    return -1;
  }
  int status = outHttp.responseStatusCode();
  if (status != 200) {
    DEBUG_PRINT("/!\\HTTP %d sur %s", status, baseUrl.c_str());
    outHttp.stop();
    return -status;
  }
  outHttp.skipResponseHeaders();

  String html;
  while (outHttp.available()) {
    char c = outHttp.read();
    if (html.length() < 200000) html += c;
  }
  outHttp.stop();

  // 2) collecte des liens .tgz
  std::vector<String> hrefs;
  findFszLinksInHtml(html, hrefs);
  if (hrefs.empty()) {
    DEBUG_PRINT("/!\\Aucun %s trouvé!!!",FW_EXTENSION);
    return 0;
  }

  // 3) sélection de la version max (comparaison en double)
  double cur = currentVersion.toFloat();
  bool foundNewer = false;
  double best = -1.0;
  String bestUrl, bestVerStr;

  for (const auto& href : hrefs) {
    String absUrl = makeAbsoluteUrl(baseUrl, href);
    double v = -1.0;
    if (!extractFloatVersionFromFilename(absUrl, v)) continue;

    if (!foundNewer || v > best) {
      foundNewer = true;
      best = v;
      bestUrl = absUrl;

      // conserver le string affichable tel que repéré (avec 2 décimales si c’est ton format)
      int slash = absUrl.lastIndexOf('/');
      String base = (slash >= 0) ? absUrl.substring(slash + 1) : absUrl; // "0.00.tgz"
      bestVerStr = base.substring(0, base.length() - 4);                 // "0.00"
    }
  }

  if (!foundNewer) {
    DEBUG_INFO("/!\\Aucune version valide trouvée");
    return 0;
  }
  if (!(best > cur)) { // strictement supérieure
    DEBUG_INFO("Version distante %.2f <= locale %.2f", best, cur);
    return 0;
  }

  outFileUrl = bestUrl;
  outRemoteVersion = bestVerStr;
  //DEBUG_PRINT("\nNouvelle version: %s @ %s", outRemoteVersion.c_str(), outFileUrl.c_str());
  // le flux est maintenant lisible via outHttp.read()/available()

  return 1;
}

void fileProgressCallback( uint8_t fileProgress ) {
  /*static int8_t lastProgress = -1;
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes  = LittleFS.usedBytes();
  size_t freeBytes  = totalBytes - usedBytes;
  if( lastProgress != progress ) {
    if( lastProgress == -1 ) {
      DEBUG_PRINT("Progress: 0% *");
    }
    lastProgress = progress;
    switch( progress ) {
      //case   0: DEBUG_PRINT("0% ▓");  break;
      case  25: DEBUG_PRINT(" 25% ");break;
      case  50: DEBUG_PRINT(" 50% ");break;
      case  75: DEBUG_PRINT(" 75% ");break;
      case 100: DEBUG_PRINT("* 100% [%u=%u-%u]",freeBytes,totalBytes,usedBytes); lastProgress = -1; break;
      default: if( progress > 0 && progress < 100) DEBUG_PRINT( "*" ); break;
    }
  }*/
  // delta de progression dans le fichier
  uint8_t delta = (fileProgress >= lastFileProgress)
                    ? (fileProgress - lastFileProgress)
                    : fileProgress; // sécurité si ça repart à 0 sans reset

  lastFileProgress = fileProgress;
  subProgressPercent = fileProgress;

  processedSize += ((size_t)delta * (size_t)proportionalSize) / 100;

  uint8_t newPercent = (totalGzSize > 0)
    ? (uint8_t)((processedSize * 100) / totalGzSize)
    : 0;

  if (newPercent > 100) newPercent = 100;

  if (newPercent != progressPercentInt) {
    progressPercentInt = newPercent;
    overallProgressPercent = (float)progressPercentInt;
    //DEBUG_PRINT("\nOverall progress callback: %.0f",overallProgressPercent);
  }

}

void updaterProgressCallback( uint8_t progress ){
  static int8_t lastProgress = -1;
  if( lastProgress != progress ) {
    lastProgress = progress;
    if( progress > 0 && progress < 100) subProgressPercent = (float)lastProgress;
  }
}

void tarStatusProgressCallback(const char* name, size_t size, size_t total_unpacked){ 
  
  proportionalSize = size;        // doit être prêt AVANT les callbacks progress
  lastFileProgress = 0;           // reset pour le nouveau fichier  
  updateProgressLabel = name;

  if (updateProgressLabel.indexOf("html") >= 0 ) updateProgressLabel = *lang.getString("Web");
  else if (updateProgressLabel.indexOf("lang") >= 0 || updateProgressLabel.indexOf("i18n") >= 0) updateProgressLabel = *lang.getString("Translations");
  else if (updateProgressLabel.indexOf("bank") >= 0 || updateProgressLabel.indexOf("user") >= 0) updateProgressLabel = *lang.getString("Configuration");  
  else if (updateProgressLabel.indexOf("firmware") >= 0) updateProgressLabel = *lang.getString("Firmware");
  else if (updateProgressLabel.indexOf("jpg") >= 0 || updateProgressLabel.indexOf("ttf") >= 0) updateProgressLabel = *lang.getString("Graphics");
  else updateProgressLabel = *lang.getString("Data");
}

TarGzUnpacker* getTarGzUnpacker(){
  processedSize = 0;
  progressPercentInt = 0;
  overallProgressPercent = 0.0;
  updateProgressLabel = "";

  TarGzUnpacker *TARGZUnpacker = new TarGzUnpacker();    
  TARGZUnpacker->haltOnError( false ); // stop on fail (manual restart/reset required)
  TARGZUnpacker->setTarVerify( true ); // true = enables health checks but slows down the overall process
  TARGZUnpacker->setupFSCallbacks( targzTotalBytesFn, targzFreeBytesFn ); // prevent the partition from exploding, recommended

  //TARGZUnpacker->setGzProgressCallback( fileGZProgressCallback ); // no effect
  TARGZUnpacker->setTarProgressCallback( fileProgressCallback ); // prints the untarring progress for each individual file
  
  TARGZUnpacker->setTarStatusProgressCallback( tarStatusProgressCallback ); // print the filenames as they're expanded
  //TARGZUnpacker->setLoggerCallback( BaseUnpacker::targzPrintLoggerCallback  );    // gz log verbosity
  //TARGZUnpacker->setTarMessageCallback( BaseUnpacker::targzPrintLoggerCallback ); // tar log verbosity
  
  return TARGZUnpacker;
}


void notifyWebappFlashing(){
  app.broadCast("{\"freeze\":\"Firmware flash\"}",IMMEDIATE);  // inform web interface  IMMEDIATE because we don't queue the message to be send in next loop , we do it immediately.
}

void removeFreezeNotification(){
  app.broadCast("{\"unfreeze\":\"\"}",IMMEDIATE);  // inform web interface  IMMEDIATE because we don't queue the message to be send in next loop , we do it immediately.
}

bool installGZBinaryFromStream(StreamParams* params){
  bool success = false;
  app.flashBinaryInprogress = true;
  DEBUG_INFO("Peforming binary update from stream");

  GzUnpacker GZUnpacker;

  lastFileProgress = 0;           // reset pour le nouveau fichier    
  proportionalSize = totalGzSize;  // we deal with a single file here
  DEBUG_INFO("TotalGZsize = %d\n", totalGzSize);  
  
  notifyWebappFlashing();

  GZUnpacker.setGzProgressCallback(updaterProgressCallback ); // targzNullProgressCallback or defaultProgressCallback
  GZUnpacker.setLoggerCallback( BaseUnpacker::targzPrintLoggerCallback  );
  success =  GZUnpacker.gzStreamUpdater(params->stream, /*totalGzSize*/UPDATE_SIZE_UNKNOWN, 0, false );  // see https://github.com/tobozo/ESP32-targz/issues/91#issuecomment-3696925826
  if(success ) {
    app.singlePromptScreen("Firmware flashed successfully. Device will restart.","OK",STATE_UTILS_RESTART_DEVICE);
  }else{          
    removeFreezeNotification();
    app.singlePromptScreen("Firmware install failed","Abort");
  }        
  if (params->ownsStream) delete params->stream;
  
  delete params;  
  app.flashBinaryInprogress = false;

  return success;
  
}

void installGZBinaryFromStreamTask(void *parameter){
  installGZBinaryFromStream(static_cast<StreamParams*>(parameter));
  vTaskDelete(NULL);
}

void checkAndInstallBinaryFromFile() {
  //DEBUG_PRINT("\nNow checking if a bin need to be installed\n");
  // we have to rename each file from tmp to the root and check if we have a binary in the pack to install
  // then we have to clean /tmp
  
  if (LittleFS.exists(FIRMWARE_BIN_FILENAME)){
    DEBUG_INFO("found %s",FIRMWARE_BIN_FILENAME);
    File* fileStream = new File(LittleFS.open(FIRMWARE_BIN_FILENAME, FILE_READ));
    if (!fileStream) {
      app.singlePromptScreen("Firmware open error","Abort");      
      return;
    }


    totalGzSize = fileStream->size();
    app.setBinaryFlashScreen();
    screen.id = SCREEN_UTILS;
    screen.state = STATE_UTILS_SHOW_PROGRESS_SINGLE;

    StreamParams* params = new StreamParams(fileStream,true);        

    // Créer la tâche FreeRTOS pour le ping
    xTaskCreate(
      installGZBinaryFromStreamTask,         // Tâche à exécuter
      "installGZBinaryFromStreamTask",       // Nom de la tâche
      4096*2,             // Taille de la pile de la tâche
      params,           // Paramètres de la tâche (adresse IP et callbacks)
      2,                // Priorité
      nullptr
    );
    DEBUG_INFO("installGZBinaryFromStreamTask created");        
    return;
  }
  DEBUG_INFO("No binary update found");
  app.restoreScreen();
}

bool expandTGZstreamToFS(StreamParams* params){
  
  app.downloadSreamInprogress = true;
  
  Stream* httpStream = params ? params->stream : nullptr;
  bool ownsStream = params ? params->ownsStream : false;  
  DEBUG_INFO("totalGzSize: %i",totalGzSize);

  bool success = false;   
  TarGzUnpacker *TARGZUnpacker = getTarGzUnpacker();
  if (!httpStream) {
    updateSWerrorString = "Missing stream";
  } else {
    notifyWebappFlashing();
    if( !TARGZUnpacker->tarGzStreamExpander( httpStream, tarGzFS ) ) {   // https://github.com/tobozo/ESP32-targz/issues/93
      updateSWerrorString = concatStrings("Package error ", String(TARGZUnpacker->tarGzGetError())  );
    } else {
      success = true;
    }
  }
  removeFreezeNotification();
  app.broadCast("{\"askRestart\":\"EffectAtNextStartup\"}",IMMEDIATE);  // inform web interface  IMMEDIATE because we don't queue the message to be send in next loop , we do it immediately.
  
  delete TARGZUnpacker;
  if (ownsStream) delete httpStream;
  delete params;

  if (success){
    DEBUG_INFO("UntarGz successful, checking for binary to install");
    screen.id = SCREEN_UTILS;
    screen.state = STATE_UTILS_CHECK_BINARY;    
  }else{
    DEBUG_INFO("UntarGz failed: %s",updateSWerrorString.c_str());
    app.singlePromptScreen(concatStrings(*lang.getString("Update failed: "),updateSWerrorString),"Abort");    
  }
  app.downloadSreamInprogress = false; 
  return success;
}

void expandTGZstreamToFSTask(void* parameter){
  expandTGZstreamToFS(static_cast<StreamParams*>(parameter));
  vTaskDelete(NULL);
}                              

WiFiEvent_t wiFiEvent;
Event evtWiFi(&wiFiEvent);
////////////////////////////////////////////////////////////////////////
///////////////////////                              ///////////////////
///////////////////////  EVENTS HANDLERS             ///////////////////
///////////////////////                              ///////////////////
//////////////////////////////////////////////////////////////////////// 

void OnWiFiEvent(WiFiEvent_t wifiEvent){
  evtWiFi.type = EVENT_NETWORK;
  app.fireEvent(evtWiFi);
  // below line is to avoid that a call to start wifi cut a pending connection DIRTY CODE
  if (wifiEvent == ARDUINO_EVENT_WIFI_STA_CONNECTED){    
    hwm.network->wiFiTimeout = 10000; // we let 10s to get the IP and setup the wifi link
  }
}

////////////////////////////////////////////////////////////////////////
///////////////////////                              ///////////////////
///////////////////////  NETWORK DISCOVERY SERVICES  ///////////////////
///////////////////////                              ///////////////////
//////////////////////////////////////////////////////////////////////// 
// ============================================================
// Améliorations callback EthernetBonjour
// - ignore les callbacks "fin de scan" (serviceName==NULL)
// - filtre UDP + _apple-midi (tolère "_apple-midi.")
// - ignore les IP 0.0.0.0
// - laisse upsertEndpoint fusionner correctement
// ============================================================
/*static bool isZeroIP(const byte ip[4]) {
  return ip && ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

static bool isAppleMidiServiceType(const char* serviceType) {
  // Accepte "_apple-midi" ou "_apple-midi." (point final parfois présent)
  if (!serviceType) return false;
  return (strncmp(serviceType, "_apple-midi", 11) == 0);
}*/

/*void WiFiRTPDiscoveryTask(void* parameter){
    DEBUG_PRINT("\n\n[RTP-WiFi]\t\tScan RTP-MIDI lancé...\n");     
    int n = MDNS.queryService(MDNS_ANNOUNCED_APPLEMIDI_SERVICE, "udp"); // -> _apple-midi._udp
    DEBUG_PRINT("\ntrouvé %d service(s) AppleMIDI\n", n);
    for (int i = 0; i < n; i++) {
      uint8_t ipBytes[4] = {MDNS.IP(i)[0], MDNS.IP(i)[1], MDNS.IP(i)[2], MDNS.IP(i)[3]};
      rtpDir.upsertEndpoint(MDNS.hostname(i).c_str(),  ipBytes, MDNS.port(i));
    }
    //fallbackDiscoveryRtpMidiAsTask(nullptr);
    app.epDirectoryUpdated();
    vTaskDelete(NULL);
}*/

// ====== Callback EthernetBonjour quand un service est trouvé ======
// Signature d'après la doc EthernetBonjour :
// (serviceType, proto, serviceName, ip[4], port, txt)
/*void onEthernetBonjourServiceFoundCallback(const char* serviceType,
                                           MDNSServiceProtocol_t proto,
                                           const char* serviceName,
                                           const byte ipAddr[4],
                                           unsigned short port,
                                           const char* txt) {
  (void)txt; // si tu ne l'utilises pas

  // Fin de scan (timeout) : la lib rappelle une fois avec serviceName==NULL
  if (serviceName == nullptr || ipAddr == nullptr) {
    // Optionnel: rtpDir.pruneOlderThanMs(15000); // ex: purge vieux de 15s    
    app.epDirectoryUpdated();
    return;
  }

  // Sécurité : on ne garde que UDP + _apple-midi
  if (proto != MDNSServiceUDP) return;
  if (!isAppleMidiServiceType(serviceType)) return;

  // Ignore IP invalide
  if (isZeroIP(ipAddr)) return;

  // Optionnel : si tu veux ignorer complètement 169.254.* dès l'entrée,
  // tu peux le faire ici, mais je recommande de laisser upsert fusionner,
  // car on veut parfois récupérer le bon port.
  // if (ipAddr[0] == 169 && ipAddr[1] == 254) return;

  rtpDir.upsertEndpoint(serviceName, ipAddr, port);
}*/



//////////////////////////////////////////////////////////////////////////
///////////////////////                                ///////////////////
///////////////////////  MENU ACTION ITEMS CALLBACKS   ///////////////////
///////////////////////                                ///////////////////
////////////////////////////////////////////////////////////////////////// 


void back_menu_item_selected(MenuComponent* p_menu_component) {
  //DEBUG_PRINT("\nback_menu_item_selected: ");
  if (!app.ms.back()) app.restoreScreen();    
  else app.menuSystemFullRefresh();
}

void shutdown_menu_item_selected(MenuComponent* p_menu_component) {
  app.shutdownInterfaces();
  goDeepSleep();
}

void brightness_menu_item_selected(MenuComponent* p_menu_component) {
  /*if (GUI::systemBrightness == 1.0) GUI::systemBrightness = 0.5;
  else GUI::systemBrightness = 1.0;
  app.triggerRedraw(); // force redraw*/
  screen.id = SCREEN_UTILS;
  screen.state = STATE_UTILS_BRIGHTNESS;
}

void fmwUpdate_menu_item_selected(MenuComponent* p_menu_component) {
  app.SWupdatePromptScreen(0);
}

void save_profile_item_selected(MenuComponent* p_menu_component) {
  //DEBUG_PRINT("\nsave_profile_item_selected ");
}

void tft_rotation_item_selected(MenuComponent* p_menu_component) {
  hwm.swapButton = true;
  if (hwm.persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO) == INVERTED_SCREEN_YES) hwm.persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO);
  else hwm.persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_YES);
  hwm.setScreenAndButtonRotation();
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
    if (app.webAppService != nullptr){
      // Do not re-run enable flow when already enabled.
      // Repeated switchDevice(true) calls can reserve network capacity multiple times on Ethernet.
      if (!app.webAppService->state.enabled) app.webAppService->switchDevice(true);
    }
    if (app.webAppService != nullptr && app.webAppService->state.code == IF_READY){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTWEBAPP;    
    }else{
      // There is no valid connection, so we launch AP mode    
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_CONNECTCONFIGMEDIA;
      hwm.startAP(systemCfg.getString(S_AP_NAME));
      app.setAPIcon();
      hwm.configMedia->registerEventListener((EventListener*)&app);

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
  if (lang.load(concatStrings("/", langName, ".lang"))){
    systemCfg.setValue(S_LANG,langName);    
    systemCfg.save();
    app.ms.back();
    app.restoreScreen();    // return to the caller
    p_menu_component->success = true;
  }else{
    p_menu_component->success = false;
    app.menuTopText = *lang.getString("File Error");
  }  
}

void switchToBank_item_selected(MenuComponent* p_menu_component){
  //DEBUG_PRINT("\n%s",p_menu_component->get_name());
  
  // retrieve the bank by its name and assign its index as the screen substate 
  app.getBank(p_menu_component->get_name(),&screen.state);
  // and jump to the bank
  screen.id = SCREEN_BANK;  

}

void menu_selected(MenuComponent* p_menu_component){
  //DEBUG_PRINT("\n\t\t ====> MENU %s SELECTED <====",p_menu_component->get_name());
  
  if (strcmp(p_menu_component->get_name(),app.langMenuString) == 0){
    // this doesn't work as the app.ms selected scrool the item above LANG and not LANG itself since the menuSelected action is not complete yet (we are inside)
    /*for (int i = 0; i < app.langMenu->get_num_components(); ++i) {
      MenuComponent const* cp_m_comp = app.langMenu->get_menu_component(i);
      String country = String(cp_m_comp->get_name()).substring(2,String(cp_m_comp->get_name()).length());
      DEBUG_PRINT("\n%s <-> %s",cp_m_comp->get_name(),app.systemCfg.getString(S_LANG)->c_str());              
      if (strcmp(country.c_str(),app.systemCfg.getString(S_LANG)->c_str()) == 0) break;
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
  gui.add(new Icon(&hwm.network->networkConnectivityIcon,Align::Left,&GUI::zero, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.networkConnectivityIconColorIdle,&GUI::defaultBcolor));               
  gui.add(new Icon(&app.apIcon,Align::Left,&app.apIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.apIconColor,&GUI::defaultBcolor));                  
  gui.add(new Icon(&app.batIcon,Align::Left,&app.batIconPosX, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.batIconColor,&GUI::defaultBcolor));          
}


void bankOverlayBuilder(uint8_t BankIndex){
  
  baseOverLayBuilder(BankIndex);

  gui.add(new Icon(&app.interfaceIcon,Align::Left,&app.iconRow2, &app.overlayIconY,OVERLAY_ICON_SIZE,&app.interfaceIconColor,&GUI::defaultBcolor,&app.interfaceIconBrightness));               

  //we do not display the bankNavigator if user deactivate the bank screen display
  if (app.disabledBankRendering == true) return;

  Interval::Bank* bank = app.banks[BankIndex];
  
  //DEBUG_PRINT("\n\n\n--------------------------------- bankOverlayBuilder %s ---------------------------------\n\n ",bank == nullptr?"nullptr":bank->name.c_str());
  if (bank != nullptr){
    app.navBar = new BankNavigator(bank,true);   
    gui.add(app.navBar);
  }

}



//////////////////////////////////////////////////////////////
////                SCREENBUILDERS CALLBACKS           ///////
//////////////////////////////////////////////////////////////

void bankScreenBuilder(uint8_t subScreen){  
  app.userScreenBuilder(SCREEN_BANK,subScreen);

  if (app.disabledBankRendering == true) return;

  bool oversize = (subScreen >= app.banks.size());

  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = TFT_FCOLOR;
  GUI::cls();
  
  Interval::Bank* bank = app.banks[subScreen];
  //DEBUG_PRINT("\n\n\n---------------------------------bankScreenBuilder ");

  if (bank != nullptr && !oversize){

    Strip* strip = app.getSelectedStrip();
    app.stripHeader = new StripHeaderWidget(strip);      
    gui.add(app.stripHeader);         

    if (strip != nullptr){

      app.interfaceIcon = *strip->interface->getIcon();                                             // retrieve interface icon from the current strip
          
      for (const auto& winstrip : bank->strips) {
        
        winstrip->visible = false;
        
        if (bank->layout == LAYOUT_DETAILS){        
          // affiche les strips de la fentre courant et les rends "visible"
          if (winstrip->win == strip->win) winstrip->pushToGUI(LAYOUT_DETAILS);
        }else{
          // ou uniquement un strip
          if (winstrip->index == strip->index) winstrip->pushToGUI(LAYOUT_ZOOM);          
        }
      }
      
      strip->focus(true);
                  
    }else{
      DEBUG_WARN("NO STRIP TO BUILD, EMPTY BANK");
      gui.add(new TextArea(lang.getString("No Strip"), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
    }
  }else{
      DEBUG_WARN("NO BANK");
      gui.add(new TextArea(lang.getString("No Bank"), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  }
  
  //DEBUG_PRINT("\n--------------bankScreenBuilder exit-----------------");
}

void addTopandBottomMenuLabel(){
  gui.add(new TextArea(&app.menuTopText, TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  gui.add(new Label(&app.menuBottomLabel, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,MENU_TITLE_SIZE,32, &GUI::defaultFcolor,&GUI::defaultBcolor));
}

void addMenuWidgets(){
  GUI::cls();

  addTopandBottomMenuLabel();

  gui.add(new Icon(&app.menuItems[0].icon, Align::Center,&GUI::leftMenuIconXpos, &GUI::menuIconYpos,MENU_ICON_SIZE,&app.menuItems[0].color,&GUI::defaultBcolor));  
  gui.add(new Icon(&app.menuItems[1].icon, Align::Center,&GUI::midScreenHoriz, &GUI::menuIconYpos,MENU_ICON_SIZE,&app.menuActiveIconColor,&GUI::defaultBcolor));
  gui.add(new Icon(&app.menuItems[2].icon, Align::Center,&GUI::rightMenuIconXpos, &GUI::menuIconYpos,MENU_ICON_SIZE,&app.menuItems[2].color,&GUI::defaultBcolor));    
}

void setMenuColors(){
  GUI::defaultBcolor = GUI::menuBackGroundColor;
  GUI::defaultFcolor = GUI::menuActiveIconColor;  
}

void menuScreenBuilder(uint8_t subScreen){
  setMenuColors();
  addMenuWidgets();
  //LogUDP::sendLog("menuScreenBuilder",hwm.network->udp);
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
    hwm.persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
    app.restoreScreen();
  } 
}

void bootScreen(uint8_t startupState){
  switch(startupState){
    case BATTERY_STARTUP:
      GUI::tft->fillScreen(TFT_BLUE);
    break;
    case RESET_STARTUP:
      GUI::tft->fillScreen(TFT_GREEN);
    break;
    case NO_FS_STARTUP:
      GUI::tft->fillScreen(TFT_RED);
    break;
    case USB_CDC_AND_FWUPDATE_STARTUP:
      gui.drawJpg(0, 62, "/fwboot.jpg");
    break;
    default :
      if (LittleFS.exists(CUSTOM_LOGO_FILE))
        gui.drawJpg(0, 62, CUSTOM_LOGO_FILE);
      else
        gui.drawJpg(0, 62, "/logo.jpg");
    break;
  }
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
      gui.add(new Label(lang.getString("Waiting network"), Align::Center,&GUI::midScreenHoriz, &GUI::midScreenVertical,MENU_TITLE_SIZE,32,&GUI::defaultFcolor,&GUI::defaultBcolor));   
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
  //gui.add(new Icon(&app.byeIcon,Align::Center,&midScreenHoriz, &app.bigWidgetMidY,WIDGET_BIG_WIDTH,&app.byeIconColor,&GUI::defaultBcolor));                     
  gui.add(new Icon(app.byeIcon,Align::Center,&GUI::midScreenHoriz, &GUI::iconMidY,ICON_MAX_SIZE,app.byeIconColor,&GUI::defaultBcolor));                     
}

void startupScreenBuilder(uint8_t id){
  gui.drawJpg(0,0,"/webapp/logo.jpg");
}

#define ledRadius  TFT_WIDTH/17
int16_t ledLabelY = GUI::midScreenVertical + ledRadius + SIZE_18;
String leftLedColor = "blue"; 
String centerLedColor = "blue";
String rightLedColor = "blue"; 

void promptScreenBuiler(uint8_t id){
  setMenuColors();
  GUI::cls(GUI::menuBackGroundColor);
  
  gui.add(new TextArea(&app.promptParams.message, TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));

  leftLedColor = "grey"; 
  centerLedColor = "grey";
  rightLedColor = "grey"; 

  if (app.promptParams.labels[0] != ""){
    gui.add(new Label(&app.promptParams.labels[0], Align::Center,&GUI::hquarter1xPos, &ledLabelY,SIZE_18,24,&GUI::defaultFcolor,&GUI::defaultBcolor));
    leftLedColor = "blue";
  }
  gui.add(new Led(&GUI::hquarter1xPos, &GUI::midScreenVertical, ledRadius, &leftLedColor, &GUI::fullBrightness));  
  
  if (app.promptParams.labels[1] != ""){
    gui.add(new Label(&app.promptParams.labels[1], Align::Center,&GUI::midScreenHoriz, &ledLabelY,SIZE_18,24,&GUI::defaultFcolor,&GUI::defaultBcolor));    
    // we only display the arc if there is a timeout
    if (app.promptParams.timeoutMs > 0) gui.add(new Arc(&app.promptRemainingTime, 0, app.promptTimeoutMax, &GUI::midScreenHoriz, &GUI::midScreenVertical, ledRadius+8, &GUI::blue, &GUI::transparentColor, 0));
    centerLedColor = "blue";
  }
  gui.add(new Led(&GUI::midScreenHoriz, &GUI::midScreenVertical, ledRadius,  &centerLedColor, &GUI::fullBrightness));  
 
    
  if (app.promptParams.labels[2] != ""){
    gui.add(new Label(&app.promptParams.labels[2], Align::Center,&GUI::hquarter4xPos, &ledLabelY,SIZE_18,24,&GUI::defaultFcolor,&GUI::defaultBcolor));    
    rightLedColor = "blue";
  }
  gui.add(new Led(&GUI::hquarter4xPos, &GUI::midScreenVertical, ledRadius,  &rightLedColor, &GUI::fullBrightness));  
  
}


void addLeftRightIcons(){
    //DEBUG_PRINT("addLeftRightIcons()");
    GUI::cls(GUI::defaultBcolor);  
    gui.add(new Icon(&app.bigLeftIcon, Align::Right,&GUI::hquarter2xPos,&GUI::menuIconYpos,MENU_ICON_SIZE,&app.bigLeftIconColor,&GUI::defaultBcolor));
    gui.add(new Icon(&app.bigRightIcon, Align::Left,&GUI::hquarter3xPos,&GUI::menuIconYpos,MENU_ICON_SIZE,&GUI::menuActiveIconColor,&GUI::defaultBcolor));
    addTopandBottomMenuLabel();    
}

void setAPIconAndText(){
  gui.add(new TextArea(lang.getString("Connect WiFi Access Point "), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  app.bigLeftIcon = ICON_AP;
}

void buildNetworkScreen(){
  GUI::cls();

  app.bigRightIcon = ICON_LAPTOP;
  app.configIconGlowingRedColor.value = &app.bigLeftIconColor;  
  //gui.add(new AnimatedIcon(app.iconRolling,2,Align::Center,&midScreenHoriz, &app.midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));              
  #ifdef USE_NETWORK
    if (!hwm.network->ethEnabled){
      //if ethernet bank is absent, we only offer to connect the AP
      setAPIconAndText();                    
    }else{
      gui.add(new TextArea(lang.getString("Connect"), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      app.bigLeftIcon = ICON_MIXETHWIFI;
    }
  #else
    setAPIconAndText();
  #endif

  gui.add(new Icon(&app.bigLeftIcon, Align::Right,&GUI::hquarter2xPos,&GUI::menuIconYpos,MENU_ICON_SIZE,&app.bigLeftIconColor,&GUI::defaultBcolor));
  gui.add(new Icon(&app.bigRightIcon, Align::Left,&GUI::hquarter3xPos,&GUI::menuIconYpos,MENU_ICON_SIZE,&GUI::menuActiveIconColor,&GUI::defaultBcolor));

  gui.add(new Label(&hwm.configMedia->SSID, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,MENU_TITLE_SIZE,32,&GUI::defaultFcolor,&GUI::defaultBcolor));
}

#define PROGRESSBAR_WIDTH 150
int16_t halfProgressBarpix = TFT_WIDTH/2 - PROGRESSBAR_WIDTH/2;
void buildIconProgressScreen(){
  gui.add(new Icon(&app.iconProgress, Align::Center,&GUI::midScreenHoriz, &GUI::menuIconYpos,MENU_ICON_SIZE/1.3,&app.menuActiveIconColor/*&GUI::blue*/,&GUI::defaultBcolor));
  gui.add(new Label(&updateProgressLabel, Align::Center,&GUI::midScreenHoriz, &GUI::bottomTextY,SIZE_20,64,&GUI::defaultFcolor,&GUI::defaultBcolor));            
  gui.add(new ProgressBarH(&subProgressPercent, 0.0,100.0 , &halfProgressBarpix /*x*/, &GUI::belowBottomTexty /*y*/, PROGRESSBAR_WIDTH /*w*/, 8 /*h*/, 2 /*offset*/, 2 /*radius*/, &GUI::blue /*fc*/ , &GUI::gold/*bc*/, &GUI::fullBrightness));
}

int16_t brightYpos = GUI::menuIconYpos*1.2;

String percentString = "%";

void utilsScreenBuilder(uint8_t state){
  //DEBUG_PRINT("\nutilsScreenBuilder(%d)",state);
  setMenuColors();  // in case it's called directly from the wizard@startup
  GUI::cls(GUI::menuBackGroundColor);  
  switch (state) {
    case STATE_UTILS_BRIGHTNESS:{
      //DEBUG_PRINT("\nSTATE_UTILS_BRIGHTNESS");
      //gui.add(new ProgressBarH(&GUI::systemBrightness /*v*/, BRIGHTNESS_MIN /*min*/,BRIGHTNESS_MAX /*max*/, &GUI::zero /*x*/, &app.menuIconYpos /*y*/, TFT_WIDTH /*w*/, TFT_HEIGHT/6 /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::gold /*fc*/ , &GUI::blue/*bc*/, &GUI::fullBrightness));
      gui.add(new Label(lang.getString("BRIGHTNESS"), Align::Center,&GUI::midScreenHoriz, &GUI::three,MENU_TITLE_SIZE*0.8,16,&GUI::defaultFcolor,&GUI::defaultBcolor));            
      gui.add(new Arc(&GUI::systemBrightness, BRIGHTNESS_MIN,BRIGHTNESS_MAX , &GUI::midScreenHoriz, &GUI::midScreenVertical, 90, &GUI::gold, &GUI::transparentColor,30));              
      gui.add(new Icon(&app.brightIcon, Align::Center,&GUI::midScreenHoriz,&brightYpos,80,&GUI::menuActiveIconColor,&GUI::defaultBcolor));
      gui.add(new NumericLabel(&GUI::systemBrightness, 0, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor,&GUI::fullBrightness,"%%",100));            // alter = 100%
    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_SHOW_PROGRESS_MULTIPLE :{      
      gui.add(new Arc(&overallProgressPercent /*v*/, 0 /*min*/,100 /*max*/, &GUI::midScreenHoriz, &GUI::midScreenVertical, 90, &GUI::blue, &GUI::transparentColor,30));              
      // this screen is shared with firmware update and binary install and others...
      buildIconProgressScreen();
    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_SHOW_PROGRESS_SINGLE :{
      // this screen is shared with firmware update and binary install and others...
      buildIconProgressScreen();
    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_INFO:{
      //DEBUG_PRINT("\n\nCPU freq:%i",getCpuFrequencyMhz());     
      
      gui.add(new Label(lang.getString(concatStrings("v",_V)),Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,SIZE_20,32,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(&app.vBatLabel, Align::Center,&GUI::midScreenHoriz,&GUI::midScreenVertical ,CONFIG_TEXTAREA_MESSAGE_SIZE,16,&GUI::menuActiveIconColor,&GUI::menuBackGroundColor));      

      gui.add(new Label(&app.FSstat, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,CONFIG_TEXTAREA_MESSAGE_SIZE,54,&GUI::defaultFcolor,&GUI::defaultBcolor));      
      gui.add(new Label(&app.heapSize,Align::Center,&GUI::midScreenHoriz,&GUI::menuIconYpos,24,24,&GUI::defaultFcolor,&GUI::defaultBcolor));              


    }break;
    ////////////////////////////////////////////////////////////////////////////////
    case STATE_UTILS_CONNECTCONFIGMEDIA:
      buildNetworkScreen();
      //DEBUG_PRINT("\nSTATE_UTILS_CONNECTCONFIGMEDIA: %s",app.menuTopText.c_str());
    break;
    case STATE_UTILS_CONNECTWEBAPP:
      app.configIconGlowingRedColor.value = &GUI::unUsedColor;
      app.bigLeftIconColor = GUI::menuActiveIconColor;
      app.menuBottomLabel = ""; // erase the title of the previous menu item
      addLeftRightIcons();  
      app.iconPending[0] = ICON_TIMER;
      app.iconPending[1] = " ";
      //gui.add(new AnimatedIcon(app.iconPending,TWOFRAMEICON,Align::Center,&GUI::midScreenHoriz, &GUI::midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));                    
      gui.add(new TextArea(lang.getString("Open browser"), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(&hwm.configURL, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,30,64,&GUI::defaultFcolor,&GUI::defaultBcolor));      
      app.bigLeftIcon = ICON_LAPTOP;
      app.bigRightIcon = ICON_FOOTSWITCH;
    break;
    case STATE_UTILS_WEBAPPCONNECTED:         
      addLeftRightIcons();
      app.iconPending[0] = ICON_LEFTDOT;
      app.iconPending[1] = ICON_RIGHTDOT;      
      gui.add(new AnimatedIcon(app.iconPending,TWOFRAMEICON,Align::Center,&GUI::midScreenHoriz, &GUI::midScreenVertical,TFT_WIDTH/8,ANIMATE_PING_PONG,250,&GUI::widgetSubLabelColor,&GUI::menuBackGroundColor));                    
      gui.add(new TextArea(lang.getString("Press any button"), TFT_WIDTH*0.90,Align::Center,Align::Center,&GUI::midScreenHoriz,&GUI::tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Label(lang.getString("Connected"), Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,MENU_TITLE_SIZE,32,&GUI::defaultFcolor,&GUI::defaultBcolor));      

      app.bigLeftIcon = ICON_LAPTOP;
      app.bigRightIcon = ICON_FOOTSWITCH;
      //DEBUG_PRINT("\nSTATE_UTILS_WEBAPPCONNECTED: %s",app.menuTopText.c_str());      
    break;
    case STATE_UTILS_BLINK_ICON_TEXT:
      gui.add(new Label(&app.utilsBlinkLabel, Align::Center,&GUI::midScreenHoriz, &GUI::menuTitleY,40,32, &GUI::defaultFcolor,&GUI::defaultBcolor));
      gui.add(new Icon(&app.utilsBlinkIcon, Align::Center,&GUI::midScreenHoriz, &GUI::menuIconYpos,MENU_ICON_SIZE,&app.blinkColorBlue,&GUI::defaultBcolor));
    break;
    ////////////////////////////////////////////////////////////////////////////////
  }
}

  //////////////////////////////////////////////////////////////////////
 ////                SERVER SOCKET CALLBACKS                   ///////
////////////////////////////////////////////////////////////////////

void socketMessageCallback(IWebSocket &ws, const WebSocketDataType dataType, const char *message, uint16_t length){
  String response;
  if (app.webAppService != nullptr) app.webAppService->reportActivity();
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
  DEBUG_INFO("\t!!! socketCloseCallback =>code:%i length:%i\n",code,length);
  //DEBUG_PRINT("\nDump after socketCloseCallback %s",hwm.network->dumpW5500Sockets().c_str());
  app.webAppConnected = false;
  // Keep the web service alive and only refresh activity timer here.
  // Stopping the service from this callback could delete the websocket server while listen() is on the stack.
  if (app.webAppService != nullptr) app.webAppService->reportSocketClosed();
}


void socketConnectionCallback(IWebSocket &ws) {
  DEBUG_INFO("[WSS]\t\tconnected!\n\n");
  app.webAppConnected = true;
  if (app.webAppService != nullptr) app.webAppService->reportSocketConnected();

  const auto protocol = ws.getProtocol();
  if (protocol) {
    //DEBUG_PRINT("\nProtocol: %i",protocol);
  }

  ws.onMessage(socketMessageCallback);
  ws.onClose(socketCloseCallback);

  app.configClientIP = ws.getRemoteIP().toString();
  DEBUG_INFO("[WSS]\t\tNew websocket client IP: %s\n\n",app.configClientIP.c_str());

  // any message to be send straigth after connexion was established would be put here.
  // app.broadCast(app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivity\"}"));  // inform webclient               could be done via ws.send as well  
  // it should only be short message as the socketbuffer out is quite small, hence the fullCOnfig is requested by Post instead
  /*String fullConfig = app.parseJsonInput("{\"command\":\"retrieve\",\"param\":\"fullConfig\"}"); 
  ws.send(WebSocketDataType::TEXT, fullConfig.c_str(), fullConfig.length());*/
}

#endif
