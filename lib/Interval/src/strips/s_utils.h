#ifndef UTILSSTRIPS_h
#define UTILSSTRIPS_h

#include "Strips.h"
#include "Interval.h"

class TextStrip : public Strip{
  
  #define STRIP_TEXT "text"
  //#define STRIP_TEXT_PROPERTY_VALUE "value"
  
  public:  
        
    TextStrip(StripSkeleton* s,bool debug=false):Strip(s){
      //icon = ICON_TEXT_STRIP;
      Address* textAddress = s->getDataSetAddress(0);
      if (textAddress != nullptr){
        textPropertyAddress = textAddress->pattern;
      }
    }

    virtual bool actionCallback(uint8_t trigger){return true;}

    virtual void composeWidgets(uint8_t layout) override{
      pushSimpleText(layout, &text);
      //drawWidgetsFrames(layout);
    }

    #define TEXT_MAX_LENGHT 64

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

    virtual void serializeProperties(){
      char* buf = (char*)malloc(textPropertyAddress.length()*sizeof(char*)+TEXT_MAX_LENGHT);
      sprintf(buf,textPropertyAddress.c_str(),text.c_str());
      properties.add(String(buf));
      free(buf);

    }

    void onEvent(const Event& evt) override {
      switch (evt.type) {
        /*case EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED:          
        break;*/
      }
    }

    /*virtual void update(){ // this code is called by process()

    } */
   
    

    virtual void queryConfig() override{ // this code is called by pushToGUI() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return &emptyString;}  


    virtual float increaseCallback(unsigned long time,uint8_t trigger){return true;}
    virtual float decreaseCallback(unsigned long time,uint8_t trigger){return true;}
    virtual float getIncrement(unsigned long time){return 0.0;}

    protected:
      String text="undef!";
      int16_t x=GUI::width()/2, y=GUI::height()/2;       
      String textPropertyAddress;
};

StripSkeleton* stripTextSkel;
Strip* textStripConstructorCallback(){
  //DEBUG_PRINT("\n\ttextStripConstructorCallback()"/*,values.c_str()*/);
  return new TextStrip(stripTextSkel);
}

#endif
