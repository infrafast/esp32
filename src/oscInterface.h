#ifndef OSCINTERFACE_h
#define OSCINTERFACE_h

#include <Arduino.h>

#include <Ethernet.h>
#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif


struct OSCValues{
    
    #define OSC_MAX_LEN 64

    uint16_t bSize = 20, oldbSize = bSize ;
    uint8_t * bValue = new uint8_t[bSize];
    int         iValue            = -1;           // values returned by the mixer
    float       fValue          = -0.1;
    char        sValue[OSC_MAX_LEN+10]; char sValue2[OSC_MAX_LEN]; char sValue1[OSC_MAX_LEN];
    char   TypeStr;
    String addressStr, expectedStr="";
};

class OSCInterface{
    public:
        IPAddress ip;
        int port;
        uint16_t latency = 49;      // default reasonnable latency
        UDP *udp = NULL;
        unsigned long RXtimerStart = 0;
        OSCValues data;
        
        OSCInterface(){}
   
        void setClient(const IPAddress defaultIP, const int defaultPort){
            ip = defaultIP;
            port = defaultPort;
            DEBUG_PRINT("\nOSC.setClient(%s,%d)",ip.toString().c_str(),port);
        }

        void setNetwork(UDP* _udp, uint16_t _latency){
            //DEBUG_PRINT("\nOSC.setUDP()");
            udp = _udp;
            latency = _latency;
        }
        
        bool sendOSCMessage(OSCMessage *msg, const IPAddress _ip, const int _port, bool countRXtime = true){           
            DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),_ip.toString(),_port);
            if (udp == NULL){
                DEBUG_PRINT(":null UDP",msg->getAddress());
                return false;
            }             
            udp->beginPacket(_ip, _port);
            msg->send(*udp);
            udp->endPacket();

            if (countRXtime){
                data.expectedStr = msg->getAddress();       // instruction is flagged to measure the latency
                RXtimerStart = millis();               // hence we start counting
            }

            msg->empty(); 
            return true;          
        }

        void SendCmd2OSC(const char* address, bool countRXtime = false){
            SendCmd2OSC(address,ip,port, countRXtime);
        }

        void SendCmd2OSC(const char* address, const IPAddress _ip, const int _port,  bool countRXtime = true){                        
            OSCMessage msg(address);
            sendOSCMessage(&msg,_ip,_port, countRXtime);
        }

        void SendInt2OSC(const char* address, int val,  bool countRXtime = true){            
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port, countRXtime)) DEBUG_PRINT("%d",address, val);          
        }

        void SendFloat2OSC(const char* address, float val, bool countRXtime = true){    
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port,countRXtime)) DEBUG_PRINT("%.2f",address, val);         
        }        

        boolean OSCReadIn() {
            
            if (udp == NULL){
                DEBUG_PRINT("\n<<OSCReadIn : null UDP");
                return false;                
            } 

            data.iValue = -1;
            data.fValue = -0.1;
            data.addressStr = "";
            strcpy(data.sValue,"");
            strcpy(data.sValue1,"");
            strcpy(data.sValue2,"");

            // Check if packet was received
            int size = udp->parsePacket();
            if (size < 1) return false;

            OSCMessage msg;

            while (size--){
                msg.fill(udp->read());
                yield();
            }
            // Check for errors
            if (msg.hasError()) {
                //DEBUG_OSCMSG("error: %s\n", msg.getError());
                return false;
            }
            // Parse return OSC command
            msg.getAddress(PacketBuffer, 0);                // get message path
            data.addressStr = String(PacketBuffer);              // assign to addressStr
            data.TypeStr = msg.getType(0);                       // get data type

            DEBUG_PRINT("\n<<%s,%c,",data.addressStr.c_str(),data.TypeStr);

            if (data.expectedStr != "" && data.expectedStr == data.addressStr){
                unsigned long t = millis()-RXtimerStart;
                latency = (latency + t) / 2;            //avg with old value
                if (latency<10) latency = 10;
                DEBUG_PRINT("[%dms], ",latency);
            }          

            if (msg.isInt(0)){
                data.iValue = msg.getInt(0);    // integer
                DEBUG_PRINT("%i",data.iValue);
            }
            if (msg.isFloat(0)){
                data.fValue = msg.getFloat(0);  // float
                DEBUG_PRINT("%.2f",data.fValue);
            }
            if (msg.isString(0)) {
                msg.getString(0, data.sValue);  // string
                msg.getString(1, data.sValue1);
                msg.getString(2, data.sValue2);
                DEBUG_PRINT("%s, %s, %s",data.sValue,data.sValue1,data.sValue2);
            }
            if (msg.isBlob(0)) {
                data.bSize = msg.getBlobLength(0);
                //DEBUG_OSCMSG("BLOB(%i)", bSize);
                if (data.bSize > data.oldbSize) {
                //DEBUG_PRINT("\nResizing blob buffer");
                delete data.bValue;
                data.bValue = new uint8_t[data.bSize];
                data.oldbSize = data.bSize;
                }
                if (msg.getBlob(0, data.bValue) != data.bSize) return false;
                //DEBUG_OSCMSG("\nbValue=(%s)", bValue);
            }
            msg.empty();    

            data.expectedStr = ""; // next receive we by default don't  count time

            return true;
        }    

        bool blockingPing(const String command, uint32_t timeOut = 0){
            if (timeOut == 0) timeOut = latency;
            udp->flush();
            SendCmd2OSC(command.c_str(),true);
            DEBUG_PRINT("\tblockingPing: %dms", timeOut);            
            unsigned long t0 = millis();
            while (millis()-t0 < timeOut){
                yield();
                if (OSCReadIn()){
                    if (data.addressStr == command) return true;
                    else{
                        DEBUG_PRINT("\t wrong frame");
                        return false;
                    }
                }                
            }
            DEBUG_PRINT("\t timed out");
            return false;
        }

        bool matchPattern(String pattern, const String& input) {
            
            DEBUG_PRINT("\nmatchPattern");

            pattern.replace("%02d", "*");
            pattern.replace("%d", "*");
            pattern.replace("%s", "*");

            DEBUG_PRINT("(%s,%s)",pattern.c_str(),input.c_str());

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

            DEBUG_PRINT(": %s",p == pLen?"true":"false");
            return p == pLen; // Vrai si tout le pattern est traité
        }        

    
    private:       
        char   PacketBuffer[UDP_TX_PACKET_MAX_SIZE];       
};

#endif