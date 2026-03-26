#ifndef _IUTILS_h
#define _IUTILS_h  

#include "Interfaces.h"
#include <aWOT.h>
#include <WebSocketServer.hpp>

using namespace net;

 
#define ADDRESS_PROVIDER      "/provider/"
#define ADDRESS_TEXT    "/text/%s"

#define GROUP_INTERFACE_UTILS "Utils"


class SystemInterface: public DeviceInterface, public DataProvider{
    #define SYSTEM_INTERFACE "system"
    public:    

    SerializableParameters systemDS = SerializableParameters(SYSTEM_INTERFACE, GROUP_INTERFACE_UTILS, &dataRepo);   // one instance
        
    DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),DataProvider(getProviderNameForData()){
      state.code = IF_READY;     
    }
  
      virtual void load() override{
        DEBUG_INFO("%s::load() default ignored => 🚫DISABLED", interfaceName.c_str()); 
        // ignore the given parameter. Just enable the interface
        state.enabled = false;       
        hasStrip = true;             // textStrip...
      }

      virtual bool save() override{
        DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      } 

    virtual void monitor(){}
    
    virtual void* read(){return nullptr;}
    
    virtual bool _begin() override{
      DEBUG_INFO("%s::_begin() => registered systemDS",interfaceName.c_str());          
      systemDS.registerDataSetAddress(dsText.address);      // register all ds             

      return true;
    }

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_TEXT) return 1;
      else{
        // IS IT AN INTERFACE ?
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX            
          uint16_t count = 0;
          for (const auto& interface : interfaceRepo.interfaces){
            // check if we request provider listing
            if (interface->group == group) count++;
          }
          //DEBUG_PRINT("\nDataSet getSize(%s) has %d items",addrr.c_str(),count);
          return count;
        }

      }
     
      DEBUG_ERROR("<%s>DataSet getSize(%s) is not implemented",dataProviderName.c_str(),addrr.c_str());
      return 0;

    }    
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_TEXT){
        textValue = concatStrings("hello", index);
        return &textValue;
      }else{
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX    
          int16_t count = -1;        
          for (const auto& interface : interfaceRepo.interfaces){
            if (interface->group == group) count++;
            if (count == index){
              String* s = &interface->interfaceName;
              //DEBUG_PRINT("\nDataSet getValuePtr(%s,%d) = %s",addrr.c_str(),index,s->c_str());
              return s;
            } 
          }
        }      
      }     

      DEBUG_ERROR("<%s>DataSet getValuePtr(%s,%d) is not implemented",dataProviderName.c_str(),addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";    
       
    
}systemInterface(false);      // not exposed


class DirectoryInterface: public NetworkDevice{
    #define DIRECTORY_INTERFACE "Directory"
    public:    

      RtpDirectory rtpDir;
      Event evtDir = Event(&rtpDir);

      DirectoryInterface(bool _configurable = false):NetworkDevice(DIRECTORY_INTERFACE,GROUP_INTERFACE_UTILS,APPLE_MIDI_PORT,0,"",_configurable){
        attachDeviceInterface(this);
        unregisterDataSetAddress(&addressIP);              // we un-expose the ping parameter as user should not modify it 
        unregisterDataSetAddress(&addressPort);              // we un-expose the ping parameter as user should not modify it 
      }

      virtual void load() override{
        DEBUG_INFO("%s::load() default ignored => ENABLE", interfaceName.c_str()); 
        // ignore the given parameter. 
        switchDevice(DEVICE_OFF);          // the interface is forced disabled at startup as we do not need to browse network until a request is sent (by startWebApp to publish DNS name or by appleMidi to advert)
        hasStrip = false;             
      }

      virtual bool _begin() override{     // called by init()
        DEBUG_INFO("%s::_begin() registering serviceFoundWrapper and _instance",interfaceName.c_str());
        instance_ = this;  // enregistrer l’instance active       // we don't do that in the constructor as the instance has multiple heritage, hence we want to be sure the object is fully created
        EthernetBonjour.setServiceFoundCallback(&DirectoryInterface::serviceFoundWrapper);  
        return true;        
      }

      virtual bool save() override{
        DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      }           

      virtual void monitorService(){
        if (hwm.network->media == CABLE_MEDIA){
          EthernetBonjour.run();
        }else if (hwm.network->media == WIFI_MEDIA){
          //Sur ESP32, mDNS n’a PAS besoin d’être appelé dans loop() une fois qu’il est démarré.
        }               
      }
     
