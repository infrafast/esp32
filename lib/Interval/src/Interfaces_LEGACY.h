#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <Arduino.h>

#include <Ethernet.h>
#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

#ifdef DEBUG_OSC  
  #define OSC_DEBUG_PRINT(...)   Serial.printf(__VA_ARGS__); Serial.flush() //ets_printf(__VA_ARGS__) //
#else
  #define OSC_DEBUG_PRINT(...) 
#endif


#define INTERFACE_NAME "name"
#define INTERFACE_KEY "interface"
#define INTERFACE_PROGRESS "progress"

#define EXEC_OSC "osc"
#define EXEC_ADDRESS "address"
#define EXEC_VALUE "value"

///////////////////// DEVICE I/O CAPTURE ////////////////////////
enum{
    NO_CAPTURE,
    CAPTURE_IN,
    CAPTURE_OUT,
    CAPTURE_INOUT
};


class DeviceInterface{
    public:
        String name="noname";
        uint8_t recordMode = NO_CAPTURE;
        uint8_t interfaceState = DISCONNECTED, oldInterfaceState = interfaceState ;        
        bool changedState = false;

        DeviceInterface(String, uint8_t);
        ~DeviceInterface();
        
        String serialize();

        bool execute(const JsonObject& data);
        bool monitor(JsonObject& data);

        virtual uint8_t progress()=0;
        virtual void connect()=0;
        virtual void disconnect()=0;           
        

    private:
        virtual bool parseOut(const JsonObject& data) = 0;
        virtual bool parseIn(JsonObject& data) = 0;  
     
};

enum{
    CAPTURE_PLAY,
    CAPTURE_RECORD,
    CAPTURE_STOP
};

struct CaptureDescriptor{
    DeviceInterface* interface;
    uint8_t state;
    File file;
};

class InterfaceRepository{
  public:
   
    void record(const JsonObject& data){
        // first we parse all interfaces to see if we find one that match the name    
        for (const auto& descriptor : descriptors){
            String interfaceName = data[INTERFACE_NAME];
            if (descriptor.interface->name == interfaceName){
                if (descriptor.state == CAPTURE_RECORD){                                        
                    // record all data, except data[INTERFACE_NAME] to avoid storing too much data
                    data.remove(interfaceName);
                    // open file if not existing
                    if (!descriptor.file) {
                        // Ouvrir en mode "append"
                        /*descriptor.file = LittleFS.open(, "w");

                        if (!descriptor.file) {
                            DEBUG_PRINT("Erreur à l'ouverture du fichier en append.");
                        } else {
                            DEBUG_PRINT("Fichier ouvert en mode append.");
                        }*/
                    }                    
                }
            }
        }
    }

    void changeState(String name, uint8_t newState){
        // first we parse all interfaces to see if we find one that match the name    
        for (auto& descriptor : descriptors){
            if (descriptor.interface->name == name){
                if (descriptor.state == CAPTURE_STOP){
                    // file.close();
                }
            }
            descriptor.state = newState;
        }        
    }

    void retrieve(String name){
        // THIS CODE IS CALLED BY THE STRIP ROUTINE THAT REPLAY A MACRO
        for (const auto& descriptor : descriptors){
            if (descriptor.interface->name == name){
                
            }
        }         
    }

    void registerInterface(DeviceInterface* device){
        CaptureDescriptor cd = {device,CAPTURE_STOP};
        descriptors.push_back(cd);
    }

    void remove(DeviceInterface* device) {
        for (auto it = descriptors.begin(); it != descriptors.end(); ++it) {
            if (it->interface == device) {
                descriptors.erase(it);
                break; // On s’arrête après la première occurrence
            }
        }
    }

    
    std::vector<CaptureDescriptor> descriptors;

};

InterfaceRepository interfaceRepo;

DeviceInterface::DeviceInterface(String _name, uint8_t _mode = NO_CAPTURE):name(_name),recordMode(_mode){
    interfaceRepo.registerInterface(this);
}

DeviceInterface::~DeviceInterface(){
    interfaceRepo.remove(this);
}

String DeviceInterface::serialize(){
    JsonDocument doc;
    doc[INTERFACE_KEY][INTERFACE_NAME] = name;
    doc[INTERFACE_KEY]["interfaceState"] = interfaceState;
    doc[INTERFACE_KEY][INTERFACE_PROGRESS] = progress();

    doc.shrinkToFit();
}

bool DeviceInterface::execute(const JsonObject& data){
    /*String s;      
    serializeJson(data, s);
    DEBUG_PRINT("\nexecute %s",s.c_str());*/
    if (recordMode == CAPTURE_OUT) interfaceRepo.record(data);
    return parseOut(data);
};

bool DeviceInterface::monitor(JsonObject& data){
    changedState = false;
    // do we have data ?        
    if (parseIn(data)){
        data[INTERFACE_NAME] = name;        
        // if yes, is this interface supposed to record data coming in  ?
        if (recordMode == CAPTURE_IN) interfaceRepo.record(data);
    }

    if (interfaceState != oldInterfaceState){
        changedState = true;
        oldInterfaceState = interfaceState;
    }

    return changedState;            
}

