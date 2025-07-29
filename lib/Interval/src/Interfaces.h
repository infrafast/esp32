#ifndef DEVICEINTERFACES_h
#define DEVICEINTERFACES_h

#include <vector>
#include "Interval.h"

#include <AppleMIDI.h>

class DeviceInterface {
  #define INTERFACE_STATE       "state"
  #define INTERFACE_PROGRESS    "progress"
  #define INTERFACE_EXPOSED     "exposed"
  #define INTERFACE_CONFIGZ     "config"
  #define INTERFACE_OFFLINE     "offline"
  #define INTERFACE_GROUP       "group"
  #define INTERFACE_USENETWORK  "network"  
  #define INTERFACE_PROVIDER    "provider"
  
  public:

  String interfaceName; 
  String interfaceState = "disconnected";
  uint8_t progress = 0;
  bool exposed = true;
  bool offline = false;
  bool network = true;  // by default an interface uses the network
  String group = "undef"; // used by the strip to define which deviceinterface they can connect

  virtual String getProviderName() = 0;

  DeviceInterface(const String&, const String&, bool);
  ~DeviceInterface();

  String serializeState(){
    JsonDocument doc;
    
    doc[interfaceName][INTERFACE_STATE] = interfaceState;
    doc[interfaceName][INTERFACE_PROGRESS] = progress;

    doc.shrinkToFit();
    String output;
    serializeJson(doc, output);    
    //{"MixerOffline":{"state":"disconnected","progress":0}}
    //DEBUG_PRINT("\n<%s> serializeState: %s",interfaceName.c_str(),output.c_str());          
    return output;
  }  

  String serializeConfig(bool mainNode = true){
    // by default we serialize the full content (main node)
    // but if an interface uses another one, it will only partially serialize 
    // means we don't include somes attributes
    JsonDocument doc;
    //JsonObject config = doc[interfaceName].createNestedObject(/*INTERFACE_CONFIG*/);
    JsonObject config;

    if (exposed) 
      config = doc.createNestedObject(interfaceName);
    else config = doc.to<JsonObject>();

    if (mainNode == true){
      // that's the attribute we do not serialize in case of partial serialization
      config[INTERFACE_EXPOSED] = exposed;
      config[INTERFACE_USENETWORK] = network;
      config[INTERFACE_OFFLINE] = offline;
      config[INTERFACE_GROUP] = group;
      config[INTERFACE_PROVIDER] = getProviderName();      
    }
    derivedConfig(config);

    doc.shrinkToFit();
    String output;
    serializeJson(doc, output);    
    //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
    //DEBUG_PRINT("\nserializeConfig: %s",output.c_str());          
    return output;
  }  

  bool restoreFromString(const char* config){    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config);
    if (error) {
      DEBUG_PRINT("\n%s restoreFromString %s",interfaceName.c_str(),error.c_str());
      return false;
    }
    JsonObject configJson = doc.as<JsonObject>();

    //by default a used interface (which means it had a partial serialization) 
    // is not exposed and is online, this can be overwritten by the deserialization 
    // of a full interface
    //offline = false;
    //exposed = false;

    restore(configJson);    

    doc.shrinkToFit();

