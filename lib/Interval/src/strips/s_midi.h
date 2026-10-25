/**
 * @file s_midi.h
 * @brief MIDI choices and strips for note and SysEx actions.
 */

#ifndef MIDISTRIPS_h
#define MIDISTRIPS_h

#include "Strips.h"
#include "Interval.h"
#include "interfaces/i_midi.h"

StripSkeleton *midiNoteSkel, *sysexSkel;

    //////////////////////////////////////////////////////////////
    /////////////////                        /////////////////////
    /////////////////    DATASETS           //////////////////////
    /////////////////                       //////////////////////
    //////////////////////////////////////////////////////////////

#define ADDRESS_SYSEX "/sysex/%s"
#define SYSEXLIB_FILENAME "sysexlib.json"

/**
 * @brief File-backed choices for configured SysEx messages.
 */
class SysexLib : public  WebLibrary{
  public:
    /**
     * @brief Initialize SysexLib and its configured member bindings.
     */
    SysexLib(Address* _address): WebLibrary(SYSEXLIB_FILENAME, _address, GROUP_INTERFACE_MIDI){}
};

SysexLib* dsSysexMessages;

/**
 * @brief Dataset of the sixteen MIDI channel labels.
 */
class MidiChannels : public DataSet{
  #define ADDRESS_MIDICHANNEL   "/channel/%d"
  #define MIDI_CHANNEL_NUMBER   16
  public:
    //String channelNames[MIDI_CHANNEL_NUMBER] = {"Piano", "Basse", "Guitare", "Cordes","Cuivres", "Choeur", "Lead", "FX","Pad", "Batterie", "Percu 2", "Synth","Harmonie", "Ambiance", "Drone", "Autre"};
    String channelNames[MIDI_CHANNEL_NUMBER] = {"channel1","channel2","channel3","channel4","channel5","channel6","channel7","channel8","channel9","channel10","channel11","channel12","channel13","channel14","channel15","channel16"};


    /**
     * @brief Initialize MidiChannels and its configured member bindings.
     */
    MidiChannels(Address* _address): DataSet(_address, GROUP_INTERFACE_MIDI){}
    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &channelNames[index];    //local javascript function 
    }  

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return MIDI_CHANNEL_NUMBER;
    }

}
/** @brief Shared sixteen-channel MIDI dropdown dataset. */
dsMidiChannels(new DropDownAddress(ADDRESS_MIDICHANNEL));

#define ADDRESS_MIDIVALUE2     "/value2/%s"
#define ADDRESS_MIDIVALUE1     "/value1/%s"
/**
 * @brief Dataset of selectable MIDI values.
 */
class MidiValue : public DataSet{
  public:
    String value="0123";
    /**
     * @brief Initialize MidiValue and its configured member bindings.
     */
    MidiValue(Address* adress, String name): DataSet(adress, name){}
    
    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){
      return &value;     
    }  
    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return 1;
    }    
}
/** @brief Shared note-value field descriptor and its fallback dataset value. */
dsMidiValue1(new FreeTextAddress(ADDRESS_MIDIVALUE1,"note", REGEX_0to126),GROUP_INTERFACE_MIDI),
/** @brief Shared velocity field descriptor and its fallback dataset value. */
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

/**
 * @brief Dataset of note-on/note-off action choices.
 */
class MidiNotes : public DataSet{
  #define ADDRESS_MIDINOTE   "/note/%s"
  public:    
     
    /**
     * @brief Initialize MidiNotes and its configured member bindings.
     */
    MidiNotes(Address* adress, String name): DataSet(adress, name){}

    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &midiMessages[index];    
    }  

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return MIDI_NOTE_SIZE;
    }  

}
/** @brief Shared dataset containing the Off and On MIDI action choices. */
dsNotesOnOff(new DropDownAddress(ADDRESS_MIDINOTE),GROUP_INTERFACE_MIDI);


    //////////////////////////////////////////////////////////////
    /////////////////                        /////////////////////
    /////////////////    STRIP DEF          //////////////////////
    /////////////////                       //////////////////////
    //////////////////////////////////////////////////////////////


/**
 * @brief MIDI strip with channel selection and one configured value.
 */
