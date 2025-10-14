#ifndef STRIPSDEF_h
#define STRIPSDEF_h

#include "Interval.h"
using IntervalButton = Interval::Button;

void addFullScreenTextWithIcon(const String* topLabel, const String* midText, const String* bottomLabel, const String* icon);
//-------------------------

#define C_PROPERTIES    "properties"
#define C_STRIPTYPE     "type"
#define C_INDEX         "id"
#define C_X             "x"
#define C_Y             "y"
#define C_WIN           "win"
#define C_SIZE          "size"
#define C_POSITIONS     "positions"
#define C_INTERFACE     "interface" 
#define C_ICON          "icon"
#define C_COLOR         "color"

//-----------------------

#define WIDGET4X4W (TFT_WIDTH)
#define WIDGET4X4H (TFT_HEIGHT-(TOP2WIDGET_SIZE*2)-OVERLAY_ICON_SIZE-WIDGET_PADDING*2)

#define WIDGET1X4W (TFT_WIDTH/4)
#define WIDGET1X4H (WIDGET4X4H)

#define WIDGET4X1H (WIDGET1X4H/4)
#define WIDGET4X1W (TFT_WIDTH)

#define WIDGET2X2W (TFT_WIDTH/2)
#define WIDGET2X2H (WIDGET4X4H/2)


class Strip;
typedef Strip* (*StripBuilderCallback)(/*String values*/);

class StripSkeleton{
  #define STRIP_DEFAULT_GROUP "utils"
  #define MAXNUMBER_OF_SKELPROPERTY 10
  #define STRIP_SKELETON_PROPERTIES "properties"

  public:
    String stripID;
    unsigned char icon;
    String group;
    StripBuilderCallback constructor;
    //JsonObject variable;
    LinkedList<Address*> dataSetAddresses = LinkedList<Address*>();

    StripSkeleton(String id, String g, StripBuilderCallback b, unsigned char i='!');

    const String* getValuePtr(String provider, String address, uint8_t index){
      const String *retVal = &emptyString;

      //DEBUG_PRINT("\nskeleton<%s>.getValuePtr(%s,%s,%i): ",stripID.c_str(),provider.c_str(),address.c_str(),index);
      DataSet* ds = dataRepository.getDataSet(provider, address);      
      
      if (ds != nullptr){
        retVal = ds->getValuePtr(index);
      }

      //DEBUG_PRINT("<%s>",retVal->c_str());

      return retVal;
    }

    bool registerDataSetAddress(Address* address){
      dataSetAddresses.add(address); 
      return true;
    }

    String serialize(){
      JsonDocument jsonDocStripSkeleton;
      JsonArray properties;
      jsonDocStripSkeleton[stripID] = group;
      jsonDocStripSkeleton[C_ICON] = icon;

      JsonDocument property;

      properties = jsonDocStripSkeleton[STRIP_SKELETON_PROPERTIES].to<JsonArray>();     
      for (uint8_t i = 0; i < dataSetAddresses.size(); i++){

        Address* address = dataSetAddresses[i];
        if (address != nullptr){
          DeserializationError err = deserializeJson(property,address->serialize()); // {"/midi/value1/%s": ["freetext","note", "0-126"]} or {"/midi/channel/%d": ["dropdown"]}
          if (err) {
            DEBUG_PRINT("\naddress properties serialization failed: %s",dataSetAddresses[i]->pattern.c_str());
            continue;
          }
        }else{
           DEBUG_PRINT("\naddress[%d] is null",i);
        }

        // doc contient maintenant ton objet JSON
        JsonObject obj = property.as<JsonObject>();    
        properties.add(property);

      }
      String data;
      serializeJson(jsonDocStripSkeleton, data);
      return data;
    }

};


class StripFactory{
  public:
    std::vector<StripSkeleton*> stripSkeletonRepo;

    bool add(StripSkeleton* ss){
      stripSkeletonRepo.push_back(ss);
      return true;
    }

