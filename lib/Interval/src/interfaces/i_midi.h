#ifndef I_MIDI_h
#define I_MIDI_h

#include "Interfaces.h"

#include "i_osc.h"

#include <BLEMIDI_Transport.h>
//#include <hardware/BLEMIDI_ESP32_NimBLE.h>
#include <hardware/BLEMIDI_ESP32.h>
//#include <hardware/BLEMIDI_ArduinoBLE.h>

#include "MIDIUSB_shim.h"   // required by USB-MIDI.h
#include <USB-MIDI.h>     // sysex but do not work

#include "tusb.h"     //tinyUSB for tud_mounted detection

#include <BLEDevice.h>

enum{
  MIDI_CC,
  MIDI_SYSEX,
  MIDI_NOTE,
  MIDI_PITCH
  //...
};

typedef struct MidiData{
  uint8_t type = MIDI_CC;
  byte channel = 0;
  byte number = 0;
  byte value = 0;
  byte* data = nullptr;
};

#ifdef DEBUG
  class USBMIDISimul{
    public:
      USBMIDISimul(){}
      void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){DEBUG_PRINT("\tUSBMIDI::sendNoteOn(%d,%d,%d)\t",inNoteNumber,inVelocity,inChannel);}
      void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){DEBUG_PRINT("\tUSBMIDI::sendNoteOff(%d,%d,%d)\t",inNoteNumber,inVelocity,inChannel);}
      void sendProgramChange(DataByte inProgramNumber,Channel inChannel){DEBUG_PRINT("\tUSBMIDI::sendProgramChange(%d,%d)\t",inProgramNumber,inChannel);}
      void sendControlChange(DataByte inControlNumber,DataByte inControlValue,Channel inChannel){DEBUG_PRINT("\tUSBMIDI::sendControlChange(%d,%d,%d)\t",inControlNumber,inControlValue,inChannel);}
      bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries=false){
        DEBUG_PRINT("\tUSBMIDI::sendSysEx(%u,[", inLength);
        for (unsigned i=0;i<inLength;i++) DEBUG_PRINT("%02X", inArray[i]);
        DEBUG_PRINT("],%d)\t", inArrayContainsBoundaries ? 1 : 0);
        return true;
      }
      void setHandleSystemExclusive(SystemExclusiveCallback fptr) { DEBUG_PRINT("\tUSBMIDI::setHandleSystemExclusive(f*)\t");}
      bool read(){return false;}
  }usbMIDI;
#else

  USBMIDI_NAMESPACE::usbMidiTransport usbMIDI2(0);
  MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport> usbMIDI((USBMIDI_NAMESPACE::usbMidiTransport&)usbMIDI2);

#endif


class MIDIInterface: public DeviceInterface{
  #define GROUP_INTERFACE_MIDI "MIDI"
  public:

    MidiData midiData;

    MIDIInterface(String _name,String _group,bool _configurable):DeviceInterface(_name,_group,_configurable){
      data = &midiData;     
    }
   
    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) = 0;
    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;
    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;

    static void handleSysexStatic(byte* data, unsigned length) {
      if (baseInstancePtr) {
          baseInstancePtr->handleSysex(data,length);
      }
    }

    static void handleChangeControlStatic(byte channel, byte number, byte value){
        if (baseInstancePtr) {
            baseInstancePtr->handleChangeControl(channel,number,value);
        }      
    }

    virtual void handleChangeControl(byte channel, byte number, byte value){
      DEBUG_PRINT("\nhandleChangeControl channel:%i, number:%i value:%i",channel,number,value);
      // FEED the midi data structure with data
      midiData.type = MIDI_CC;
      midiData.channel = channel;
      midiData.number = number;
      midiData.value = value;
      // and indicate how much is available
      dataAvailable = 1;
    }

    virtual void handleSysex(byte* sysexdata, unsigned length){
      DEBUG_PRINT("\nhandleSysex: %c, %i bytes: ",getSysExStatus(sysexdata, length),length);
      for (uint16_t i = 0; i < length; i++) DEBUG_PRINT("%02X ",sysexdata[i]);
      // FEED the midi data structure with data

      midiData.type = MIDI_SYSEX;
      // Libère l'ancien buffer si nécessaire
      if (midiData.data != nullptr) {
          delete[] midiData.data;
          midiData.data = nullptr;
      }
      // Allocation du nouveau buffer
      if (length > 0) {
        midiData.data = new byte[length];
        if (midiData.data != nullptr) {
            memcpy(midiData.data, sysexdata, length);
        }
      }

      // and indicate how much is available
      dataAvailable = length;
    }    


    char getSysExStatus(const byte* data, uint16_t length){
      if (!data || length==0) return 'E';
      if (data[0]==0xF0 && data[length-1]==0xF7) return 'F';
      if (data[0]==0xF0) return 'S';
      if (data[length-1]==0xF7) return 'E';
      return 'M';
    }    

    void init(){
      baseInstancePtr = this;
      //set the interface data pointer to the midi data structure
    }

  protected:
      static inline MIDIInterface* baseInstancePtr = nullptr;

};


