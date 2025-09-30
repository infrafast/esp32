#ifndef I_MIDI_h
#define I_MIDI_h

#include "Interfaces.h"
#include "i_osc.h"

class MIDIInterface: public DeviceInterface{
  #define GROUP_INTERFACE_MIDI "MIDI"
  public:
    virtual String getProviderName(){
      return GROUP_INTERFACE_MIDI;        // the provider is common to all MIDI strips
    }

    MIDIInterface(const String& name,bool _configurable):DeviceInterface(name,GROUP_INTERFACE_MIDI,_configurable){}

    virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) = 0;
    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;
    virtual void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;
    virtual void handleSysex(byte* data, unsigned length) = 0;
    virtual void handleChangeControl(byte channel, byte number, byte value)  = 0;

    static void handleSysexStatic(byte* data, unsigned length) {
        if (baseInstancePtr) {
            baseInstancePtr->handleSysex(data,length);
        }
    }

  protected:
      static inline MIDIInterface* baseInstancePtr = nullptr;

};


#define BLEMIDI_INTERFACE "BLEMIDI"
class BLEMIDIInterface: public MIDIInterface{
  #define CONFIG_BLE "voidBLEparam" 
  public:
    String voidBLEparam = "abc";

    BLEMIDIInterface(bool _configurable):MIDIInterface(BLEMIDI_INTERFACE,_configurable){
      useNetwork = false;  // no network dependency
      icon = ICON_BLEMIDI;
    }

    virtual uint16_t getSize(String addrr){
      return 0;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      return &emptyString;
    }

    virtual void monitor(){}

    virtual void* read(){return nullptr;}

    virtual void begin(){
      baseInstancePtr = this; // do not forget otherwise callback function will crash      
    }

    virtual void end(){}     




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

    virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){}
    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};
    virtual void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};
    virtual void handleSysex(byte* data, unsigned length){}
    virtual void handleChangeControl(byte channel, byte number, byte value){}


};

#define USBMIDI_INTERFACE "USBMIDI"
class USBMIDIInterface: public MIDIInterface{
  #define CONFIG_USB "voidUSBparam"
  public:
    String voidUSBparam = "toto";

    USBMIDIInterface(bool _configurable):MIDIInterface(USBMIDI_INTERFACE,_configurable){
      icon = ICON_USBMIDI;      
    }

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
        MIDI.read();
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

    virtual void* read(){return nullptr;}

    virtual void begin(){
      state.subLabel = "";
      baseInstancePtr = this; // do not forget otherwise callback function will crash
      MIDI.setHandleSystemExclusive(handleSysexStatic);
      // if the fooswtich has started in FW update or debug mode, USB MIDI is not active, hence we set the status once for all
      if (!useUSBMidi){
        state.code = IF_NOT_READY;
        state.label = IF_STATE_DISCONNECTED;
        state.details = "restartUSB";
      }      
    }

    virtual void end(){}     

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){      
      MIDI.sendNoteOn(inNoteNumber,inVelocity,inChannel);
    }  

    virtual void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
      MIDI.sendNoteOff(inNoteNumber,inVelocity,inChannel);
    }

    virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){    
      MIDI.sendSysEx(inLength,inArray,inArrayContainsBoundaries);
    }    

    virtual void handleSysex(byte* data, unsigned length){}

    virtual void handleChangeControl(byte channel, byte number, byte value){}

};


#define DINMIDI_INTERFACE "DINMIDI"
class DINMIDIInterface: public MIDIInterface{
  #define CONFIG_DIN "voidUSBparam"
  public:
    String voidDinparam = "din1";

    DINMIDIInterface(bool _configurable):MIDIInterface(DINMIDI_INTERFACE,_configurable){
      useNetwork = false; // no network dependency
      icon = ICON_DINMIDI;
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

    virtual void* read(){return nullptr;}

    virtual void begin(){
      baseInstancePtr = this; // do not forget otherwise callback function will crash      
    }

    virtual void end(){}     

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};    
    virtual void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};
    virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){}
    virtual void handleSysex(byte* data, unsigned length){}
    virtual void handleChangeControl(byte channel, byte number, byte value){}    

};



#define APPLEMIDI_INTERFACE "AppleMIDI"
class AppleMIDIInterface: public MIDIInterface,NetworkInterface{
  #define CONFIG_APPLE "voidAppleMidiParam"
  public:
    String voidAppleMidiParam = "titi";      

    AppleMIDIInterface(bool _configurable):MIDIInterface(APPLEMIDI_INTERFACE,_configurable),NetworkInterface(this){
      icon = ICON_APPLEMIDI;
    }

    virtual void monitor(){      
      monitorNetwork();
      
      if (hwm.network->state == CONNECTED){

        if (hwm.network->midiClient != nullptr){
          hwm.network->midiClient->read();          //we prefer  to put it here instead than in Network (who controls the object) as in case we are offline, monitor is not called and we save time
        }
                
        if (hwm.network->refreshed) setStatePendingSession();    // network has just connected so we now wait for a session to connect

      }      

    }

