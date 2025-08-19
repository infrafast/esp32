#ifndef I_MIDI_h
#define I_MIDI_h

#include "Interfaces.h"
#include "i_osc.h"

class MIDIInterface: public DeviceInterface{
  #define GROUP_INTERFACE_MIDI "MIDI"
  public:
    virtual void sendSysex() = 0;

    virtual String getProviderName(){
      return GROUP_INTERFACE_MIDI;        // the provider is common to all MIDI strips
    }

    MIDIInterface(const String& name,bool _configurable):DeviceInterface(name,GROUP_INTERFACE_MIDI,_configurable){
    }

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel) = 0;


};

class MidiChannels : public StaticDataSet{
  #define ADDRESS_MIDICHANNEL   "/midi/channel/%d"
  #define MIDI_CHANNEL_NUMBER   16
  public:
    //String channelNames[MIDI_CHANNEL_NUMBER] = {"Piano", "Basse", "Guitare", "Cordes","Cuivres", "Chœur", "Lead", "FX","Pad", "Batterie", "Percu 2", "Synth","Harmonie", "Ambiance", "Drone", "Autre"};
    String channelNames[MIDI_CHANNEL_NUMBER] = {"channel1","channel2","channel3","channel4","channel5","channel6","channel7","channel8","channel9","channel10","channel11","channel12","channel13","channel14","channel15","channel16"};


    MidiChannels(const Address& adress, String name): StaticDataSet(adress, name,MIDI_CHANNEL_NUMBER){}
    virtual const String* getValuePtr(uint16_t index){
      if (index>MIDI_CHANNEL_NUMBER-1) return &errorDatasetOverflow;
      return &channelNames[index];    //local javascript function 
    }  
}DSmidiChannels(Address{ADDRESS_MIDICHANNEL,INPUTSELECT},String(GROUP_INTERFACE_MIDI));

class MidiValue : public StaticDataSet{
  #define ADDRESS_MIDIVALUE     "/midi/value/%s"
  public:
    String value="1234";
    MidiValue(const Address& adress, String name): StaticDataSet(adress, name,1){}
    virtual const String* getValuePtr(uint16_t index){
      return &value;     
    }  
}DSmidiValue(Address{ADDRESS_MIDIVALUE,INPUTUINT},String(GROUP_INTERFACE_MIDI));

class MidiMessage : public StaticDataSet{
  #define ADDRESS_MIDIMSG_SIZE 7
  #define ADDRESS_MIDIMESSAGE   "/midi/message/%d"
  public:
    
  enum MIDIMESSAGESID{
      MIDI_MSG_NOTEON,
      MIDI_MSG_NOTEOFF,
      MIDI_MSG_PA,
      MIDI_MSG_CC,
      MIDI_MSG_PC,
      MIDI_MSG_CA,      
    };


    String midiMessages[ADDRESS_MIDIMSG_SIZE] = {
      "Note On",
      "Note Off",
      "Polyphonic Aftertouch",
      "Control Change",
      "Program Change",
      "Channel Aftertouch",
      "Pitch Bend",
    };       

    MidiMessage(const Address& adress, String name): StaticDataSet(adress, name, ADDRESS_MIDIMSG_SIZE){}
    virtual const String* getValuePtr(uint16_t index){
      if (index>ADDRESS_MIDIMSG_SIZE-1) return &errorDatasetOverflow;
      return &midiMessages[index];    
    }  

}DSmidiMessages(Address{ADDRESS_MIDIMESSAGE,INPUTSELECT},String(GROUP_INTERFACE_MIDI));

class SysexValue : public StaticDataSet{
  #define ADDRESS_SYSEXVALUE     "/midi/sysex/%s"
  public:
    String value="sysexmsg";
    SysexValue(const Address& adress, String name): StaticDataSet(adress, name,1){}
    virtual const String* getValuePtr(uint16_t index){
      return &value;     
    }  
}DSmidiSysex(Address{ADDRESS_SYSEXVALUE,INPUTTEXT},String(GROUP_INTERFACE_MIDI));

#define BLEMIDI_INTERFACE "BLEMIDI"
class BLEMIDIInterface: public MIDIInterface{
  #define CONFIG_BLE "voidBLEparam" 
  public:
    String voidBLEparam = "abc";

    BLEMIDIInterface(bool _configurable):MIDIInterface(BLEMIDI_INTERFACE,_configurable){
      network = false;  // no network dependency
    }

    virtual uint16_t getSize(String addrr){
      return 0;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      return &emptyString;
    }

