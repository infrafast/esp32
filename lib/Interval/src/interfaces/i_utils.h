/**
 * @file i_utils.h
 * @brief System configuration, network discovery and browser services.
 */

#ifndef _IUTILS_h
#define _IUTILS_h  

#include "Interfaces.h"
#include <aWOT.h>
#include <WebSocketServer.hpp>


using namespace net;

 
#define ADDRESS_PROVIDER      "/provider/"
#define ADDRESS_TEXT    "/text/%s"

#define GROUP_INTERFACE_UTILS "Utils"


/**
 * @brief Exposes system commands and provider choices to strips and configuration.
 */
class SystemInterface: public DeviceInterface, public DataProvider{
    #define SYSTEM_INTERFACE "system"
    public:    

      SerializableParameters systemDS = SerializableParameters(SYSTEM_INTERFACE, GROUP_INTERFACE_UTILS, &dataRepo);   // one instance
          
      DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

      /** @brief Initialize SystemInterface with the supplied configuration and bindings. */
      SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),DataProvider(getProviderNameForData()){
        state.code = IF_READY;     
      }

      virtual String getDefaultHint() const override { return *lang.getString("hint.system"); }

      /** @brief Start this interface using its current configuration. */
      virtual bool begin(){
        DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
        return true;
      }

      /** @brief End the interface session and release its active connection. */
      virtual bool end(){
        DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());         
        return true;
      }    
  
      /**
       * @brief Internal system interface hook; no separate interface configuration is loaded.
       */
      virtual void load() override{
        DEBUG_INFO("%s::load() default ignored => 🚫DISABLED", interfaceName.c_str()); 
        // ignore the given parameter. Just enable the interface
        state.enabled = false;       
        hasStrip = true;             // textStrip...
      }

      /**
       * @brief Internal system interface hook; no separate interface configuration is saved.
       */
      virtual bool save() override{
        DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      } 

    /**
     * @brief Placeholder hook; no active processing in this implementation.
     *
     * @return Always false.
     */
    virtual bool monitor() override{
      return false;
    }
    
    /** @brief Read pending input from this transport or hardware source. */
    virtual void* read(){return nullptr;}
    
    /** @brief Run the initialization-specific interface startup path. */
    virtual bool _begin() override{
      DEBUG_INFO("%s::_begin() => registered systemDS",interfaceName.c_str());          
      systemDS.registerDataSetAddress(dsText.address);      // register all ds             

      return true;
    }

    /** @brief Return the number of choices exposed by this dataset. */
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
    
    /** @brief Return a borrowed choice label for the requested index. */
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
       
    
}
/** @brief Internal system-command interface, hidden from interface configuration. */
systemInterface(false);      // not exposed


/**
 * @brief Discovers RTP-MIDI services and updates the shared endpoint directory.
 *
 * Workflow:
 *
 *   Enable or Refresh
 *          |
 *          v
 *   handleConnectService()
 *          |
 *          v
 *   startDiscovery()
 *          |
 *          v
 *   pending / Scanning network
 *          |
 *          +--> normal callback or timeout
 *                         |
 *                         v
 *              ready / N endpoint(s)
 *
 * The timeout is a safety net when the Ethernet Bonjour callback or Wi-Fi
 * discovery task does not report completion.
 */
class DirectoryInterface: public NetworkDevice{
    #define DIRECTORY_INTERFACE "Directory"
    public:    

      RtpDirectory rtpDir;
      Event evtDir = Event(&rtpDir);

      /** @brief Initialize DirectoryInterface with the supplied configuration and bindings. */
      DirectoryInterface(bool _configurable = false):NetworkDevice(DIRECTORY_INTERFACE,GROUP_INTERFACE_UTILS,APPLE_MIDI_PORT,0,"",_configurable){
        attachDeviceInterface(this);
        unregisterDataSetAddress(&addressIP);              // we un-expose the ping parameter as user should not modify it 
        unregisterDataSetAddress(&addressPort);              // we un-expose the ping parameter as user should not modify it 
      }

