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
      handleDisconnected();
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

    virtual void goOffline(bool _offline){
      // THE BLEMIDI GO OFFLINE MUST NOT TOUCH THE STATE; IT JUST CONTINUES NORMALLY OTHERWISE IT IS NOT POSSIBLE TO RE-ESTABLISH THE CONNEXION AFTER GOING BACK ONLINE
      // THIS HAS SOMETHING TO DEAL WITH HANDLING OF END()
      /*if (_offline){
        end();
      }else{
        bleMIDI.begin();
      }*/
      DeviceInterface::goOffline(_offline);            
      if (!_offline) begin();

    }

    virtual bool begin(){    
      MIDIInterface::init();  
      bleInstancePtr = this;   // do not forget otherwise callback function will crash

      BLEbleMIDI.setHandleConnected(handleConnectedStatic);
      BLEbleMIDI.setHandleDisconnected(handleDisconnectedStatic);
      
      bleMIDI.setHandleSystemExclusive(handleSysexStatic);

      if (alreadyBegun){
        DEBUG_PRINT("\n%s begin() ignored (alreadyBegun)",interfaceName.c_str());   // this is a workaround becaue of offline may recall it 
      }else if (!state.offline){
        DEBUG_PRINT("\n%s begin()",interfaceName.c_str());
        bleMIDI.begin();
        alreadyBegun = true;
      }else{
        DEBUG_PRINT("\n%s begin() ignored (offline))",interfaceName.c_str());
      }

      return true;
    }

    virtual bool end(){
        DEBUG_PRINT("\nbleMIDI disconnected");
        auto transport = bleMIDI.getTransport();  // ✅ On récupère le transport BLE
        if (transport) {
            transport->end();
            // bleMIDI.end() doesnt exists
            return true;
        }
      return false;    
    }     

    void handleConnected() {
      DEBUG_PRINT("\nBLEMidi paired\n");
      state.code = IF_READY;
      state.label = IF_STATE_CONNECTED;
      state.subState = IF_STATE_CONNECTED;
      state.details = ""; //String(name);// + concatStrings(" (", hwm.network->appleMidiClient->getParticipantDetails(ssrc), ")");
      state.led = "blue";
      state.progress = -1;
    }    

    void handleDisconnected() {
      DEBUG_PRINT("\nBLEMidi disconnected\n");
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PAIRING;
      state.subState = IF_STATE_PAIRING;
      state.details = concatStrings(*lang.getString("Connect Blutooth"), " ", BLE_CONNECTION_NAME);
      state.led = "blue medium";
      state.progress = -1;      
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
    bool alreadyBegun = false;

};

#define USBMIDI_INTERFACE "USBMIDI"

class USBMIDIInterface: public MIDIInterface{

  public:

    USBMIDIInterface(bool _configurable):MIDIInterface(USBMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_USBMIDI;      
    }

    virtual void monitor(){
      if (useUSBMidi){  // only if USB is active
        /*if (hwm.battery.refreshed){
          // a better way would be to use callback function from USB.onEvent(...)
          if (hwm.battery.batteryState != BAT_USB_PLUGGED){
            state.code = IF_NOT_READY;
            state.label = IF_STATE_DISCONNECTED;
            state.details = "Plugin USB-MIDI";
          }else{
            state.code = IF_READY;
            state.label = IF_STATE_CONNECTED;
            state.details = "";
          }
        }*/

        if (tud_mounted() && state.code == IF_NOT_READY) {
          state.code = IF_READY;
          state.label = IF_STATE_CONNECTED;
          state.subState = IF_STATE_CONNECTED;
          state.details = "";
          state.led = "green";
          state.progress = -1;          
        }

        if (!tud_mounted() && state.code == IF_READY) {
          state.code = IF_NOT_READY;
          state.label = IF_STATE_DISCONNECTED;
          state.subState = IF_STATE_DISCONNECTED;
          state.details = "Plugin USB-MIDI";
          state.led = "red";
          state.progress = -1;
        }


        usbMIDI.read();
      }
    }

    virtual bool begin(){
      MIDIInterface::init();
      usbMIDI.setHandleSystemExclusive(handleSysexStatic);
      // if the fooswtich has started in FW update or debug mode, USB MIDI is not active, hence we set the status once for all
      if (!useUSBMidi){
        state.code = IF_NOT_READY;
        state.label = IF_STATE_DISABLED;
        state.subState = IF_STATE_DISABLED;
        state.details = "restartUSB";
        state.led = "black";
        state.progress = -1;
      }
      return true;
    }

    virtual bool end(){
      return false;
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

};


#define DINMIDI_INTERFACE "DINMIDI"
class DINMIDIInterface: public MIDIInterface{
  #define CONFIG_DIN "voidUSBparam"
  public:
    DINMIDIInterface(bool _configurable):MIDIInterface(DINMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_DINMIDI;
    }