    #ifdef DEBUG
      String output;
      serializeJson(doc, output);    
      //{"MixerOffline":{"config":{FILLED BY DERIVED CLASS}}}
      DEBUG_PRINT("\nrestoreFromString: %s",output.c_str());          
    #endif
    return true;      
  }

  virtual void serialize(JsonDocument& interfaceDoc){
    //start with the config
    deserializeJson(interfaceDoc, serializeConfig());      
    JsonDocument interfaceState;
    // use another document to grab the state
    deserializeJson(interfaceState, serializeState());
    // fuzion the two, we have to do this on object as otherwise, key content would be overwritteb
    // on utilise le nom de l'interface (qui est la clef commune aux deux documents)
    //const char* interfaceName = interfaceDoc.as<JsonObject>().begin()->key().c_str();
    JsonObject state = interfaceState[interfaceName];
    JsonObject fullDoc = interfaceDoc[interfaceName];
    for (JsonPair kv : state) {
      fullDoc[kv.key()] = kv.value();  // Ajoute ou écrase les clés dans name1
    }

    // in case the "top level" interface is not exposed we have to put back the interface name as the key
    // as it was filtered in the serializeConfig
    if (!exposed){

      // Sauvegarder l'ancien contenu dans un objet temporaire
      JsonObject oldContent = interfaceDoc.as<JsonObject>();
      JsonDocument temp;
      temp.set(oldContent);

      // Vider le document original
      interfaceDoc.clear();

      // Créer une nouvelle clé contenant l'ancien contenu
      interfaceDoc[interfaceName].set(temp);

    }

  }

  bool update(){
    // reset the flag so strips will not be updated and re-processed already consummed data
    dataAvailable = false;
    // in case interface is offline, then easy: we do not waste time
    if (offline) return false;
    return monitor();
  }

  bool available(){ // called by strip 
    return dataAvailable; 
  }

  virtual void restore(const JsonObject& config) = 0;  
  virtual void derivedConfig(JsonObject& config) = 0;  
 
  virtual bool monitor() = 0; // => set the dataAvailable flag using internal functions to indicate data are available to be read with read() method
  virtual void* read() = 0; // called by strip 
  virtual void begin() = 0;

  protected:

  bool dataAvailable;

};

class InterfaceRepository{

  public:
    std::vector<DeviceInterface*> interfaces;

    void serialize(JsonArray* interfacesArray){
      for (DeviceInterface* interface : interfaces) {
        // serializing is used to save the interfaces config in the config file or to send them to the web app to be modified
        // hence, we only do that for interface which are "persistant"
        //if (interface->exposed == true){
          JsonDocument interfaceDoc;
          interface->serialize(interfaceDoc);
          interfaceDoc.shrinkToFit();
          interfacesArray->add(interfaceDoc);
        //}
      }     
    }  

    DeviceInterface* retrieve(String _interfaceName);
};

InterfaceRepository interfaceRepo;


DeviceInterface::DeviceInterface(const String& name, const String& _group,bool _configurable = true):exposed(_configurable){
  interfaceRepo.interfaces.push_back(this);
  interfaceName = name;
  group = _group;
}

DeviceInterface::~DeviceInterface(){
  interfaceRepo.interfaces.erase(std::remove(interfaceRepo.interfaces.begin(), interfaceRepo.interfaces.end(), this), interfaceRepo.interfaces.end());
}

#define MACRO_NUMBER_MAX      10
#define ADDRESS_SIMPLETEXT    "/text/%s"
#define ADDRESS_STRIPIDS      "/strip/id/%s"
#define ADDRESS_ACTION        "/action/%s"
  #define ADDRESS_ACTION_SIZE 5
#define ADDRESS_MACRONAME     "/macro/name/%s"
#define ADDRESS_PROVIDER      "/provider/"

#define SYSTEM_INTERFACE "system"
class SystemInterface: public DeviceInterface, public DataProvider{
  #define GROUP_INTERFACE_SYSTEM "utils"
  public:    

