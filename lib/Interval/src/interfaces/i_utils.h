#ifndef _IUTILS_h
#define _IUTILS_h  

#include "Interfaces.h"

 
#define ADDRESS_PROVIDER      "/provider/"
#define ADDRESS_TEXT    "/text/%s"

#define GROUP_INTERFACE_UTILS "utils"


class SystemInterface: public DeviceInterface, public DataProvider{
    #define SYSTEM_INTERFACE "system"
    public:    

    SerializableParameters systemDS = SerializableParameters(SYSTEM_INTERFACE, GROUP_INTERFACE_UTILS, &dataRepo);   // one instance
        
    DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),DataProvider(getProviderNameForData()){
      state.code = IF_READY;     
    }
  
      virtual void load() override{
        DEBUG_PRINT("\n%s::load() default ignored => DISABLED", interfaceName.c_str()); 
        // ignore the given parameter. Just enable the interface
        state.enabled = false;       
        hasStrip = true;             // textStrip...
      }

      virtual bool save() override{
        DEBUG_PRINT("\n%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      } 

    virtual void monitor(){}
    
    virtual void* read(){return nullptr;}
    
    virtual bool begin(){
      DEBUG_PRINT("\n%s::begin() => registered systemDS",interfaceName.c_str());          
      systemDS.registerDataSetAddress(dsText.address);      // register all ds             
      jsonInputRepo.add(&systemDS);     // registered once

      //dataProviderName = getProviderNameForData();      

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
     
      DEBUG_PRINT("\n<%s>DataSet getSize(%s) is not implemented",dataProviderName.c_str(),addrr.c_str());
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

      DEBUG_PRINT("\n<%s>DataSet getValuePtr(%s,%d) is not implemented",dataProviderName.c_str(),addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";    
        
    
}systemInterface(false);      // not exposed

class DirectoryInterface: public DeviceInterface, public NetWorkService{
    #define DIRECTORY_INTERFACE "Directory"
    public:    

      RtpDirectory rtpDir;

      DirectoryInterface(bool _configurable = false):DeviceInterface(DIRECTORY_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),NetWorkService(){
        attachDeviceInterface(this);
      }

      virtual void load() override{
        DEBUG_PRINT("\n%s::load() default ignored => ENABLE", interfaceName.c_str()); 
        // ignore the given parameter. Just enable the interface
        state.enabled = true;       
        hasStrip = false;             
      }

      virtual bool begin(){     // called by init()
        DEBUG_PRINT("\n%s::begin() registering serviceFoundWrapper and _instance",interfaceName.c_str());
        instance_ = this;  // enregistrer l’instance active       // we don't do that in the constructor as the instance has multiple heritage, hence we want to be sure the object is fully created
        EthernetBonjour.setServiceFoundCallback(&DirectoryInterface::serviceFoundWrapper);  
        return true;        
      }

      virtual bool save() override{
        DEBUG_PRINT("\n%s::save() IGNORED", interfaceName.c_str()); 
        return true;
      }      

      virtual void monitor(){
          monitorNetwork();
      }

      virtual void switchDevice(bool _enabled) override{
        switchNetworkService(_enabled);
      }         

      virtual void* read(){return nullptr;}

      virtual void monitorService(){
        if (hwm.network->media == CABLE_MEDIA){
          EthernetBonjour.run();
        }else if (hwm.network->media == WIFI_MEDIA){
          //Sur ESP32, mDNS n’a PAS besoin d’être appelé dans loop() une fois qu’il est démarré.
        }         
      }
     
      virtual bool handleConnectService(){
        DEBUG_PRINT("\n%s::handleConnectService()",interfaceName.c_str());
        String baseName = *systemCfg.getString(S_AP_NAME);

        if (hwm.network->media == WIFI_MEDIA){
          if (MDNS.begin(baseName.c_str())){
            MDNS.addService(MDNS_ANNOUNCED_APPLEMIDI_SERVICE, "udp", APPLE_MIDI_PORT);
            MDNS.addService("http", "tcp", 80);                                
          }else{
            DEBUG_PRINT("\nMDNS registration <%s> failed",baseName.c_str());
          }
        }else if (hwm.network->media == CABLE_MEDIA){
          String midiName = concatStrings(baseName, "._", MDNS_ANNOUNCED_APPLEMIDI_SERVICE);
          String httpName = concatStrings(baseName, "._http");
          if (EthernetBonjour.begin(baseName.c_str())){
            EthernetBonjour.addServiceRecord(midiName.c_str(), APPLE_MIDI_PORT,MDNSServiceUDP);
            EthernetBonjour.addServiceRecord(httpName.c_str(), 80, MDNSServiceTCP);
          }else{
            DEBUG_PRINT("\nEthernetBonjour registration <%s> failed", baseName.c_str());
          } 
        }

        //startDiscovery();  // we do not spontaneously trigger a discovery, it has to be requested by someone, this will be helpful when managing the locks

        setStateReady("green",*lang.getString("online"),"",*systemCfg.getString(S_AP_NAME));

        return true;
      }

      void startDiscovery(){
        DEBUG_PRINT("\n%s::startDiscovery()",interfaceName.c_str());

        if (deviceInterface->state.code == IF_NOT_READY){
          epDirectoryNotReady();
        }else{
          // SCAN FOR APPLE MIDI SESSION ON THE NETWORK
          rtpDir.clear();
          if (hwm.network->media == CABLE_MEDIA){
            // Découvre le type de service "_apple-midi" en UDP ethernet = non bloquant
            EthernetBonjour.startDiscoveringService("_apple-midi", MDNSServiceUDP, 6000); // :contentReference[oaicite:3]{index=3} 
            DEBUG_PRINT("\n\n[RTP-bonjour]\t\tScan RTP-MIDI lancé...\n");  
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
      }      
      
      virtual bool handleDisconnectService(){
        DEBUG_PRINT("\n%s::handleDisconnectService()",interfaceName.c_str());
        if (hwm.network->lastActiveMedia == CABLE_MEDIA){
          // eth
          EthernetBonjour.stop();
        }else if (hwm.network->lastActiveMedia == WIFI_MEDIA){
          // wifi
          MDNS.end();
        }          

        setStateOffline(concatStrings(*lang.getString("Device is not detectable on the network. Use")," ",hwm.network->IP.toString()," ",*lang.getString("to connect this device")));                

        return true;
      }        

    protected:
      
    static DirectoryInterface* instance_;
      
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
        DEBUG_PRINT("\n\n[RTP]\t\tScan terminé (timeout).");
        DEBUG_PRINT("\n[RTP]\t\t%d endpoint(s) mémorisé(s):\n", rtpDir.epCount);      
        for (int i = 0; i < rtpDir.epCount; i++) {
          DEBUG_PRINT("\n  #%d  %s  %s:%u\n", i+1, rtpDir.ep[i].name, rtpDir.ep[i].ip.toString().c_str(), rtpDir.ep[i].port);
        }
        // midi interface has to update its config and regenerate its state      
        evt.type = EVENT_DIRECTORY_SERVICE_REFRESHED;
        evt.data = &rtpDir;
        fireEvent();
      }

      void epDirectoryNotReady(){
        DEBUG_PRINT("\n\n[RTP]\t\tDirectory not ready");
        evt.type = EVENT_DIRECTORY_NOT_READY;
        evt.data = &rtpDir;
        fireEvent();
      }      

      void wiFiRTPDiscoveryTask(){
        DEBUG_PRINT("\n\n[RTP-WiFi]\t\tScan RTP-MIDI lancé...\n");     
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


#endif