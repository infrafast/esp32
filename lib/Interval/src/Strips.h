#ifndef STRIPSDEF_h
#define STRIPSDEF_h

#include "Interval.h"
using IntervalButton = Interval::Button;

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

      properties = jsonDocStripSkeleton[STRIP_SKELETON_PROPERTIES].to<JsonArray>();     
      for (uint8_t i = 0; i < dataSetAddresses.size(); i++){

        JsonObject kv = properties.add<JsonObject>();
        kv[dataSetAddresses[i]->pattern] = dataSetAddresses[i]->object;

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

enum {
  STRIP_DISCONNECTED,
  STRIP_PENDING,
  STRIP_CONNECTING,
  STRIP_ONLINE,
  STRIP_UNDEF_STATE,
};

float oneHundedFloat = 100.0;

struct StripState{
  uint8_t code = STRIP_DISCONNECTED;
  const String* stateLabel = &emptyString;
  const String* subStateLabel = &emptyString;
  float progress = 0;
};

class Strip {  
  #define JSON_STRIP_PROPERTY_SIZE 100
  #define  JSON_STRIP_TEMPLATE_SIZE 300
  public:
    TFT_eSprite* sprite = nullptr;

    // flag to indicate if position is out of range
    bool renderError;

    bool toggle1 = false;
    bool toggle2 = false;

    bool updated = false;

    uint16_t frontColor, backColor;    

    String type="undef";

    String interfaceName = SYSTEM_INTERFACE; //

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
      //id = skeleton->stripID;
    }    

    virtual ~Strip(){
    }

    StripState state;
    
    virtual uint8_t getNumberOfProperties(){
      return skeleton->dataSetAddresses.size();      
    }

    virtual bool actionCallback(uint8_t trigger)=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;        
    virtual void queryValues() = 0;  // // this code is called when entering a strip to get its initial state (values, etc...)
    virtual const String* getTopLabel()=0;
    virtual const String* getBottomLabel()=0;
    virtual bool update() = 0;

    void focus(bool f=true){
      if (f){
        brightness = 1.0;
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

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      queryValues();
      renderError = false;

      frontColor = colorMap[colorID].fColor;
      backColor = colorMap[colorID].bColor;       

      if (x>3 || y>3){
        renderError = true;
        return;
      }

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
        renderError = true;
        return;
      }      
   
      composeWidgets();  // call virtual method
    }

    virtual void composeWidgets(){
      if (size == SHAPE4X4) gui.add(new Canva(&renderX, &renderY, WIDGET4X4W, WIDGET4X4H, &GUI::gold, &frontColor,&brightness));  
      else if ( size == SHAPE1X4) gui.add(new Canva(&renderX, &renderY, WIDGET1X4W, WIDGET1X4H, &GUI::gold, &frontColor,&brightness));
      else if (size == SHAPE2X2) gui.add(new Canva(&renderX, &renderY, WIDGET2X2W, WIDGET2X2H, &GUI::gold, &frontColor,&brightness));  
      else if (size == SHAPE4X1) gui.add(new Canva(&renderX, &renderY, WIDGET4X1W, WIDGET4X1H, &GUI::gold, &frontColor,&brightness));
    } 

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;
  protected:
    unsigned long lastCall;     
    uint8_t layout = LAYOUT_DETAILS;
    //Label* defaultLabel = nullptr;
    static DeviceInterface* interface;
};

DeviceInterface* Strip::interface = nullptr;

StripSkeleton* stripTextSkel;
Strip* textStripConstructorCallback();

class TextStrip : public Strip{
  
  #define STRIP_TEXT "text"
  //#define STRIP_TEXT_PROPERTY_VALUE "value"
  
  public:  
        
    TextStrip(StripSkeleton* s,bool debug=false):Strip(s){
      //icon = ICON_TEXT_STRIP;
      textPropertyAddress = s->dataSetAddresses.get(0)->pattern;
      state.code = STRIP_ONLINE;      
    }

    virtual bool update() {     
      // IMPLEMENTS CODE TO PROCESS DATA HERE
      //Type* params = static_cast<Type*>(data);
      return false;
    }

    virtual bool actionCallback(uint8_t trigger){return true;}

    virtual void composeWidgets() override{

      uint8_t fontSize;
      if (size == SHAPE4X4){
        renderX += WIDGET4X4W/2;
        renderY += WIDGET4X4H/4;    
        fontSize = WIDGET4X4W / text.length() * 3.3;
      }
      else if (size == SHAPE1X4){
        renderX += WIDGET1X4W/2;
        renderY += WIDGET1X4H/4;    
        fontSize = WIDGET1X4W / text.length() * 3.3;
      }
      else if (size == SHAPE2X2){
        renderX += WIDGET2X2W/2;
        renderY += WIDGET2X2H/4;    
        fontSize = WIDGET2X2W / text.length() * 3.3;
      }
      else if (size == SHAPE4X1){
        renderX += WIDGET4X1W/2;
        renderY += WIDGET4X1H/4;    
        fontSize = WIDGET4X1W / text.length() * 3.3;
      }

      Label* label = new Label(&text, Align::Center,&renderX,&renderY,fontSize,&frontColor,&backColor,&brightness);
      //widgetHandles.add(label);          
      gui.add(label);
    }

    virtual void queryValues(){
    }

    virtual void deserializeProperty(String property)  {  
      // property = /text/toto   
      // textProperty = /text/%s
      // value = toto
      char buffer[50];

      if (sscanf(property.c_str(), textPropertyAddress.c_str(), buffer) == 1){
        text = String(buffer);
      }           
    }

    virtual void serializeProperties(){
      char* buf = (char*)malloc(textPropertyAddress.length()*sizeof(char*));
      sprintf(buf,textPropertyAddress.c_str(),text.c_str());
      properties.add(String(buf));
      delete buf;

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
    virtual const String* getBottomLabel(){return select3Content;}      

  protected:
    uint8_t select3ID=1; 
    const String *select3Content;
    String select3PropertyAddress;
    Top2Toggles* topLabels;  
};

StripSkeleton *stripMidiSkel, *stripSysexSkel;
Strip* midiStripConstructorCallback();
Strip* sysexStripConstructorCallback();

class MidiStrip : public ThreeSelectStrip{
  #define STRIP_MIDI "MIDI"

  public:
    MidiStrip(StripSkeleton* s,bool debug=false):ThreeSelectStrip(s,debug){

    }

    ~MidiStrip(){};

    virtual bool update() {     
      // IMPLEMENTS CODE TO PROCESS DATA HERE
      return false;
    }
    
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

    virtual void composeWidgets() override{      

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

    virtual void queryValues(){
      
    }

    virtual bool actionCallback(uint8_t trigger){
      return false;
    }
       
  private:
    String unitString = "dB";
    uint16_t midiValue=0;

};


class SysexStrip : public TwoSelectStrip{
  #define STRIP_SYSEX "Sysex"
  #define SYSEX_MAX_MESSAGE_LENGTH 128

  public:
    SysexStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug){

    }

    ~SysexStrip(){};

    virtual bool update() {     
      // IMPLEMENTS CODE TO PROCESS DATA HERE
      return false;
    }

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

    virtual void composeWidgets() override{      

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

    virtual void queryValues(){
      
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




#endif