#define BLE_CONNECTION_NAME INTERVAL_MANUFACTURER
BLEMIDI_CREATE_INSTANCE(BLE_CONNECTION_NAME, bleMIDI);

#define BLEMIDI_INTERFACE "BLEMIDI"
class BLEMIDIInterface: public MIDIInterface{

  public:

    BLEMIDIInterface(bool _configurable):MIDIInterface(BLEMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_BLEMIDI;
    }

    virtual uint16_t getSize(String addrr){
      return 0;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      return &emptyString;
    }

    virtual void monitor(){     
      if (state.label == IF_STATE_CONNECTED) 
        bleMIDI.read(); // maybe remove the if as read() may calls the connect and disconnect callbacks ? (as appleMidi)
    }


    virtual bool begin(){
      DEBUG_PRINT("\n%s::begin()",interfaceName.c_str());          
      MIDIInterface::init();  
      bleInstancePtr = this;   // do not forget otherwise callback function will crash

      BLEbleMIDI.setHandleConnected(handleConnectedStatic);
      BLEbleMIDI.setHandleDisconnected(handleDisconnectedStatic);
      
      bleMIDI.setHandleSystemExclusive(handleSysexStatic);

      // workaround: remove the 2 lines below once stop is implemeted correctly. This workaround force to set the interface to enable
      state.switchable = false;  // we cant desactivate the interface  until the stop() is properly implemented
      state.enabled = true;  
      /////////////////////////

      if (!hasBegun){
        if (state.enabled){
          setStatePending();            
          DEBUG_PRINT("\n\tbleMIDI.begin()");          
          bleMIDI.begin();                
          hasBegun = true;
        }else{
          DEBUG_PRINT("\n\tbleMIDI.begin() ignored (disabled)");          
        }
      }else{
        DEBUG_PRINT("\n\tbleMIDI.begin() ignored (hasBegun)");          
      }             

      return true;
    }

    virtual bool stop() override{
      DEBUG_PRINT("\n%s::stop()",interfaceName.c_str());          
      auto transport = bleMIDI.getTransport();  // ✅ On récupère le transport BLE
      hasBegun = false;
      if (transport) {
          if (hasBegun){
            //transport->end();     // stops the connexion but does not trigger the disconnect callback, hence we set the state to disconnected before to avoid having a wrong state until the next connection attempt
            //BLEbleMIDI.end();    // does nothing     
            DEBUG_PRINT("\n\ttransport->end()");          
            //BLEDevice::getAdvertising()->stop();            
          }else{
            DEBUG_PRINT("\n\ttransport->end() ignored (!hasBegun)");    
          }
          // bleMIDI.end() doesnt exists
          return true;
      }
      return false;    
    }     

    void handleConnected() {
      DEBUG_PRINT("\n%s::handleConnected()",interfaceName.c_str());          
      setStateConnected();
    }    

    void handleDisconnected() {
      DEBUG_PRINT("\n%s::handleDisconnected()",interfaceName.c_str());          
      if (state.enabled) setStatePending();
      //else setStateDisabled();                // uncomment this when stop() is correctly implemented
    }