    virtual void monitor(){}

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    virtual void end(){}     


    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_BLE] = voidBLEparam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidBLEparam = config[CONFIG_BLE].as<String>();
    }  

    virtual void sendSysex(){}

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};



};

#define USBMIDI_INTERFACE "USBMIDI"
class USBMIDIInterface: public MIDIInterface{
  #define CONFIG_USB "voidUSBparam"
  public:
    String voidUSBparam = "toto";

    USBMIDIInterface(bool _configurable):MIDIInterface(USBMIDI_INTERFACE,_configurable){
      network = false; // no network dependency
    }

    virtual void monitor(){}

    virtual void sendSysex(){}

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_USB] = voidUSBparam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidUSBparam = config[CONFIG_USB].as<String>();
    }      

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    virtual void end(){}     

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};    

};


#define APPLEMIDI_INTERFACE "AppleMIDI"
class AppleMIDIInterface: public MIDIInterface,NetworkInterface{
  #define CONFIG_APPLE "voidAppleMidiParam"
  public:
    String voidAppleMidiParam = "titi";      

    AppleMIDIInterface(bool _configurable):MIDIInterface(APPLEMIDI_INTERFACE,_configurable),NetworkInterface(this){
    }

    virtual void monitor(){      
      computeNetworkState();
      
      if (hwm.network->state == CONNECTED){

        if (hwm.network->midiClient != nullptr){
          hwm.network->midiClient->read();          //we prefer  to put it here instead than in Network (who controls the object) as in case we are offline, monitor is not called and we save time
        }
                
        if (hwm.network->refreshed) setStatePendingSession();    

      }      

    }

    virtual void sendSysex(){}

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_APPLE] = voidAppleMidiParam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidAppleMidiParam = config[CONFIG_APPLE].as<String>();
    }        

    virtual void* read(){return nullptr;}

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){
        hwm.network->midiClient->sendNoteOn(inNoteNumber,inVelocity, inChannel);
    };

    static void handleConnectedStatic(const ssrc_t& ssrc, const char* name) {
        if (instancePtr) {
            instancePtr->handleConnected(ssrc, name);
        }
    }

    static void handleDisconnectedStatic(const ssrc_t& ssrc) {
        if (instancePtr) {
            instancePtr->handleDisconnected(ssrc);
        }
    }

    static void OnAppleMidiExceptionStatic(const ssrc_t& ssrc, const Exception& e, const int32_t value ){
        if (instancePtr) {
            instancePtr->OnAppleMidiException(ssrc,e,value);
        }      
    }

    void OnAppleMidiException(const ssrc_t& ssrc, const Exception& e, const int32_t value ) {
      switch (e)
      {
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
        state.code = IF_STATE_READY;
        state.label = "connected";
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
        DEBUG_PRINT("\nDisconnected from session\n");
        setStatePendingSession();
    }

    void setStatePendingSession(){
      state.code = IF_STATE_NOT_READY;      
      state.label = "Pending";
      state.subLabel =  hwm.network->midiClient->getName() +String(" (or ")+hwm.network->IP.toString() + String(", port: ")+hwm.network->midiClient->getPort()+ String(")");
      state.userInstructions = "Connect your MIDI gateway to to the following Apple Midi session";          
    }

    virtual void begin(){
      
      MidiClient* midiClient = hwm.network->midiClient;
      const char* midiname = midiClient->getName();
      DEBUG_PRINT("\n\tAppleMIDIInterface-%s->begin()...",midiname);
      if (midiClient != nullptr){
        midiClient->setHandleDisconnected(handleDisconnectedStatic);
        midiClient->setHandleConnected(handleConnectedStatic);
        midiClient->setHandleException(OnAppleMidiExceptionStatic);
        DEBUG_PRINT("ok");
      }else{
        DEBUG_PRINT("\tfailed(nullptr)");
      }
      DEBUG_PRINT("\toffline:%s",state.offline?"true":"false");
    }

    virtual void end(){

    }

    // À appeler dans `setup()` ou avant `begin()` pour enregistrer l'instance courante
    static void setInstance(AppleMIDIInterface* inst) {
        instancePtr = inst;
    }    

  private:
    static inline AppleMIDIInterface* instancePtr = nullptr;

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

    virtual void restore(const JsonObject& config){
      voidSysexParam = config[CONFIG_SYSEX].as<String>();
      /*JsonObject midiConfig = config[INTERFACE_USES]
      midiTransport->restore(midiConfig);*/
      midiTransport->restore(config);
    }            
};

#endif