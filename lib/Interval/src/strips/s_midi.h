#ifndef MIDISTRIPS_h
#define MIDISTRIPS_h

#include "Strips.h"
#include "Interval.h"
#include "interfaces/i_midi.h"

StripSkeleton *midiNoteSkel, *sysexSkel;

    ///////////////////////////////////////////////////////////////
    //////////////////                        /////////////////////
    //////////////////    DATASETS           //////////////////////
    //////////////////                       //////////////////////    
    ///////////////////////////////////////////////////////////////

#define ADDRESS_SYSEX "/sysex/%s"
#define SYSEXLIB_FILENAME "sysexlib.json"

class SysexLib : public  WebLibrary{
  public:
    SysexLib(Address* _address): WebLibrary(SYSEXLIB_FILENAME, _address, GROUP_INTERFACE_MIDI){}
};

SysexLib* dsSysexMessages;

class MidiChannels : public DataSet{
  #define ADDRESS_MIDICHANNEL   "/channel/%d"
  #define MIDI_CHANNEL_NUMBER   16
  public:
    //String channelNames[MIDI_CHANNEL_NUMBER] = {"Piano", "Basse", "Guitare", "Cordes","Cuivres", "Choeur", "Lead", "FX","Pad", "Batterie", "Percu 2", "Synth","Harmonie", "Ambiance", "Drone", "Autre"};
    String channelNames[MIDI_CHANNEL_NUMBER] = {"channel1","channel2","channel3","channel4","channel5","channel6","channel7","channel8","channel9","channel10","channel11","channel12","channel13","channel14","channel15","channel16"};


    MidiChannels(Address* _address): DataSet(_address, GROUP_INTERFACE_MIDI){}
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &channelNames[index];    //local javascript function 
    }  

    virtual uint16_t getSize(){
      return MIDI_CHANNEL_NUMBER;
    }

}dsMidiChannels(new DropDownAddress(ADDRESS_MIDICHANNEL));

#define ADDRESS_MIDIVALUE2     "/value2/%s"
#define ADDRESS_MIDIVALUE1     "/value1/%s"
class MidiValue : public DataSet{
  public:
    String value="0123";
    MidiValue(Address* adress, String name): DataSet(adress, name){}
    
    virtual const String* getValuePtr(uint16_t index){
      return &value;     
    }  
    virtual uint16_t getSize(){
      return 1;
    }    
}
dsMidiValue1(new FreeTextAddress(ADDRESS_MIDIVALUE1,"note", REGEX_0to126),GROUP_INTERFACE_MIDI),
dsMidiValue2(new FreeTextAddress(ADDRESS_MIDIVALUE2,"velocity", REGEX_0to126),GROUP_INTERFACE_MIDI);


enum {
  MIDI_NOTEOFF,
  MIDI_NOTEON,
  MIDI_NOTE_SIZE
};

String midiMessages[MIDI_NOTE_SIZE] = {
  "Off",
  "On",
};  

class MidiNotes : public DataSet{
  #define ADDRESS_MIDINOTE   "/note/%s"
  public:    
     
    MidiNotes(Address* adress, String name): DataSet(adress, name){}

    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &midiMessages[index];    
    }  

    virtual uint16_t getSize(){
      return MIDI_NOTE_SIZE;
    }  

}dsNotesOnOff(new DropDownAddress(ADDRESS_MIDINOTE),GROUP_INTERFACE_MIDI);


    ///////////////////////////////////////////////////////////////
    //////////////////                        /////////////////////
    //////////////////    STRIP DEF          //////////////////////
    //////////////////                       //////////////////////    
    ///////////////////////////////////////////////////////////////


class OneValueMidiStrip : public TwoSelectStrip{
  public:
    OneValueMidiStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug){
      select2PropertyAddress = ADDRESS_MIDICHANNEL;    
    }

    ~OneValueMidiStrip(){};
    
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      // ""/midi/channel/%d"    => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE1,String(midiValue1).c_str());
      properties.add(String(buf));
      free(buf);

    }


    virtual void deserializeProperty(String property)  {  
      // channel = "/midi/channel/%d"  => select2PropertyAddress => select2id
      TwoSelectStrip::deserializeProperty(property);

      // sValue = "/midi/value1/%s"     => DSmidiValue.address.pattern
      char sValue[5];                  
      if (sscanf(property.c_str(), ADDRESS_MIDIVALUE1, sValue) == 1){
        midiValue1 = String(sValue).toInt();
        //DEBUG_PRINT("\nMidiStrip=>deserializeProperty midiValue: %d",midiValue);
      }  
    }
       
  protected:
    uint16_t midiValue1=0;

};

class TwoValuesMidiStrip: public OneValueMidiStrip{
  public:

    TwoValuesMidiStrip(StripSkeleton* s,bool debug=false):OneValueMidiStrip(s,debug){}


    void serializeProperties(){
      OneValueMidiStrip::serializeProperties();
      // "/midi/value2/127"     => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE2,String(midiValue2).c_str());
      properties.add(String(buf));
      free(buf);
    }