    void setStateConnected(){            
      DeviceInterface::setStateConnected("blue",*lang.getString("paired"));
    }

    void setStatePending(){
      DeviceInterface::setStatePending("blue medium",concatStrings(*lang.getString("Connect Blutooth"), " ", BLE_CONNECTION_NAME));
    } 

    static void handleConnectedStatic() {
        if (bleInstancePtr) {
            bleInstancePtr->handleConnected();
        }
    }

    static void handleDisconnectedStatic() {
        if (bleInstancePtr) {
            bleInstancePtr->handleDisconnected();
        }
    } 

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
      DEBUG_PRINT("\nBLEMIDIInterface::sendSysex(%i) ",inLength);
      if (state.code == IF_READY) bleMIDI.sendSysEx(inLength,inArray,inArrayContainsBoundaries);      
      else return false;
      return true; 
    }

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      DEBUG_PRINT("\tnBLEMIDIInterface::sendNoteOn(%d,%d,%d)",inNoteNumber, inVelocity, inChannel);
      if (state.code == IF_READY) bleMIDI.sendNoteOn(inNoteNumber,inVelocity,inChannel);      
      else return false;
      return true; 
    }

    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      DEBUG_PRINT("\tnBLEMIDIInterface::sendNoteOff(%d,%d,%d)",inNoteNumber, inVelocity, inChannel);
      if (state.code == IF_READY) bleMIDI.sendNoteOff(inNoteNumber,inVelocity,inChannel);      
      else return false;
      return true; 
    }

  private:
    static inline BLEMIDIInterface* bleInstancePtr = nullptr;    
    bool hasBegun = false;
};

#define USBMIDI_INTERFACE "USBMIDI"

class USBMIDIInterface: public MIDIInterface{

  public:  

    USBMIDIInterface(bool _configurable):MIDIInterface(USBMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_USBMIDI;  
    }

    virtual void monitor() override{
      if (useUSBMidi){  // only if USB is active
        #define USB_POLL_DELAY  1000
        if (millis()-lastPoll > USB_POLL_DELAY){  
          // check usb connection periodically but not permenently as it is quite expensive and may cause audio dropouts

          lastPoll = millis();

          if (tud_mounted() && state.code == IF_NOT_READY) setStateConnected();

          if (!tud_mounted() && state.code == IF_READY) {
            setStateDisconnected();
            state.details = "Plugin USB-MIDI";
          }
        }
        usbMIDI.read();
      }
    }

    void setStateRestart(){
      DEBUG_PRINT("\n%s::setStateRestart()",interfaceName.c_str());
      state.code = IF_NOT_READY;
      state.label = IF_STATE_DISABLED;
      state.subState = IF_STATE_DISABLED;
      state.details = "restartUSB";
      state.led = "black";
      state.progress = -1;
    }

    virtual bool begin() override{
      DEBUG_PRINT("\n%s::begin()",interfaceName.c_str());
      MIDIInterface::init();
      // if the fooswtich has started in FW update or debug mode, USB MIDI is not active, hence we set the status once for all
      if (!useUSBMidi){
        setStateRestart();
        state.switchable = false; 
      }else{
        usbMIDI.setHandleSystemExclusive(handleSysexStatic);        
        state.switchable = true; 
      }
      return true;
    }

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){      
      usbMIDI.sendNoteOn(inNoteNumber,inVelocity,inChannel);
      return true;
    }  

    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      usbMIDI.sendNoteOff(inNoteNumber,inVelocity,inChannel);
      return true;
    }

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){    
      DEBUG_PRINT("\nUSBMIDI::sendSysex(%i) ",inLength);      
      usbMIDI.sendSysEx(inLength,inArray,inArrayContainsBoundaries);
      return true;
    }    

  private:
    unsigned long lastPoll = 0;

};


#define DINMIDI_INTERFACE "DINMIDI"
class DINMIDIInterface: public MIDIInterface{
  #define CONFIG_DIN "voidUSBparam"
  public:
    DINMIDIInterface(bool _configurable):MIDIInterface(DINMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_DINMIDI;
      state.switchable = false;
    }