///////////////////// DATA STRUCTURES ////////////////////////

typedef std::function<void(bool, int)> OSCPingCallback;    // ping success true/false, port

class OSCInterface;

struct OSCPingParameters {
  String address;
  JsonObject data;
  IPAddress ip;
  int port;
  bool isPinging = false;
  uint16_t timeOut = 32;
  OSCPingCallback callback;
  OSCInterface* osc;
};

void OSCpingTask(void* parameter);

class OSCInterface: public DeviceInterface{
    public:
        IPAddress ip;
        int port;
        uint16_t latency = 49;      // default reasonnable latency
        unsigned long RXtimerStart = 0;        
        
        char buffer[64];

        OSCInterface(String name, uint8_t capture):DeviceInterface(name,capture){
            latency = hwm.network->LANlatency;
        }

        uint8_t progress(){
            return 100; //%
        }

        virtual void init(){
        }

        virtual bool parseOut(const JsonObject& command){
            bool success = true;

            String s;
            serializeJson(command, s);
            DEBUG_PRINT("\n%s parseOut %s",name.c_str(),s.c_str());

            // {key: value}§
            // {"/bus/03/config/name": «laurent»}
            // {"/ch.3.mix.sends.2.on" :1 }
            JsonPair kv = *command.begin();  // pointeur sur l’unique entrée

            const char* address = kv.key().c_str(); // ex: "osc"
            JsonVariant val = kv.value().as<JsonVariant>(); // ex: {"address": "...", "value": ...}
            
            if (val.isNull()) {
                // no value
                SendCmd2OSC(address);
            } else if (val.is<int>()) {
                SendInt2OSC(address,val.as<int>());
            }else if (val.is<float>()) {
                SendFloat2OSC(address,val.as<float>());
            } else if (0 /*BLOB, etc...*/) {
                /*Serial.println("Type : bool");
                bool v = val.as<bool>();*/
            } else {
                success = false;
            }
            return success;
        }

        virtual bool parseIn(JsonObject& data){
            if (hwm.network->udp == NULL){
                //OSC_DEBUG_PRINT("\n<<OSCReadIn : null UDP");
                return false;                
            } 

            // Check if packet was received
            int size = hwm.network->udp->parsePacket();
            if (size < 1) return false;

            OSCMessage msg;

            while (size--){
                msg.fill(hwm.network->udp->read());
                yield();
            }

            // Check for errors
            if (msg.hasError()) {
                //DEBUG_OSCMSG("error: %s\n", msg.getError());
                return false;
            }                       

            int numberOfValues = msg.size();

            for (int i = 0; i < numberOfValues; i++) {
                char type = msg.getType(i);

                switch (type) {
                case 's': {
                    char str[64];
                    msg.getString(i, str, sizeof(str));
                    data[EXEC_VALUE][i] = str;
                    break;
                }
                case 'i':
                    data[EXEC_VALUE][i] = msg.getInt(i);
                    break;
                case 'f':
                    data[EXEC_VALUE][i] = msg.getFloat(i);
                    break;
                case 'T':
                    data[EXEC_VALUE][i] = true;
                    break;
                case 'F':
                    data[EXEC_VALUE][i] = false;
                    break;
                case 'b': { // blob (tableau d’octets)
                    int blobSize = msg.getBlobLength(i);
                    const uint8_t* blobData = msg.getBlob(i);
                    // Créer un tableau JSON pour le blob
                    JsonArray blobArray = data[EXEC_VALUE][i].createNestedArray();
                    for (int j = 0; j < blobSize; j++) {
                        blobArray.add(blobData[j]);
                    }
                    break;
                }                    
                default:
                    data[EXEC_VALUE][i] = "unsupported";
                    break;
                }
            }

            // Ajoute l’adresse OSC comme champ
            
            msg.getAddress(buffer);
            data[EXEC_ADDRESS] = buffer;            

            return true;            

        }
        
        virtual void connect(){}
        virtual void disconnect(){}
   
        void setClient(const IPAddress defaultIP, const int defaultPort){
            ip = defaultIP;
            port = defaultPort;
            OSC_DEBUG_PRINT("\nOSC.setClient(%s,%d)",ip.toString().c_str(),port);
        }
        
        bool sendOSCMessage(OSCMessage *msg, const IPAddress _ip, const int _port){           
            //OSC_DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),_ip.toString().c_str(),_port);
            if (hwm.network->udp == NULL){
                OSC_DEBUG_PRINT(":null UDP",msg->getAddress());
                return false;
            }             
            hwm.network->udp->beginPacket(_ip, _port);
            msg->send(*hwm.network->udp);
            hwm.network->udp->endPacket();

            msg->empty(); 
            return true;          
        }

        void SendCmd2OSC(const char* address){
            SendCmd2OSC(address,ip,port);
        }

