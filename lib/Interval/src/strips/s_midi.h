#ifndef MIDISTRIPS_h
#define MIDISTRIPS_h

#include "Strips.h"
#include "Interval.h"

StripSkeleton *stripMidiSkel, *stripSysexSkel;

class MidiStrip : public ThreeSelectStrip{
  #define STRIP_MIDI "MIDI"

  public:
    MidiStrip(StripSkeleton* s,bool debug=false):ThreeSelectStrip(s,debug){

    }

    ~MidiStrip(){};
    
    void serializeProperties(){
      ThreeSelectStrip::serializeProperties();

      // "/midi/value/000000"     => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_MIDIVALUE)*sizeof(char*)+5);    // 5 digit max
      sprintf(buf, ADDRESS_MIDIVALUE,String(midiValue).c_str());
      properties.add(String(buf));
      delete buf;

    }


    virtual void deserializeProperty(String property)  {  
      // message = "/midi/message/%d"  => select3PropertyAddress => select3id
      // channel = "/midi/channel/%d"  => select2PropertyAddress => select2id
      ThreeSelectStrip::deserializeProperty(property);

      // sValue = "/midi/value/%s"     => DSmidiValue.address.pattern
      char sValue[50];                  
      if (sscanf(property.c_str(), ADDRESS_MIDIVALUE, sValue) == 1){
        midiValue = String(sValue).toInt();
        //DEBUG_PRINT("\nMidiStrip=>deserializeProperty midiValue: %d",midiValue);
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

    virtual void queryInterface(){ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual bool actionCallback(uint8_t trigger){
      ((AppleMIDIInterface*)interface)->sendNoteOn(32,64,1); // ==> just to see it
      return false;
    }
       
  private:
    String unitString = "dB";
    uint16_t midiValue=0;

};

Strip* midiStripConstructorCallback(){
  return new MidiStrip(stripMidiSkel);
}

class SysexStrip : public TwoSelectStrip{
  #define STRIP_SYSEX "Sysex"
  #define SYSEX_MAX_MESSAGE_LENGTH 128

  public:
    SysexStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug){

    }

    ~SysexStrip(){};


    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      // "/midi/sysex/000000"     => DSmidiValue.address.pattern
      char* buf = (char*)malloc( strlen(ADDRESS_SYSEXVALUE)*sizeof(char*)+SYSEX_MAX_MESSAGE_LENGTH);    // syex message length max
      sprintf(buf, DSmidiSysex.address.pattern.c_str(),sysexMessage.c_str());
      properties.add(String(buf));
      delete buf;

    }


    virtual void deserializeProperty(String property)  {  
      // channel = "/midi/channel/%d"  => select2PropertyAddress => select2id
      TwoSelectStrip::deserializeProperty(property);

      // sValue = "/midi/sysex/%s"     => DSmidiSysex.address.pattern
      char buffer[SYSEX_MAX_MESSAGE_LENGTH];                  
      if (sscanf(property.c_str(), ADDRESS_SYSEXVALUE, buffer) == 1){
        sysexMessage = String(buffer);
      }  
    }

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


    virtual bool actionCallback(uint8_t trigger){
      return false;
    }
       
  private:
    String unitString = "dB";
    String sysexMessage="";

};


Strip* sysexStripConstructorCallback(){
  return new SysexStrip(stripSysexSkel);
}

#endif