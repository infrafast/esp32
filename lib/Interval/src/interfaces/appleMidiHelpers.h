#ifndef MIDIHELPERS_h
#define MIDIHELPERS_h

#define USE_EXT_CALLBACKS     // as from example => required for MIDI callbacks

//#define SerialMon Serial

#include <AppleMIDI.h>

//https://github.com/lathoub/Arduino-AppleMIDI-Library

using namespace APPLEMIDI_NAMESPACE;

/*
class Utilities for creating a generic pointer to AppleMIDISession and MidiInterface
from a WiFiUDP or EThernetUDP (template) and be able to manipulate it as a generic instance

*/




class MidiClient {
public:
  virtual void read() = 0;
  virtual ~MidiClient() {}
  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *))= 0;
  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)) = 0;
  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value))=0;
  virtual void setHandleSystemExclusive(void (*fptr)(byte* , unsigned )) = 0;
  virtual void setHandleControlChange(void (*fptr)(byte , byte , byte )) = 0;
  virtual void endSession() = 0;
  //virtual String getParticipantDetails(const ssrc_t& ssrc) = 0;
  virtual const char *getName() = 0;
  virtual const uint16_t getPort() = 0;
  virtual void begin() = 0;
  // all methods you need to be wrapped below
  virtual void sendNoteOn(byte note, byte velocity, byte channel) = 0;
  virtual void sendNoteOff(byte note, byte velocity, byte channel) = 0;  
  virtual void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries) = 0;
  // etc...

};

template <typename UdpType>
class AppleMidiWithInterfaceWrapper : public MidiClient {
public:
  AppleMIDISession<UdpType>* session;
  MidiInterface<AppleMIDISession<UdpType>, AppleMIDISettings>* appleMIDI;

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

  void stop(){
  }

  virtual void begin(){
    DEBUG_PRINT("\nmidiClient(%s)->begin()",session->getName());
    session->begin();    
  }

  void read() override {
    appleMIDI->read();
  }  

  virtual void endSession(){
    session->sendEndSession();
  }

  virtual const char *getName(){
    return session->getName();
  }

  virtual const uint16_t getPort(){
    return session->getPort();
  }
  ///////////////////////////////
  virtual void setHandleConnected(void (*fptr)(const ssrc_t &, const char *)){
    session->setHandleConnected(fptr);
  }

  virtual void setHandleDisconnected(void (*fptr)(const ssrc_t &)){
    session->setHandleDisconnected(fptr);
  }

  virtual void setHandleException(void (*fptr)(const ssrc_t &, const Exception &, const int32_t value)){
    session->setHandleException(fptr);
  }

  virtual void setHandleSystemExclusive(void (*fptr)(byte* , unsigned )){
    appleMIDI->setHandleSystemExclusive(fptr);
  }

  virtual void setHandleControlChange(void (*fptr)(byte , byte , byte )){
    appleMIDI->setHandleControlChange(fptr);
  }
  /////////////////////////
  void sendNoteOn(byte note, byte velocity, byte channel) override {
    DEBUG_PRINT("\tAppleMidiWithInterfaceWrapper::sendNoteOn(%d,%d,%d)",note, velocity, channel);
    appleMIDI->sendNoteOn(note, velocity, channel);
  }

  void sendNoteOff(byte note, byte velocity, byte channel) override {
    DEBUG_PRINT("\tAppleMidiWithInterfaceWrapper::sendNoteOff(%d,%d,%d)",note, velocity, channel);
    appleMIDI->sendNoteOff(note, velocity, channel);    
  }

  void sendSysEx(unsigned inLength,const byte* inArray,bool inArrayContainsBoundaries){
    DEBUG_PRINT("\tAppleMidiWithInterfaceWrapper::sendSysex ");
    appleMIDI->sendSysEx(inLength,inArray,inArrayContainsBoundaries);
    for (uint16_t i = 0; i < inLength; i++) DEBUG_PRINT("%02X ",inArray[i]);    
  }

  ~AppleMidiWithInterfaceWrapper() {
    stop();
    delete appleMIDI;
    delete session;
  }

};

#endif