      /** @brief Return the static explanation shown while Directory is idle. */
      String directoryBaseHint() const {
        return *lang.getString("hint.directory");
      }

      /** @brief Return the explanation shown while endpoint discovery is running. */
      String directoryScanningHint() const {
        return *lang.getString("hint.directory.scanning");
      }

      /** @brief Return the explanation and discovered endpoint list for hover display. */
      String directoryEndpointHint() const {
        String value = directoryBaseHint();
        if (rtpDir.epCount == 0){
          return concatStrings(value, " ", *lang.getString("hint.directory.no_endpoints"));
        }

        value += " ";
        value += *lang.getString("hint.directory.discovered");
        value += " ";
        for (int i = 0; i < rtpDir.epCount; i++){
          if (i > 0) value += " | ";
          value += rtpDir.ep[i].name;
          value += " / ";
          value += rtpDir.ep[i].ip.toString();
          value += ":";
          value += String(rtpDir.ep[i].port);
        }
        return value;
      }

      /** @brief Return the localized default Directory tooltip. */
      virtual String getDefaultHint() const override {
        return *lang.getString("hint.directory");
      }

      /** @brief Project the disabled Directory state into its hover hint. */
      virtual void setStateDisabled() override{
        DeviceInterface::setStateDisabled();
        state.hint = directoryBaseHint();
      }

      /** @brief Start this interface using its current configuration. */
      virtual bool begin(){
        DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
        return true;
      }