      virtual bool handleConnectService(){
        DEBUG_INFO("%s::handleConnectService()",interfaceName.c_str());
        String baseName = *systemCfg.getString(S_AP_NAME);

        if (hwm.network->media == WIFI_MEDIA){
          if (MDNS.begin(baseName.c_str())){
            MDNS.addService(MDNS_ANNOUNCED_APPLEMIDI_SERVICE, "udp", localPort);
            //MDNS.addService("http", "tcp", 80);                                
          }else{
            DEBUG_ERROR("MDNS registration <%s> failed",baseName.c_str());
          }
        }else if (hwm.network->media == CABLE_MEDIA){
          String midiName = concatStrings(baseName, "._", MDNS_ANNOUNCED_APPLEMIDI_SERVICE);
          String httpName = concatStrings(baseName, "._http");
          if (EthernetBonjour.begin(baseName.c_str())){
            EthernetBonjour.addServiceRecord(midiName.c_str(), localPort,MDNSServiceUDP);
            //EthernetBonjour.addServiceRecord(httpName.c_str(), 80, MDNSServiceTCP);           // NOT USED ....
          }else{
            DEBUG_INFO("EthernetBonjour registration <%s> failed", baseName.c_str());
          } 
        }

        //discoveryRequest();  // we do not spontaneously trigger a discovery, it has to be requested by someone, this will be helpful when managing the locks

        DeviceInterface::setStateReady("green",lang.getString("online")->c_str(),"",systemCfg.getString(S_AP_NAME)->c_str());

        return true;
      }

      void onEvent(const Event& evt) override {
        switch(evt.type){
          case EVENT_REQUEST_DIRECTORY_SERVICE_REFRESH:
            discoveryRequest();
          break;        
        }
      }

      void discoveryRequest(){
        DEBUG_INFO("%s::discoveryRequest()",interfaceName.c_str());   
        if (deviceInterface->state.enabled){       
          // drop the request if interface is up and running
          startDiscovery();
        }else{
          // if interface is not up and running, we try to bring it up
          switchNetworkService(true);
          startDiscovery();
        }
      }      

      virtual bool ping(){return true;}
      
      virtual bool handleDisconnectService(){
        DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
        if (hwm.network->lastActiveMedia == CABLE_MEDIA){
          // eth
          EthernetBonjour.stop();
        }else if (hwm.network->lastActiveMedia == WIFI_MEDIA){
          // wifi
          MDNS.end();
        }          

        if (deviceInterface->state.enabled == false) setStateDisabled();
        deviceInterface->state.details = concatStrings(*lang.getString("Device is not detectable on the network. Use")," ",hwm.network->IP.toString()," ",*lang.getString("to connect"));

        return true;
      }        

    protected:
      
      static DirectoryInterface* instance_;

      void startDiscovery(){
        if (deviceInterface->state.code == IF_NOT_READY){
          epDirectoryNotReady();
        }else{
          epDirectoryRefresh();
        }        
      }      
      
      // Méthode statique compatible FreeRTOS
      static void wiFiRTPDiscoveryTaskWrapper(void* pvParameters) {        
        instance_->wiFiRTPDiscoveryTask();   // appel méthode réelle
        vTaskDelete(NULL);
      }           

      // ============================================================
      // Améliorations callback EthernetBonjour
      // - ignore les callbacks "fin de scan" (serviceName==NULL)
      // - filtre UDP + _apple-midi (tolère "_apple-midi.")
      // - ignore les IP 0.0.0.0
      // - laisse upsertEndpoint fusionner correctement
      // ============================================================
      static bool isZeroIP(const byte ip[4]) {
        return ip && ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
      }

      static bool isAppleMidiServiceType(const char* serviceType) {
        // Accepte "_apple-midi" ou "_apple-midi." (point final parfois présent)
        if (!serviceType) return false;
        return (strncmp(serviceType, "_apple-midi", 11) == 0);
      }