    void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
      DEBUG_PRINT("\nsendSysEx: %i bytes: ",inLength);
      for (uint16_t i = 0; i < inLength; i++) DEBUG_PRINT("%02X ",inArray[i]);
      hwm.network->midiClient->sendSysEx(inLength,inArray,inArrayContainsBoundaries);
    }
    
    char getSysExStatus(const byte* data, uint16_t length)
    {
      if (data[0] == 0xF0 && data[length - 1] == 0xF7)
        return 'F'; // Full SysEx Command
      else if (data[0] == 0xF0 && data[length - 1] != 0xF7)
        return 'S'; // Start of SysEx-Segment
      else if (data[0] != 0xF0 && data[length - 1] != 0xF7)
        return 'M'; // Middle of SysEx-Segment
      else
        return 'E'; // End of SysEx-Segment
    }

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
        hwm.network->midiClient->sendNoteOn(inNoteNumber,inVelocity, inChannel);
    };

    virtual void sendNoteOff(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
        hwm.network->midiClient->sendNoteOff(inNoteNumber,inVelocity, inChannel);
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

    virtual void* read(){return nullptr;}


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

    static void handleChangeControlStatic(byte channel, byte number, byte value){
        if (appleInstancePtr) {
            appleInstancePtr->handleChangeControl(channel,number,value);
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

    // Méthode de l'instance réellement appelée
    void handleConnected(const ssrc_t& ssrc, const char* name) {
      DEBUG_PRINT("\nConnected to session: %s\n", name);
      state.code = IF_READY;
      state.label = IF_STATE_CONNECTED;
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
      DEBUG_PRINT("\nDisconnected from session\n");
      setStatePendingSession();
    }

    virtual void handleChangeControl(byte channel, byte number, byte value){
        DEBUG_PRINT("\nhandleChangeControl channel:%i, number:%i value:%i",channel,number,value);
    }

    void handleSysex(byte* data, unsigned length){
      DEBUG_PRINT("\nhandleSysex: %c, %i bytes: ",getSysExStatus(data, length),length);
      for (uint16_t i = 0; i < length; i++) DEBUG_PRINT("%02X ",data[i]);
    }


    void setStatePendingSession(){
      state.code = IF_NOT_READY;      
      state.label = IF_STATE_PENDING;
      state.subLabel =  hwm.network->midiClient->getName() +String(" (or ")+hwm.network->IP.toString() + String(", port: ")+hwm.network->midiClient->getPort()+ String(")");
      state.details = "ConnectMIDIsession";          
    }

    virtual void begin(){
      
      appleInstancePtr = this;   // do not forget otherwise callback function will crash
      baseInstancePtr = this; // do not forget otherwise callback function will crash

      MidiClient* midiClient = hwm.network->midiClient;
      const char* midiname = midiClient->getName();
      DEBUG_PRINT("\n\tAppleMIDIInterface-%s->begin()...",midiname);
      if (midiClient != nullptr){
        midiClient->setHandleDisconnected(handleDisconnectedStatic);
        midiClient->setHandleConnected(handleConnectedStatic);
        midiClient->setHandleException(OnAppleMidiExceptionStatic);
        midiClient->setHandleSystemExclusive(handleSysexStatic);
        midiClient->setHandleControlChange(handleChangeControlStatic);
        DEBUG_PRINT("ok");
      }else{
        DEBUG_PRINT("\tfailed(nullptr)");
      }
      DEBUG_PRINT("\toffline:%s",state.offline?"true":"false");
    }

    virtual void end(){
      DEBUG_PRINT("\n\tAppleMIDIInterface->stop()...");
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

    MIDIOSC(JsonObject& config){
      //JsonObject uses = config[INTERFACE_USES].to<JsonObject>();
      String midiInterfaceName ;//= uses.begin()->key().c_str();
      serializeJson(config, midiInterfaceName);
      //DEBUG_PRINT("\nZOGZOG: %s",midiInterfaceName.c_str());
      bool _configurable = config[INTERFACE_EXPOSED];
      instantiate(midiInterfaceName,_configurable);
      restore(config);
    }

    ~MIDIOSC(){
      delete midiTransport;
    }

    void instantiate(String midiInterface, bool _configurable){
      interfaceName = "MIDI-"+midiInterface;
      if (midiInterface == APPLEMIDI_INTERFACE) midiTransport = new AppleMIDIInterface(_configurable);
      else if (midiInterface == BLEMIDI_INTERFACE) midiTransport = new BLEMIDIInterface(_configurable);
      else midiTransport = new USBMIDIInterface(_configurable);      
    }

    virtual void derivedConfig(JsonObject& config){   
      //{"sysex":""}}
      config[CONFIG_SYSEX] = voidSysexParam;   // fill any parameters that are required by sysex

      // we retrieve the config of the OSCInterface instance which is specific
      String usesConf = midiTransport->serializeConfig(false);  // serializing a subnode
      JsonDocument midiConf;

      // convert it back to an object that can be attached to the parent config 
      DeserializationError error = deserializeJson(midiConf, usesConf);
      if (error) {
        DEBUG_PRINT("\n%s serialize %s",interfaceName.c_str(),error.c_str());
        return;
      }
      midiConf.shrinkToFit();
      // "uses":{"USBMIDIInterface":{"voidUSBparam":""}}
      // "uses":{"BLEMIDI":{"config":{"voidBLEparam":""}}
      // "uses":{"AppleMIDI":{"config":{"voidAppleMidiParam":""}}

      // partial serialization means we embbed the "uses" into the core config when it comes to unexposed interface
      // in the first case we create a key "uses", in the second we just copy the elements ne by one
      //if (exposed) config[INTERFACE_USES] = midiConf;
      //else
      {
        // copie manuelle
        for (JsonPair kv : midiConf.as<JsonObject>()) config[kv.key()] = kv.value();
      }
    } 

    virtual bool restore(const JsonObject& config){
      bool changed = false;
      if (config.containsKey(CONFIG_SYSEX)){        
        voidSysexParam = config[CONFIG_SYSEX].as<String>();
        changed = true;
      }
      /*JsonObject midiConfig = config[INTERFACE_USES]
      midiTransport->restore(midiConfig);*/
      changed |= midiTransport->restore(config);
      return changed;
    }            
};

#endif