      /** @brief End the interface session and release its active connection. */
      virtual bool end(){
        DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());         
        return true;
      }      


      /**
       * @brief Apply the internal discovery service startup defaults.
       */
      virtual void load() override{
        DEBUG_INFO("%s::load() default ignored => ENABLE", interfaceName.c_str()); 
        // ignore the given parameter. 
        switchDevice(DEVICE_OFF);          // the interface is forced disabled at startup as we do not need to browse network until a request is sent (by startWebApp to publish DNS name or by appleMidi to advert)
        state.hint = directoryBaseHint();
        hasStrip = false;             
      }

      /** @brief Run the initialization-specific interface startup path. */
      virtual bool _begin() override{     // called by init()
        DEBUG_INFO("%s::_begin() registering serviceFoundWrapper and _instance",interfaceName.c_str());
        instance_ = this;  // enregistrer l’instance active       // we don't do that in the constructor as the instance has multiple heritage, hence we want to be sure the object is fully created
        EthernetBonjour.setServiceFoundCallback(&DirectoryInterface::serviceFoundWrapper);  

        // NetworkDevice::_begin() registers this interface as a listener of
        // hwm.network and then calls DirectoryInterface::begin().  This call
        // is required because Directory overrides _begin(); without it,
        // EVENT_NETWORK_CONNECTED never reaches Directory and its disabled
        // message remains stuck at the boot-time value 0.0.0.0.
        const bool initialized = NetworkDevice::_begin();

        // If the network was already connected before this interface was
        // initialized, no connection event will be replayed. Refresh once
        // here so a subsequent fullConfig response is already correct.
        if (initialized && !deviceInterface->state.enabled &&
            hwm.network->state == CONNECTED){
          refreshDisabledNetworkDetails();
        }

        return initialized;
      }

      /**
       * @brief Internal discovery-service save hook; no file is written here.
       */
      virtual bool save() override{
        DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      }           

      /** @brief Perform one service-specific connection and data-processing pass. */
      virtual bool monitorService() override{
        // Discovery normally completes through a Bonjour callback or the
        // Wi-Fi worker task. The timeout is a safety net when a backend fails
        // to emit its completion callback.
        if (discoveryInProgress &&
            millis() - discoveryStartedAt >= EP_DISCOVERY_TIME_OUT) {
          DEBUG_WARN("%s: discovery timeout after %lu ms",
                     interfaceName.c_str(),
                     static_cast<unsigned long>(millis() - discoveryStartedAt));
          epDirectoryUpdated();
        }
        if (hwm.network->media == CABLE_MEDIA){
          EthernetBonjour.run();
        }else if (hwm.network->media == WIFI_MEDIA){
          //Sur ESP32, mDNS n’a PAS besoin d’être appelé dans loop() une fois qu’il est démarré.
        }          
        return false;     
      }
     
      /** @brief Start this service on the available network connection. */
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
        state.hint = directoryScanningHint();
        // Direct deviceEnable(true) calls do not pass through
        // discoveryRequest(). Start discovery here so Enable and Refresh
        // follow the same workflow.
        startDiscovery();

        return true;
      }

      /** @brief Handle relevant incoming events for this component. */
      void onEvent(const Event& evt) override {
        switch(evt.type){
          case EVENT_REQUEST_DIRECTORY_SERVICE_REFRESH:
            discoveryRequest();
          break;        
          case EVENT_NETWORK_CONNECTED:
            // Directory is deliberately disabled at boot.  The network can
            // therefore acquire its address after Directory::load() has
            // already prepared the disabled state. Refresh that state now so
            // the Web GUI does not keep displaying the initial 0.0.0.0 value.
            if (!deviceInterface->state.enabled && hwm.network->state == CONNECTED){
              refreshDisabledNetworkDetails(true);
            }
          break;
        }
      }

      /** @brief Request an endpoint discovery pass. */
      void discoveryRequest(){
        DEBUG_INFO("%s::discoveryRequest()",interfaceName.c_str());   
        if (deviceInterface->state.enabled){       
          // drop the request if interface is up and running
          startDiscovery();
        }else{
          // if interface is not up and running, we try to bring it up
          switchNetworkService(true);
        }
      }      

      /**
       * @brief Refresh Directory discovery from the generic web configuration action.
       *
       * Directory is intentionally disabled when idle, but the web "refresh"
       * action is also the explicit user request to enable it and scan for
       * endpoints.  Connecting this hook keeps the generic framework action
       * useful without changing refresh behavior for other interfaces.
       */
      void handleRequestRefreshState() override {
        discoveryRequest();
      }

      /**
       * @brief Placeholder hook; no active operation in this implementation.
       *
       * @return Always true; no device action is performed here.
       */
      virtual bool ping(){return true;}
      /** @brief Placeholder hook; no active operation in this implementation. */
      virtual void echo(void* data){}
      
      /** @brief Update the quality-of-service indicator used by the display. */
      virtual void determineQoS() override{
        this->state.QoSlevel = QoS::GOOD;     // to be set based on webserver data flow speed
      }

      /**
       * @brief Update the disabled-state explanation with the current IP.
       *
       * Directory is disabled during normal startup and may be initialized
       * before Wi-Fi/Ethernet has completed. In that case getIP() initially
       * returns 0.0.0.0. This helper is reused both when the service is
       * disconnected and when the network later reports EVENT_NETWORK_CONNECTED.
       * The optional broadcast flag is used for the latter case because the
       * Web GUI must be notified even though the interface enabled state did
       * not change.
       */
      void refreshDisabledNetworkDetails(bool broadcast = false){
        deviceInterface->state.details = concatStrings(
          *lang.getString("Device is not detectable on the network."),
          " ",
          *lang.getString("Use"),
          " ",
          hwm.network->getIP(),
          " ",
          *lang.getString("to connect"));

        if (broadcast){
          deviceInterface->state.broadcastState = true;
        }
      }

      /** @brief Close this service and release its transport resources. */
      virtual bool handleDisconnectService() override{
        DEBUG_INFO("%s::handleDisconnectService()",interfaceName.c_str());
        if (hwm.network->lastActiveMedia == CABLE_MEDIA){
          // eth
          EthernetBonjour.stop();
        }else if (hwm.network->lastActiveMedia == WIFI_MEDIA){
          // wifi
          MDNS.end();
        }          

        if (deviceInterface->state.enabled == false) setStateDisabled();
        refreshDisabledNetworkDetails();

        return true;
      }        

    protected:
      
      static DirectoryInterface* instance_;

      /** @brief True while a discovery pass is waiting for completion. */
      bool discoveryInProgress = false;
      /** @brief Start time used by the non-blocking discovery timeout guard. */
      unsigned long discoveryStartedAt = 0;

      /**
       * @brief Start RTP-MIDI discovery using the active network connection.
       *
       * Ethernet discovery uses Bonjour callbacks; Wi-Fi uses an mDNS query worker.
       * Results update the bounded shared endpoint directory before notifying listeners.
       */
      void startDiscovery(){
        // IF_NOT_READY also represents the intentional pending state used
        // during a scan. The network manager is authoritative for readiness.
        if (hwm.network->state != CONNECTED ||
            (hwm.network->media != CABLE_MEDIA && hwm.network->media != WIFI_MEDIA)){
          epDirectoryNotReady();
        }else{
          epDirectoryRefresh();
        }        
      }      
      
      // Méthode statique compatible FreeRTOS
      /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
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
      /** @brief Test whether an address contains only zero bytes. */
      static bool isZeroIP(const byte ip[4]) {
        return ip && ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
      }

      /** @brief Recognize the AppleMIDI service name, with an optional trailing dot. */
      static bool isAppleMidiServiceType(const char* serviceType) {
        // Accepte "_apple-midi" ou "_apple-midi." (point final parfois présent)
        if (!serviceType) return false;
        return (strncmp(serviceType, "_apple-midi", 11) == 0);
      }

      /**
       * @brief Accept valid UDP AppleMIDI discoveries and recognize end-of-scan callbacks.
       *
       * Ignore zero addresses and unrelated service types. A missing serviceName means
       * the scan ended, not a new endpoint; notify consumers through the directory flow.
       */
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

      /** @brief Notify consumers that endpoint discovery has finished. */
      void epDirectoryUpdated(){
        // Normal callbacks and the timeout converge here. Ignore a late
        // callback after timeout finalization to avoid duplicate broadcasts.
        if (!discoveryInProgress) {
          return;
        }
        discoveryInProgress = false;
        DEBUG_INFO("[RTP]\t\tScan terminé (timeout).");
        DEBUG_INFO("[RTP]\t\t%d endpoint(s) mémorisé(s):\n", rtpDir.epCount);      
        for (int i = 0; i < rtpDir.epCount; i++) {
          DEBUG_INFO("  #%d  %s  %s:%u\n", i+1, rtpDir.ep[i].name, rtpDir.ep[i].ip.toString().c_str(), rtpDir.ep[i].port);
        }
        // NetworkDevice::enable() marks Directory as pending while discovery
        // runs.  The completion callback must close that transition itself;
        // otherwise the web GUI remains stuck on "Scanning network" forever.
        String endpointDetails = rtpDir.epCount == 0
          ? *lang.getString("No endpoints detected")
          : concatStrings(String(rtpDir.epCount), " endpoint(s) — ",
                          *lang.getString("Device is discoverable on the network"),
                          " (", hwm.network->getIP(), ")");
        state.hint = directoryEndpointHint();
        DeviceInterface::setStateReady(
          "green", lang.getString("online")->c_str(), "", endpointDetails.c_str());
        state.broadcastState = true;
        state.broadcastConfig = true;

        // midi interface has to update its config and regenerate its state      
        evtDir.type = EVENT_DIRECTORY_SERVICE_REFRESHED;
        fireEvent(evtDir);
      }

      /** @brief Notify consumers that endpoint discovery is unavailable. */
      void epDirectoryNotReady(){
        discoveryInProgress = false;
        DEBUG_INFO("[RTP]\t\tDirectory not ready");
        // The network is unavailable, so do not expose endpoints from a
        // previous discovery pass in the runtime hint.
        rtpDir.clear();
        state.hint = directoryEndpointHint();
        DeviceInterface::setStateReady(
          "green", lang.getString("online")->c_str(), "",
          lang.getString("No endpoints detected")->c_str());
        state.broadcastState = true;
        evtDir.type = EVENT_DIRECTORY_NOT_READY;
        fireEvent(evtDir);        
      }      

      /** @brief Refresh the endpoint directory and associated interface status. */
      void epDirectoryRefresh(){
        // SCAN FOR APPLE MIDI SESSION ON THE NETWORK
        rtpDir.clear();
        // A refresh can be requested while Directory is already online. In
        // that case NetworkDevice::enable() is not involved, so publish the
        // pending state explicitly before starting the new scan.
        DeviceInterface::setStatePending();
        state.hint = directoryScanningHint();
        state.broadcastState = true;
        // This flag is cleared by the normal completion callback or by the
        // timeout in monitorService(), so Directory cannot remain pending
        // indefinitely when a discovery backend is silent.
        discoveryInProgress = true;
        discoveryStartedAt = millis();
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
    
      /** @brief Bridge the callback/task entry to the responsible application or transport instance. */
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

      /**
       * @brief wrapper compatible avec la signature attendue par la lib
       */
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
      

}
/** @brief Shared RTP-MIDI discovery service exposed to configuration. */
directoryService(true);    // exposed
DirectoryInterface* DirectoryInterface::instance_ = nullptr;


