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

#define ADDRESS_SYSEX "/sysex/%d"
class SysexLibEntry : public DataSet, public JsonListFile{
  public:
    SysexLibEntry(Address* _address): DataSet(_address, GROUP_INTERFACE_MIDI){ 
      #define DEFAULT_SYSEX_MSG_EXAMPLE "{\"sysexkey1\":\"00 01 02\",\"sysexkey2\":\"AA BB CC\"}"
      if (!load("/sysexmsg.json", DEFAULT_SYSEX_MSG_EXAMPLE)) addValue(filename,"error");
    }      

    virtual const String* getValuePtr(uint16_t index){      
      if (index>size()-1) return &errorDatasetOOR;      
      return getString(getName(index));      
    }  

    virtual uint16_t getSize(){
      return size();                //number of pair k/v loaded from the file
    }

}dsSysexMessages(new DropDownAddress(ADDRESS_SYSEX));


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
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char*)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE1,String(midiValue1).c_str());
      properties.add(String(buf));
      delete buf;

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
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE1)*sizeof(char*)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE2,String(midiValue2).c_str());
      properties.add(String(buf));
      delete buf;
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

    MidiNoteStrip(StripSkeleton* s,bool debug=false):TwoValuesMidiStrip(s,debug){}

    #define NOTE_VALUE_LENGTH 4

    virtual void serializeProperties(){
      TwoValuesMidiStrip::serializeProperties();

      //#define ADDRESS_MIDINOTE   "/midi/note/%s"
      char* buf = (char*)malloc( strlen(ADDRESS_MIDINOTE)*sizeof(char*)+NOTE_VALUE_LENGTH);    // on/off takes 4 char
      sprintf(buf, ADDRESS_MIDINOTE,String(note).c_str());
      properties.add(String(buf));
      delete buf;

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
        w = WIDGET4X4W;
        renderY += h/2; 
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE1X4){
        w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET1X4H;
        renderX += w/2;           
        gui.add(new ProgressBarV(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE2X2){
        w = WIDGET2X2W;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET2X2H;
        renderX += w/2;   
        renderY += h/2;      
        RingMeter* ringMeter = new RingMeter(&value, valueMin, valueMax, 1, &renderX,&renderY, h/2, &emptyString,&GUI::blue,&GUI::transparentColor,&brightness);
        gui.add(ringMeter);
      }
      else if (size == SHAPE4X1){
        h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
        w = WIDGET4X1W;
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }     

      //DEBUG_PRINT("\nchGainStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    virtual void update(){ // this code is called by process()

    }  

    virtual void queryInterface(){ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual bool actionCallback(uint8_t trigger){
      if (note == midiMessages[MIDI_NOTEON]) ((MIDIInterface*)interface)->sendNoteOn(midiValue1,midiValue2,select2ID); 
      else if (note == midiMessages[MIDI_NOTEOFF]) ((MIDIInterface*)interface)->sendNoteOff(midiValue1,midiValue2,select2ID);   // TO BE IMPLEMENTED
      return false;
    }    

    String note = midiMessages[MIDI_NOTEON];
};


Strip* midiNoteStripConstructorCallback(){
  return new MidiNoteStrip(midiNoteSkel);
}

class SysexStrip : public Strip{
  #define STRIP_SYSEX "Sysex"
  #define SYSEX_MAX_MESSAGE_LENGTH 256

  public:
    SysexStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    ~SysexStrip(){};

    virtual void composeWidgets(uint8_t layout) override{      

      uint16_t h = 0, w = 0;

      if (size == SHAPE4X4){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W;
        renderY += h/2; 
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE1X4){
        w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET1X4H;
        renderX += w/2;           
        gui.add(new ProgressBarV(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE2X2){
        w = WIDGET2X2W;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET2X2H;
        renderX += w/2;   
        renderY += h/2;      
        RingMeter* ringMeter = new RingMeter(&value, valueMin, valueMax, 1, &renderX,&renderY, h/2, &unitString,&GUI::blue,&GUI::transparentColor,&brightness);
        gui.add(ringMeter);
      }
      else if (size == SHAPE4X1){
        h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
        w = WIDGET4X1W;
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }     

      //DEBUG_PRINT("\nchGainStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    virtual void update(){ // this code is called by process()

    }  

    virtual void queryInterface(){ // this code is called by focus() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual float increaseCallback(unsigned long time,uint8_t trigger){
      return 0.0;
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger){
      return 0.0;
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

      uint8_t* buffer = (uint8_t*)malloc(outMax);
      if (!buffer) {
        Log::drop(interface->interfaceName.c_str(), "Invalid sysex: OOM");
        return false;
      }

      size_t length = 0;
      if (hexStringToBytes(sysexMessage, buffer, outMax, length)) {
        // TODO: protéger broadCast/sendSysEx par mutex si ressource partagée

        ((MIDIInterface*)interface)->sendSysEx(length, buffer, false);
        //Log::drop(interface->interfaceName.c_str(), (const char*)buffer);  // => for SOME REASON THIS CREATE A DISCONNECTION OF WEBSOCKET...MAYBE THE GENERATED FRAME IS INCORRECT?
        success = true;          

      } else {
        String err = String("Invalid sysex: ") + sysexMessage; // garder l'objet en vie
        Log::drop(interface->interfaceName.c_str(), err.c_str());
      }

      free(buffer);
      return success;
    }

    virtual void serializeProperties(){
      char* buf = (char*)malloc(strlen(ADDRESS_SYSEX)*sizeof(char*));
      sprintf(buf,ADDRESS_SYSEX,sysexID);
      properties.add(String(buf));
      DEBUG_PRINT("\n************** ADDRESS_SYSEX");
      delete buf;
    }

    virtual void deserializeProperty(String property){
      if (sscanf(property.c_str(), ADDRESS_SYSEX, &sysexID)){
        const String* sysexKeymsg = skeleton->getValuePtr(interface->getProviderName(),ADDRESS_SYSEX,sysexID-1);
        //sysexMessage = *dsSysexMessages.getString(*sysexKeymsg);         // return the value corresponding to the key from the jsonfile library
      }    
    }

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return &sysexMessage;}      
       
  protected:
    String unitString = "dB";
    String sysexMessage="";
    uint8_t sysexID = 0;

};


Strip* sysexStripConstructorCallback(){
  return new SysexStrip(sysexSkel);
}


#endif