#ifndef STRIPSDEF_h
#define STRIPSDEF_h

#include "Interval.h"
#include "GUIComponents.h"
#include <vector>

using IntervalButton = Interval::Button;

//-------------------------

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

#define MAXNUMBER_OF_SKELPROPERTY 10

class StripSkeleton: public SerializableParameters{
  public:
    unsigned char icon;
    StripBuilderCallback constructor;
    //JsonObject variable;
    using SerializableParameters::serialize;  // 🔴 réexpose String serialize()

    StripSkeleton(String id, String g, StripBuilderCallback b, unsigned char i='!');

    // 🚩 Redéfinition : on enrichit simplement le JsonObject
    void serialize(JsonObject& json) override {
        // Appelle la version de base
        SerializableParameters::serialize(json);
        // Ajoute la donnée spécifique
        json[C_ICON] = icon;
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
        if ((*it)->name == ss->name) {
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
        String stripName = skeleton->name;
        jsonSkeletons->add(skeleton->serialize());
      }
    }

  private:
    StripSkeleton* getStripSkeleton(String stripID){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* s = stripSkeletonRepo[i];        
        if (stripID == s->name){
          return s;
        } 
      }
      return nullptr;
    }
}stripFactory;

StripSkeleton::StripSkeleton(String id, String g, StripBuilderCallback c, unsigned char i):SerializableParameters(id,g, &dataRepo),constructor(c){
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
    //bool redraw = false;

    bool toggle1 = false;
    bool toggle2 = false;
    //bool updated = false;
    uint16_t frontColor, backColor;    

    String iconInterfaceConnect = ICON_FOOTSWITCH;

    String type="undef";

    String interfaceName = "interfaceName"; //
    DeviceInterface* interface = nullptr;

    bool visible = false;

    // logical position
    uint8_t x, y, win, index;
    
    // top left corner position on TFT screen
    int16_t topX, topY;
    
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    //GUIManager* gui;

    float brightness = BRIGHTNESS_NOFOCUS;

    Strip(StripSkeleton* s){
      skeleton = s;
      value = (valueMax-valueMin)/2;
      //id = skeleton->stripID;
    }    

    virtual ~Strip(){
      interface->stripCount -= 1;
    }
  
    virtual uint8_t getNumberOfProperties(){
      return skeleton->parameterAddresses.size();      
    }

    virtual bool actionCallback(uint8_t trigger)=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;        
    virtual void queryInterface() = 0; // this method is the one which is used to force poll data from the interface, 
    // it will usually drop a command to the interface which outcome be lately read by the process() method
    // is called from the focus() method so to force data to be updated in case some whre missed
    virtual const String* getTopLabel()=0;
    virtual const String* getBottomLabel()=0;
    virtual void update() = 0;  // // this code is called by process() if data are availabe and must therefore implement the code that uses what is returned by read() by casting it to the right data structure
    // it assign the GUI values the values that were acquired or processed in the process() method
    // basedon its internal state, it will set the redraw flag 
    
    void process(){ // this code is requesting the strip to update its state (values, etc...) using data from the interface
    // process() is called from main app on all strip: the strip decide then if he has to do the query (for instance if it is focused) as       
    // some strip may do some backgroud tasks (like playing sample or executing a macro), other will only do this if they have focus         
      if (!interface) return;
      if (interface->available()>0) update();   //update is called if data are availabe and must therefore implement data casting to the right data structure
    }; 


    void focus(bool f=true){
      //DEBUG_PRINT("\nStrip(%d)",index);
      if (f){
        brightness = BRIGHTNESS_MAX;
        //DEBUG_PRINT(" has focus");
        visible = true;
      }else{
        brightness = BRIGHTNESS_NOFOCUS;
        //DEBUG_PRINT(" lost focus");
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
        interface->stripCount++;
        //DEBUG_PRINT("\n%s has %i strips",interfaceName.c_str(), interface->stripCount);

        if (colorID>MAX_COLOR_ID-1) colorID = 0;
        frontColor = colorMap[colorID].fColor;
        backColor = colorMap[colorID].bColor;  

        icon = jsonStrip[C_ICON];//.as<unsigned char>();
        properties = jsonStrip[KEY_PROPERTIES];
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
      
      properties = doc[KEY_PROPERTIES].to<JsonArray>();
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
    
    virtual void pushToGUI(uint8_t layout){
      
      visible = true;
      focus(false);
      renderError = false;

      if (!interface){
        renderError = true;
        DEBUG_PRINT("\nStrip interface %s not instanciated");
        pushSimpleText(layout, lang.getString("No interface"));
        return;
      }


      if (x>3 || y>3){
        renderError = true;
        DEBUG_PRINT("\nIncorrect strip location: %dx%d",x,y);
        pushSimpleText(layout, lang.getString("Strip x/y error"));
        return;
      }
            
      if (!determineXY(layout)){
        renderError = true;
        DEBUG_PRINT("\nIncorrect strip size of layout: %s / %d",size.c_str(), layout);
        pushSimpleText(layout, lang.getString("Strip shape error"));
        return;
      }

      // strip size and position are correct, we therefore go ahead and render it
      drawWidgetsFrames(layout);      

      // if connected, we call the virtual function that display the strip
      if (interface->state.code == IF_READY || interface->state.offline == true){
        composeWidgets(layout);
      }else{
        //DEBUG_PRINT("\nStrip %s is syncing",strip->type.c_str());          
        composeInterfaceState(layout);  
      }

    }

    bool determineXY(uint8_t layout){      

      //DEBUG_PRINT("\nstrip[%d] %s topLeft=",index,type.c_str());
      // retrieve the corner of the strip where we can start drawing, which is topX, topY
      if (layout == LAYOUT_ZOOM || size == SHAPE4X4){
        topX = 0;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);
        areaW = WIDGET4X4W;
        areaH = WIDGET4X4H;        
      }
      else if (size == SHAPE1X4){
        topX = x * WIDGET1X4W;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);        
        areaW = WIDGET1X4H;   
        areaH = WIDGET4X1H*0.6; // some margin to avoid text being rendered with overlap on top2label  
      }
      else if (size == SHAPE4X1){
        topX = 0;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET4X1H) ;        
        areaW = WIDGET4X1W;
        areaH = WIDGET4X1H;        
      }
      else if (size == SHAPE2X2){
        topX = (x * WIDGET2X2W/2);
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET2X2H/2);
        areaW = WIDGET2X2W;
        areaH = WIDGET2X2H;
    
      }else{
        return false;
      }         

      centerX = topX + areaW / 2;       //middle
      centerY = topY + (areaH/2) -2;       //middle

      // fix position according to some widget size specificity, but only when layout is DETAILS:
      if (layout == LAYOUT_DETAILS){
        if (size == SHAPE4X1) simpleTextY -= WIDGET4X1H/2-3;     // fix vertical location for flat strip
        else if (size == SHAPE1X4) {
          // special treatment for vertical widget where text is rotated
          centerX = topX + WIDGET1X4W/2;   // exactly middle
          centerY = topY + WIDGET1X4H/2.5; // we want middle but for some reason we have to give it some margin
        }
      }

      //DEBUG_PRINT("(%i,%i)",topX,topY);
      return true;
    }

    #define SIMPLE_TEXT_MAX_LEN 20
    #define MIN_SIMPLETEXT_SIZE 40
    void pushSimpleText(uint8_t layout, const String* textOriginal) {
      // texte limité par SIMPLE_TEXT_MAX_LEN
      int16_t angle = 0;
      uint8_t maxLen = 28;

      int len = textOriginal->length();

     if (size == SHAPE1X4){
        angle = 90;
        maxLen = 15;        
      }else if (size == SHAPE2X2){
        maxLen = 13;        
      }

      #define FONT_SIZE_RATIO 1.5
      int fontSize = areaH * FONT_SIZE_RATIO ;  // affect the maximum size of font possible, which is the widget height

      // Réduire la taille jusqu'à tenir, ou jusqu'à MIN
      while (fontSize > MIN_SIMPLETEXT_SIZE) {
        float charWidth = 0.4f * fontSize;
        float estimatedWidth = charWidth * len;

        if (estimatedWidth <= areaW) break;  // ça rentre, on s'arrête
        fontSize -= 2;                    
      }

      simpleTextX = centerX;
      simpleTextY = centerY;

      // Label GUI
      Label* label = new Label(textOriginal, Align::Center, &simpleTextX, &simpleTextY,
                              fontSize, maxLen,
                              &GUI::white, &backColor, &brightness);
      label->angle = angle;
      gui.add(label);
    }


    virtual void composeInterfaceState(uint8_t layout){
      if (!interface) return;
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        pushIFState4x4();        
        
        //gui.add (new ProgressBarH(&hwm.network->networkConnectAttemptProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                             
        //gui.add (new PendingBar(&oneHundedFloat /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                                                  

      }else if (size == SHAPE1X4){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState1x4();
      }else if (size ==SHAPE2X2 ){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState2x2();
      }else if (size == SHAPE4X1){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState4x1();
      }else{

      }      
    }    

    void pushIFState4x4(){
      if (!interface) return;
      gui.add(new Label(&interface->state.label, Align::Center,&GUI::midScreenHoriz, &GUI::topTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::transparentColor));    
      gui.add(new Led(&GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20, &interface->state.led));      
      gui.add(new Arc(&interface->state.progress, 0,1.0 , &GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20+10, &GUI::gold, &GUI::transparentColor,0));
      gui.add(new Icon(interface->getIcon(),Align::Right,&GUI::rightMenuIconXpos, &GUI::iconY,MEDIUM_ICON_SIZE,&GUI::gold,&GUI::transparentColor));   
      gui.add(new TextArea(&interface->state.details,TFT_WIDTH/2 ,Align::Left,Align::Center,&GUI::midScreenHoriz,&GUI::iconY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::transparentColor));           
      gui.add(new Label(lang.getString("Double clic on/off to toggle offline"), Align::Center,&GUI::midScreenHoriz, &GUI::belowBottomTexty,SIZE_14,40,&GUI::green,&GUI::transparentColor));          
      gui.add(new Label(&interface->state.subState, Align::Center,&GUI::midScreenHoriz, &GUI::bottomTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::defaultBcolor));
    }   

    void pushIFState1x4(){      
      IFledX = topX + WIDGET1X4W / 1.25;
      IFledY = topY + WIDGET1X4H / 2.5;

      IFiconX = topX + WIDGET1X4W / 1.05;
      IFiconY = topY + WIDGET1X4H / 1.8;      

      pushSmallLedandIcon();
    }      


    void pushIFState2x2(){      
      IFledX = topX + WIDGET2X2W / 2.5;
      IFledY = topY + SIZE_20;

      IFiconX = topX + WIDGET2X2W / 1.4;
      IFiconY = topY + SIZE_10 ;

      pushSmallLedandIcon();
    }     
    
    void pushIFState4x1(){      
      IFledX = GUI::leftMenuIconXpos+3;
      IFledY = topY + WIDGET4X1H/2;

      IFiconX = TFT_WIDTH - SIZE_26/2;
      IFiconY = topY + SIZE_26/4;

      pushSmallLedandIcon();
    }     

    void pushSmallLedandIcon(){
      if (!interface) return;
      gui.add(new Led(&IFledX, &IFledY, SIZE_10, &interface->state.led,&brightness));      
      gui.add(new Arc(&interface->state.progress, 0,1.0 , &IFledX, &IFledY, SIZE_16, &GUI::gold, &GUI::transparentColor,0,&brightness));
      gui.add(new Icon(interface->getIcon(),Align::Right,&IFiconX, &IFiconY,SIZE_26,&GUI::gold,&GUI::transparentColor,&brightness));    
    }

    virtual void composeWidgets(uint8_t layout){
      drawWidgetsFrames(layout);
    } 

    void drawWidgetsFrames(uint8_t layout){
      
      if (interface->state.offline) frameColor = TFT_RED;
      else frameColor = TFT_GOLD;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        // having a frame when we are 4x4 is useless unless we want to indicate interface is offline
        //if (interface->state.offline) gui.add(new 
          gui.add(new Canva(&topX, &topY, WIDGET4X4W, WIDGET4X4H,4, &frameColor, &GUI::transparentColor,&brightness));  
      }
      else if ( size == SHAPE1X4) gui.add(new Canva(&topX, &topY, WIDGET1X4W, WIDGET1X4H, 4,&frameColor, &GUI::transparentColor,&brightness));
      else if (size == SHAPE2X2) gui.add(new Canva(&topX, &topY, WIDGET2X2W, WIDGET2X2H,4, &frameColor, &GUI::transparentColor,&brightness));  
      else if (size == SHAPE4X1) gui.add(new Canva(&topX, &topY, WIDGET4X1W, WIDGET4X1H,4, &frameColor, &GUI::transparentColor,&brightness));      
    }

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;

    int16_t simpleTextX = 0, simpleTextY = 0;

    int16_t centerX = 0, centerY = 0;

    uint16_t areaW = 0, areaH = 0;

    int16_t IFledX = 0, IFledY = 0;
    int16_t IFiconX = 0, IFiconY = 0;

  protected:
    unsigned long lastCall;     
    uint16_t frameColor = TFT_GOLD;
    //Label* defaultLabel = nullptr;
};