    void remove(StripSkeleton* ss) {
      for (auto it = stripSkeletonRepo.begin(); it != stripSkeletonRepo.end(); ++it) {
        if ((*it)->stripID == ss->stripID) {
          stripSkeletonRepo.erase(it);
          break;
        }
      }
    }

    Strip* build(String stripID/*, String values*/){
      Strip* strip = nullptr;
      StripSkeleton* skel = getStripSkeleton(stripID);
      if (skel != nullptr){
        strip = skel->constructor(/*values*/);
      }
      return strip;
    }

    String getDefinition(String stripID){
      String stringDef = "{\"null\":0}";
      StripSkeleton* skel = getStripSkeleton(stripID);
      if(skel != nullptr) stringDef = skel->serialize();
      return stringDef;
    }

    void serialize(JsonArray* jsonSkeletons){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* skeleton = stripSkeletonRepo[i];
        String stripName = skeleton->stripID;
        jsonSkeletons->add(skeleton->serialize());
      }
    }

  private:
    StripSkeleton* getStripSkeleton(String stripID){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* s = stripSkeletonRepo[i];        
        if (stripID == s->stripID){
          return s;
        } 
      }
      return nullptr;
    }
};

StripFactory stripFactory;

StripSkeleton::StripSkeleton(String id, String g, StripBuilderCallback c, unsigned char i):stripID(id),group(g),icon(i),constructor(c){
  stripFactory.add(this);
}


///////////////////// STRIP MANAGEMENT ////////////////////////

#define LAYOUT_ZOOM     0b00000001
#define LAYOUT_DETAILS  0b00000010

#define SHAPE4X4 "4x4"
#define SHAPE4X1 "4x1"
#define SHAPE1X4 "1x4"
#define SHAPE2X2 "2x2"

float oneHundedFloat = 100.0;

class Strip {  
  #define JSON_STRIP_PROPERTY_SIZE 100
  #define  JSON_STRIP_TEMPLATE_SIZE 300
  public:
    TFT_eSprite* sprite = nullptr;

    // flag to indicate if position is out of range
    bool renderError;
    bool redraw = false;

    bool toggle1 = false;
    bool toggle2 = false;
    bool updated = false;
    uint16_t frontColor, backColor;    

    String iconInterfaceConnect = ICON_FOOTSWITCH;

    String type="undef";

    String interfaceName = SYSTEM_INTERFACE; //
    DeviceInterface* interface = nullptr;

    // logical position
    uint8_t x, y, win, index;
    
    // position on TFT screen
    int16_t renderX, renderY;
    
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    //GUIManager* gui;

    float brightness = 1.0;

    Strip(StripSkeleton* s){
      skeleton = s;
      value = (valueMax-valueMin)/2;
      interface = &systemInterface;     // default
      //id = skeleton->stripID;
    }    

    virtual ~Strip(){
    }
   
    virtual uint8_t getNumberOfProperties(){
      return skeleton->dataSetAddresses.size();      
    }

    virtual bool actionCallback(uint8_t trigger)=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;        
    virtual void queryInterface() = 0; // this method is the one which is used to force poll data from the interface, 
    // it will usually drop a command to the interface which outcome be lately read by the process() method
    // is called from the focus() method so to force data to be updated in case some whre missed
    virtual const String* getTopLabel()=0;
    virtual const char* getBottomLabel()=0;
    virtual void update() = 0;  // // this code is called by process() if data are availabe and must therefore implement the code that uses what is returned by read() by casting it to the right data structure
    // it assign the GUI values the values that were acquired or processed in the process() method
    // basedon its internal state, it will set the redraw flag 
    
    virtual bool process(){ // this code is requesting the strip to update its state (values, etc...) using data from the interface (with read())
    // process() is called from main app on all strip: the strip decide then if he has to do the query (for instance if it is focused) as       
    // some strip may do some backgroud tasks (like playing sample or executing a macro), other will only do this if they have focus         
      redraw = false;
      if (interface->state.stateChanged) redraw = true;
      if (interface->available()) update();   //update is called if data are availabe and must therefore implement the code that uses what is returned by read() by casting it to the right data structure
      return redraw;      
    }; 


