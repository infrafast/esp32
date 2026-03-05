#ifndef _MSTATION_h
#define _MSTATION_h

#include "mixer.h"

#define INTERFACE_MIXINGSTATION "MixingStation"
class MixingStation: public NetworkDevice, public MixerInterface{
  public:
  
    MixingStation():NetworkDevice(INTERFACE_MIXINGSTATION,GROUP_INTERFACE_MIXER,8887,true){
      //destination->host = "192.168.99.99";
      destination = new TargetEcho("192.168.0.1");    
      // prepare websocket API callback
      wsClientOnTXTmessage =
      std::bind(&MixingStation::routeTXTmessage,
                this,
                std::placeholders::_1,
                std::placeholders::_2);        
    }

     virtual void monitorService(){

      // time to ping the device to see if it's still alive after certain time or if we have no valid UDP, we reconnect
      #define STATE_PING_DELAY 3000
      // delay expired
      if (millis()-destination->lastQueryTime > STATE_PING_DELAY){        

      }                                                                   
    }

    virtual bool handleConnectService(){return false;}

    virtual bool handleDisconnectService(){
      hwm.network->clientSocketClose(); 
      return false;
    }    

  protected:
    #define JSON_RX_BUFFER 4096
    static char rxBuf[JSON_RX_BUFFER];
    static JsonDocument doc;

    void routeTXTmessage(const char *message, uint16_t length) {

      if (length >= sizeof(rxBuf)) return; // trop gros, à gérer
      // copie dans buffer local modifiable afin que JsonArduino travaille directement sans mode copie lui meme
      // = gain de CPU et de memoire
      memcpy(rxBuf, message, length);
      rxBuf[length] = '\0';

      doc.clear();
      if (deserializeJson(doc, rxBuf)) return;

      const char* path = doc["path"] | "";
      JsonVariant body = doc["body"];
      // dispatch(path, body) ...
    }  
  
};

char MixingStation::rxBuf[JSON_RX_BUFFER];
JsonDocument MixingStation::doc;


#endif