        void SendCmd2OSC(const char* address, const IPAddress _ip, const int _port){                        
            OSCMessage msg(address);
            sendOSCMessage(&msg,_ip,_port);
        }

        void SendInt2OSC(const char* address, int val){            
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port)) OSC_DEBUG_PRINT("%i",val);          
        }

        void SendFloat2OSC(const char* address, float val){    
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port)) OSC_DEBUG_PRINT("%.2f",val);         
        }        

        // Fonction pour démarrer la tâche de ping non bloquante
        virtual bool asyncPing(OSCPingParameters* params) {
            OSC_DEBUG_PRINT("\nOSCasyncPing %s %s:%d T.O:%d",params->data->expectedStr,params->ip.toString().c_str(),params->port,params->timeOut);
            if (params->isPinging){
                OSC_DEBUG_PRINT("\t=>ping pending, no more allowed, stop.");
                return false;
            }
            params->isPinging = true;
            // Créer la tâche FreeRTOS pour le ping
            xTaskCreatePinnedToCore(
                OSCpingTask,         
                "OSCPingTask",       
                4096*2,             
                params,           
                2,                
                NULL,   
                APP_CPU_NUM  
            );
            return true;
        }

        bool blockingPing(String address,JsonObject& data, uint32_t timeOut){
            return blockingPing(ip,port,address,data, timeOut);
        }

        bool blockingPing(IPAddress _ip, int _port, String address, JsonObject& data, uint32_t timeOut = 0){
            //OSC_DEBUG_PRINT("\tblockingPing: %dms", timeOut);
            if (timeOut == 0) timeOut = latency;

            JsonObject dataFlush;
            while(parseIn(dataFlush)) yield();   //we flush the OSC queue

            RXtimerStart = millis();

            SendCmd2OSC(address.c_str(),_ip,_port);            
            unsigned long t0 = millis();

            while (millis()-t0 < timeOut){
                yield();
                if (parseIn(data)){
                    if (data[EXEC_ADDRESS] == address){
                        latency = millis()-RXtimerStart;
                        //OSC_DEBUG_PRINT("\t new latency: %d", latency);
                        return true;
                    }
                    else{
                        //OSC_DEBUG_PRINT("\t wrong frame");
                        return false;
                    }
                }               
            }
            //OSC_DEBUG_PRINT("\t timed out");
            return false;
        }

        static bool matchPattern(String pattern, const String& input) {

            pattern.replace("%02d", "*");
            pattern.replace("%d", "*");
            pattern.replace("%i", "*");
            pattern.replace("%s", "*");

            int pLen = pattern.length();
            int iLen = input.length();
            int p = 0, i = 0;
            int starIdx = -1, match = 0;

            while (i < iLen) {               
                // Cas 1 : Correspondance exacte ou wildcard '?'
                if (p < pLen && (pattern[p] == input[i] /*|| pattern[p] == '?'*/)) {
                    p++;
                    i++;
                }
                // Cas 2 : Wildcard '*'
                else if (p < pLen && pattern[p] == '*') {
                    starIdx = p; // Enregistrer la position du '*'
                    match = i;   // Enregistrer la position actuelle d'entrée
                    p++;         // Passer au caractère suivant dans le pattern
                }
                // Cas 3 : Retour à un '*' précédent
                else if (starIdx != -1) {
                    p = starIdx + 1; // Reprendre après le '*'
                    match++;
                    i = match; // Réessayer la correspondance avec un caractère supplémentaire
                }
                // Cas 4 : Pas de correspondance
                else {
                    return false;
                }
            }

            // Vérifier si les caractères restants dans le pattern sont uniquement des '*'
            while (p < pLen && pattern[p] == '*') {
                p++;
            }
            
            OSC_DEBUG_PRINT("\nmatchPattern(%s,%s)",pattern.c_str(),input.c_str());

            OSC_DEBUG_PRINT(": %s",p == pLen?"true":"false");
            return p == pLen; // Vrai si tout le pattern est traité
        }        
    
};

// Tâche de ping non bloquante
void OSCpingTask(void* parameter) {

    // Récupérer les paramètres passés à la tâche
    OSCPingParameters* params = static_cast<OSCPingParameters*>(parameter);

    if (xSemaphoreTake(hwm.network->networkBusy, pdMS_TO_TICKS(500)) == pdTRUE){ 

        OSC_DEBUG_PRINT("pingTask=>%s:%i",params->ip.toString().c_str(),params->port);
        if (params->osc->blockingPing(params->ip,params->port,params->address,params->data,params->timeOut)) {
            params->callback(true, params->port);  // Appel du callback en cas de succès
        } else {
            params->callback(false, params->port);  // Appel du callback en cas d'échec
        }
        
        xSemaphoreGive(hwm.network->networkBusy);

    }else{
        DEBUG_PRINT("\nOSCpingTask: networkbusy");
    }

    vTaskDelay(1);

    params->isPinging = false;

    // Suspendre la tâche après chaque ping (exécution ponctuelle)
    vTaskDelete(NULL);
}

#endif