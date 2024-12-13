#ifndef OSCINTERFACE_h
#define OSCINTERFACE_h

#include <Arduino.h>

#include <Ethernet.h>
#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#ifndef DEBUG_PRINT 
    #define DEBUG_PRINT(...) 
#endif

class OSCInterface{
    public:
        #define OSCLEN 40
        uint8_t * bValue = new uint8_t[bSize];
        int iValue            = -1;           // values returned by the mixer
        float   fValue          = -0.1;
        char    sValue[OSCLEN+10]; char sValue2[OSCLEN]; char sValue1[OSCLEN];
        char   TypeStr;
        String CommandStr;


        OSCInterface(){}
   
        void settings(IPAddress defaultIP, int defaultPort,UDP *_udp){
            ip = defaultIP;
            port = defaultPort;
            udp = _udp;
        }

        bool sendOSCMessage(OSCMessage *msg, IPAddress _ip, int _port){           
            DEBUG_PRINT("\n>>%s(%s,%i) ",msg->getAddress(),_ip.toString(),_port);
            ExpectedStr = msg->getAddress();
            if (udp == NULL){
                DEBUG_PRINT(":null UDP",ExpectedStr.c_str());
                return false;
            }             
            udp->beginPacket(_ip, _port);
            msg->send(*udp);
            udp->endPacket();
            //yield();
            msg->empty(); 
            return true;          
        }

        void SendCmd2OSC(const char* address){
            SendCmd2OSC(address,ip,port);
        }

        void SendCmd2OSC(const char* address, IPAddress _ip, int _port){                        
            OSCMessage msg(address);
            sendOSCMessage(&msg,_ip,_port);
        }

        void SendInt2OSC(const char* address, int val){            
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port)) DEBUG_PRINT("%d",address, val);;          
        }

        void SendFloat2OSC(const char* address, float val){    
            OSCMessage msg(address);
            msg.add(val);
            if (sendOSCMessage(&msg,ip,port)) DEBUG_PRINT("%.2f",address, val);         
        }        

        boolean OSCReadIn() {

            if (udp == NULL){
                DEBUG_PRINT("\n<<OSCReadIn : null UDP");
                return false;                
            } 

            iValue = -1;
            fValue = -0.1;
            /*strcpy(sValue1,"");
            strcpy(sValue2,"");*/

            // Check if packet was received

            //  delay(latency);
            int size = udp->parsePacket();
            if (size < 1) return false;
            /*
            uint16_t waitRead = 0;
            int size = 0;
            do{
                size = OscUDP->parsePacket();
                delay(1);
                //DEBUG_PRINT("°");
            }while(waitRead++<=latency && size<1);
            if (size < 1){
                //DEBUG_PRINT("¦");
                return false;
            }*/
            //DEBUG_PRINT("@");

            // Get the packet
            //DEBUG_OSCMSG("\n<<:");
            OSCMessage msg;
            /*size = OscUDP->available();   
            OscUDP->read(udpRxBuffer, size); // Mise en tampon du paquet  
            msg.fill(udpRxBuffer,size);*/
            while (size--) msg.fill(udp->read());
            // Check for errors
            if (msg.hasError()) {
                //DEBUG_OSCMSG("error: %s\n", msg.getError());
                return false;
            }
            // Parse return OSC command
            msg.getAddress(PacketBuffer, 0);                // get message path
            CommandStr = String(PacketBuffer);              // assign to CommandStr
            TypeStr = msg.getType(0);                       // get data type

            DEBUG_PRINT("\n<<%s,%c,",CommandStr.c_str(),TypeStr);

            if (msg.isInt(0)){
                iValue = msg.getInt(0);    // integer
                DEBUG_PRINT("%i",iValue);
            }
            if (msg.isFloat(0)){
                fValue = msg.getFloat(0);  // float
                DEBUG_PRINT("%.2f",fValue);
            }
            if (msg.isString(0)) {
                msg.getString(0, sValue);  // string
                msg.getString(1, sValue1);
                msg.getString(2, sValue2);
                DEBUG_PRINT("%s, %s, %s",sValue,sValue1,sValue2);
            }
            if (msg.isBlob(0)) {
                bSize = msg.getBlobLength(0);
                //DEBUG_OSCMSG("BLOB(%i)", bSize);
                if (bSize > oldbSize) {
                //DEBUG_PRINT("\nResizing blob buffer");
                delete bValue;
                bValue = new uint8_t[bSize];
                oldbSize = bSize;
                }
                if (msg.getBlob(0, bValue) != bSize) return false;
                //DEBUG_OSCMSG("\nbValue=(%s)", bValue);
            }
            msg.empty();     
            return true;
        }    

    
    private:
        //OSC AND MESSAGING MANAGEMENT
        uint16_t bSize = 20, oldbSize = bSize ;
        
        char   PacketBuffer[UDP_TX_PACKET_MAX_SIZE];       
        String ExpectedStr;        


        IPAddress ip;
        int port;

        UDP *udp = NULL;
};

#endif