    virtual void monitor(){}

    virtual bool begin(){
      MIDIInterface::init();
      return true;
    }

    virtual bool end(){return false;}     

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){return false;};    
    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){return false;};

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
      DEBUG_PRINT("\nDINMIDI::sendSysex(%i) ",inLength);
      return false;
    }
 

};

enum{
  APPLEMIDI_PASSIVE=1,
  APPLEMIDI_ACTIVE
};

#define APPLEMIDI_INTERFACE "AppleMIDI"
#define ADDRESS_APPLEMIDISESSION   "/session/%d"
class AppleMidiSessionType : public DataSet{
  #define APPLEMIDI_SESSIONS_NUMBER   2
  public:
    String sessionsNames[APPLEMIDI_SESSIONS_NUMBER] = {"passive","active"};

    AppleMidiSessionType(Address* _address): DataSet(_address, APPLEMIDI_INTERFACE, &IFconfigRepo){}
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &sessionsNames[index];    //local javascript function 
    }  

    virtual uint16_t getSize(){
      return APPLEMIDI_SESSIONS_NUMBER;
    }

}dsAppleMidiSessions(new DropDownAddress(ADDRESS_APPLEMIDISESSION));



class AppleMIDIInterface: public MIDIInterface, public NetWorkInterface {
  #define CONFIG_APPLE "voidAppleMidiParam"
  #define ADDRESS_RTPPORT "/rtpPort/%d"

  public: 

    FreeTextAddress* addressAppleMidiPort = new FreeTextAddress(ADDRESS_RTPPORT,"rtpPort", REGEX_0to65535);
    String midiSessionName ="undef";
    NetworkInterfaceMonitor netHelper = NetworkInterfaceMonitor(this,this);

    int rtpPort = 5004;;
    int sessionType = APPLEMIDI_PASSIVE; // 0:passive, 1:active
    
    String exception = "";

    AppleMIDIInterface(bool _configurable):MIDIInterface(APPLEMIDI_INTERFACE,GROUP_INTERFACE_MIDI, _configurable){
      icon = ICON_APPLEMIDI;

      registerDataSetAddress(addressAppleMidiPort);
      registerDataSetAddress(dsAppleMidiSessions.address);
      registerDataSetAddress(&addressIP);           // global     
      
      destination = new PingEcho("0.0.0.0");

      jsonInputRepo.add(this); 

    }

    ~AppleMIDIInterface(){
      delete destination;
    }    