    virtual void deserializeProperty(String property)  {  
      // "/midi/value1/127"  
      OneValueMidiStrip::deserializeProperty(property);

     // "/midi/value2/127" 
      char sValue[5];                  
      if (sscanf(property.c_str(), ADDRESS_MIDIVALUE2, sValue) == 1){
        midiValue2 = String(sValue).toInt();
        //DEBUG_PRINT("\nMidiStrip=>deserializeProperty midiValue: %d",midiValue);
      }  
    }

  protected:
    uint16_t midiValue2=0;    

};


class MidiNoteStrip: public TwoValuesMidiStrip{

  #define STRIP_MIDINOTE "Note"
  public:
    String note = midiMessages[MIDI_NOTEON];      

    MidiNoteStrip(StripSkeleton* s,bool debug=false):TwoValuesMidiStrip(s,debug){}

    #define NOTE_VALUE_LENGTH 4

    virtual void serializeProperties(){
      TwoValuesMidiStrip::serializeProperties();

      //#define ADDRESS_MIDINOTE   "/midi/note/%s"
      char* buf = (char*)malloc( strlen(ADDRESS_MIDINOTE)*sizeof(char)+NOTE_VALUE_LENGTH);    // on/off takes 4 char
      sprintf(buf, ADDRESS_MIDINOTE,String(note).c_str());
      properties.add(String(buf));
      free(buf);

    }

    virtual void deserializeProperty(String property)  {  
      TwoValuesMidiStrip::deserializeProperty(property);

      //#define ADDRESS_MIDINOTE   "/midi/note/%s"
      char sValue[NOTE_VALUE_LENGTH];                  
      if (sscanf(property.c_str(), ADDRESS_MIDINOTE, sValue) == 1){ 
        note = sValue;
        if (note != midiMessages[MIDI_NOTEON] && note != midiMessages[MIDI_NOTEOFF]){
          note = midiMessages[MIDI_NOTEON];
        }        
      }
    }


    virtual void composeWidgets(uint8_t layout) override{      

      uint16_t h = 0, w = 0;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W-GUI::three*2;
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &GUI::three /*x*/, &GUI::midScreenHoriz /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else{

      } 

      //DEBUG_PRINT("\nchGainStrip topX: %d, topY: %d, w: %d, h: %d",topX,topY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    virtual void update(){ // this code is called by process()

     
    }  

    virtual void queryInterface(){ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual bool actionCallback(uint8_t trigger){
      bool success = false;

      if (midiValue1 <= 127 &&
        midiValue2 >= 1 && midiValue2 <= 127 &&
        select2ID  >= 1 && select2ID  <= 16) {

        if (note == midiMessages[MIDI_NOTEON]) success = ((MIDIInterface*)interface)->sendNoteOn(midiValue1,midiValue2,select2ID);
        else if (note == midiMessages[MIDI_NOTEOFF]) success = ((MIDIInterface*)interface)->sendNoteOff(midiValue1,midiValue2,select2ID);
      }

      return success;
    }    

  protected:
    int16_t barY = 0;
    
};


Strip* midiNoteStripConstructorCallback(){
  return new MidiNoteStrip(midiNoteSkel);
}

class SysexStrip : public Strip{  // should inherit from MidiStrip before so methode like data read castng are common
  #define STRIP_SYSEX "Sysex"
  #define SYSEX_MAX_KEY_LENGTH 64

  public:
    SysexStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    ~SysexStrip(){};

    virtual void composeWidgets(uint8_t layout) override{      

      uint16_t h = 28, w = 28;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W-GUI::three*2;
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &GUI::three /*x*/, &GUI::midScreenHoriz /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
        gui.add(new RingMeter(&value, valueMin, valueMax, 1, &centerX,&centerY, w, &unitString,&GUI::blue,&GUI::transparentColor,&brightness,true));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
      }else{

      }     


      //DEBUG_PRINT("\nchGainStrip topX: %d, topY: %d, w: %d, h: %d",topX,topY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    virtual void update() override {
      // --------------------------------------------------------------------
      // Ancien mécanisme conservé temporairement pour compatibilité
      // --------------------------------------------------------------------
      if (!interface || !interface->data) return;

      // On caste encore l'ancien buffer partagé tant que toute la migration
      // n'est pas terminée.
      MidiData* midiData = static_cast<MidiData*>(interface->data);

      // On ne traite que les événements SysEx
      if (midiData->type != MIDI_SYSEX) return;

      // IMPORTANT :
      // Dans l'ancien mécanisme, la longueur utile est encore portée par
      // interface->available(), qui retourne dataAvailable.
      const int availableBytes = interface->available();
      if (availableBytes <= 0) {
        value = 0;
        return;
      }

      handleSysexBuffer(midiData->data, (size_t)availableBytes);
    }