class OneValueMidiStrip : public OneSelectStrip{
  public:
    /** @brief Initialize OneValueMidiStrip with the supplied configuration and bindings. */
    OneValueMidiStrip(StripSkeleton* s,bool debug=false):OneSelectStrip(s,debug){
      select1PropertyAddress = ADDRESS_MIDICHANNEL;    
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~OneValueMidiStrip(){};
    
    /** @brief Append this type's property strings to the active JSON properties array. */
    void serializeProperties(){
      OneSelectStrip::serializeProperties();

      // ""/midi/channel/%d"    => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE1,String(midiValue1).c_str());
      properties.add(String(buf));
      free(buf);

    }


    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String property)  {  
      // channel = "/midi/channel/%d"  => select1PropertyAddress => select1id
      OneSelectStrip::deserializeProperty(property);

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

/**
 * @brief Adds a second configured MIDI value.
 */
class TwoValuesMidiStrip: public OneValueMidiStrip{
  public:

    /**
     * @brief Initialize TwoValuesMidiStrip and its configured member bindings.
     */
    TwoValuesMidiStrip(StripSkeleton* s,bool debug=false):OneValueMidiStrip(s,debug){}


    /** @brief Append this type's property strings to the active JSON properties array. */
    void serializeProperties(){
      OneValueMidiStrip::serializeProperties();
      // "/midi/value2/127"     => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE2,String(midiValue2).c_str());
      properties.add(String(buf));
      free(buf);
    }


    /** @brief Restore a type-specific setting from its saved property string. */
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


/**
 * @brief Sends MIDI note actions using configured channel, note and velocity.
 */
class MidiNoteStrip: public TwoValuesMidiStrip{

  #define STRIP_MIDINOTE "Note"
  public:
    String note = midiMessages[MIDI_NOTEON];      

    /**
     * @brief Initialize MidiNoteStrip and its configured member bindings.
     */
    MidiNoteStrip(StripSkeleton* s,bool debug=false):TwoValuesMidiStrip(s,debug){}

    #define NOTE_VALUE_LENGTH 4

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(){
      TwoValuesMidiStrip::serializeProperties();

      //#define ADDRESS_MIDINOTE   "/midi/note/%s"
      char* buf = (char*)malloc( strlen(ADDRESS_MIDINOTE)*sizeof(char)+NOTE_VALUE_LENGTH);    // on/off takes 4 char
      sprintf(buf, ADDRESS_MIDINOTE,String(note).c_str());
      properties.add(String(buf));
      free(buf);

    }

    /** @brief Restore a type-specific setting from its saved property string. */
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


    /** @brief Add this strip's widgets for the selected bank layout. */
    virtual void composeWidgets(uint8_t layout) override{      
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        push4x4hBar();
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," Note")));
      }else{

      }     
    } 

    /** @brief Handle relevant incoming events for this strip. */
    void onEvent(const Event& evt) override {
      switch (evt.type) {
        /*case EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED:          
        break;*/
      }
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasFocus() override{ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasVisibility() override{
      // request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
    }


    /**
     * @brief Send the configured MIDI note action on the selected channel.
     */
    virtual bool actionCallback(uint8_t trigger){
      bool success = false;

      if (midiValue1 <= 127 &&
        midiValue2 >= 1 && midiValue2 <= 127 &&
        select1ID  >= 1 && select1ID  <= 16) {

        if (note == midiMessages[MIDI_NOTEON]) success = ((MIDIInterface*)interface)->sendNoteOn(midiValue1,midiValue2,select1ID);
        else if (note == midiMessages[MIDI_NOTEOFF]) success = ((MIDIInterface*)interface)->sendNoteOff(midiValue1,midiValue2,select1ID);
      }

      return success;
    }    

  protected:
    int16_t barY = 0;
    
};


/** @brief Create the concrete strip associated with this registered type. */
Strip* midiNoteStripConstructorCallback(){
  return new MidiNoteStrip(midiNoteSkel);
}

/**
 * @brief Sends a configured SysEx library message.
 */
class SysexStrip : public Strip{  // should inherit from MidiStrip before so methode like data read castng are common
  #define STRIP_SYSEX "Sysex"
  #define SYSEX_MAX_KEY_LENGTH 64

  public:
    /**
     * @brief Initialize SysexStrip and its configured member bindings.
     */
    SysexStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~SysexStrip(){};

    /** @brief Add this strip's widgets for the selected bank layout. */
    virtual void composeWidgets(uint8_t layout) override{      

      uint16_t h = 28, w = 28;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W-GUI::three*2;
        gui.add(new ProgressBarH(value /*v*/, valueMin /*min*/,valueMax /*max*/, &GUI::three /*x*/, &GUI::midScreenHoriz /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
        gui.add(new RingMeter(value, valueMin, valueMax, 1, &centerX,&centerY, w, &unitString,&GUI::blue,&GUI::transparentColor,&brightness,true));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString(concatStrings(interface->interfaceName," SYSEX")));
      }else{

      }     
      //DEBUG_PRINT("\nchGainStrip topX: %d, topY: %d, w: %d, h: %d",topX,topY,w,h);
      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    /** @brief Handle relevant incoming events for this strip. */
    void onEvent(const Event& evt) override {
      switch (evt.type) {
        /*case EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED:          
        break;*/
      }
    }    

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasFocus() override{ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasVisibility() override{
      // request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
    }


    /**
     * @brief Send the configured library SysEx message through the selected MIDI interface.
     */
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

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(){
      char* buf = (char*)malloc((strlen(ADDRESS_SYSEX)+sysexKey.length())*sizeof(char)+1);
      sprintf(buf,ADDRESS_SYSEX,sysexKey);
      properties.add(String(buf)); //force la copie
      //DEBUG_PRINT("\nserializeProperties: %s",buf);
      free(buf);
    }

    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String property){
      char sysexKeyBuf[SYSEX_MAX_KEY_LENGTH];
      if (sscanf(property.c_str(), ADDRESS_SYSEX, &sysexKeyBuf)){
        sysexKey = sysexKeyBuf;
        //DEBUG_PRINT("\ndeserializeProperty: %s = %s",property.c_str(),sysexKey.c_str());
        sysexMessage = *dsSysexMessages->getString(sysexKey,"undef sysex");         // return the value corresponding to the key from the jsonfile library        
      }    
    }

    /** @brief Return the borrowed text used for the strip header top line. */
    virtual const String* getTopLabel(){return &type;}
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel(){return &sysexKey;}      
       
  protected:
    String unitString = "dB";
    String sysexMessage="";
    String sysexKey ="";
    int16_t widgetX, widgetY;
   
};


/** @brief Create the concrete strip associated with this registered type. */
Strip* sysexStripConstructorCallback(){
  return new SysexStrip(sysexSkel);
}


#endif