    DynamicDataSet simpleText = DynamicDataSet({ ADDRESS_SIMPLETEXT, INPUTTEXT },this);
    DynamicDataSet stripIDs = DynamicDataSet({ADDRESS_STRIPIDS,INPUTSELECT},this);
    DynamicDataSet dropDownActions = DynamicDataSet({ADDRESS_ACTION,INPUTSELECT},this);        
    DynamicDataSet macroNames = DynamicDataSet({ADDRESS_MACRONAME,INPUTSELECT},this); 


    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_SYSTEM,_configurable){}

    //SystemInterface uses himself as data provider
    virtual String getProviderName(){
      return interfaceName;
    }

    virtual String getDataSetName(){
      return interfaceName;
    }

    virtual void restore(const JsonObject& config){};  
    virtual void derivedConfig(JsonObject& config){};    
    virtual bool monitor(){return false;}
    virtual void* read(){return nullptr;}
    virtual void begin(){}

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_SIMPLETEXT) return 1;
      
      else if (addrr == ADDRESS_STRIPIDS) return 0;
      else if (addrr == ADDRESS_ACTION) return ADDRESS_ACTION_SIZE;
      else if (addrr == ADDRESS_MACRONAME) return MACRO_NUMBER_MAX;
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

      DEBUG_PRINT("\nDataSet getSize(%s) is not implemented",addrr.c_str());
      return 0;

    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_SIMPLETEXT){
        textValue = "hello"+String(index);
        return &textValue;
      }
      else if (addrr == ADDRESS_STRIPIDS){
        return &getStripID;         //javascript local function callback
      }else if (addrr == ADDRESS_ACTION){
        return &dropDownActionsValues[index];   
      }else if (addrr == ADDRESS_MACRONAME){
        return &macroNamesValues[index];      
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
      
      DEBUG_PRINT("\nDataSet getValuePtr(%s,%d) is not implemented",addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";
    
    // javascript function that generates the data locally
    String getStripID = "getStripID";
        
    String dropDownActionsValues[ADDRESS_ACTION_SIZE] = {
      "play",
      "pause",
      "stop",
      "FF",
      "RW"      
    };   
    
    String macroNamesValues[MACRO_NUMBER_MAX] = {
      "macro1",
      "macro2",
      "macro3",      
      "macro4",
      "macro5",
      "macro6",
      "macro7",
      "macro8",
      "macro9",
      "macro10"
    };            
  
}systemInterface(false);

DeviceInterface* InterfaceRepository::retrieve(String requestedInterface){
  for (const auto& interface : interfaceRepo.interfaces){ 
    //DEBUG_PRINT("\nInterfaceRepository::retrieve(%s)=>%s",requestedInterface.c_str(),interface->interfaceName.c_str())     ;
    if (interface->interfaceName == requestedInterface) return interface;
  }
  DEBUG_PRINT("\nError: requested interface <%s> is not existing, returning <%s>",requestedInterface.c_str(),systemInterface.interfaceName.c_str() );
  return &systemInterface;
}


class OSCInterface: public DeviceInterface{
  #define GROUP_INTERFACE_OSC "OSC"
  public:
    OSCInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_OSC,_configurable){}

    virtual String getProviderName(){
      return GROUP_INTERFACE_OSC;        // the provider is common to all MIDI strips
    }    
};

#define NETWORKOSC_INTERFACE "Network(OSC)"
class NetworkOSC: public OSCInterface{
  #define CONFIG_OSCIP  "oscIP"
  #define CONFIG_OSCPORT  "oscPort"
  public:
    String OSC_IPaddress;
    int OSC_port;

    NetworkOSC(String _OSC_IPaddress,int _OSC_port,bool _configurable=false):OSC_IPaddress(_OSC_IPaddress),OSC_port(_OSC_port), OSCInterface(_configurable){
      interfaceName = NETWORKOSC_INTERFACE;      
    }

    virtual void derivedConfig(JsonObject& config){   
      //{"oscIP":"5.6.7.8","oscPort":3000}
      config[CONFIG_OSCIP] = OSC_IPaddress;
      config[CONFIG_OSCPORT] = OSC_port;         
    }

    virtual void restore(const JsonObject& config){
      OSC_IPaddress = config[CONFIG_OSCIP].as<String>();
      OSC_port = config[CONFIG_OSCPORT];        
    }        

    virtual bool monitor(){return false;}
    virtual void* read(){return nullptr;}

    virtual void begin(){}


};


class DMXInterface: public DeviceInterface{
  #define GROUP_INTERFACE_DMX "DMX"
  public:
  
    DMXInterface(const String& name, bool _configurable):DeviceInterface(name,GROUP_INTERFACE_DMX,_configurable){}

    virtual String getProviderName(){
      return GROUP_INTERFACE_DMX;        // the provider is common to all MIDI strips
    }    

};

#define ARTNETDMX_INTERFACE "Artnet"
class ArtnetDMXInterface: public DMXInterface{
  #define CONFIG_ARTNET "voidArtnetparam" 
  public:
    String voidArtnetParam = "artnet123";

    ArtnetDMXInterface(bool _configurable):DMXInterface(ARTNETDMX_INTERFACE,_configurable){}

    virtual bool monitor(){return false;}    

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_ARTNET] = voidArtnetParam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidArtnetParam = config[CONFIG_ARTNET].as<String>();
    }  

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    
};