      void onEthernetBonjourServiceFoundCallback(const char* serviceType,
                                           MDNSServiceProtocol_t proto,
                                           const char* serviceName,
                                           const byte ipAddr[4],
                                           unsigned short port,
                                           const char* txt) {
        (void)txt; // si tu ne l'utilises pas

        // Fin de scan (timeout) : la lib rappelle une fois avec serviceName==NULL
        if (serviceName == nullptr || ipAddr == nullptr) {
          // Optionnel: rtpDir.pruneOlderThanMs(15000); // ex: purge vieux de 15s    
          epDirectoryUpdated();
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
      }

      void epDirectoryUpdated(){
        DEBUG_INFO("[RTP]\t\tScan terminé (timeout).");
        DEBUG_INFO("[RTP]\t\t%d endpoint(s) mémorisé(s):\n", rtpDir.epCount);      
        for (int i = 0; i < rtpDir.epCount; i++) {
          DEBUG_INFO("  #%d  %s  %s:%u\n", i+1, rtpDir.ep[i].name, rtpDir.ep[i].ip.toString().c_str(), rtpDir.ep[i].port);
        }
        // midi interface has to update its config and regenerate its state      
        evtDir.type = EVENT_DIRECTORY_SERVICE_REFRESHED;
        fireEvent(evtDir);
      }

      void epDirectoryNotReady(){
        DEBUG_INFO("[RTP]\t\tDirectory not ready");
        evtDir.type = EVENT_DIRECTORY_NOT_READY;
        fireEvent(evtDir);        
      }      

      void epDirectoryRefresh(){
        // SCAN FOR APPLE MIDI SESSION ON THE NETWORK
        rtpDir.clear();
        if (hwm.network->media == CABLE_MEDIA){
          // Découvre le type de service "_apple-midi" en UDP ethernet = non bloquant
          EthernetBonjour.startDiscoveringService("_apple-midi", MDNSServiceUDP, EP_DISCOVERY_TIME_OUT); // :contentReference[oaicite:3]{index=3} 
          DEBUG_INFO("\n\n[RTP-bonjour]\t\tScan RTP-MIDI lancé...\n");  
        }else{
          // Wifi = bloquant
          xTaskCreate(
            wiFiRTPDiscoveryTaskWrapper,  // Function to implement the task
            "WiFiRTPDiscoveryTask",     // Name of the task
            4096,              // Stack size in words
            NULL,               // Task input parameter
            1,                  // Priority of the task
            NULL                // Task handle
          );          
        }      
      }
    
      void wiFiRTPDiscoveryTask(){
        DEBUG_INFO("[RTP-WiFi]\t\tScan RTP-MIDI lancé...\n");     
        int n = MDNS.queryService(MDNS_ANNOUNCED_APPLEMIDI_SERVICE, "udp"); // -> _apple-midi._udp
        DEBUG_PRINT("\ntrouvé %d service(s) AppleMIDI\n", n);
        for (int i = 0; i < n; i++) {
          uint8_t ipBytes[4] = {MDNS.IP(i)[0], MDNS.IP(i)[1], MDNS.IP(i)[2], MDNS.IP(i)[3]};
          rtpDir.upsertEndpoint(MDNS.hostname(i).c_str(),  ipBytes, MDNS.port(i));
        }
        epDirectoryUpdated();
      }    

      // wrapper compatible avec la signature attendue par la lib
      static void serviceFoundWrapper(const char* serviceType,
                                           MDNSServiceProtocol_t proto,
                                           const char* serviceName,
                                           const byte ipAddr[4],
                                           unsigned short port,
                                           const char* txt) {
        if (instance_) {
          instance_->onEthernetBonjourServiceFoundCallback(serviceType,proto,serviceName,ipAddr,port,txt);
        }
      }      
      

}directoryService(true);    // exposed
DirectoryInterface* DirectoryInterface::instance_ = nullptr;


class WebApplicationService: public NetworkDevice{
  #define WEBAPP_INTERFACE "WebApplication"
  #define WEBAPP_INACTIVITY_TIMEOUT_MS 20000

  public:
    using WebServerHandler = void (*)(Request&, Response&);
    using WebSocketHandler = void (*)(IWebSocket&);

    Application* webServer = nullptr;
    IWebSocketServer* wss = nullptr;

    WebApplicationService(bool _configurable = false):NetworkDevice(WEBAPP_INTERFACE,GROUP_INTERFACE_UTILS,80,80,"",_configurable){}

    virtual uint8_t getServiceWeight(){
      return 0;                               // in reality it consume 3 but we book 8-3 = 5 as SERVICES_WEIGHT_LIMIT so it can always start (priority)
    }


    virtual void load() override{
      DEBUG_INFO("%s::load() default ignored => 🚫DISABLED", interfaceName.c_str());
      state.enabled = false;
      state.switchable = false;
      hasStrip = false;
    }

    /*virtual bool _begin() override{
      DEBUG_INFO("%s::begin()", interfaceName.c_str());
      return NetworkDevice::begin();
    }*/