/**
 * @brief Manages the configuration HTTP and WebSocket services.
 */
class WebApplicationService: public NetworkDevice{
  #define WEBAPP_INTERFACE "WebApplication"
  #define WEBAPP_INACTIVITY_TIMEOUT_MS 20000

  public:
    using WebServerHandler = void (*)(Request&, Response&);
    using WebSocketHandler = void (*)(IWebSocket&);

    Event evtWebApp = Event();  //no payload/data

    Application* webServer = nullptr;
    IWebSocketServer* wss = nullptr;

    /**
     * @brief Initialize WebApplicationService and its configured member bindings.
     */
    WebApplicationService(bool _configurable = false):NetworkDevice(WEBAPP_INTERFACE,GROUP_INTERFACE_UTILS,80,80,"",_configurable){}

    virtual String getDefaultHint() const override { return *lang.getString("hint.webapplication"); }

    /** @brief Return the Ethernet connection-budget weight used by this service. */
    virtual uint8_t getServiceWeight(){
      return 0;                               // in reality it consume 3 but we book 8-3 = 5 as SERVICES_WEIGHT_LIMIT so it can always start (priority)
    }


    /**
     * @brief Start with this internal service disabled and hidden from strip creation.
     *
     * No separate interface file is loaded here. Opening configuration enables the
     * service explicitly instead of restoring it as an always-on strip interface.
     */
    virtual void load() override{
      DEBUG_INFO("%s::load() default ignored => 🚫DISABLED", interfaceName.c_str());
      state.enabled = false;
      state.switchable = false;
      hasStrip = false;
    }