    void focus(bool f=true){
      if (f){
        brightness = 1.0;
        // we get the latest data if they are available        
        queryInterface();
      }else{
        brightness = 0.5;
      }
    }

    bool deserialize(String values){
      bool ret = false;
      uint8_t n = getNumberOfProperties();
      JsonDocument jsonStrip;
      DeserializationError error = deserializeJson(jsonStrip, values);
      if (error){
        
      }else{      
        type = jsonStrip[C_STRIPTYPE].as<String>();
        // set index; 
        x = jsonStrip["position"][C_X].as<int>();
        y = jsonStrip["position"][C_Y].as<int>();
        win = jsonStrip["position"][C_WIN].as<int>();
        index = jsonStrip["position"][C_INDEX].as<int>();
        size = jsonStrip[C_SIZE].as<String>();
        colorID = jsonStrip[C_COLOR];//.as<int>();
        interfaceName = jsonStrip[C_INTERFACE].as<String>();//.as<int>();
        
        interface = interfaceRepo.retrieve(interfaceName);

        if (colorID>MAX_COLOR_ID-1) colorID = 0;
        icon = jsonStrip[C_ICON];//.as<unsigned char>();
        properties = jsonStrip[C_PROPERTIES];
        for (uint8_t i = 0; i< properties.size(); i++){
          deserializeProperty(properties[i].as<String>());
        }
        ret = true;
        // each derived strip then deserialiuze its own other values
        
      }
      return ret;
    };

    String serialize(){
      
      JsonDocument doc;
      doc[C_STRIPTYPE] = type;  // this is a text
      doc[C_COLOR] = colorID;
      doc[C_ICON] = icon;
      doc[C_SIZE] = size;
      //DEBUG_PRINT("\nserialize)))))))))interfaceName: %s",interfaceName.c_str());
      doc[C_INTERFACE] = interfaceName;

      doc["position"][C_X] = x;
      doc["position"][C_Y] = y;
      doc["position"][C_INDEX] = index;
      doc["position"][C_WIN] = win;
      
      properties = doc[C_PROPERTIES].to<JsonArray>();
      serializeProperties();
      String data;
      serializeJson(doc, data);

      return data;
    }

    virtual bool progressiveDelay(unsigned long time,uint8_t trigger){
      
      uint16_t TIME_MAX_PROGRESSIVITY = 3000;
      if (valueMax-valueMin<=20) TIME_MAX_PROGRESSIVITY = 5000;
      else if (valueMax-valueMin>500) TIME_MAX_PROGRESSIVITY = 1000;

      //we start counting the time between two calls when we have called the first time, means during debounce duration
      if (time<DEBOUCE_TIME){
        lastCall = 0;      
        // first call we start timer

      }
      if (time>=TIME_MAX_PROGRESSIVITY) time = TIME_MAX_PROGRESSIVITY;

      unsigned long waitBetween2update; 
      
      if (trigger == IntervalButton::BUTTON_HOLD) waitBetween2update = (TIME_MAX_PROGRESSIVITY-time)/5;
      else waitBetween2update = 50 ; // 50ms 

      unsigned long delaySoFar = millis() - lastCall;

      if (delaySoFar < waitBetween2update) return false;  
      lastCall = millis();
 
      return true;    
    }

    /*virtual float increaseCallback(unsigned long time) = 0;
    virtual float decreaseCallback(unsigned long time) = 0;
    virtual float getIncrement(unsigned long time)=0;*/

    virtual float getIncrement(unsigned long time,uint8_t trigger){
      float increment;

      if (trigger == IntervalButton::BUTTON_HOLD){
        //value = roundHalf(value);
        if (time<2500){
          increment = valueMax*scaleValue(time, 0, 2500, 0.007, 0.1);
        }else{
          float diff = valueMax - value;
          float percentageToTarget = map(diff,0,valueMax-valueMin,100,1)/2000.0;  // 3000 seems to be a good compromise between delay and precision, 2000 react quicker but skip some decimal
          increment = diff*percentageToTarget;          
        }
      } else increment = valueMax*0.05;

      return increment;
    }    