    virtual bool save() override{
      DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str());
      return true;
    }

    virtual void monitorService() override{
      handleWeb();
      monitorInactivityTimeout();
    }

    virtual bool ping() override{return true;}

    virtual bool handleConnectService() override{
      bool started = startWebApp();
      if (started) setStateReady();
      return started;
    }

    virtual bool handleDisconnectService() override{
      stopWebApp();
      setStateDisconnected();
      return true;
    }

    virtual void switchDevice(bool _enabled) override{
      if (!_enabled){
        socketConnected = false;
      }else{
        reportActivity();
      }
      NetworkDevice::switchDevice(_enabled);
    }

    void setWebServerHandler(WebServerHandler callback){
      webServerHandler = callback;
    }

    void setWebSocketHandler(WebSocketHandler callback){
      webSocketHandler = callback;
    }

    void reportActivity(){
      lastActivityMs = millis();
    }

    void reportSocketConnected(){
      socketConnected = true;
      reportActivity();
    }

    void reportSocketClosed(){
      socketConnected = false;
      reportActivity();
    }

    void handleWeb(){
      Network* configMedia = hwm.configMedia;

      if (configMedia != nullptr && configMedia->state == CONNECTED){
        if(webServer != nullptr){
          if (configMedia->media == CABLE_MEDIA){
            EthernetClient newEthClient = ((EthernetServer*)configMedia->server)->available();
            if (newEthClient) {
              webServer->process(&newEthClient);
              newEthClient.stop();
            }
          }else if (configMedia->media == WIFI_MEDIA){
            WiFiClient newWiFiClient = ((WiFiServer*)configMedia->server)->available();
            if (newWiFiClient.connected()) {
              webServer->process(&newWiFiClient);
              newWiFiClient.stop();
            }
          }
        }
        if (wss != nullptr) wss->listen();
      }
    }

    bool startWebApp(){
      if (hwm.configMedia == nullptr) return false;
      if (hwm.configMedia->state != CONNECTED) return false;

      if (webServer == nullptr){
        DEBUG_INFO("[WEBAPP]\t\tstart");

        webServer = new Application();
        webServer->setTimeout(3000);
        webServer->header("Content-Type", hdrContentType, sizeof(hdrContentType));
        webServer->header("Content-Length", hdrContentLength, sizeof(hdrContentLength));

        if (webServerHandler != nullptr) webServer->use(webServerHandler);

        setWSServer();
      }

      hwm.configURL = concatStrings("http://", hwm.configMedia->IP.toString());
      DEBUG_INFO("[WEBAPP]\t\tserving on %s", hwm.configURL.c_str());
      reportActivity();
      return true;
    }

    void stopWebApp(){
      shutdownWSS();
      if (webServer != nullptr){
        delete webServer;
        webServer = nullptr;
      }
      socketConnected = false;
      DEBUG_INFO("WebAppservices down");
    }

    void setWSServer(){
      if (webServer == nullptr) return;

      shutdownWSS();

      if (hwm.configMedia->media == CABLE_MEDIA){
        wss = new WebSocketServer<EthernetServer, EthernetClient, 1>{};
      }else if (hwm.configMedia->media == WIFI_MEDIA){
        wss = new WebSocketServer<WiFiServer, WiFiClient, 1>{};
      }else{
        DEBUG_ERROR("[WSS]\t\tinstanciation failed, no media");
        return;
      }

      if (webSocketHandler != nullptr) wss->onConnection(webSocketHandler);
      wss->begin();
    }

  protected:
    WebServerHandler webServerHandler = nullptr;
    WebSocketHandler webSocketHandler = nullptr;
    uint32_t lastActivityMs = 0;
    bool socketConnected = false;

    char hdrContentType[120];
    char hdrContentLength[20];

    void shutdownWSS(){
      if (wss != nullptr){
        wss->shutdown();
        delete wss;
        wss = nullptr;
      }
    }

    void monitorInactivityTimeout(){
      if (lastActivityMs == 0) return;
      if (socketConnected) return;

      const uint32_t now = millis();
      if ((uint32_t)(now - lastActivityMs) > WEBAPP_INACTIVITY_TIMEOUT_MS){
        DEBUG_INFO("[WEBAPP]\t\tInactivity timeout reached (%ums), stopping service", WEBAPP_INACTIVITY_TIMEOUT_MS);
        switchDevice(false);
      }
    }
};

WebApplicationService gWebApplicationService(false);    // internal web app lifecycle service


#endif
