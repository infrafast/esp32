#include <WiFi.h>

//#define ETHERNET3
#include "ETH_Helper.h"       

#include "interfaces\midiHelpers.h"
#ifndef ETHERNET3
  #include <EthernetBonjour.h>  //for Eth registration
#endif
#include <ESPmDNS.h>          // for WIFI registration

#define SESSION_NAME "ESP-32"

char ssid[] = "intervalmix1"; //  your network SSID (name)
char pass[] = "intervalpop74";    // your network password (use for WPA, or use as key for WEP)

MidiClient* midiClient;     // generic class offered as an alternative of the MACRO



void OnAppleMidiException(const APPLEMIDI_NAMESPACE::ssrc_t&, const APPLEMIDI_NAMESPACE::Exception&, const int32_t);   

bool useEth = false;

void WIFI_startup(){
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    AM_DBG(F("Establishing connection to WiFi.."));
  }
}

void setup(){

  AM_DBG_SETUP(115200);

  utility.init();

  if (useEth){
    ETH_startup();
    midiClient = new AppleMidiWithInterfaceWrapper<EthernetUDP>(SESSION_NAME,DEFAULT_CONTROL_PORT);
  }else{
    WIFI_startup();
    midiClient = new AppleMidiWithInterfaceWrapper<WiFiUDP>(SESSION_NAME,DEFAULT_CONTROL_PORT);             
    if (MDNS.begin(midiClient->getName())){
      MDNS.addService("apple-midi", "udp", midiClient->getPort());
    }else{
      AM_DBG("\nMDNS registration failed");
    }    
  }
    
  midiClient->begin();

  midiClient->setHandleConnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const char* name) {
    AM_DBG(F("Connected to session"), ssrc, name);
    utility.draw(0x070E);
  });
  midiClient->setHandleDisconnected([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc) {
    AM_DBG(F("Disconnected"), ssrc);
    utility.draw(0xF800);
  });

  //midiClient->setHandleException(OnAppleMidiException);

  AM_DBG(F("OK, now make sure you have an rtpMIDI session that is Enabled"));
  AM_DBG("Add device named ",useEth?"Arduino":midiClient->getName()," with Host", useEth?Ethernet.localIP():WiFi.localIP(), "Port", midiClient->getPort(), "(Name", midiClient->getName(), ")");
  AM_DBG(F("Select and then press the Connect button"));
  AM_DBG(F("Then open a MIDI listener and monitor incoming notes")); 
}

void loop(){
  midiClient->read();
  #ifndef ETHERNET3
    if (useEth) EthernetBonjour.run();
  #endif
}