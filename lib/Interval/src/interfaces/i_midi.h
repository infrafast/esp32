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
  //USBMIDI_CREATE_DEFAULT_INSTANCE();
  //USBMIDI_CREATE_INSTANCE(0,usbMIDI);
  USBMIDI_NAMESPACE::usbMidiTransport usbMIDI2(0);
  MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport> usbMIDI((USBMIDI_NAMESPACE::usbMidiTransport&)usbMIDI2);

#endif


class MIDIInterface{
  #define GROUP_INTERFACE_MIDI "MIDI"
  public:

    MidiData midiData;

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) = 0;
    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;
    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;

    virtual DeviceInterface* getDevice() = 0;

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
      getDevice()->dataAvailable = 1;
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
      getDevice()->dataAvailable = length;
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
      // set the interface data pointer to the midi data structure
      getDevice()->data = &midiData;
      
    }


  protected:
      static inline MIDIInterface* baseInstancePtr = nullptr;

};


#define BLE_CONNECTION_NAME INTERVAL_MANUFACTURER
BLEMIDI_CREATE_INSTANCE(BLE_CONNECTION_NAME, bleMIDI);

#define BLEMIDI_INTERFACE "BLEMIDI"
class BLEMIDIInterface: public MIDIInterface, public DeviceInterface{
  #define CONFIG_BLE "voidBLEparam" 
  public:
    String voidBLEparam = "abc";

    BLEMIDIInterface(bool _configurable):DeviceInterface(BLEMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_BLEMIDI;
      handleDisconnected();
    }

    String getFilename(){
      return BLEMIDI_INTERFACE"_if.cfg";
    }    

    virtual uint16_t getSize(String addrr){
      return 0;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      return &emptyString;
    }

    DeviceInterface* getDevice() override { return this; }

    virtual void monitor(){
      if (state.label == IF_STATE_CONNECTED) 
        bleMIDI.read(); // maybe remove the if as read() may calls the connect and disconnect callbacks ? (as appleMidi)
    }

    virtual bool derivedBegin(){    
      init();  
      bleInstancePtr = this;   // do not forget otherwise callback function will crash

      BLEbleMIDI.setHandleConnected(handleConnectedStatic);
      BLEbleMIDI.setHandleDisconnected(handleDisconnectedStatic);
      
      bleMIDI.setHandleSystemExclusive(handleSysexStatic);

      bleMIDI.begin();

      return true;
    }

    void handleConnected() {
      DEBUG_PRINT("\nBLEMidi connected\n");
      state.code = IF_READY;
      state.label = IF_STATE_CONNECTED;
      state.details = ""; //String(name);// + " (" + hwm.network->midiClient->getParticipantDetails(ssrc) + ")";
    }    

    void handleDisconnected() {
      DEBUG_PRINT("\nBLEMidi disconnected\n");
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.subLabel =  BLE_CONNECTION_NAME;
      state.details = "connectBT";  
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

    virtual bool end(){
      return false;
    }     


    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_BLE] = voidBLEparam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_BLE)){ 
        voidBLEparam = config[CONFIG_BLE].as<String>();
        changed = true;
      }
      return changed;
    }  

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
      DEBUG_PRINT("\nBLEMIDIInterface::sendSysex(%i) ",inLength);
      if (state.code == IF_READY) bleMIDI.sendSysEx(inLength,inArray,inArrayContainsBoundaries);
      else return false;
      return true; 
    }

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){return false;};
    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){return false;};

  private:
    static inline BLEMIDIInterface* bleInstancePtr = nullptr;    

};

#define USBMIDI_INTERFACE "USBMIDI"
class USBMIDIInterface: public MIDIInterface, public DeviceInterface{
  #define CONFIG_USB "voidUSBparam"
  public:
    String voidUSBparam = "toto";

    USBMIDIInterface(bool _configurable):DeviceInterface(USBMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_USBMIDI;      
    }

    String getFilename(){
      return USBMIDI_INTERFACE"_if.cfg";
    }

    DeviceInterface* getDevice() override { return this; }