    virtual bool begin(){     // called by init()
      setStateNotImplemented();
      return true;
    };        


    virtual void monitor() override{}

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) override {return false;}
    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) override {return false;}
    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) override {return false;}    

};

enum{
  APPLEMIDI_PASSIVE=1,
  APPLEMIDI_ACTIVE
};

#define APPLEMIDI_INTERFACE "AppleMIDI"
#define APPLEMIDI_SESSIONS_NUMBER   MAX_RTP_DIR_ENDPOINTS+1     // the dropdown will contains as many sessions as there are RTP endpoints + 1 for "passive"

struct SessionOptions{
  String name[APPLEMIDI_SESSIONS_NUMBER];
  int count = 0;
};

#define ADDRESS_APPLEMIDISESSION   "/session/%s"
#define PASSIVE_SESSION_NAME "passive"
class AppleMidiSessionType : public DataSet{
  
  public:
    SessionOptions* sessionOptions = nullptr;
   
    AppleMidiSessionType(Address* _address, SessionOptions* s): sessionOptions(s), DataSet(_address, APPLEMIDI_INTERFACE, &IFdataRepo){}
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &sessionOptions->name[index];    //local javascript function 
    }  

    virtual uint16_t getSize(){
      return sessionOptions->count;
    }
};


class AppleMIDIInterface: public MIDIInterface, public NetWorkInterface {
  #define CONFIG_APPLE "voidAppleMidiParam"
  #define SESSION_DESC_LENGHT 128

  public: 
    
    SessionOptions sessionOptions;
    AppleMidiSessionType dsAppleMidiSessions = AppleMidiSessionType(new DropDownAddress(ADDRESS_APPLEMIDISESSION),&sessionOptions);
    RtpDirectory* rtpDirectory;     // directory of rtp end point populated by the application that we will expose to the user via configuration drop down so he can pick up one
    
    State savedState;

    String midiSessionName = PASSIVE_SESSION_NAME;
    int sessionType = APPLEMIDI_PASSIVE; // 0:passive, 1:active

    int rtpPort = 5004;;
    
    #define UNDEF_EXCEPTION "UndefinedException"
    String exception = UNDEF_EXCEPTION;

    AppleMIDIInterface(bool _configurable, RtpDirectory* _rtpDirectory): rtpDirectory(_rtpDirectory),MIDIInterface(APPLEMIDI_INTERFACE,GROUP_INTERFACE_MIDI, _configurable){
      icon = ICON_APPLEMIDI;
      attachDeviceInterface(this);

      registerDataSetAddress(dsAppleMidiSessions.address);
      
      destination = new PingEcho("0.0.0.0");

      jsonInputRepo.add(this); 

      populateSessionOptions();      

    }

    ~AppleMIDIInterface(){
      delete destination;
    }    

    virtual bool begin() override{      
      DEBUG_PRINT("\nAppleMIDIInterface::begin()");
      MIDIInterface::init();
      appleInstancePtr = this;   // do not forget otherwise callback function will crash
      return true;      
    }

    virtual bool stop() override{          // disconnect the network for network interface
      DEBUG_PRINT("\n%s::stop()",interfaceName.c_str());
      return handleDisconnectService();
    };   

    virtual bool end() override{
      DEBUG_PRINT("\nAppleMIDIInterface::end()");
      //switchDevice(DEVICE_OFF);     // disable the device completely
      switchDevice(DEVICE_OFF);     // disable the device completely
      return true;
    }    


    virtual void userRequestedRefreshInterface(){      
      evt.type = EVENT_START_EP_DIRECTORY_DISCOVERY_SERVICE;
      fireEvent(&evt);
      setStateRefreshing();
    }

    void refreshInternalStateAndConfig(){
      populateSessionOptions();  // regenerate the config data set
      //state = savedState;              // recall the state we had before refresh request
      state.broadcastState = true; // communicate the new state
      state.broadcastConfig = true; // communicate the new parameters
    }