#define THREEEPINDMX_INTERFACE "3PinDMX"
class ThreePinDMXInterface: public DMXInterface{
  #define CONFIG_3PINDMX "void3PinDMXparam" 
  public:
    String void3pinDMXParam = "3pin123";

    ThreePinDMXInterface(bool _configurable):DMXInterface(THREEEPINDMX_INTERFACE,_configurable){
      network = false;  // no network dependency
    }

    virtual bool monitor(){return false;} 

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_3PINDMX] = void3pinDMXParam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      void3pinDMXParam = config[CONFIG_3PINDMX].as<String>();
    }  

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    
};

using namespace APPLEMIDI_NAMESPACE;

void OnAppleMidiException(const ssrc_t& ssrc, const Exception& e, const int32_t value ) {
  switch (e)
  {
    case APPLEMIDI_NAMESPACE::Exception::BufferFullException:
      DEBUG_PRINT("\n***MIDI BufferFullException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::ParseException:
      DEBUG_PRINT("\n***MIDI ParseException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::TooManyParticipantsException:
      DEBUG_PRINT("\n***MIDI TooManyParticipantsException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::UnexpectedInviteException:
      DEBUG_PRINT("\n***MIDI UnexpectedInviteException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::ParticipantNotFoundException:
      DEBUG_PRINT("\n***MIDI ParticipantNotFoundException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::ComputerNotInDirectory:
      DEBUG_PRINT("\n***MIDI ComputerNotInDirectory");
      break;
    case APPLEMIDI_NAMESPACE::Exception::NotAcceptingAnyone:
      DEBUG_PRINT("\n***MIDI NotAcceptingAnyone");
      break;
    case APPLEMIDI_NAMESPACE::Exception::ListenerTimeOutException:
      DEBUG_PRINT("\n***MIDI ListenerTimeOutException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::MaxAttemptsException:
      DEBUG_PRINT("\n***MIDI MaxAttemptsException");
      break;
    case APPLEMIDI_NAMESPACE::Exception::NoResponseFromConnectionRequestException:
      DEBUG_PRINT("\n***MIDI:yyy did't respond to the connection request. Check the address and port, and any firewall or router settings. (time)");
      break;
    case APPLEMIDI_NAMESPACE::Exception::SendPacketsDropped:
      DEBUG_PRINT("\n***MIDI SendPacketsDropped");
      break;
    case APPLEMIDI_NAMESPACE::Exception::ReceivedPacketsDropped:
      DEBUG_PRINT("\n***MIDI***************************************** ReceivedPacketsDropped");
      break;
    case APPLEMIDI_NAMESPACE::Exception::UdpBeginPacketFailed:
      DEBUG_PRINT("\n***MIDI UdpBeginPacketFailed");
      break;
  }
}

class MyMidiSessionInterface {
public:
  //virtual void begin() = 0;
  virtual void read() = 0;
  /*virtual void sendNoteOn(byte note, byte velocity, byte channel) = 0;
  virtual void sendNoteOff(byte note, byte velocity, byte channel) = 0;*/
  virtual ~MyMidiSessionInterface() {}
  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *))= 0;
  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)) = 0;
  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value))=0;
  virtual const char *getName() = 0;
  virtual const uint16_t getPort() = 0;
};

template <typename UdpType>
class AppleMidiWithInterfaceWrapper : public MyMidiSessionInterface {
public:
  //using SessionType = AppleMIDISession<UdpType>(const char* sessionName, uint16_t port);
  //using MidiInterfaceType = MidiInterface<SessionType, AppleMIDISettings>;

  AppleMIDISession<UdpType>* session;
  MidiInterface<AppleMIDISession<UdpType>, AppleMIDISettings>* midi;

  AppleMidiWithInterfaceWrapper<UdpType>(const char* sessionName, uint16_t port) {
    DEBUG_PRINT("\n\n\n\t\tAppleMidiWithInterfaceWrapper(%s,%d)\n\n\n", sessionName, port);
    session = new AppleMIDISession<UdpType>(sessionName, port);
    midi = new MidiInterface<AppleMIDISession<UdpType>, AppleMIDISettings>(*session);   
    session->begin();
    DEBUG_PRINT("\n\n\n\t\tsession->begin()");
  }

  virtual const char *getName(){
    return session->getName();
  }

