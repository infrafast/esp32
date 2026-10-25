/**
 * @file s_osc.h
 * @brief OSC library choices and the generic OSC strip skeleton.
 */

#ifndef OSCSTRIPS_h
#define OSCSTRIPS_h

#include "Strips.h"
#include "Interval.h"
#include "interfaces/i_osc.h"


#define ADDRESS_OSCMSG "/osc/%s"
#define OSCLIB_FILENAME "osclib.json"

/**
 * @brief File-backed choices for configured OSC messages.
 */
class OSCLib : public  WebLibrary{
  public:
    /**
     * @brief Initialize OSCLib and its configured member bindings.
     */
    OSCLib(Address* _address): WebLibrary(OSCLIB_FILENAME, _address, GROUP_INTERFACE_OSC){}
};

OSCLib* dsOSCMessages;



/**
 * @brief Generic OSC strip skeleton; implementation remains incomplete.
 */
class OSCStrip : public Strip{  // should inherit from MidiStrip before so methode like data read castng are common
  #define STRIP_OSC "SendOSC"
  #define OSC_MAX_KEY_LENGTH 64

  public:
    /**
     * @brief Initialize OSCStrip and its configured member bindings.
     */
    OSCStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    OSCMessage* osc = nullptr;

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~OSCStrip(){};

    /** @brief Add this strip's widgets for the selected bank layout. */
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

    /** @brief Handle relevant incoming events for this strip. */
    void onEvent(const Event& evt) override {
      // only receive evt.type == EVENT_OSC_MESSAGE
      osc = (OSCMessage*)evt.data;

      const char* addr = osc->getAddress();
      if (!addr) return;
      
      // OSC payload handling remains incomplete.

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
     * @brief Send the configured OSC address through the selected NetworkOSC interface.
     */
    virtual bool actionCallback(uint8_t trigger) {
        if (!interface) return false;        
        OSCMessage msg(oscMessageString.c_str());        
        return  ((NetworkOSC*)interface)->send(msg);
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(){
      char* buf = (char*)malloc((strlen(ADDRESS_OSCMSG)+oscKey.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_OSCMSG,oscKey);
      properties.add(String(buf)); //force la copie
      //DEBUG_PRINT("\nserializeProperties: %s",buf);
      free(buf);
    }

    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String property){ 
      char OSCKeyBuf[OSC_MAX_KEY_LENGTH];
      if (sscanf(property.c_str(), ADDRESS_OSCMSG, &OSCKeyBuf)){
        oscKey = OSCKeyBuf;
        //DEBUG_PRINT("\ndeserializeProperty: %s = %s",property.c_str(),oscKey.c_str());
        oscMessageString = *dsSysexMessages->getString(oscKey,"undef sysex");         // return the value corresponding to the key from the jsonfile library       
      }    
    }

    /** @brief Return the borrowed text used for the strip header top line. */
    virtual const String* getTopLabel(){return &type;}
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel(){return &oscKey;}      
       
  protected:
    String oscMessageString="";
    String oscKey ="";

};

StripSkeleton *oscSkel;

/** @brief Create the concrete strip associated with this registered type. */
Strip* oscStripConstructorCallback(){
  return new OSCStrip(oscSkel);
}



#endif