    void populateSessionOptions(){
      sessionOptions.name[0] = PASSIVE_SESSION_NAME;   // first entry is always "passive"
      for (int i = 0; i < MAX_RTP_DIR_ENDPOINTS; i++) {
        if (rtpDirectory->ep[i].name != "") {
          sessionOptions.name[i+1] = concatStrings(rtpDirectory->ep[i].name, " (", rtpDirectory->ep[i].ip.toString(), ":", rtpDirectory->ep[i].port, ")");          
        } else {
          sessionOptions.name[i+1] = "";  // empty entry
        }
      }
      sessionOptions.count = rtpDirectory->epCount+1;
    }

    virtual void serializeProperties(JsonArray& properties) override{   
      
      String sessionOptions = "";
      if (sessionType == APPLEMIDI_ACTIVE){
        sessionOptions = concatStrings(midiSessionName, " (", destination->ip.toString(), ":", rtpPort, ")");  // /sessions/session name (192.168.0.1:5004)
      }else{
        sessionOptions = PASSIVE_SESSION_NAME;     // /session/passive
      }
      

      char* buf = (char*)malloc((strlen(ADDRESS_APPLEMIDISESSION)+sessionOptions.length()+1)*sizeof(char*));
      sprintf(buf,ADDRESS_APPLEMIDISESSION,sessionOptions.c_str());
      properties.add(String(buf)); 
      free(buf);
    }

    /*
      "/session/session name (192.168.0.1:5004)"
      Objectif :
      sessionOptions → "session name (192.168.0.1:5004)"
      sessionName → "session name"
      IPaddress → "192.168.0.1"
      port → 5004
      return true si OK, false si erreur de parsing

      Si la partie après /session/ vaut exactement "passive" (ou commence par "passive" et ne contient pas (), alors c’est OK et :
      sessionOptions = "passive"
      sessionName = "passive"
      ipAddress = ""
      port = 0
      retour true
      Sinon, on applique le parsing normal "name (ip:port)".      
    */
    bool parseSession(const char* input,
                      char* sessionOptions,
                      size_t sessionDescriptorSize,
                      char* sessionName,
                      size_t sessionNameSize,
                      char* ipAddress,
                      size_t ipAddressSize,
                      int& port)
    {
        if (!input || !sessionOptions || !sessionName || !ipAddress)
            return false;

        // Init sorties (évite les restes)
        sessionOptions[0] = '\0';
        sessionName[0] = '\0';
        ipAddress[0] = '\0';
        port = 0;

        // 1) Chercher "/session/"
        const char* base = strstr(input, "/session/");
        if (!base) return false;
        base += strlen("/session/");

        if (*base == '\0') return false; // rien après /session/

        // 2) Copier sessionOptions (tout ce qui suit)
        size_t descLen = strlen(base);
        if (descLen + 1 > sessionDescriptorSize) return false;
        strcpy(sessionOptions, base);

        // --- Cas spécial: "/session/passive" ---
        // OK si pas de '(' et que le contenu est exactement "passive"
        // (si tu veux tolérer des espaces autour, dis-moi et je l'ajuste)
        if (strcmp(sessionOptions, "passive") == 0) {
            if (strlen("passive") + 1 > sessionNameSize) return false;
            strcpy(sessionName, "passive");
            // ipAddress déjà "", port déjà 0
            return true;
        }

        // 3) Parsing normal : "<name> (<ip>:<port>)"
        const char* parenOpen = strchr(sessionOptions, '(');
        if (!parenOpen) return false;

        // Vérifier qu'il y a bien un espace avant '('
        if (parenOpen == sessionOptions || *(parenOpen - 1) != ' ')
            return false;

        // 4) sessionName = avant " ("
        size_t nameLen = (size_t)(parenOpen - sessionOptions) - 1;
        if (nameLen == 0 || nameLen + 1 > sessionNameSize) return false;

        strncpy(sessionName, sessionOptions, nameLen);
        sessionName[nameLen] = '\0';

        // 5) Extraire IP et port depuis "(IP:PORT)"
        // NOTE: ipAddressSize limite la copie finale, sscanf n'en tient pas compte.
        // On lit d'abord dans un buffer local pour éviter overflow.
        char ipTmp[64];
        ipTmp[0] = '\0';
        int pTmp = 0;

        int parsed = sscanf(parenOpen, "(%63[^:]:%d)", ipTmp, &pTmp);
        if (parsed != 2) return false;

        if (strlen(ipTmp) + 1 > ipAddressSize) return false;
        strcpy(ipAddress, ipTmp);

        if (pTmp <= 0 || pTmp > 65535) return false;
        port = pTmp;

        return true;
    }