class TwoSelectStrip : public Strip{

  public: 
    TwoSelectStrip(StripSkeleton* s, bool debug=false):Strip(s){
      Address* select2Address = s->getDataSetAddress(0);
      if (select2Address != nullptr){
        select2PropertyAddress = select2Address->pattern;
      }
    }

    void serializeProperties(){
      char* buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select2PropertyAddress.c_str(),select2ID);
      properties.add(String(buf));
      free(buf);

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){      
      Strip::pushToGUI(layout);
    }    

    void deserializeProperty(String property){
      if (!interface) return;
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
        select2Content = skeleton->getValuePtr(interface->getDataProviderNameForThisInterface(),select2PropertyAddress,select2ID);
      }
    }  

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return select2Content;}     
 
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
      Address* select3Address = s->getDataSetAddress(1);
      if (select3Address != nullptr){
        select3PropertyAddress = select3Address->pattern;
      }
    }
  
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      char* buf = (char*)malloc(select3PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select3PropertyAddress.c_str(),select3ID);
      properties.add(String(buf));
      free(buf);

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      TwoSelectStrip::pushToGUI(layout);
    }    
    

    void deserializeProperty(String property){
      if (!interface) return;
      TwoSelectStrip::deserializeProperty(property);
      if (sscanf(property.c_str(), select3PropertyAddress.c_str(), &select3ID)) {
        select3Content = skeleton->getValuePtr(interface->getDataProviderNameForThisInterface(),select3PropertyAddress,select3ID-1);         
      }
    }  

    virtual const String* getTopLabel(){return select2Content;}
    virtual const String* getBottomLabel(){return select3Content;}      

  protected:
    uint8_t select3ID=1; 
    const String *select3Content;
    String select3PropertyAddress;
    Top2Toggles* topLabels;  
};


#endif
