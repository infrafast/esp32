#ifndef OSCSTRIPS_h
#define OSCSTRIPS_h

#include "Strips.h"
#include "Interval.h"
#include "interfaces/i_osc.h"


#define ADDRESS_OSCMSG "/osc/%s"
#define OSCLIB_FILENAME "osclib.json"

class OSCLib : public  WebLibrary{
  public:
    OSCLib(Address* _address): WebLibrary(OSCLIB_FILENAME, _address, GROUP_INTERFACE_OSC){}
};

OSCLib* dsOSCMessages;



class OSCStrip : public Strip{  // should inherit from MidiStrip before so methode like data read castng are common
  #define STRIP_OSC "SendOSC"
  #define OSC_MAX_KEY_LENGTH 64

  public:
    OSCStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    OSCMessage* osc = nullptr;

    ~OSCStrip(){};

    virtual void composeWidgets(uint8_t layout) override{      
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        push4x4hBar();
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString("SHAPE1X4 OSC"));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString("SHAPE2X2 OSC"));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString("SHAPE4X1 OSC"));
      }else{

      }         
    }

    void onEvent(const Event& evt) override {
      // only receive evt.type == EVENT_OSC_MESSAGE
      osc = (OSCMessage*)evt.data;

      const char* addr = osc->getAddress();
      if (!addr) return;
      
      ////  .... 

    }     

    virtual void hasFocus() override{ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual void hasVisibility() override{
      // request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
    }


    virtual bool actionCallback(uint8_t trigger) {
        if (!interface) return false;        
        OSCMessage msg(oscMessageString.c_str());        
        return  ((NetworkOSC*)interface)->send(msg);
    }

    virtual void serializeProperties(){
      char* buf = (char*)malloc((strlen(ADDRESS_OSCMSG)+oscKey.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_OSCMSG,oscKey);
      properties.add(String(buf)); //force la copie
      //DEBUG_PRINT("\nserializeProperties: %s",buf);
      free(buf);
    }

    virtual void deserializeProperty(String property){ 
      char OSCKeyBuf[OSC_MAX_KEY_LENGTH];
      if (sscanf(property.c_str(), ADDRESS_OSCMSG, &OSCKeyBuf)){
        oscKey = OSCKeyBuf;
        //DEBUG_PRINT("\ndeserializeProperty: %s = %s",property.c_str(),oscKey.c_str());
        oscMessageString = *dsSysexMessages->getString(oscKey,"undef sysex");         // return the value corresponding to the key from the jsonfile library       
      }    
    }

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return &oscKey;}      
       
  protected:
    String oscMessageString="";
    String oscKey ="";

};

StripSkeleton *oscSkel;

Strip* oscStripConstructorCallback(){
  return new OSCStrip(oscSkel);
}



#endif