    virtual void handlePropertyChange() override{
      DEBUG_PRINT("\n%s::handlePropertyChange()",interfaceName.c_str());
      handleDisconnectService();
      handleConnectService();  // is called by handleDisconnect()
    }

    virtual bool restoreProperty(const char* property) override{

      char sessionOptions[SESSION_DESC_LENGHT];
      char sessionName[64];
      char ipAddress[32];
      int port = 0;      


      bool changed = false;
      if (parseSession(property, sessionOptions, sizeof(sessionOptions), sessionName, sizeof(sessionName), ipAddress, sizeof(ipAddress), port)){

        DEBUG_PRINT("\nParsed sessionOptions: %s", sessionOptions);
        DEBUG_PRINT("\nsessionName: %s", sessionName);
        DEBUG_PRINT("\nipAddress: %s", ipAddress);
        DEBUG_PRINT("\nport: %d", port);

        if (strcmp(sessionName, PASSIVE_SESSION_NAME) == 0) {
            if (sessionType != APPLEMIDI_PASSIVE) changed = true;
            sessionType = APPLEMIDI_PASSIVE;
        } else {
          if (sessionType != APPLEMIDI_ACTIVE) changed = true;
          sessionType = APPLEMIDI_ACTIVE;

          if (rtpPort != port) changed = true;   
          rtpPort = port;

          midiSessionName = String(sessionName);

          if (destination->host != String(ipAddress))   changed = true;
          destination->host = String(ipAddress);
          destination->ip.fromString(ipAddress);            

          // the session description is added to the list of end point so it appears on the drop down even if it was not detected (as a recall from saved interface parameter)
          uint8_t ipBytes[4] = {destination->ip[0], destination->ip[1], destination->ip[2], destination->ip[3]};          
          rtpDirectory->upsertEndpoint(sessionName, ipBytes, port);          
        }
       

      }      

      return changed;
    }      

    virtual void monitor() override{     
      monitorNetwork();      
    }

    #define MIDI_INVITE_PERIOD 15000   // should be bigger than the timeout of invite()
    virtual void monitorService(){
      MidiClient* appleMidiClient = hwm.network->appleMidiClient;  
      
      if (appleMidiClient != nullptr){
        // the read trigger callback like connection, hence it has to be called independantly from being connected to a session or not
        //we prefer  to put it here instead than in Network (who controls the object) as in case we are offline, monitor is not called and we save time        
        
        hwm.network->appleMidiClient->read();                       

        if (state.code != IF_READY) {
          if (millis() - lastInvite >= MIDI_INVITE_PERIOD) {
            startSession();
          }
        }              
      }            
    }
    
