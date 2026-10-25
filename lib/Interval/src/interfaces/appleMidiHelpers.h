/**
 * @file appleMidiHelpers.h
 * @brief Wraps AppleMIDI sessions behind a common Wi-Fi/Ethernet API.
 */

#ifndef MIDIHELPERS_h
#define MIDIHELPERS_h

#define USE_EXT_CALLBACKS     // as from example => required for MIDI callbacks

//#define SerialMon Serial
#define APPLEMIDI_INITIATOR   // to be able to send invitations

#include <AppleMIDI.h>

//https://github.com/lathoub/Arduino-AppleMIDI-Library

using namespace APPLEMIDI_NAMESPACE;

/*
class Utilities for creating a generic pointer to AppleMIDISession and MidiInterface
from a WiFiUDP or EThernetUDP (template) and be able to manipulate it as a generic instance

*/




/**
 * @brief Common AppleMIDI operations independent of the UDP transport type.
 */
class MidiClient {
public:
  /** @brief Read pending input from this transport or hardware source. */
  virtual void read() = 0;
  /** @brief Placeholder hook; no active operation in this implementation. */
  virtual ~MidiClient() {}
  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *))= 0;
  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)) = 0;
  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value))=0;
  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleSystemExclusive(void (*fptr)(byte* , unsigned )) = 0;
  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleControlChange(void (*fptr)(byte , byte , byte )) = 0;
  /** @brief Send a MIDI real-time message through the wrapped transport. */
  virtual void sendRealTime(MidiType inType) = 0;
  /** @brief Request termination of the AppleMIDI session. */
  virtual void endSession() = 0;
  /** @brief Invite the supplied endpoint to join the AppleMIDI session. */
  virtual bool sendInvite(IPAddress ip, uint16_t port = DEFAULT_CONTROL_PORT) = 0;
  //virtual String getParticipantDetails(const ssrc_t& ssrc) = 0;
  /** @brief Return the name supplied by the wrapped session or collection. */
  virtual const char *getName() = 0;
  /** @brief Return the configured session port. */
  virtual const uint16_t getPort() = 0;
  /** @brief Start this interface using its current configuration. */
  virtual void begin() = 0;
  /**
   * @brief all methods you need to be wrapped below
   */
  virtual void sendNoteOn(byte note, byte velocity, byte channel) = 0;
  /** @brief Send a MIDI note-off message through this transport. */
  virtual void sendNoteOff(byte note, byte velocity, byte channel) = 0;  
  /** @brief Send a MIDI system-exclusive message through this transport. */
  virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) = 0;
  // etc...

};

/**
 * @brief Adapts a concrete AppleMIDI UDP session to MidiClient.
 *
 * @tparam UdpType Concrete WiFiUDP or EthernetUDP transport.
 * Owns the concrete AppleMIDI session and MIDI interface while callers use MidiClient.
 */
template <typename UdpType>
class AppleMidiWithInterfaceWrapper : public MidiClient {
public:
  AppleMIDISession<UdpType>* session;
  MidiInterface<AppleMIDISession<UdpType>, AppleMIDISettings>* appleMIDI;

  /** @brief Initialize AppleMidiWithInterfaceWrapper with the supplied configuration and bindings. */
  AppleMidiWithInterfaceWrapper<UdpType>(const char* sessionName, uint16_t port){
    session = new AppleMIDISession<UdpType>(sessionName, port);
    appleMIDI = new MidiInterface<AppleMIDISession<UdpType>, AppleMIDISettings>(*session);
  }

  /*virtual String getParticipantDetails(const ssrc_t& ssrc){
    fix in applemidisession:

    #ifdef KEEP_SESSION_NAME
        char            sessionName[24 + 1];
    #endif

    and  make getParticipantBySSRC public

    Participant<AppleMIDISettings>* p = (Participant<AppleMIDISettings>*)session->getParticipantBySSRC(ssrc);
    return p->remoteIP.toString();
  }*/

  /** @brief Placeholder hook; no active operation in this implementation. */
  void stop(){
  }

  /** @brief Start this interface using its current configuration. */
  virtual void begin(){
    DEBUG_INFO("midiClient(%s)->begin()",session->getName());
    session->begin();    

    session->setHandleReceivedRtp([](const APPLEMIDI_NAMESPACE::ssrc_t & ssrc, const APPLEMIDI_NAMESPACE::Rtp_t & rtp, const int32_t& latency) {
      //Serial.printf("\nsession->setHandleReceivedRtp latency: %i",latency);
    });
  }

  /** @brief Read pending input from this transport or hardware source. */
  void read() override {
    appleMIDI->read();  
  }  

  /** @brief Request termination of the AppleMIDI session. */
  virtual void endSession(){
    session->sendEndSession();
  }

  /** @brief Send a MIDI real-time message through the wrapped transport. */
  virtual void sendRealTime(MidiType inType) {
    appleMIDI->sendRealTime(inType);
  }

  /** @brief Invite the supplied endpoint to join the AppleMIDI session. */
  virtual bool sendInvite(IPAddress ip, uint16_t port = DEFAULT_CONTROL_PORT) {
    return session->sendInvite(ip, port);
  }

  /** @brief Return the name supplied by the wrapped session or collection. */
  virtual const char *getName(){
    return session->getName();
  }

  /** @brief Return the configured session port. */
  virtual const uint16_t getPort(){
    return session->getPort();
  }
  //////////////////////////////
  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *)){
    session->setHandleConnected(fptr);
  }

  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)){
    session->setHandleDisconnected(fptr);
  }

  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value)){
    session->setHandleException(fptr);
  }

  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleSystemExclusive(void (*fptr)(byte* , unsigned )){
    appleMIDI->setHandleSystemExclusive(fptr);
  }

  /** @brief Register the callback used by the wrapped MIDI/session API. */
  virtual void setHandleControlChange(void (*fptr)(byte , byte , byte )){
    appleMIDI->setHandleControlChange(fptr);    
  }
  ////////////////////////
  void sendNoteOn(byte note, byte velocity, byte channel) override {
    DEBUG_INFO("AppleMidiWithInterfaceWrapper::sendNoteOn(%d,%d,%d)",note, velocity, channel);
    appleMIDI->sendNoteOn(note, velocity, channel);
  }

  /** @brief Send a MIDI note-off message through this transport. */
  void sendNoteOff(byte note, byte velocity, byte channel) override {
    DEBUG_INFO("AppleMidiWithInterfaceWrapper::sendNoteOff(%d,%d,%d)",note, velocity, channel);
    appleMIDI->sendNoteOff(note, velocity, channel);    
  }

  /** @brief Send a MIDI system-exclusive message through this transport. */
  void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
    DEBUG_INFO("AppleMidiWithInterfaceWrapper::sendSysex ");
    appleMIDI->sendSysEx(inLength,inArray,inArrayContainsBoundaries);
    for (uint16_t i = 0; i < inLength; i++) DEBUG_PRINT("%02X ",inArray[i]);    
  }

  /** @brief Release resources held by this interface. */
  ~AppleMidiWithInterfaceWrapper() {
    stop();
    delete appleMIDI;
    delete session;
  }

};

#endif