    virtual void monitor(){
      if (useUSBMidi){  // only if USB is active
        if (hwm.battery.refreshed){
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
        }
        usbMIDI.read();
      }
    }

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_USB] = voidUSBparam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_USB)){
        voidUSBparam = config[CONFIG_USB].as<String>();
        changed = true;
      }
      return changed;
    }      

    virtual bool derivedBegin(){
      init();
      state.subLabel = "";
      usbMIDI.setHandleSystemExclusive(handleSysexStatic);
      // if the fooswtich has started in FW update or debug mode, USB MIDI is not active, hence we set the status once for all
      if (!useUSBMidi){
        state.code = IF_NOT_READY;
        state.label = IF_STATE_DISCONNECTED;
        state.details = "restartUSB";
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
class DINMIDIInterface: public MIDIInterface, public DeviceInterface{
  #define CONFIG_DIN "voidUSBparam"
  public:
    String voidDinparam = "din1";

    DINMIDIInterface(bool _configurable):DeviceInterface(DINMIDI_INTERFACE,GROUP_INTERFACE_MIDI,_configurable){
      icon = ICON_DINMIDI;
    }

    String getFilename(){
      return DINMIDI_INTERFACE"_if.cfg";
    }

    virtual void monitor(){}


    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_DIN] = voidDinparam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_DIN)){
        voidDinparam = config[CONFIG_DIN].as<String>();
        changed = true;
      }
      return changed;
    }      

    DeviceInterface* getDevice() override { return this; }

    virtual bool derivedBegin(){
      init();
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



#define APPLEMIDI_INTERFACE "AppleMIDI"
class AppleMIDIInterface: public MIDIInterface, public NetworkDevice{
  #define CONFIG_APPLE "voidAppleMidiParam"
  public:
    String voidAppleMidiParam = "titi";      

    AppleMIDIInterface(bool _configurable):NetworkDevice(APPLEMIDI_INTERFACE,GROUP_INTERFACE_MIDI, _configurable){
      icon = ICON_APPLEMIDI;
    }

    String getFilename(){
      return APPLEMIDI_INTERFACE"_if.cfg";
    }

    DeviceInterface* getDevice() override { return this; }

    virtual void derivedMonitor(){
      
      if (hwm.network->state == CONNECTED){
        // the read trigger callback like connection, hence it has to be called independantly from being connected or not, only network count
        if (hwm.network->midiClient != nullptr){
          hwm.network->midiClient->read();                        //we prefer  to put it here instead than in Network (who controls the object) as in case we are offline, monitor is not called and we save time
        }
                
        if (hwm.network->refreshed) setStatePendingSession();    // network has just connected so we now wait for a session to connect

      } 
      
    }

    // Méthode de l'instance réellement appelée
    void handleConnected(const ssrc_t& ssrc, const char* name) {
      DEBUG_PRINT("\nConnected to session: %s\n", name);
      state.code = IF_READY;
      state.label = IF_STATE_CONNECTED;
      state.details = String(name);// + " (" + hwm.network->midiClient->getParticipantDetails(ssrc) + ")";
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
      DEBUG_PRINT("\nDisconnected from session\n");
      setStatePendingSession();
    }

    virtual bool sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) override{                  
      if (state.code == IF_READY) hwm.network->midiClient->sendSysEx(inLength,inArray,inArrayContainsBoundaries);
      else return false;
      return true; 
    }
    

    virtual bool sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      if (state.code == IF_READY) hwm.network->midiClient->sendNoteOn(inNoteNumber,inVelocity, inChannel);
      else return false;
      return true;       
        
    };

    virtual bool sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){     
      if (state.code == IF_READY) hwm.network->midiClient->sendNoteOff(inNoteNumber,inVelocity, inChannel);
      else return false;
      return true;       

    };

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_APPLE] = voidAppleMidiParam;   // fill any parameters that are required by sysex
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_APPLE)){        
        voidAppleMidiParam = config[CONFIG_APPLE].as<String>();
        changed = true;
      }
      return changed;
    }        

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
          DEBUG_PRINT("\n***MIDI BufferFullException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::ParseException:
          DEBUG_PRINT("\n***MIDI ParseException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::TooManyParticipantsException:
          DEBUG_PRINT("\n***MIDI TooManyParticipantsException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::UnexpectedInviteException:
          DEBUG_PRINT("\n***MIDI UnexpectedInviteException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::ParticipantNotFoundException:
          DEBUG_PRINT("\n***MIDI ParticipantNotFoundException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::ComputerNotInDirectory:
          DEBUG_PRINT("\n***MIDI ComputerNotInDirectory\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::NotAcceptingAnyone:
          DEBUG_PRINT("\n***MIDI NotAcceptingAnyone\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::ListenerTimeOutException:
          DEBUG_PRINT("\n***MIDI ListenerTimeOutException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::MaxAttemptsException:
          DEBUG_PRINT("\n***MIDI MaxAttemptsException\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::NoResponseFromConnectionRequestException:
          DEBUG_PRINT("\n***MIDI:yyy did't respond to the connection request. Check the address and port, and any firewall or router settings. (time)\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::SendPacketsDropped:
          DEBUG_PRINT("\n***MIDI SendPacketsDropped\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::ReceivedPacketsDropped:
          DEBUG_PRINT("\n***MIDI***************************************** ReceivedPacketsDropped\n");
          break;
        case APPLEMIDI_NAMESPACE::Exception::UdpBeginPacketFailed:
          DEBUG_PRINT("\n***MIDI UdpBeginPacketFailed\n");
          break;
      }
    }    


    void setStatePendingSession(){
      // network is connected but not the session
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.subLabel =  hwm.network->midiClient->getName() +String(" (or ")+hwm.network->IP.toString() + String(", port: ")+hwm.network->midiClient->getPort()+ String(")");
      state.details = "ConnectMIDIsession";          
    }

    virtual bool derivedBegin(){      
      //DEBUG_PRINT("\tAppleMIDIInterface::derivedBegin()");
      init();
      appleInstancePtr = this;   // do not forget otherwise callback function will crash
      return disconnect();      
    }

    virtual bool end(){
      //DEBUG_PRINT("\n\tAppleMIDIInterface->end()...");
      return true;
    }

    virtual bool connect(){
      //DEBUG_PRINT("\tAppleMIDIInterface::connect()");
      MidiClient* midiClient = hwm.network->midiClient;      
      if (midiClient == nullptr){
        DEBUG_PRINT("\t(nullptr)\t");
        return false;
      }
      midiClient->setHandleDisconnected(handleDisconnectedStatic);
      midiClient->setHandleConnected(handleConnectedStatic);
      midiClient->setHandleException(OnAppleMidiExceptionStatic);
      midiClient->setHandleSystemExclusive(handleSysexStatic);
      midiClient->setHandleControlChange(handleChangeControlStatic);
      return true;
    }

    virtual bool disconnect(){
      return true;
    }


  private:
    static inline AppleMIDIInterface* appleInstancePtr = nullptr;


};


class MIDIOSC: public OSCInterface{
  #define CONFIG_SYSEX "voidSysexParam"  
  public:
    MIDIInterface* midiTransport = nullptr;
    String voidSysexParam = "xyz";

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