    // Méthode de l'instance réellement appelée
    void handleConnected(const ssrc_t& ssrc, const char* name) {
      DEBUG_PRINT("\n\nappleMidi session #%i %s connected\n", ssrc, name);
      midiSessionName = String(name);
      setStateConnected();
      // wake up
      // sendRealTime
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
      DEBUG_PRINT("\n\nappleMidi session #%i disconnected\n", ssrc);
      if (state.enabled == false) setStateDisabled();
      else setStateDisconnected();
      lastInvite = 0;    // we reset the timer that controls the periodicity of the invites because in case of disconnection, we want to trigger a new session (invite or passive immediately to reconnect as soon as possible)
    }

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) override{                  
      if (state.code == IF_READY) hwm.network->appleMidiClient->sendSysEx(inLength,inArray,inArrayContainsBoundaries);
      else return false;
      return true; 
    }
    

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      if (state.code == IF_READY) hwm.network->appleMidiClient->sendNoteOn(inNoteNumber,inVelocity, inChannel);
      else return false;
      return true;              
    };

    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){     
      if (state.code == IF_READY) hwm.network->appleMidiClient->sendNoteOff(inNoteNumber,inVelocity, inChannel);
      else return false;
      return true;       

    };


    static void handleConnectedStatic(const ssrc_t& ssrc, const char* name) {
        if (appleInstancePtr) {
            appleInstancePtr->handleConnected(ssrc, name);
        }
    }

    static void handleDisconnectedStatic(const ssrc_t& ssrc) {
        if (appleInstancePtr) {
            appleInstancePtr->handleDisconnected(ssrc);
        }
    }

    static void OnAppleMidiExceptionStatic(const ssrc_t& ssrc, const Exception& e, const int32_t value ){
        if (appleInstancePtr) {
            appleInstancePtr->OnAppleMidiException(ssrc,e,value);
        }      
    }    

    void OnAppleMidiException(const ssrc_t& ssrc, const Exception& e, const int32_t value ) {
      switch (e){
        case APPLEMIDI_NAMESPACE::Exception::BufferFullException:
          exception = "BufferFullException";
          break;
        case APPLEMIDI_NAMESPACE::Exception::ParseException:
          exception = "ParseException";
          break;
        case APPLEMIDI_NAMESPACE::Exception::TooManyParticipantsException:
          exception = "TooManyParticipants";
          break;
        case APPLEMIDI_NAMESPACE::Exception::UnexpectedInviteException:
          exception = "UnexpectedInvite";
          break;
        case APPLEMIDI_NAMESPACE::Exception::ParticipantNotFoundException:
          exception = "ParticipantNotFound";
          break;
        case APPLEMIDI_NAMESPACE::Exception::ComputerNotInDirectory:
          exception = "ComputerNotInDirectory";
          break;
        case APPLEMIDI_NAMESPACE::Exception::NotAcceptingAnyone:
          exception = "NotAcceptingAnyone";
          break;
        case APPLEMIDI_NAMESPACE::Exception::ListenerTimeOutException:
          exception = "ListenerTimeOut";
          break;
        case APPLEMIDI_NAMESPACE::Exception::MaxAttemptsException:
          exception = "MaxAttempts";
          break;
        case APPLEMIDI_NAMESPACE::Exception::NoResponseFromConnectionRequestException:
          exception ="NoResponse";
          break;
        case APPLEMIDI_NAMESPACE::Exception::SendPacketsDropped:
          exception = "SendPacketsDropped";
          break;
        case APPLEMIDI_NAMESPACE::Exception::ReceivedPacketsDropped:
          exception = "ReceivedPacketsDropped";
          break;
        case APPLEMIDI_NAMESPACE::Exception::UdpBeginPacketFailed:
          exception = "UdpBeginPacketFailed";
          break;
        default:
          exception = UNDEF_EXCEPTION;
          break;
      }
      DEBUG_PRINT("\nAppleMIDI Exception: %s (value:%ld)",exception.c_str(),value);
      //handleDisconnected(); is then called by the library
    }    

    virtual void switchDevice(bool _enabled) override{
      DeviceInterface::switchDevice(_enabled);
      NetWorkInterface::switchDevice(_enabled);
      savedState = state;     // save current state as it may be recall after the refreshState
    }   


    void setStatePassiveSession(){
      DEBUG_PRINT("\nAppleMIDIInterface::setStatePassiveSession");      
      //"details": "footswitch (192.168.0.56/5004) ready to be invited",
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.led = "green slow";
      state.progress = -1;

      int port = 0;
      if (hwm.network->appleMidiClient != nullptr){
          midiSessionName = hwm.network->appleMidiClient->getName();
          port = hwm.network->appleMidiClient->getPort();
      }
      state.subState = midiSessionName;      
      state.details = concatStrings(
        midiSessionName,
        " (",
        hwm.network->IP.toString(),
        "/",
        port,
        ") ",
        *lang.getString("ready to be invited")
      );
    }

    void setStateRefreshing(){
      state.details = "Getting things up to date...";   // retour semantique for the user, without changing the interface state
      state.broadcastState = true; // no interface change but inform the webapp that we triggered startDirectoryServiceDiscovery
      state.progress = -1;  // hide the progress bar
      state.led = "green fast";
    }

    void setStateActiveSession(){
      DEBUG_PRINT("\nAppleMIDIInterface::setStateActiveSession() to %s:%d",destination->host.c_str(), rtpPort);  

      MidiClient* appleMidiClient = hwm.network->appleMidiClient;

      appleMidiClient->sendInvite(destination->ip, rtpPort);

      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;

      state.led = "green slow";
      state.progress = -1;
    
      state.subState = destination->host;      
      state.details = concatStrings(
        *lang.getString("Inviting "),
        " ",
        destination->host,
        "/",
        rtpPort
      );
    }

    virtual void setStateConnected(){
      DeviceInterface::setStateConnected();
      state.details = String(midiSessionName);// + concatStrings(" (", hwm.network->appleMidiClient->getParticipantDetails(ssrc), ")");     
    }     

    void setStateDisconnected() override{
      DeviceInterface::setStateDisconnected();       
      state.subState = destination->host;      
      if (exception == UNDEF_EXCEPTION){
        // no exception were raised, hence it is a normal disconnect
        state.label = IF_STATE_DISCONNECTED;
        state.details = *lang.getString("session disconnected");        
      }else{
        // oherwise, this is considered as an error
        state.label = IF_STATE_ERROR;        
        state.details = *lang.getString(exception);
        exception = UNDEF_EXCEPTION; //acknowledged
      }
    }    

    void startSession(){      
      state.progress = -1;  // hide the progress bar      
      populateSessionOptions();   // refresh the session descriptors in case the rtp directory has changed
      lastInvite = millis();    // we reset the timer that controls the periodicity of the invites
      if (sessionType == APPLEMIDI_ACTIVE) setStateActiveSession();
      else setStatePassiveSession();
      savedState = state;     // save current state as it may be recall after the refreshState
    }

    virtual bool handleConnectService() override{
      DEBUG_PRINT("\n%s::handleConnectService()",name.c_str());
      // we connect the midi client to its callback
      // this should not go to the begin
      
      MidiClient* appleMidiClient = hwm.network->appleMidiClient;      
      if (appleMidiClient == nullptr) {
        DEBUG_PRINT("\nError: AppleMIDI client is null");
        return false;
      }

      appleMidiClient->setHandleDisconnected(handleDisconnectedStatic);
      appleMidiClient->setHandleConnected(handleConnectedStatic);
      appleMidiClient->setHandleException(OnAppleMidiExceptionStatic);
      appleMidiClient->setHandleSystemExclusive(handleSysexStatic);
      appleMidiClient->setHandleControlChange(handleChangeControlStatic);

      appleMidiClient->begin();      

      startSession();
      
      return true;
    }

    virtual bool handleDisconnectService() override{      
      DEBUG_PRINT("\n%s::handleDisconnectService()",name.c_str());
      if (state.enabled == false) setStateDisabled();
      MidiClient* appleMidiClient = hwm.network->appleMidiClient;       
      if (appleMidiClient != nullptr) appleMidiClient->endSession();
      return true;
    }


  private:
    static inline AppleMIDIInterface* appleInstancePtr = nullptr;
    unsigned long lastInvite = 0;

};

class MIDIOSC/*: public OSCInterface*/{
  public:
    MIDIInterface* midiTransport = nullptr;

    MIDIOSC(String midiInterface, bool _configurable){
      instantiate(midiInterface,_configurable);

    };

    ~MIDIOSC(){
        //if (midiTransport && std::find(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), midiTransport) == interfaceRepo.interfaces.end())
        delete midiTransport; // => check double free in repo ?
    }

    void instantiate(String midiInterface, bool _configurable){
      if (midiInterface == APPLEMIDI_INTERFACE) midiTransport = new AppleMIDIInterface(_configurable, &rtpDir);
      else if (midiInterface == BLEMIDI_INTERFACE) midiTransport = new BLEMIDIInterface(_configurable);
      else midiTransport = new USBMIDIInterface(_configurable);      
    }

           
};

#endif