  virtual const uint16_t getPort(){
    return session->getPort();
  }

  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *)){
    session->setHandleConnected(fptr);
  }

  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)){
    session->setHandleDisconnected(fptr);
  }

  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value)){
    session->setHandleException(fptr);
  }


  void read() override {
    midi->read();
  }

  /*void sendNoteOn(byte note, byte velocity, byte channel) override {
    midi->sendNoteOn(note, velocity, channel);
  }

  void sendNoteOff(byte note, byte velocity, byte channel) override {
    midi->sendNoteOff(note, velocity, channel);
  }*/


  ~AppleMidiWithInterfaceWrapper() {
    delete midi;
    delete session;
  }
};


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

    virtual bool monitor(){return false;}

    virtual void* read(){return nullptr;}

    virtual void begin(){}

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

    virtual bool monitor(){return false;}

    virtual void sendSysex(){}

    virtual void derivedConfig(JsonObject& config){   
      config[CONFIG_USB] = voidUSBparam;   // fill any parameters that are required by sysex
    } 

    virtual void restore(const JsonObject& config){
      voidUSBparam = config[CONFIG_USB].as<String>();
    }      

    virtual void* read(){return nullptr;}

    virtual void begin(){}

    virtual void sendNoteOn(DataByte inNoteNumber,DataByte inVelocity,Channel inChannel){};    

};


#define APPLEMIDI_INTERFACE "AppleMIDI"
class AppleMIDIInterface: public MIDIInterface{
  #define CONFIG_APPLE "voidAppleMidiParam"
  public:
    String voidAppleMidiParam = "titi";      

    MyMidiSessionInterface* midiSession;

    AppleMIDIInterface(bool _configurable):MIDIInterface(APPLEMIDI_INTERFACE,_configurable){
      //AppleMIDI_RTP_ETH.setHandleException(OnAppleMidiException);
            
    }

    virtual bool monitor(){
      if (hwm.network->state == CONNECTED){
        //if (hwm.network->media == CABLE_MEDIA) MIDI_RTP_ETH.read(); else
        //AppleMIDI.read();
        midiSession->read();
      }
      return false;
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
      /*if (hwm.network->media == CABLE_MEDIA) MIDI_RTP_ETH.sendNoteOn(inNoteNumber,inVelocity,inChannel);
      else myMidi->sendNoteOn(inNoteNumber,inVelocity,inChannel);*/
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

    // Méthode de l'instance réellement appelée
    void handleConnected(const ssrc_t& ssrc, const char* name) {
        //isConnected++;
        DEBUG_PRINT("\nConnected to session: %s\n", name);
    }    

    void handleDisconnected(const ssrc_t& ssrc) {
        //isConnected--;
        DEBUG_PRINT("\nDisconnected from session\n");
    }

    virtual void begin(){
      //AppleMIDI_RTP_ETH.setHandleConnected(handleConnectedStatic);      
      //AppleMIDI_RTP_ETH.setHandleDisconnected(handleDisconnectedStatic);      
      
      /*appleMidiSession->setHandleDisconnected(handleDisconnectedStatic);
      appleMidiSession->setHandleConnected(handleConnectedStatic);
      appleMidiSession->setHandleException(OnAppleMidiException);*/
      
      if (hwm.network->media == CABLE_MEDIA) {
          midiSession = new AppleMidiWithInterfaceWrapper<EthernetUDP>("ESP-ETH",DEFAULT_CONTROL_PORT);
      } else {
        midiSession = new AppleMidiWithInterfaceWrapper<WiFiUDP>("ESP-WiFi",DEFAULT_CONTROL_PORT);
        if (MDNS.begin(midiSession->getName())) {  // Nom réseau : esp32-midi.local
          DEBUG_PRINT("\n\n\n\t\t\tmDNS initialisé!\n\n\n");
          MDNS.addService(MDNS_ANNOUNCED_SERVICE, "udp", midiSession->getPort());
        } else {
          DEBUG_PRINT("\n\n\nmDNS échec\n\n\n");
        }         
      }     
      

      midiSession->setHandleDisconnected(handleDisconnectedStatic);
      midiSession->setHandleConnected(handleConnectedStatic);
      midiSession->setHandleException(OnAppleMidiException);
      

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