    virtual float increaseCallback(unsigned long time,uint8_t trigger){  
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC          
      
      if (!Strip::progressiveDelay(time,trigger)) return 0.0;
      
      float delta = getIncrement(time,trigger);

      value += delta;

      if (value > valueMax) value = valueMax;
      //value = truncateToOneDecimal(value);
      
      return delta;      
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger){
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      if (!Strip::progressiveDelay(time,trigger)) return 0.0;

      float delta = getIncrement(time,trigger); 

      value -= delta;
      
      if (value < valueMin) value = valueMin;

      //value = truncateToOneDecimal(value);

      return delta;      
    }
    
    #define SIMPLE_TEXT_MAX_LEN 20
    void pushSimpleText(uint8_t layout, const String* text){
      // this is a utility function which display the text provided as parameter into the strip 
      // to take its full size, it is essentially used to display the strip state when not connected
      // or used by utility strips
      uint16_t fontSize;
      int16_t angle = 0;
      uint8_t len = text->substring(0,SIMPLE_TEXT_MAX_LEN).length();
      //DEBUG_PRINT("\npushText(%s) %s layout:%i",text->c_str(),size.c_str(), layout);
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        textX = renderX + WIDGET4X4W/2;
        textY = renderY + WIDGET4X4H/4;        

        fontSize = WIDGET4X4W / len * 3.3;
      }
      else if (size == SHAPE1X4){
        textX = renderX + WIDGET1X4W*1.2;
        textY = renderY + WIDGET1X4H/7;    

        fontSize = WIDGET1X4W / len * 4;
        // put it vertically
        angle = 90;
      }
      else if (size == SHAPE2X2){
        textX = renderX + WIDGET2X2W/2;
        textY = renderY + WIDGET2X2H/4;    
        fontSize = WIDGET2X2W / len * 3.3;
      }
      else if (size == SHAPE4X1){
        textX = renderX + WIDGET4X1W/2;
        textY = renderY + WIDGET4X1H/7;            
        fontSize = 40;     
      }