    /** @brief Start this interface using its current configuration. */
    virtual bool begin(){
      DEBUG_INFO("%s::begin() => nothing to do",interfaceName.c_str());
      return true;
    }

    /** @brief End the interface session and release its active connection. */
    virtual bool end(){
      DEBUG_INFO("%s::end() => nothing to do",interfaceName.c_str());         
      return true;
    }

    /** @brief Stop the active operation or connection. */
    virtual bool stop() override{
      DEBUG_INFO("%s::stop()",interfaceName.c_str()); 
      return handleDisconnectService();
    }

    /** @brief Set the interface status to pending. */
    virtual void setStatePending(const char* led = "green medium", const char* details = lang.getString("Scanning network")->c_str()){
      setStateConnected();  // web interface when pending is in fact connected...
    }

    /**
     * @brief Acknowledge saving without creating an internal-service configuration file.
     */
    virtual bool save() override{
      DEBUG_INFO("%s::save() IGNORED", interfaceName.c_str());
      return true;
    }

    /** @brief Perform one service-specific connection and data-processing pass. */
    virtual bool monitorService() override{
      monitorInactivityTimeout();
      return handleWeb(); // report if we processed any data
    }

    /** @brief Update the quality-of-service indicator used by the display. */
    virtual void determineQoS() override{
      state.QoSlevel = QoS::GOOD;     // to be set based on webserver data flow speed
    }

    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always true; no device action is performed here.
     */
    virtual bool ping() override{return true;}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void echo(void* data){}

