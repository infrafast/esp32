/**
 * @file s_utils.h
 * @brief Text-only strip and its saved text property.
 */

#ifndef UTILSSTRIPS_h
#define UTILSSTRIPS_h

#include "Strips.h"
#include "Interval.h"

/**
 * @brief Displays configured text without controlling a device value.
 */
class TextStrip : public Strip{
  
  #define STRIP_TEXT "text"
  //#define STRIP_TEXT_PROPERTY_VALUE "value"
  
  public:  
        
    /** @brief Initialize TextStrip with the supplied configuration and bindings. */
    TextStrip(StripSkeleton* s,bool debug=false):Strip(s){
      //icon = ICON_TEXT_STRIP;
      Address* textAddress = s->getDataSetAddress(0);
      if (textAddress != nullptr){
        textPropertyAddress = textAddress->pattern;
      }
    }

    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always true; no device action is performed here.
     */
    virtual bool actionCallback(uint8_t trigger){return true;}

    /** @brief Add this strip's widgets for the selected bank layout. */
    virtual void composeWidgets(uint8_t layout) override{
      pushSimpleText(layout, &text);
      //drawWidgetsFrames(layout);
    }

    #define TEXT_MAX_LENGHT 64

    /**
     * @brief Extract text from the configured address pattern, such as /text/%s.
     *
     * The parser uses a fixed intended text limit but the current scanf pattern is
     * not width-bounded; oversized properties are not safely validated here.
     */
    virtual void deserializeProperty(String property)  {  
      // property = /text/toto   
      // textProperty = /text/%s
      // value = toto
      char* buf = (char*)malloc(textPropertyAddress.length()*sizeof(char*)+TEXT_MAX_LENGHT);
      if (sscanf(property.c_str(), textPropertyAddress.c_str(), buf) == 1){
        text = buf;
      }           
      free(buf);
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(){
      char* buf = (char*)malloc(textPropertyAddress.length()*sizeof(char*)+TEXT_MAX_LENGHT);
      sprintf(buf,textPropertyAddress.c_str(),text.c_str());
      properties.add(String(buf));
      free(buf);

    }

    /** @brief Handle relevant incoming events for this strip. */
    void onEvent(const Event& evt) override {
      switch (evt.type) {
        /*case EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED:          
        break;*/
      }
    }

    /*virtual void update(){ // this code is called by process()

    } */
   
    

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasFocus() override{ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void hasVisibility() override{
      // request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
    }


    /** @brief Return the borrowed text used for the strip header top line. */
    virtual const String* getTopLabel(){return &type;}
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel(){return &emptyString;}  


    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always true; no device action is performed here.
     */
    virtual float increaseCallback(unsigned long time,uint8_t trigger){return true;}
    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always true; no device action is performed here.
     */
    virtual float decreaseCallback(unsigned long time,uint8_t trigger){return true;}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual float getIncrement(unsigned long time){return 0.0;}

    protected:
      String text="undef!";
      int16_t x=GUI::width()/2, y=GUI::height()/2;       
      String textPropertyAddress;
};

StripSkeleton* stripTextSkel;
/** @brief Create the concrete strip associated with this registered type. */
Strip* textStripConstructorCallback(){
  //DEBUG_PRINT("\n\ttextStripConstructorCallback()"/*,values.c_str()*/);
  return new TextStrip(stripTextSkel);
}

#endif