      Label* label = new Label(text, Align::Center,&textX,&textY,fontSize,SIMPLE_TEXT_MAX_LEN,&frontColor,&backColor,&brightness);
      label->angle = angle;
      gui.add(label);

    }

    virtual void pushToGUI(uint8_t layout){
      
      renderError = false;

      frontColor = colorMap[colorID].fColor;
      backColor = colorMap[colorID].bColor;       

      if (x>3 || y>3){
        renderError = true;
        DEBUG_PRINT("\nIncorrect strip location: %dx%d",x,y);
        pushSimpleText(layout, lang.getString("Strip x/y error"));
        return;
      }
            
      if (!determineTopLeft(layout)){
        renderError = true;
        DEBUG_PRINT("\nIncorrect strip size of layout: %s / %d",size.c_str(), layout);
        pushSimpleText(layout, lang.getString("Strip shape error"));
        return;
      }

      // strip size and position are correct, we therefore go ahead and render it

      uint8_t state = interface->state.code;
      
      // if connected, we call the virtual function that display the strip
      if (interface->state.code == IF_READY || interface->state.offline == true){
        if (interface->state.offline == true){
          // if offline , we may want to add a widget indicating it in addition to all the rest which is displayed
          gui.add(new Canva(&GUI::zero, &GUI::zero, TFT_WIDTH, TFT_HEIGHT, &GUI::red, &GUI::transparentColor,&brightness));  
          DEBUG_PRINT("\nStrip %s(%i) is offline",type.c_str(),index);
        }        
        // go ahead with normal rendering
        composeWidgets(layout);
      }else{
        //DEBUG_PRINT("\nStrip %s is syncing",strip->type.c_str());          
        composeInterfaceState(layout);  
        //gui.add (new ProgressBarH(&&stripState.progress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                      
      }

    }

    bool determineTopLeft(uint8_t layout){
      // retrieve the corner of the strip where we can start drawing, which is renderX, renderY
      if (layout == LAYOUT_ZOOM || size == SHAPE4X4){
        renderX = 0;
        renderY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);
      }
      else if (size == SHAPE1X4){
        renderX = x * WIDGET1X4W;
        renderY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);        
      }
      else if (size == SHAPE4X1){
        renderX = 0;
        renderY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET4X1H) ;        
      }
      else if (size == SHAPE2X2){
        renderX = (x * WIDGET2X2W/2);
        renderY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET2X2H/2);
      }else{
        return false;
      }         
      return true;
    }

    virtual void composeInterfaceState(uint8_t layout){
      const String* iconState = &interface->icon; // take the interface icon
      // unless network is not connected
      if (hwm.network->state != CONNECTED) iconState = &GUI::iconMixEthWiFi;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        addFullScreenTextWithIcon(lang.getString(interface->state.label), lang.getString(interface->state.details), lang.getString(interface->state.subLabel), iconState);        
        
        //gui.add (new ProgressBarH(&hwm.network->wiFiConnectAttemptProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                             
        //gui.add (new PendingBar(&oneHundedFloat /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                                                  

      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString(interface->state.label));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString(interface->state.label));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString(interface->state.label));
      }else{

      }      
    }    

    virtual void composeWidgets(uint8_t layout){
      defaultFrame(layout);
    } 

    void defaultFrame(uint8_t layout){
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM) gui.add(new Canva(&renderX, &renderY, WIDGET4X4W, WIDGET4X4H, &GUI::gold, &frontColor,&brightness));  
      else if ( size == SHAPE1X4) gui.add(new Canva(&renderX, &renderY, WIDGET1X4W, WIDGET1X4H, &GUI::gold, &frontColor,&brightness));
      else if (size == SHAPE2X2) gui.add(new Canva(&renderX, &renderY, WIDGET2X2W, WIDGET2X2H, &GUI::gold, &frontColor,&brightness));  
      else if (size == SHAPE4X1) gui.add(new Canva(&renderX, &renderY, WIDGET4X1W, WIDGET4X1H, &GUI::gold, &frontColor,&brightness));      
    }

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;

    int16_t textX = 0, textY = 0;

  protected:
    unsigned long lastCall;     
    //Label* defaultLabel = nullptr;
};

class TwoSelectStrip : public Strip{

  public: 
    TwoSelectStrip(StripSkeleton* s, bool debug=false):Strip(s){
      select2PropertyAddress = s->dataSetAddresses.get(0)->pattern;
    }

    void serializeProperties(){
      char* buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select2PropertyAddress.c_str(),select2ID);
      properties.add(String(buf));
      delete buf;

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){      
      Strip::pushToGUI(layout);
    }    


    void deserializeProperty(String property){
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
        select2Content = skeleton->getValuePtr(interface->getProviderName(),select2PropertyAddress,select2ID-1);
      }
    }  

    virtual const String* getTopLabel(){return &type;}
    virtual const char* getBottomLabel(){return select2Content->c_str();}     
 
  protected:
    Top1Toggle1Label* topLabels;
    uint8_t select2ID=1; 
    const String *select2Content;
    String select2PropertyAddress; 
    String label = "Gain";    
};


class ThreeSelectStrip: public TwoSelectStrip{
  public:
    ThreeSelectStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug){
      select3PropertyAddress = s->dataSetAddresses.get(1)->pattern;
    }
  
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      char* buf = (char*)malloc(select3PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select3PropertyAddress.c_str(),select3ID);
      properties.add(String(buf));
      delete buf;

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      TwoSelectStrip::pushToGUI(layout);
    }    
    

    void deserializeProperty(String property){
      TwoSelectStrip::deserializeProperty(property);
      if (sscanf(property.c_str(), select3PropertyAddress.c_str(), &select3ID)) {
        select3Content = skeleton->getValuePtr(interface->getProviderName(),select3PropertyAddress,select3ID-1);         
      }
    }  

    virtual const String* getTopLabel(){return select2Content;}
    virtual const char* getBottomLabel(){return select3Content->c_str();}      

  protected:
    uint8_t select3ID=1; 
    const String *select3Content;
    String select3PropertyAddress;
    Top2Toggles* topLabels;  
};


#endif