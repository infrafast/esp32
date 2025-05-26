#ifndef OSCINTERFACE_h
#define OSCINTERFACE_h

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

///////////////////// DEVICE I/O CAPTURE ////////////////////////
enum{
    NO_CAPTURE,
    CAPTURE_IN,
    CAPTURE_OUT,
    CAPTURE_INOUT
};

class DeviceInterface{
    public:
        String name;
        uint8_t recordMode = CAPTURE_IN;

        DeviceInterface(String, uint8_t);
        bool execute(JsonObject& data);
        bool read(JsonObject& data);

    private:
        virtual bool parseOut(JsonObject& data) = 0;
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

class DeviceInterfaceCapture{
  public:
    
    void record(JsonObject& data, String& name){
        // first we parse all interfaces to see if we find one that match the name    
        for (const auto& descriptor : descriptors){
            if (descriptor.interface->name == name){
                if (descriptor.state == CAPTURE_RECORD){
                    // file.add(data);
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

    private:
        std::vector<CaptureDescriptor> descriptors;
};

DeviceInterfaceCapture interfaceCapture;

DeviceInterface::DeviceInterface(String _name, uint8_t _mode):name(_name),recordMode(_mode){
    if (recordMode != NO_CAPTURE){
        interfaceCapture.registerInterface(this);
    }
}

bool DeviceInterface::execute(JsonObject& data){
    if (recordMode == CAPTURE_OUT) interfaceCapture.record(data,name);
    return parseOut(data);
};

bool DeviceInterface::read(JsonObject& data){
    // do we have data ?
    bool ret = parseIn(data);
    if (ret){
        // if yes, is this interface supposed to record data coming in  ?
        if (recordMode == CAPTURE_IN) interfaceCapture.record(data,name);
    }
    return ret;            
}










///////////////////// DATA STRUCTURES ////////////////////////

struct OSCValues{   
    #define OSC_MAX_LEN 128
    uint16_t    bSize = 20, oldbSize = bSize ;
    uint8_t *   bValue;
    int         iValue            = -1;           // values returned by the mixer
    float       fValue          = -0.1;
    char        sValue[OSC_MAX_LEN]; char sValue2[OSC_MAX_LEN]; char sValue1[OSC_MAX_LEN];
    char        TypeStr;
    char        addressStr[OSC_MAX_LEN];
    const char* expectedStr;
    bool        incoming = false;      // this is to indicate if someone has consummed the received data
};

typedef std::function<void(bool, OSCValues*, int)> OSCPingCallback; 

class OSCInterface;

struct OSCPingTaskParameters {
  OSCValues* data;
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
        Network* network;
        unsigned long RXtimerStart = 0;
        String name  = "OSC1";
        
        OSCInterface(Network* _net):network(_net),DeviceInterface(name,CAPTURE_INOUT){
            latency = network->LANlatency;
        }

        virtual bool parseOut(JsonObject& data){};
        virtual bool parseIn(JsonObject& data){}; 
   
        void setClient(const IPAddress defaultIP, const int defaultPort){
            ip = defaultIP;
            port = defaultPort;
            OSC_DEBUG_PRINT("\nOSC.setClient(%s,%d)",ip.toString().c_str(),port);
        }
        
        bool sendOSCMessage(OSCMessage *msg, const IPAddress _ip, const int _port){           
            OSC_DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),_ip.toString().c_str(),_port);
            if (network->udp == NULL){
                OSC_DEBUG_PRINT(":null UDP",msg->getAddress());
                return false;
            }             
            network->udp->beginPacket(_ip, _port);
            msg->send(*network->udp);
            network->udp->endPacket();

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

        boolean OSCReadIn(OSCValues* data) {
            
            data->iValue = -1;
            data->fValue = -0.1;
            strcpy(data->addressStr,"");
            strcpy(data->sValue,"");
            strcpy(data->sValue1,"");
            strcpy(data->sValue2,"");
            data->incoming = false; // data are fresh and not yet consummed
            

            if (network->udp == NULL){
                //OSC_DEBUG_PRINT("\n<<OSCReadIn : null UDP");
                return false;                
            } 

            // Check if packet was received
            int size = network->udp->parsePacket();
            if (size < 1) return false;

            OSCMessage msg;

            while (size--){
                msg.fill(network->udp->read());
                yield();
            }

            // Check for errors
            if (msg.hasError()) {
                //DEBUG_OSCMSG("error: %s\n", msg.getError());
                return false;
            }
            // Parse return OSC command
            msg.getAddress(data->addressStr, 0);      // get message path
            data->TypeStr = msg.getType(0);                       // get data type

            OSC_DEBUG_PRINT("\n<<%s,%c,",data->addressStr,data->TypeStr);

            /*if (data->expectedStr != "" && data->expectedStr == data->addressStr){
                unsigned long t = millis()-RXtimerStart;
                latency = (latency + t) / 2;            //avg with old value
                if (latency<10) latency = 10;
                OSC_DEBUG_PRINT("[%dms], ",latency);
            } */         

            data->incoming = true;

            if (msg.isInt(0)){
                data->iValue = msg.getInt(0);    // integer
                OSC_DEBUG_PRINT("%i",data->iValue);
            }
            if (msg.isFloat(0)){
                data->fValue = msg.getFloat(0);  // float
                OSC_DEBUG_PRINT("%.2f",data->fValue);
            }
            if (msg.isString(0)) {
                msg.getString(0, data->sValue);  // string
                msg.getString(1, data->sValue1);
                msg.getString(2, data->sValue2);
                OSC_DEBUG_PRINT("%s, %s, %s",data->sValue,data->sValue1,data->sValue2);
            }
            if (msg.isBlob(0)) {
                data->bSize = msg.getBlobLength(0);
                //OSC_DEBUG_PRINT("BLOB(%i)", bSize);
                if (data->bSize > data->oldbSize) {
                //OSC_DEBUG_PRINT("\nResizing blob buffer");
                delete data->bValue;
                data->bValue = new uint8_t[data->bSize];
                data->oldbSize = data->bSize;
                }
                if (msg.getBlob(0, data->bValue) != data->bSize) return false;
                //DEBUG_OSCMSG("\nbValue=(%s)", bValue);
            }
            msg.empty();    

            return true;
        }    

        // Fonction pour démarrer la tâche de ping non bloquante
        virtual bool asyncPing(OSCPingTaskParameters* params) {
            OSC_DEBUG_PRINT("\nOSCasyncPing %s %s:%d T.O:%d",params->data->expectedStr,params->ip.toString().c_str(),params->port,params->timeOut);
            if (params->isPinging){
                OSC_DEBUG_PRINT("\t=>ping pending, no more allowed, stop.");
            }
            params->isPinging = true;
            // Créer la tâche FreeRTOS pour le ping
            xTaskCreatePinnedToCore(
                OSCpingTask,         
                "OSCPingTask",       
                4096*2,             
                params,           
                1,                
                NULL,   
                APP_CPU_NUM  
            );
            return true;
        }

        bool blockingPing(OSCValues* data,uint32_t timeOut){
            return blockingPing(ip,port,data, timeOut);
        }

        bool blockingPing(IPAddress _ip, int _port, OSCValues* data, uint32_t timeOut = 0){
            //OSC_DEBUG_PRINT("\tblockingPing: %dms", timeOut);
            if (timeOut == 0) timeOut = latency;

            OSCValues dataFlush;
            while(OSCReadIn(&dataFlush)) yield();   //we flush the OSC queue

            RXtimerStart = millis();

            SendCmd2OSC(data->expectedStr,_ip,_port);            
            unsigned long t0 = millis();
            while (millis()-t0 < timeOut){
                yield();
                if (OSCReadIn(data)){
                    if (strcmp(data->addressStr,data->expectedStr) == 0){
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

    
    private:       
        char   PacketBuffer[OSC_MAX_LEN/*UDP_TX_PACKET_MAX_SIZE*/];       
};

// Tâche de ping non bloquante
void OSCpingTask(void* parameter) {
  // Récupérer les paramètres passés à la tâche
  OSCPingTaskParameters* params = static_cast<OSCPingTaskParameters*>(parameter);
  OSC_DEBUG_PRINT("pingTask=>%s:%i",params->ip.toString().c_str(),params->port);
  if (params->osc->blockingPing(params->ip,params->port,params->data,params->timeOut)) {
    params->callback(true, params->data,params->port);  // Appel du callback en cas de succès
  } else {
    params->callback(false, params->data,params->port);  // Appel du callback en cas d'échec
  }

  // Libérer la mémoire allouée pour les paramètres

  params->isPinging = false;

  // Suspendre la tâche après chaque ping (exécution ponctuelle)
  vTaskDelete(NULL);
}

#endif