    virtual void serializeProperties(JsonArray& properties) override{
      char* buf = (char*)malloc((strlen(ADDRESS_RTPPORT)+6)*sizeof(char*)+1);
      sprintf(buf,ADDRESS_RTPPORT,rtpPort);
      properties.add(String(buf)); 
      free(buf);

      buf = (char*)malloc((strlen(ADDRESS_IP)+destination->ip.toString().length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_IP,destination->ip.toString());
      properties.add(String(buf)); 
      free(buf);         
    
      buf = (char*)malloc((strlen(ADDRESS_APPLEMIDISESSION)+6)*sizeof(char*)+1);
      sprintf(buf,ADDRESS_APPLEMIDISESSION,sessionType);
      properties.add(String(buf)); 
      free(buf);

    }

    virtual void handlePropertyChange() override{
      DEBUG_PRINT("\n%s::handlePropertyChange()",interfaceName.c_str());
      disconnectDeviceFromNetwork();
      connectDeviceToNetwork();  // is called by handleDisconnect()
    }

    virtual bool restoreProperty(const char* property) override{
      bool changed = false;
      uint16_t intVal;
      #define MAXLENGHTINPUT 32
      char inputStr[MAXLENGHTINPUT];      
      if (sscanf(property, ADDRESS_RTPPORT, &intVal) == 1) {
        if (intVal != rtpPort)   changed = true;
        rtpPort = intVal;       
      }else if (sscanf(property, ADDRESS_IP, &inputStr)) {
        IPAddress ip;
        if (ip.fromString(inputStr)){
          if (destination->ip != ip)  changed = true;
          destination->ip = ip;      
          destination->host = ip.toString();
        }else{
          DEBUG_PRINT("\ninvalid IP address:%s",inputStr);
        }
      }else if (sscanf(property, ADDRESS_APPLEMIDISESSION, &intVal) == 1) {
        if (intVal != sessionType)   changed = true;
        sessionType = intVal;       
      }
      return changed;
    }      

    virtual void monitor(){     
      netHelper.monitor();
    }

    #define MIDI_INVITE_PERIOD 3000
    virtual void connectedNetworkMonitor(){
      MidiClient* appleMidiClient = hwm.network->appleMidiClient;  
      
      if (appleMidiClient != nullptr){
        // the read trigger callback like connection, hence it has to be called independantly from being connected to a session or not
        //we prefer  to put it here instead than in Network (who controls the object) as in case we are offline, monitor is not called and we save time        
        hwm.network->appleMidiClient->read();                       

        if (state.code != IF_READY) {
          if (millis() - lastInvite >= MIDI_INVITE_PERIOD) {
            lastInvite = millis();
            startSession();
          }
        }

      }            
    }
    
    // Méthode de l'instance réellement appelée
    void handleConnected(const ssrc_t& ssrc, const char* name) {
      midiSessionName = String(name);
      setStateConnected();
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
      DEBUG_PRINT("\n\nDisconnected from session\n");     
      setStateDisconnected();
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

    #define UNDEF_EXCEPTION "UndefinedException"
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

    virtual void goOffline(bool _offline){
      MIDIInterface::goOffline(_offline);      
      if (_offline) disconnectDeviceFromNetwork();     // force disconnect in all case: either you will reconnect 
      else connectDeviceToNetwork();
      // maybe add a if (_offline) {} else {}
    }


    void setStatePassiveSession(){
      DEBUG_PRINT("\nAppleMIDIInterface::passive, waiting for invitation");      
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

    void setStateActiveSession(){
      //"details": "Invitation sent to 10.0.0.5/5004 pending reply",      

      MidiClient* appleMidiClient = hwm.network->appleMidiClient;

      appleMidiClient->sendInvite(destination->ip, rtpPort);
      DEBUG_PRINT("\nsetStateActiveSession::sendInvite() to %s:%d",destination->host.c_str(), rtpPort);  

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

    void setStateConnected(){
      DEBUG_PRINT("\n\nConnected to session: %s\n", midiSessionName.c_str());
      state.code = IF_READY;
      state.label = IF_STATE_CONNECTED;
      state.subState = IF_STATE_CONNECTED;
      state.details = String(name);// + concatStrings(" (", hwm.network->appleMidiClient->getParticipantDetails(ssrc), ")");
      state.progress = -1;
      state.led = "green";     
    }

    void setStateDisconnected(){
      state.code = IF_NOT_READY;
      state.progress = -1;
      state.led = "red fast";     
      if (exception != ""){
        state.label = IF_STATE_ERROR;
        state.details = *lang.getString(exception);
        exception = ""; //acknowledged
      }else{
        state.label = IF_STATE_DISCONNECTED;
        state.details = *lang.getString("session disconnected");
      }
    }    

    virtual bool begin(){      
      //DEBUG_PRINT("\tAppleMIDIInterface::derivedBegin()");
      MIDIInterface::init();
      appleInstancePtr = this;   // do not forget otherwise callback function will crash
      return disconnectDeviceFromNetwork();      
    }

    virtual bool end(){
      //DEBUG_PRINT("\n\tAppleMIDIInterface->end()...");
      return true;
    }

    void startSession(){
      if (sessionType == APPLEMIDI_ACTIVE) setStateActiveSession();
      else setStatePassiveSession();
    }

    virtual bool connectDeviceToNetwork(){
      DEBUG_PRINT("\n%s::connectDeviceToNetwork()",name.c_str());
      // we connect the midi client to its callback
      // this should not go to the begin
      
      MidiClient* appleMidiClient = hwm.network->appleMidiClient;      
      
      if (appleMidiClient == nullptr){
        DEBUG_PRINT("\nappleMidiClient (nullptr)");
        return false;
      }

      appleMidiClient->setHandleDisconnected(handleDisconnectedStatic);
      appleMidiClient->setHandleConnected(handleConnectedStatic);
      appleMidiClient->setHandleException(OnAppleMidiExceptionStatic);
      appleMidiClient->setHandleSystemExclusive(handleSysexStatic);
      appleMidiClient->setHandleControlChange(handleChangeControlStatic);

      startSession();

      return true;
    }

    virtual bool disconnectDeviceFromNetwork() {      
      DEBUG_PRINT("\nAppleMIDIInterface::disconnectDeviceFromNetwork()");     
      MidiClient* appleMidiClient = hwm.network->appleMidiClient; 
      if (appleMidiClient == nullptr){
        DEBUG_PRINT("\t(nullptr)\t");
        return false;
      }
      appleMidiClient->endSession();
      setStateDisconnected();
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
      if (midiInterface == APPLEMIDI_INTERFACE) midiTransport = new AppleMIDIInterface(_configurable);
      else if (midiInterface == BLEMIDI_INTERFACE) midiTransport = new BLEMIDIInterface(_configurable);
      else midiTransport = new USBMIDIInterface(_configurable);      
    }

           
};

#endif