    virtual void queryInterface(){ // this code is called by focus() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual bool actionCallback(uint8_t trigger) {
      bool success = false;

      if (!interface){
        return false;
      }

      // borne sup : ~1 octet pour 2 caractères (séparateurs ignorés par hexStringToBytes)
      const size_t outMax = (sysexMessage.length() + 1) / 2; // arrondi haut
      if (outMax == 0) {
        Log::drop(interface->interfaceName.c_str(), "Invalid sysex: empty");
        return false;
      }

      byte* buffer = (byte*)malloc(outMax);
      if (!buffer) {
        Log::drop(interface->interfaceName.c_str(), "Invalid sysex: OOM");
        return false;
      }

      size_t length = hexStringToBytes(sysexMessage, buffer, outMax);
      if (length>0) {
        // polymorphism doesnt work
        /*if (interface->interfaceName == BLEMIDI_INTERFACE ) success = ((BLEMIDIInterface*)interface)->sendSysEx(length, buffer, false);
        else if (interface->interfaceName == USBMIDI_INTERFACE) success = ((USBMIDIInterface*)interface)->sendSysEx(length, buffer, false);
        else if (interface->interfaceName == DINMIDI_INTERFACE) success = ((DINMIDIInterface*)interface)->sendSysEx(length, buffer, false);
        else if (interface->interfaceName == APPLEMIDI_INTERFACE) success = ((AppleMIDIInterface*)interface)->sendSysEx(length, buffer, false);
        else{
          DEBUG_PRINT("\n%s invalid interface",type.c_str());
        }*/
        success = ((MIDIInterface*)interface)->sendSysEx(length, buffer, false);
        //Log::drop(interface->interfaceName.c_str(), (const char*)buffer);  // => for SOME REASON THIS CREATE A DISCONNECTION OF WEBSOCKET...MAYBE THE GENERATED FRAME IS INCORRECT?
      } else {
        String err = concatStrings("Invalid sysex: ", sysexMessage);
        Log::drop(interface->interfaceName.c_str(), err.c_str());
      }

      free(buffer);
      return success;
    }

    virtual void serializeProperties(){
      char* buf = (char*)malloc((strlen(ADDRESS_SYSEX)+sysexKey.length())*sizeof(char)+1);
      sprintf(buf,ADDRESS_SYSEX,sysexKey);
      properties.add(String(buf)); //force la copie
      //DEBUG_PRINT("\nserializeProperties: %s",buf);
      free(buf);
    }

    virtual void deserializeProperty(String property){
      char sysexKeyBuf[SYSEX_MAX_KEY_LENGTH];
      if (sscanf(property.c_str(), ADDRESS_SYSEX, &sysexKeyBuf)){
        sysexKey = sysexKeyBuf;
        //DEBUG_PRINT("\ndeserializeProperty: %s = %s",property.c_str(),sysexKey.c_str());
        sysexMessage = *dsSysexMessages->getString(sysexKey,"undef sysex");         // return the value corresponding to the key from the jsonfile library        
      }    
    }

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return &sysexKey;}      
       
  protected:
    String unitString = "dB";
    String sysexMessage="";
    String sysexKey ="";
    int16_t widgetX, widgetY;

    // ----------------------------------------------------------------------
    // Traite un message SysEx à partir d'un buffer stable
    // ----------------------------------------------------------------------
    // Cette méthode devient le point central de traitement.
    // Ainsi, on peut l'appeler aussi bien depuis :
    // - l'ancien mécanisme (update + interface->data)
    // - le nouveau mécanisme (onInterfaceEventFrame + queue)
    // ----------------------------------------------------------------------
    void handleSysexBuffer(const uint8_t* dataPtr, size_t dataLen) {
      if (!dataPtr || dataLen == 0) {
        value = 0;
        return;
      }

      // Pour ce premier pas, on se contente d'afficher / mémoriser la taille.
      // C'est déjà suffisant pour valider que le strip reçoit bien les frames.
      value = (int16_t)dataLen;

      // Optionnel : si tu veux borner l'affichage à valueMax
      if (value > valueMax) {
        value = valueMax;
      }

      // Debug utile pendant la migration
      DEBUG_PRINT("\nSysexStrip::handleSysexBuffer len=%u", (unsigned)dataLen);
    }    

    // ----------------------------------------------------------------------
    // Nouveau mécanisme :
    // traitement des frames publiées dans la queue de l'interface
    // MIDIInterface publie  une InterfaceEventFrame de type IE_MIDI_SYSEX avec une copie bornée du message dans payload[].
    // ----------------------------------------------------------------------
    virtual void onInterfaceEventFrame(const InterfaceEventFrame& frame) override {
      // On ne traite que les événements SysEx
      if (frame.kind != IE_MIDI_SYSEX) return;

      // Le payload de la frame est une copie stable du SysEx publié
      handleSysexBuffer(frame.payload, frame.size);
    }    

};


Strip* sysexStripConstructorCallback(){
  return new SysexStrip(sysexSkel);
}


#endif