    /**
     * @brief Start browser services and publish EVENT_WEBAPP_STARTED on success.
     */
    virtual bool handleConnectService() override{
      bool started = startWebApp();
      if (started){
        setStateReady();
        evtWebApp.type = EVENT_WEBAPP_STARTED; // notifies listeners that the webapp is started, so they can for example publish their API documentation on it
        fireEvent(evtWebApp);           
      }
      return started;
    }

    /**
     * @brief Stop browser services and publish EVENT_WEBAPP_STOPPED.
     */
    virtual bool handleDisconnectService() override{
      stopWebApp();

      evtWebApp.type =  EVENT_WEBAPP_STOPPED;  
      fireEvent(evtWebApp);           


      setStateDisconnected();
      return true;
    }

    /** @brief Apply the requested enabled state through the interface lifecycle. */
    virtual void switchDevice(bool _enabled) override{
      if (!_enabled){
        socketConnected = false;
      }else{
        reportActivity();
      }
      NetworkDevice::switchDevice(_enabled);
    }

    /** @brief Register the HTTP request handler used by the configuration service. */
    void setWebServerHandler(WebServerHandler callback){
      webServerHandler = callback;
    }

    /** @brief Register the WebSocket connection handler used by the configuration service. */
    void setWebSocketHandler(WebSocketHandler callback){
      webSocketHandler = callback;
    }

    /** @brief Record browser activity to postpone the inactivity timeout. */
    void reportActivity(){
      lastActivityMs = millis();
    }

    /** @brief Record a newly connected browser session. */
    void reportSocketConnected(){
      socketConnected = true;
      reportActivity();
    }

    /** @brief Record browser disconnection and update service activity. */
    void reportSocketClosed(){
      socketConnected = false;
      reportActivity();
    }

    /**
     * @brief Serve pending HTTP clients and poll the browser WebSocket server.
     *
     * Choose EthernetClient or WiFiClient from configMedia. Each accepted HTTP client
     * is processed and closed; WebSocket handling is polled separately.
     * @return Whether an HTTP client was processed during this pass.
     */
    bool handleWeb(){
      Network* configMedia = hwm.configMedia;

      bool processedData = false;

      if (configMedia != nullptr && configMedia->state == CONNECTED){
        if(webServer != nullptr){
          if (configMedia->media == CABLE_MEDIA){
            EthernetClient newEthClient = ((EthernetServer*)configMedia->server)->available();
            if (newEthClient) {
              webServer->process(&newEthClient);
              newEthClient.stop();
              processedData = true;
            }
          }else if (configMedia->media == WIFI_MEDIA){
            WiFiClient newWiFiClient = ((WiFiServer*)configMedia->server)->available();
            if (newWiFiClient.connected()) {
              webServer->process(&newWiFiClient);
              newWiFiClient.stop();
              processedData = true;
            }
          }
        }
        if (wss != nullptr) wss->listen();
      }

      return processedData;

    }

    /** @brief Start the HTTP server and browser configuration service. */
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

    /** @brief Stop browser configuration services. */
    void stopWebApp(){
      shutdownWSS();
      if (webServer != nullptr){
        delete webServer;
        webServer = nullptr;
      }
      socketConnected = false;
      DEBUG_INFO("%s::down",interfaceName.c_str());          
    }

    /** @brief Create or start the WebSocket server for the configuration connection. */
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

    /** @brief Close and release the configuration WebSocket server. */
    void shutdownWSS(){
      if (wss != nullptr){
        Network* configMedia = hwm.configMedia;
        const bool networkReadyForGracefulShutdown =
          (configMedia != nullptr && configMedia->state == CONNECTED && configMedia->media != NO_MEDIA);

        if (networkReadyForGracefulShutdown){
          wss->shutdown();
        }else{
          DEBUG_INFO("[WSS]\t\tskip graceful shutdown (network down)");
        }
        delete wss;
        wss = nullptr;
      }
    }

    /**
     * @brief Apply the inactivity timeout to idle browser configuration services.
     *
     * Activity reports keep the session alive. This service lifecycle is separate
     * from the mixer/device transports used by playing strips.
     */
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

#endif
