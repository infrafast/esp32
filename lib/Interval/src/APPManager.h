#ifndef APPManager_h
#define APPManager_h

#include <Preferences.h>
#include "nvs_flash.h" 
#include <LinkedList.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "Interfaces.h"

//#include <esp32FOTA.hpp>

#include <aWOT.h>                 // awOT lasselukkari/aWOT@^3.5.0 wifi+eth server
                                  // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
#include <MenuSystem.h>           // https://github.com/jonblack/arduino-menusystem                        
#include <LittleFS.h>

#include <WebSocketServer.hpp>    // https://github.com/skaarj1989/mWebSockets#meta     // use a local repository with SOCKETBUFFER SIZE CHANGED one because of https://github.com/skaarj1989/mWebSockets/issues/72
using namespace net;
using WiFiWebSocketServer = WebSocketServer<WiFiServer, WiFiClient, 1>;
using EthernetWebSocketServer = WebSocketServer<EthernetServer, EthernetClient, 1>;

using IntervalButton = Interval::Button;

void socketConnectionCallback(IWebSocket &ws);

///////////////////// WIDGET LOCATION & SIZE ////////////////////////
#define OVERLAY_ICON_SIZE TFT_HEIGHT/15

#define WIDGET4X4W (TFT_WIDTH)
#define WIDGET4X4H (TFT_HEIGHT-(TOP2WIDGET_SIZE*2)-OVERLAY_ICON_SIZE-WIDGET_PADDING*2)

#define WIDGET1X4W (TFT_WIDTH/4)
#define WIDGET1X4H (WIDGET4X4H)

#define WIDGET4X1H (WIDGET1X4H/4)
#define WIDGET4X1W (TFT_WIDTH)

#define WIDGET2X2W (TFT_WIDTH/2)
#define WIDGET2X2H (WIDGET4X4H/2)

///////////////////// SYSTEM PARAMETERS ////////////////////////
#define S_PUSHDELAY         "pushDelay"
#define S_DCLICREACTIVITY   "dClicReactivity"
#define S_FADERSPEED        "faderSpeed"
#define S_AP_NAME           "apName"
#define S_LANG              "lang"
#define S_CONFIG_FILE       "profile"
#define S_SSID              "ssid"
#define S_LAST_USED_SSID    "lastSSID"
#define S_BUTTON_ASSIGN     "buttonsAssignements"
#define S_SHORTCUTS         "shortcuts"

///////////////////// CONFIG PARAMETERS ////////////////////////

#define C_CURRENTLAYOUT     "currentLayout"
#define C_POSSIBLELAYOUT    "possibleLayout"
#define C_BANKS             "banks"
  #define C_STRIPS          "strips"
    #define C_PROPERTIES    "properties"
    #define C_STRIPTYPE     "type"
    #define C_INDEX         "id"
    #define C_X             "x"
    #define C_Y             "y"
    #define C_WIN           "win"
    #define C_SIZE          "size"
    #define C_POSITIONS     "positions"
#define C_ICON              "icon"
#define C_COLOR             "color"

///////////////////// VOLATILE PARAMETERS ///////////////////////
//#define V_USE_NETWORK       "myNetworkIP"

// C to C++ class function wrapper
void commonFileServerWrapper(Request &req, Response &res);  
void unpackProgressCallback( uint8_t progress );
void tarStatusProgressCallback( const char* name, size_t size, size_t total_unpacked );

//void gzFileServer(Request &req, Response &res);
void scanNetworksTaskWrapper(void * parameter);         // parrallel task
void webServerTaskWrapper(void * parameter);            // parrallel task
void defaultWizardStepFlow();



// Prototypage de la fonction de gestion de la connexion
//void handleWebConnection(void *parameter);

#define VOID_JSON_ITEM "\"void\": 0"

///////////////////// SCREEN APPLICATION MANAGEMENT ////////////////////////

//main screen (id)
enum{
  SCREEN_STARTUP,
  SCREEN_WIZARD,
  SCREEN_MENU,
  SCREEN_BANK_SWITCH,
  SCREEN_PROMPT,
  SCREEN_UTILS,
  SCREEN_BYE,
  SCREEN_BANK,
  SCREEN_MAX
};

//sub screen (state) 
enum{
  STATE_PROMPT_SEQUENCE_X,
  STATE_PROMPT_SEQUENCE_Y,
  STATE_PROMPT_SEQUENCE_Z,
  STATE_WIZARD_LANG_STEP1,
  STATE_WIZARD_CONNECT_CONFIG_STEP2,
  STATE_WIZARD_CONNECT_NETWORK_STEP3,
  STATE_WIZARD_USER_STEP4,
  STATE_UTILS_SEARCHING_CONNECTION,
  STATE_UTILS_INFO,
  STATE_UTILS_BRIGHTNESS,
  STATE_UTILS_CONNECTCONFIGMEDIA,
  STATE_UTILS_CONNECTWEBAPP,
  STATE_UTILS_WEBAPPCONNECTED,
  STATE_BYE_GOSLEEP_MESSAGE,
  STATE_BYE_MODAL_MESSAGE,
};


void startupScreenBuilder(uint8_t id);
void wizardScreenBuilder(uint8_t id);
void menuScreenBuilder(uint8_t id);
void bankSwitchScreenBuilder(uint8_t id);
void promptScreenBuiler(uint8_t id);
void utilsScreenBuilder(uint8_t id);
void byeScreenBuilder(uint8_t id);
void bankScreenBuilder(uint8_t id);
void voidOverlay(uint8_t id);

void baseOverLayBuilder(uint8_t id);
void bankOverlayBuilder(uint8_t id);

screenBuilder screens[SCREEN_MAX]={
  {SCREEN_STARTUP,startupScreenBuilder,baseOverLayBuilder},
  {SCREEN_WIZARD,wizardScreenBuilder,baseOverLayBuilder},
  {SCREEN_MENU,menuScreenBuilder,baseOverLayBuilder},
  {SCREEN_BANK_SWITCH,bankSwitchScreenBuilder,baseOverLayBuilder},
  {SCREEN_PROMPT,promptScreenBuiler,baseOverLayBuilder},
  {SCREEN_UTILS,utilsScreenBuilder,baseOverLayBuilder},  
  {SCREEN_BYE,byeScreenBuilder,voidOverlay},
  {SCREEN_BANK,bankScreenBuilder,bankOverlayBuilder},
};

struct JsonAnswer{
  String callback;
  String value = String(true);
};

/*
struct JsonUserEvent{
  JsonDocument jsonInput;
  JsonAnswer& jsonAnswer;
};
*/

typedef bool (*ParseJsonInputCallback)(JsonObject&,JsonAnswer*);
//typedef void (*NetworkChangeStateCallback)(bool state);

bool defaultParseJsonInputCallback(JsonObject& jsonObject,JsonAnswer* jsonAnswer){
  String jsonInputString;
  serializeJson(jsonObject, jsonInputString);
  return false;
}

String defaultStringReturn(){return "";};
void defaultVoidReturn(){};
void defaultVoidInt(int i){};
void defaultVoidDoubleInt(int i,int j){};
void defaultVoidBool(bool state){}
void defaultWizardScreenBuilder();
void defaultWizardStepFlow();
void defaultBootScreen();

void info_item_selected(MenuComponent* p_menu_component);
void config_menu_item_selected(MenuComponent* p_menu_component);
void back_menu_item_selected(MenuComponent* p_menu_component);
void save_profile_item_selected(MenuComponent* p_menu_component);
void loadLangFromMenu_item_selected(MenuComponent* p_menu_component);
void switchToBank_item_selected(MenuComponent* p_menu_component);
void tft_rotation_item_selected(MenuComponent* p_menu_component);
void reset_item_selected(MenuComponent* p_menu_component);
void shutdown_menu_item_selected(MenuComponent* p_menu_component);
void menu_selected(MenuComponent* p_menu_component);
void brightness_menu_item_selected(MenuComponent* p_menu_component);

void handleConfigConnection();  // 




///////////////////// DATA MANAGEMENT ////////////////////////
#include <vector>

class DataProvider{
  public :
    virtual uint16_t getSize(String addrr) = 0;
    virtual const String* getValuePtr(String addrr, uint16_t index) = 0;
  protected:
    String voidStatic="voidStatic";
};


class DataSet{
  public:
    String address;
    String object;

    ~DataSet(){
    }

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() = 0;
    virtual const String* getValuePtr(uint16_t i) = 0;

    DataSet(const String& addr,const String& obj): address(addr), object(obj) {}

    virtual String serialize(){
      JsonDocument doc;
      // a dataset either return an array of value from a data provider (size>0)
      // or a no value (size=0) which is the name of a javascript function callback
      uint16_t n = getSize();
      if (n>0){
        JsonArray jsonValues = doc[address].to<JsonArray>();
        for (uint8_t i=0; i< n; i++) jsonValues.add(*getValuePtr(i));
      }else{
        doc[address] = *getValuePtr(0);
      }

      String data;
      serializeJson(doc,data);
      return data;
    }     
};

class StaticDataSet : public DataSet{
  public:

    StaticDataSet(const String& addr,const String& obj): DataSet(addr, obj){}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize(){
        return size;
    }

  protected:
    uint16_t size=0;
};




class DynamicDataSet : public DataSet{
  public:
    DataProvider* dataProvider;    

    DynamicDataSet(const String& addr,const String& obj, DataProvider* dp): dataProvider(dp), DataSet(addr, obj) {}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() {
        return (dataProvider) ? dataProvider->getSize(address) : 0;
    }

    virtual const String* getValuePtr(uint16_t i) {
        return (dataProvider) ? dataProvider->getValuePtr(address, i) : nullptr;
    }
};


class DataRepository{
  public:
    DataRepository(){}

    DataSet* getDataSet(int i){
      if (i<size()) return dataSets[i];
      else return nullptr;
    }

    DataSet* getDataSet(String address){
      for (uint8_t i=0;i<dataSets.size();i++){
        DataSet* ds = dataSets[i];        
        if (address == ds->address){          
          return ds;
        } 
      }
      return nullptr;
    }

    bool addDataSet(DataSet* ds){
      dataSets.push_back(ds);
      /*String object = ds->object;
      String address = ds->address;      */
      return true;
    }

    void removeDataSet(DataSet* ds) {
      for (auto it = dataSets.begin(); it != dataSets.end(); ++it) {
        if ((*it)->address == ds->address) {
          dataSets.erase(it);
          break;
        }
      }
    }


    void serialize(JsonArray* jsonDataSets){
      for (uint8_t i=0;i<dataSets.size();i++){        
        DataSet* ds = dataSets[i];
        jsonDataSets->add(ds->serialize());
      }
    }

    int size(){
      return dataSets.size();
    }

  private:
    //LinkedList<DataSet*> dataSets = LinkedList<DataSet*>();
    std::vector<DataSet*> dataSets;
};


class StripSkeleton;
///////////////////// STRIP MANAGEMENT ////////////////////////
#define INPUTSELECT "dropdown"
#define INPUTTEXT   "freetext"

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

    // logical position
    uint8_t x, y, win, index;
    
    // position on TFT screen
    int16_t renderX, renderY;
    
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    GUIManager* gui;

    float brightness = 1.0;

    Strip(StripSkeleton* s, GUIManager* g){
      skeleton = s;
      gui = g;
      value = (valueMax-valueMin)/2;
      //id = skeleton->stripID;
    }    

    virtual ~Strip(){
    }

    StripState state;
    
    virtual uint8_t getNumberOfProperties()=0;
    virtual bool actionCallback(uint8_t trigger)=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;        
    virtual void queryState() = 0;  // // this code is called when entering a strip to get its initial state (values, etc...)
    virtual const String* getTopLabel()=0;
    virtual const String* getBottomLabel()=0;       
    virtual bool update(JsonObject& data) = 0;

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
      queryState();
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
      if (size == SHAPE4X4) gui->add(new Canva(&renderX, &renderY, WIDGET4X4W, WIDGET4X4H, &GUI::gold, &frontColor,&brightness));  
      else if ( size == SHAPE1X4) gui->add(new Canva(&renderX, &renderY, WIDGET1X4W, WIDGET1X4H, &GUI::gold, &frontColor,&brightness));
      else if (size == SHAPE2X2) gui->add(new Canva(&renderX, &renderY, WIDGET2X2W, WIDGET2X2H, &GUI::gold, &frontColor,&brightness));  
      else if (size == SHAPE4X1) gui->add(new Canva(&renderX, &renderY, WIDGET4X1W, WIDGET4X1H, &GUI::gold, &frontColor,&brightness));
    } 

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;
  protected:
    unsigned long lastCall;     
    uint8_t layout = LAYOUT_DETAILS;
    //Label* defaultLabel = nullptr;
};

typedef Strip* (*StripBuilderCallback)(/*String values*/);

#define MAXNUMBER_OF_SKELETON 20

class StripSkeleton{
  #define STRIP_DEFAULT_GROUP "utils"
  #define MAXNUMBER_OF_SKELPROPERTY 10
  #define STRIP_SKELETON_PROPERTIES "properties"

  #define JSON_SKELETON_SIZE 30*MAXNUMBER_OF_SKELPROPERTY
  public:
    String stripID;
    unsigned char icon;
    String group;
    StripBuilderCallback constructor;
    //JsonObject variable;
    LinkedList<String> dataSetAddresses = LinkedList<String>();
    static DataRepository* dataRepo;

    uint8_t numberOfProperties = 0;  

    StripSkeleton(String id, String g, StripBuilderCallback b, unsigned char i='!'){
      stripID = id;
      group = g;
      icon = i;
      constructor = b;
    }

    const String* getValuePtr(String address, uint8_t index){
      DataSet* ds = dataRepo->getDataSet(address);
      if (ds != nullptr) return ds->getValuePtr(index);
      else return &emptyString;
    }

    bool registerDataSetAddress(String address){
      dataSetAddresses.add(address); 
      numberOfProperties++;
      if (numberOfProperties>MAXNUMBER_OF_SKELPROPERTY) return false;  
      return true;
    }

    String serialize(){
      JsonDocument jsonDocStripSkeleton;
      JsonArray properties;
      jsonDocStripSkeleton[stripID] = group;
      jsonDocStripSkeleton[C_ICON] = icon;

      properties = jsonDocStripSkeleton[STRIP_SKELETON_PROPERTIES].to<JsonArray>();     
      for (uint8_t i = 0; i < numberOfProperties; i++){
        DataSet* ds = dataRepo->getDataSet(dataSetAddresses.get(i));
        if (ds != nullptr){
          JsonObject kv = properties.add<JsonObject>();
          kv[ds->address] = ds->object;
        }         
      }
      String data;
      serializeJson(jsonDocStripSkeleton, data);
      return data;
    }

};

// default skeleton
StripSkeleton* stripTextSkel;
Strip* textStripConstructorCallback();

class TextStrip : public Strip{
  
  #define STRIP_TEXT "text"
  //#define STRIP_TEXT_PROPERTY_VALUE "value"
  
  public:  
        
    TextStrip(StripSkeleton* s,GUIManager* g,bool debug=false):Strip(s,g){
      //icon = ICON_TEXT_STRIP;
      textPropertyAddress = s->dataSetAddresses.get(0);
      state.code = STRIP_ONLINE;
    }

    virtual bool update(JsonObject& data) {     
      // IMPLEMENTS CODE TO PROCESS DATA HERE
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
      gui->add(label);
    }

    virtual void queryState(){
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

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
    }

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
    TwoSelectStrip(StripSkeleton* s,GUIManager* g, bool debug=false):Strip(s,g){
      select2PropertyAddress = s->dataSetAddresses.get(0);
      select2Content = skeleton->getValuePtr(select2PropertyAddress,select2ID-1);
    }

    void serializeProperties(){
      char* buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select2PropertyAddress.c_str(),select2ID);
      properties.add(String(buf));
      delete buf;

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      select2Content = skeleton->getValuePtr(select2PropertyAddress,select2ID-1);
      Strip::pushToGUI(layout);
    }    


    void deserializeProperty(String property){
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
      }    
    }  

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
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
    ThreeSelectStrip(StripSkeleton* s,GUIManager* g, bool debug=false):TwoSelectStrip(s,g,debug){
      select3PropertyAddress = s->dataSetAddresses.get(1);
      select3Content = skeleton->getValuePtr(select3PropertyAddress,select3ID-1);      
    }
  
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      char* buf = (char*)malloc(select3PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select3PropertyAddress.c_str(),select3ID);
      properties.add(String(buf));
      delete buf;

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      select3Content = skeleton->getValuePtr(select3PropertyAddress,select3ID-1); 
      TwoSelectStrip::pushToGUI(layout);
    }    
    

    void deserializeProperty(String property){
      TwoSelectStrip::deserializeProperty(property);
      if (sscanf(property.c_str(), select3PropertyAddress.c_str(), &select3ID)){
      }  

    }  

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
    }

    virtual const String* getTopLabel(){return select2Content;}
    virtual const String* getBottomLabel(){return select3Content;}      

  protected:
    uint8_t select3ID=1; 
    const String *select3Content;
    String select3PropertyAddress;
    Top2Toggles* topLabels;  
};

class StripFactory{
  public:
    LinkedList <StripSkeleton*> stripSkeletonRepo = LinkedList<StripSkeleton*>();
    //std::vector<StripSkeleton*> stripSkeletonRepo;
    //StaticJsonDocument<10000> stripsDefinitions;

    bool add(StripSkeleton* ss){
      if (stripSkeletonRepo.size()>=MAXNUMBER_OF_SKELETON) return false;
      stripSkeletonRepo.add(ss);
      return true;
    }

    Strip* build(String stripID/*, String values*/){
      Strip* strip = nullptr;
      StripSkeleton* skel = getStripSkeleton(stripID);
      if (skel != nullptr) strip = skel->constructor(/*values*/);
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
        StripSkeleton* skeleton = stripSkeletonRepo.get(i);
        String stripName = skeleton->stripID;
        jsonSkeletons->add(skeleton->serialize());
      }
    }

  private:
    StripSkeleton* getStripSkeleton(String stripID){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* s = stripSkeletonRepo.get(i);        
        if (stripID == s->stripID){
          return s;
        } 
      }
      return nullptr;
    }
};

///////////////////// BANKS MANAGEMENT ////////////////////////
namespace Interval {
  class Bank{  
    #define BANK_MAXWINDOW 3
    #define STRIP_PER_WINDOW 4
    #define BANK_MAX_STRIPS BANK_MAXWINDOW*STRIP_PER_WINDOW
  
    public:
      uint8_t selector=1; 
      uint8_t oldSelector=0;
   
      //uint8_t numberOfStrips = 0;
      //uint8_t maxWindow = 0;
  
      String name;
      unsigned char icon=' ';
  
      uint8_t possibleLayout;
      uint8_t currentLayout;
      
      //LinkedList<Strip*> strips;
      std::vector<Strip*> strips; // Vecteur de strips
  
      Bank(String n, String value, StripFactory* sf){ 
        name = n;
        //Strip* s = nullptr;
        //for (size_t i = 0; i < BANK_MAX_STRIPS; ++i) strips[i] = nullptr; //strips.add(s); // strips.push_back(nullptr);
        stripFactory = sf;
        deserialize(value);
      }
  
      ~Bank(){
        for (uint8_t i=0; i<strips.size(); i++){
          if (strips[i] != nullptr ) delete strips[i];
        }      
      }
  
      void resetStripNav(){
        selector = 1;
        /*if (strips.size() > 0){   //bank is not empty
          // find the strip which has the lower index;
          for (uint8_t i=0;i<strips.size();i++) nav.selector = min(nav.selector,strips[i]->index);  
          nav.window = strips[nav.selector]->win;
        }else nav.selector = 1;*/
      }
  
      void init(){
        //set stripIndex to the first available strip
        resetStripNav();
      }
  
      /*uint8_t getMaxWindow(){
        //determine how many strips are in use ... means if there are free strip,in between (value nullptr) we skip but as soon as we found one not 0 we set it to the current index
        uint8_t numberOfBusyStrip = 0;
        for (uint8_t i = 0; i < BANK_MAX_STRIPS; i++) {
          if (strips[i] != nullptr) {
            numberOfBusyStrip = i;
          }
        }
        uint8_t newMaxWindow = numberOfBusyStrip / STRIP_PER_WINDOW + 1;
        return newMaxWindow;
      }*/
  
      bool deserialize(String value){
        bool success = false;
        //DynamicJsonDocument bank(JSON_BANK_SIZE);
        JsonDocument bank;
        DeserializationError error = deserializeJson(bank, value);
        if (error){
  
        }else{
  
          possibleLayout = bank[C_POSSIBLELAYOUT];
          currentLayout = bank[C_CURRENTLAYOUT];
          icon = bank[C_ICON].as<unsigned char>();
          
  
          JsonArray stripArray = bank[C_STRIPS];
          for (int i=0; i<stripArray.size() && i<BANK_MAX_STRIPS; i++){        
            JsonObject jsonStrip = stripArray[i];        
            if (!addStrip(jsonStrip)) return false;
          }
          init();
          success = true;
        }
        return success;
      }
  
      /*bool insertStrip(JsonObject jsonStrip){
  
      }*/
  
      Strip* buildStrip(JsonObject jsonStrip){      
        String stripID = jsonStrip[C_STRIPTYPE];
        Strip* strip = stripFactory->build(stripID);  // build the strip by passing values to deserialize
        return strip;
      }
  
      Strip* addStrip(JsonObject jsonStrip){
        Strip* strip = buildStrip(jsonStrip);
        if (strip != nullptr){    
          String jsonStripString;
          serializeJson(jsonStrip, jsonStripString);                 
          if (strip->deserialize(jsonStripString)){
            strips.push_back(strip);
          }  
        }
        return strip;
      }
  
      /*void add(Strip* strip, uint8_t index){
        if (index<BANK_MAX_STRIPS){
  
          strips[index] = strip;
        }
      }*/
  
      Strip* getStrip(uint8_t x, uint8_t y, uint8_t window){
  
        Strip* strip = nullptr;
        // retrieve strip at given position
        uint8_t i = 0;
        while (i<strips.size()){
  
          if (strips[i]->x == x && strips[i]->y == y && strips[i]->win == window){
  
            strip = strips[i];
            break;
          }
          i++;
        }
        return strip;
      }
  
      Strip* getStrip(uint8_t id){
  
        if (id>strips.size()) return nullptr;
  
        // retrieve strip at given position
        
        for (const auto& strip : strips) {
          if (strip->index == id) return strip;
        }
  
        return nullptr;
      }
  
      bool trash(Strip* strip){
        bool success = false;
        for (uint8_t i=0;i<strips.size();i++){
          if (strips[i] == strip){
  
            //strips.remove(i);
            strips.erase(std::remove(strips.begin(), strips.end(), strip), strips.end());          
            delete strip;
            success = true;
            break;
          }
        }
        return success;
      }
  
      String serialize(){
        //DynamicJsonDocument doc(JSON_STRIP_TEMPLATE_SIZE*BANK_MAX_STRIPS);   
        JsonDocument doc;
  
        doc[C_POSSIBLELAYOUT] = possibleLayout;
        doc[C_CURRENTLAYOUT] = currentLayout;
        doc[C_ICON] = icon;
  
        //JsonArray jsonStrips = doc.createNestedArray(C_STRIPS);            
        JsonArray jsonStrips = doc[C_STRIPS].to<JsonArray>();            
        for (uint8_t i = 0; i<strips.size(); i++){
          if (strips[i] != nullptr){
            jsonStrips.add(strips[i]->serialize());
          }
        }
        String data;
        serializeJson(doc, data);
        removeJsonQuote(&data);
  
        return data;
      }
  
    private:
      StripFactory* stripFactory;
      //std::vector<Strip*> strips;
      //Strip* strips[BANK_MAX_STRIPS];
  
  
  };
}

using IntervalBank = Interval::Bank;  

class BankNavigator: public Widget{
  //#define BANK_NAME_SIZE TFT_WIDTH/15
  public:

    BankNavigator(IntervalBank* b/*, LinkedList<Widget*>* w*/, bool debug = false):Widget(&xPos, &yPos, &fC, &bC, &GUI::fullBrightness, debug){      
      bank = b;
      //widgetList = w;
      oldSelector = bank->selector - 1; //this is to force first refresh
      width = TFT_WIDTH/2; height = OVERLAY_ICON_SIZE;
      id = "BankNavigator"+String(xPos)+"x"+String(yPos); 

    }

    virtual bool refresh(bool force = false) {
      refreshed = force;
      Widget::refresh(force);

      if (bank == nullptr) return false;

      // Check if an update is needed
      bool bankChanged = (bank->selector != oldSelector) || 
                        (bank->strips.size() != oldNumberOfStrips) || 
                        (id != bank->name) || refreshed;

      if (!bankChanged) return false;

      refreshed = true;
      oldNumberOfStrips = bank->strips.size();
      id = bank->name;
      oldSelector = bank->selector;

      if (debug) {

      }

      // Define UI sizes
      #define NAV_HEIGHT OVERLAY_ICON_SIZE
      #define WIN_WIDTH OVERLAY_ICON_SIZE * 1.6
      #define YPADDING 1
      #define XPADDING 2
      #define SPACE_BETWEEN_WIN  3
      #define NAV_WIDTH (WIN_WIDTH * BANK_MAXWINDOW) + (SPACE_BETWEEN_WIN * BANK_MAXWINDOW)
      #define CELL_WIDTH WIN_WIDTH / STRIP_PER_WINDOW
      #define CELL_HEIGHT NAV_HEIGHT / STRIP_PER_WINDOW
      #define CANVA_VS_BANKNAME_SPACE 2

      uint16_t navFrameColor = GUI::luminance(GUI::gold, 100);
      
      // Create a canvas for the navigation UI
      Canva* canva = new Canva(&GUI::zero, &GUI::zero, NAV_WIDTH, NAV_HEIGHT, &GUI::defaultBcolor, &GUI::defaultBcolor);
      canva->refresh(true);

      // Draw each strip on the navigation UI
      drawStripMap(canva);

      // Adjust width
      width = canva->width + CANVA_VS_BANKNAME_SPACE;

      // Display focused strip ID
      //drawStripID(canva);

      // Display bank name
      Label* label = new Label(&bank->name, Align::Right, &GUI::zero, &GUI::zero, OVERLAY_ICON_SIZE * 2, &fC, &bC, brightness, debug);
      label->refresh(true);
      width += label->width;
      height = max(label->height, canva->height);
      width = max(width, oldWidth);
      oldWidth = width;

      createSprite();
      
      // Push canvas and label to the main sprite
      canva->sprite->pushToSprite(sprite, 0, 2);
      label->sprite->pushToSprite(sprite, canva->width + CANVA_VS_BANKNAME_SPACE, -OVERLAY_ICON_SIZE / 5);

      // Clean up dynamically allocated objects
      delete canva;
      delete label;

      return refreshed;
    }

    virtual int16_t getX(){
      return xPos;
    }

    void drawStripMap(Canva* canva){
      for (uint8_t i = 0; i < bank->strips.size(); i++) {
        Strip* strip = bank->strips[i];
        if (strip == nullptr) continue;

        // Calculate strip position
        uint8_t cellX = (strip->x * CELL_WIDTH) + (strip->win * WIN_WIDTH) + (SPACE_BETWEEN_WIN * strip->win);
        uint8_t cellY = strip->y * CELL_HEIGHT;

        // Extract width & height from "3x2" style string
        int index = strip->size.indexOf('x');
        int widgetWidth = strip->size.substring(0, index).toInt();
        int widgetHeight = strip->size.substring(index + 1).toInt();

        uint8_t cellWidth = (widgetWidth * CELL_WIDTH) - XPADDING;
        uint8_t cellHeight = (widgetHeight * CELL_HEIGHT) - YPADDING;

        // Draw strip on the canvas
        canva->sprite->fillRect(cellX + XPADDING, cellY + YPADDING, cellWidth, cellHeight, GUI::luminance(colorMap[strip->colorID].fColor, strip->brightness * GUI::screenBrightness * 255));
        canva->sprite->drawRect(cellX, cellY, cellWidth + XPADDING, cellHeight + YPADDING,  GUI::luminance(colorMap[strip->colorID].bColor, strip->brightness * GUI::screenBrightness * 255));
      }      
    }

    void drawStripID(Canva* canva){
      String idStr = String(bank->selector);
      Label* uniqueID = new Label(&idStr, Align::Center, &GUI::zero, &GUI::zero, OVERLAY_ICON_SIZE * 2, &GUI::white,&GUI::defaultBcolor);
      uniqueID->refresh(true);
      uniqueID->sprite->pushToSprite(canva->sprite, canva->width / 2 - NAV_HEIGHT / 2, -5);
      delete uniqueID;
    }

  protected:
    IntervalBank* bank;
    //LinkedList<Widget*>* widgetList;
    uint8_t oldSelector=255;
    int8_t oldNumberOfStrips = 0;    
    uint16_t oldWidth=0;

    //int16_t xPos = TFT_WIDTH/2, yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE;
    int16_t xPos = OVERLAY_ICON_SIZE*2 , yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE-3;
    uint16_t fC = GUI::defaultFcolor, bC = GUI::defaultBcolor;

};

///////////////////// JSON AND SERIALIZATION  ////////////////////////

class JsonListFile : public JsonList{
  public:
      String filename="voidconf.conf";

      JsonListFile():JsonList(){};

      bool load(String f,String defaultValues=""){

        // We first clean up existing values
        /*while (keyValues.size()>0){

          KeyValue* kv = keyValues.shift();

          delete(kv);
        }*/
        bool success = false;

        if (defaultValues != "") deserialize(defaultValues);

        filename = f;
        // we try to allocate maximum memory and keep 20K free for security
        //DynamicJsonDocument doc(getMaxAlloc());
        JsonDocument doc;

        if (!LittleFS.exists(filename)){

        }else{
          File file = LittleFS.open(filename);
          DeserializationError error = deserializeJson(doc, file);      
          if (!error){
            jsonToKeyValues(&doc);
            success = true;
          }else{

          }
          doc.shrinkToFit();   

        }
        return success;      
      }

      bool save(String f=""){
        bool success = true;
        if (f !="") filename = f;
        String s;
        serialize(&s);

        File file = LittleFS.open(filename, FILE_WRITE);
        if(!file){

            success = false;
        }
        if(!file.print(s)){

            success = false;
        }else{

        }
        file.close();  
        return success;
      }
};

class Config : public JsonListFile{
  public:
    Config():JsonListFile(){}  

    const int getInt(String name) {
      return getString(name,"0")->toInt();
    };
    const float getFloat(String name) {
      return getString(name,"0")->toFloat();
    };
    const boolean getBool(String name) {
      return (*getString(name,"0") != "0");
    };

};

class I18n : public JsonListFile{
  public:
    I18n():JsonListFile(){}

    virtual const String* getString(const String name, const String defaultValue="") {
      int8_t index;
      index = getIndex(String(name));
      if (index < 0) {

        addValue(name,name);  // if it doesnt exist we create it in the list
        return getString(name);
      } else {
        
        KeyValue* kv = keyValues.get(index);  

        return &kv->value;
      }
    };
};

class StripHeaderWidget: public Top2Toggles{
  public:

    StripHeaderWidget(Strip* s,I18n* i18n, bool debug=false):lang(i18n),strip(s),Top2Toggles(&topLabel,&bottomLabel,&topReverse ,&bottomReverse,&topFcolor, &topBcolor , &GUI::defaultFcolor, &GUI::silver,&GUI::fullBrightness,debug){}
    //StripHeaderWidget(Strip* s,I18n* i18n, bool debug=false):lang(i18n),strip(s),Top2Toggles(&topLabel,&bottomLabel,&topReverse ,&bottomReverse,&GUI::red, &GUI::green , &GUI::blue, &GUI::yellow,&GUI::fullBrightness,debug){}

    virtual bool refresh(bool force=false){    
      
      if (strip == nullptr) return refreshed;
           
      if (oldStrip != strip || strip->updated || force){
        strip->updated = false; // we acknowledge the strip change
        oldStrip = strip;
        setValues();
        refreshed = true;
      }

      Top2Toggles::refresh(force);

      return refreshed;
    }

    void setStrip(Strip* s) {
      if (strip != s) {
        strip = s;
      }
    }    

  private:
    Strip* strip = nullptr;
    Strip* oldStrip = nullptr; 

    I18n* lang = nullptr;

    String topLabel = "";
    bool topReverse = false;    
    uint16_t topFcolor = GUI::defaultFcolor;
    uint16_t topBcolor = GUI::defaultBcolor;
        
    String bottomLabel = "";
    bool bottomReverse = false;

    void setValues(){            
      //DEBUG_PRINT("\n setValues()");
      topReverse = strip->toggle1;      
      if (strip->renderError) topLabel = *lang->getString("Strip render error");
      else topLabel = *strip->getTopLabel();
      topFcolor = strip->frontColor;
  
      bottomReverse = strip->toggle2;
      bottomLabel = *strip->getBottomLabel();
      topBcolor = strip->backColor;      
    }    
};


#define ADDRESS_SIMPLETEXT "/text/%s"
#define ADDRESS_STRIPIDS  "/strip/id/%s"


class DefaultDynamicProvider: public DataProvider{
  public :
    
    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_SIMPLETEXT) return 1;
      else if (addrr == ADDRESS_STRIPIDS) return 0;
      else return 0;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_SIMPLETEXT){
        simpleText = "hello"+String(index);
        return &simpleText;
      } 
      else if (addrr = ADDRESS_STRIPIDS){
        return &getStripID;         //javascript local function callback
      } 
      else return &voidStatic;
    }

  private:
    String simpleText, getStripID="getStripID";

};

class Shortcuts{
  public:
    void serialize(String* buffer){

      *buffer = "[{\"jump\":\"shortcut\",\"properties\":[{\"/strip/id/%s\":\"dropdown\"}]},{\"macro\":\"shortcut\",\"properties\":[{\"/action/%s\":\"dropdown\"},{\"/macro/name/%s\":\"dropdown\"}]},{\"advanced\":\"shortcut\",\"properties\":[{\"/command/%s\":\"freetext\"}]}]";

      removeJsonQuote(buffer);

    }    
};

///////////////////// MAIN APPLICATION CLASS ////////////////////////

class ApplicationManager : public GenericComponent,MenuComponentRenderer,public EventDispatcher{
  public:   

    Event evt;    

    Application* webServer = nullptr;
    bool  httpServing = false;  //flag to indicate if http file are currently streamed
    ///////////////////// USER CALLBACKS ////////////////////////////////////////////////////
    ParseJsonInputCallback userParseJsonInputCallback = defaultParseJsonInputCallback;    
    ReturnStringCallback userJsonPongCallback = defaultStringReturn;
    VoidCallback userWizardstepFlow = defaultWizardStepFlow;
    VoidCallback bootScreen = defaultBootScreen;
    VoidDoubleIntCallback userScreenBuilder = defaultVoidDoubleInt;

    ///////////////////// STRIPS & BANKS CONTAINERS AND DEFINITION ////////////////////////
    LinkedList<IntervalBank*> banks = LinkedList<IntervalBank*>(); 
    StripFactory stripFactory;
    DataRepository dataRepository;
    
    DefaultDynamicProvider defaultDynamicProvider;
    DynamicDataSet simpleText = DynamicDataSet(ADDRESS_SIMPLETEXT,INPUTTEXT,&defaultDynamicProvider);
    DynamicDataSet stripIDs = DynamicDataSet(ADDRESS_STRIPIDS,INPUTSELECT,&defaultDynamicProvider);
    
    ///////////////////// OVERLAY ICONS ////////////////////////
    String networkIcon, apIcon;
    uint16_t networkIconColorIdle = TFT_RED, apIconColor = TFT_RED, batIconColor = TFT_GREEN, leftIconColor = GUI::menuActiveIconColor;   

    String batteryIcons[BAT_MAX_LEVEL]={ICON_BAT_LEVEL_0,ICON_BAT_LEVEL_1,ICON_BAT_LEVEL_2,ICON_BAT_LEVEL_3};
    String batIcon = ICON_WAIT; // by default we display a timeelapser while battery level is undetermined
    String* byeIcon = &iconForbid; const uint16_t* byeIconColor = &GUI::defaultFcolor; 
    String iconReset = ICON_RESET;
    String iconForbid = ICON_FORBID;
    String powerIcon = ICON_POWER; 
    String iconX = "X"; 
    String footSwitchIcon = ICON_FOOTSWITCH;
    String intervalIcon = ICON_INTERVAL;

    AlteredColor networkIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor apIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor configIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    
    int16_t overlayIconY = TFT_HEIGHT-OVERLAY_ICON_SIZE, apIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE*2;
    int16_t batIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE;

    float pendingBarWidth = TFT_WIDTH;

    ///////////////////// WIDGETS & ATTRIBUTES ////////////////////////
    #define WIDGET_SMALL_WIDTH TFT_WIDTH/8-5
    #define WIDGET_BIG_WIDTH TFT_WIDTH/5
    #define ICON_MAX_SIZE TFT_WIDTH/2

    const int16_t tenPixels = 10, midScreenHoriz = TFT_WIDTH/2, rightScreen = TFT_WIDTH, midScreenVertical = TFT_HEIGHT/2, bigWidgetMidY = midScreenVertical-WIDGET_BIG_WIDTH/2, iconMidY = midScreenVertical-ICON_MAX_SIZE/2 ;
    const int16_t iconRow2 = OVERLAY_ICON_SIZE*1.5;

    uint16_t rainbowC;
    AlteredColor rainbowColor = AlteredColor{.baseColor = TFT_WHITE, .value = &rainbowC};    

    int16_t hquarter1xPos = TFT_HQUARTER-TFT_HQUARTER/2;
    int16_t hquarter2xPos = TFT_HQUARTER*2-TFT_HQUARTER/2;
    int16_t hquarter3xPos = TFT_HQUARTER*3-TFT_HQUARTER/2;
    int16_t hquarter4xPos = TFT_HQUARTER*4-TFT_HQUARTER/2;

    
    int16_t vquarter1xPos = TFT_VQUARTER-TFT_VQUARTER/2;
    int16_t vquarter2xPos = TFT_VQUARTER*2-TFT_VQUARTER/2;
    int16_t vquarter3xPos = TFT_VQUARTER*3-TFT_VQUARTER/2;
    int16_t vquarter4xPos = TFT_VQUARTER*4-TFT_VQUARTER/2;
   
    int16_t hthird3xPos = TFT_HTHIRD*2+TFT_HQUARTER/2;

    String leftIcon,rightIcon, brightIcon = ICON_BRIGHTNESS;

    bool disabledBankRendering = false;   // this is to stop rendering the bank
  
    Strip* focusedStrip = nullptr;
    StripHeaderWidget* stripHeader = nullptr;

    ///////////////////// MENU SYSTEM ATTRIBUTES AND MESSAGE ////////////////////////
    #define MENU_ICON_SIZE TFT_WIDTH/3
    #define MENU_TITLE_SIZE MENU_ICON_SIZE/2
    #define CONFIG_TEXTAREA_MESSAGE_SIZE MENU_TITLE_SIZE/1.8

    MenuSystem ms = MenuSystem(*this);
    MenuSystem* bankMenu = nullptr;
    MenuSystem* wizardMenu = nullptr;

    Menu* langMenu = nullptr;
    Menu* confMenu = nullptr;  
    String menuTopText=" ", vBatLabel; //menuTitle
    String menuItemIcon[3]={"","",""}; // left, mid, right
    String menuBottomLabel=" ";

    uint16_t menuActiveIconColor;

    const int16_t menuIconYpos = TFT_HEIGHT/2-MENU_ICON_SIZE/2.2, leftMenuIconXpos = midScreenHoriz-MENU_ICON_SIZE*1.4,rightMenuIconXpos = midScreenHoriz+MENU_ICON_SIZE*1.4;
    const int16_t menuTitleY = TFT_HEIGHT-MENU_TITLE_SIZE*1.1, belowMenuIcon = menuIconYpos+MENU_ICON_SIZE; 

    #define TWOFRAMEICON 2
    String iconRolling[TWOFRAMEICON] = {ICON_ROLLING1,ICON_ROLLING2};
    String iconPending[TWOFRAMEICON];

    const char* quitMenuString ={ICON_QUIT":QUIT"};
    const char* infoMenuString ={ICON_INFO":INFO"};
    const char* configMenuString ={ICON_CONFIG":CONFIGURATION"};
    const char* profileMenuString ={ICON_PROFILES":PROFILE"};
    const char* saveMenuString ={ICON_SAVE":SAVE"};
    const char* langMenuString ={ICON_LANG":LANGUAGE"};
    const char* tftRotationMenuScreen{ICON_TFT_ROTATE":ROTATE"};
    const char* resetMenuScreen{ICON_RESET":RESET"};
    const char* shutdownMenuString{ICON_POWER":OFF"};
    const char* brightnessMenuString{ICON_BRIGHTNESS":BRIGHTNESS"};

    ///////////////////// UTILITIES ATTRIBUTES ////////////////////////
    I18n lang = I18n();
    I18n userLang = I18n();
    
    Config bankCfg = Config();
    Config system = Config();
    Config userConfig;
    
    Preferences persistants;
    Shortcuts shortcuts;

    ///////////////// INTERFACES ///////////////////
    //OSCInterface osc;

    ////////////////////////////////////////////////

    unsigned long goSleepCountdownStart;

    bool webAppConnected;

    String configClientIP; // ip of the client connect to the AP during configuration    

    String firmwareUpdateFilename="";

    bool swapButton = false;

    ///////////////////////////////////////////////////////////////
    //////////////////                        /////////////////////
    //////////////////    MAIN CLASS START   //////////////////////
    //////////////////                       //////////////////////    
    ///////////////////////////////////////////////////////////////

    ApplicationManager(HardwareManager* h, GUIManager* g, bool debug=true):GenericComponent(debug){
      hwm = h;
      gui = g;
      id = "ApplicationManager";

      stripTextSkel = new StripSkeleton(STRIP_TEXT,STRIP_DEFAULT_GROUP,textStripConstructorCallback,'*');
      
      dataRepository.addDataSet(&simpleText);
      dataRepository.addDataSet(&stripIDs);
      
      stripTextSkel->registerDataSetAddress(ADDRESS_SIMPLETEXT);

      hwm->network->userInternetCallback = [this](bool* online) {
          this->internetCallBack(online);
      };         

      /*callback = [this](Event* event) {
          this->defaultEventHandling(event);
      };*/         
    }

    bool getButtonState(uint8_t B){
      return hwm->buttons[B]->refreshed;
    }

    /*void defaultEventHandling(EventDispatcher::Event* event){  
      switch(event->type){
        case EventDispatcher::EVENT_APPLICATION:{
        
        }break;
      } 
    }*/


    void internetCallBack(bool* online){
      evt.type = EVENT_INTERNET_PING;
      evt.data = online;
      fireEvent(&evt);
    }

    void setBatIcon(){
      batIconColor = TFT_GREEN;
      switch(hwm->battery.batteryState){
        case BAT_CHARGING:
          batIcon = ICON_PLUG; //ICON_BAT_CHARGING
        break;
        case BAT_DISCHARGING:
        {
          uint8_t vBatIconIndex = hwm->battery.indicator;
          batIcon = batteryIcons[vBatIconIndex];      

          if (vBatIconIndex == 0 && hwm->battery.vBat < VBAT_WARNING) batIconColor = TFT_RED; 
        }break;
        case BAT_CHECKING:
        default:
          batIcon = ICON_WAIT;
        break;  
      }
      vBatLabel = *lang.getString("Battery")+": "+String(hwm->battery.vBat)+"v";
    }

    void setAPandNetIcons(){
      setAPIcon();
      setNetworkIcon();
    }

    void setAPIcon(){
      if (hwm->ap == nullptr){
        apIcon = emptyString;
      }else{
        apIcon = ICON_AP;
        if (hwm->ap->state == CONNECTED){
          apIconGlowingRedColor.value = &GUI::unUsedColor;
          apIconColor = GUI::green;
        }else{
          apIconGlowingRedColor.value = &apIconColor;
        }
      }
    }

    void rollingIcon(bool on){
      if (on){
        iconRolling[0]=ICON_ROLLING1;
        iconRolling[1]=ICON_ROLLING2;
      }else{
        iconRolling[0]=" ";
        iconRolling[1]=" ";
      }
    }

    void setNetworkIcon(){

      #ifdef USE_NETWORK
              
        if (hwm->network->state == CONNECTED){
          if (hwm->network->media == WIFI_MEDIA) refreshWiFiSignalIcon();
          else networkIcon = ICON_ETH;
          networkIconGlowingRedColor.value = &GUI::unUsedColor;
          networkIconColorIdle = GUI::green;
        }else{
          networkIconGlowingRedColor.value = &networkIconColorIdle;

          if (hwm->network->ethEnabled && hwm->network->WiFiEnabled && hwm->network->getLink() == LinkOFF) networkIcon = ICON_MIXETHWIFI; // both WiFi and ETh are up but there is no cable on, we don't know who will connect first
          else if ((hwm->network->ethEnabled && !hwm->network->WiFiEnabled) || (hwm->network->getLink() == LinkON)) networkIcon = ICON_ETH;  //case when we bootSprite for a DHCP lease or have no WiFi enabled
          else if (!hwm->network->ethEnabled && hwm->network->WiFiEnabled) networkIcon = ICON_WIFI_STRENGTH_3; 
          else networkIcon = emptyString;  // when it is only wifi and ap is launched.
        }

      #else
        networkIcon = emptyString;
      #endif

    }

    void goToResetScreenAndSleep(String message){
      clearConfig();
      menuTopText = *lang.getString(message);
      menuBottomLabel = emptyString;
      byeIcon = &iconReset; byeIconColor = &GUI::red;
      goSleepCountdownStart = millis();          
      screen.id = SCREEN_BYE; screen.state = STATE_BYE_GOSLEEP_MESSAGE;         //we could create a screen dedicated for reset         
    }

    void goToBigBatteryIconScreenAndSleep(){
      byeIcon = &batIcon; byeIconColor = &batIconColor;
      screen.id = SCREEN_BYE;
      screen.state = STATE_BYE_GOSLEEP_MESSAGE;       
    }

    IntervalBank* deserializeBanks(const String* bankJsonString){
      IntervalBank* bank = nullptr;     
       
      JsonDocument jsonBanks;
      
      DeserializationError error = deserializeJson(jsonBanks, *bankJsonString);
      if (error){

      }else{              
        JsonObject root = jsonBanks.as<JsonObject>();      
        for (JsonPair kv : root) {

          bank = new IntervalBank(kv.key().c_str(),kv.value().as<String>(),&stripFactory);
          bank->init();
          /*if (bank->strips.size()>0) banks.add(bank);
          else delete bank; // bank has no strips and is threfore invalid*/
          banks.add(bank);
        }
        hookBankToMenu();
      }

      if (banks.size()>0) return banks.get(banks.size() - 1); // return the last bank
      else return nullptr;
    }


    void applySystemSettings(){
      // this function applies all settings from the system conf file
      wiFiList.deserialize(*system.getString(S_SSID));
      hwm->setWiFi(&wiFiList,*system.getString(S_LAST_USED_SSID));

      hwm->setButtonReactivity(system.getInt(S_PUSHDELAY),system.getInt(S_DCLICREACTIVITY));     

      String langFile = "/"+*system.getString(S_LANG,"ENGLISH");
      langFile.toUpperCase();

      lang.load(langFile+".lang"); 
      userLang.load(langFile+".i18n"); 


    }

    #define DEFAULT_CONFIG_FILE_NAME "default"

    void loadConfig(){
      // this function applies all settings from the banks and user conf file
      String profileFile = "/"+*system.getString(S_CONFIG_FILE,DEFAULT_CONFIG_FILE_NAME);
      bankCfg.load(profileFile+".conf",defaultBanksConf);  
      userConfig.load(profileFile+".user",defaultUserConf);            
    }  

    #define SYSTEM_FILE_NAME "/settings.sys"
    void clearConfig(){
      // add a callback usere function userClearConfig()
      // delete all config files here, add the code to iterate FS on *.conf
      LittleFS.remove(userConfig.filename);
      LittleFS.remove(DEFAULT_CONFIG_FILE_NAME);
      // delete system settings
      LittleFS.remove(SYSTEM_FILE_NAME);
      persistants.clear();
      persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO);      
      persistants.putFloat(PERSISTANT_BRIGHTNESS,GUI::screenBrightness);    //default brightness 
    }

    #define CUSTOM_LOGO_FILE "/custom.jpg"

    bool initBanks(){
      if (deserializeBanks(bankCfg.getString(C_BANKS)) == nullptr){
        menuTopText = *lang.getString("IntervalBank creation");
        menuBottomLabel = emptyString;
        byeIconColor = &GUI::red; byeIcon = &iconForbid;
        screen.id = SCREEN_BYE; screen.state = STATE_BYE_MODAL_MESSAGE;
        return false;
      }
      return true;
    }   

    void init(String _defaultUserConf="{}"){     
      defaultUserConf = _defaultUserConf;
      #ifdef DEBUG
        //DEBUG is enabled so we don't use MIDI USB, the USB C is used for Serial printf debug
        Serial.begin(115200);
        /*unsigned long start = millis();
        while (!Serial && (millis() - start < 1000)) {
          delay(1); // petite pause pour éviter de monopoliser la CPU
        }*/
      #else
        Serial.setDebugOutput(false);             
      #endif

      GenericComponent::init();

      LittleFS.begin();

      persistants.begin("interval", false);      

      system.load(SYSTEM_FILE_NAME,defaultSysConf);  
      applySystemSettings();

      loadConfig();      

      #ifdef DEBUG
        String s;
        system.serialize(&s);
        DEBUG_PRINT("\n\n system:\t%s [...]",s.substring(0,200).c_str());
        s="";
        bankCfg.serialize(&s);
        DEBUG_PRINT("\n\n bankCfg:\t%s [...]",s.substring(0,200).c_str());
        s="";
        userConfig.serialize(&s);
        DEBUG_PRINT("\n\n userConfig:\t%s [...]>",s.substring(0,200).c_str());        
        s="";
        lang.serialize(&s);
        DEBUG_PRINT("\n\n langfile:\t%s [...]",s.substring(0,200).c_str());        
        DEBUG_PRINT("\nexample of langfile token: Connect:<%s>",lang.getString("Connect")->c_str());        
        s="";
        userLang.serialize(&s);
        DEBUG_PRINT("\n\n userLang:\t%s [...]",s.substring(0,200).c_str());                
      #endif

      hwm->init();
      
      GUI::screenBrightness = persistants.getFloat(PERSISTANT_BRIGHTNESS,DEFAULT_BRIGHTNESS);
      if (GUI::screenBrightness < BRIGHTNESS_MIN) GUI::screenBrightness = BRIGHTNESS_MIN;
      if (GUI::screenBrightness > BRIGHTNESS_MAX) GUI::screenBrightness = BRIGHTNESS_MAX;      

      gui->init();         
      

      setScreenAndButtonRotation();
      //GUI::tft->setRotation(persistants.getUChar(PERSISTANT_TFT_ROTATION,1)); 

      for (uint8_t i=0;i<SCREEN_MAX;i++) screenBuilders.add(&screens[i]);
            
      for (uint8_t i=0;i<MAXB;i++){
        hwm->buttons[i]->registerCallback([this](Event* event) {
            this->buttonCallBack(event);
        });
      }

      hwm->onOff.registerCallback([this](Event* event) {
        this->onOffCallback(event);
      });

      back_menu_item = new MenuItem(quitMenuString, &back_menu_item_selected);
      info_menu_item = new MenuItem(infoMenuString, &info_item_selected);
      config_menu_item = new MenuItem(configMenuString,&config_menu_item_selected);
      profile_menu_item = new Menu(profileMenuString,&menu_selected);
      save_profile_menu_item = new MenuItem(saveMenuString,&save_profile_item_selected);
      tft_rotation_menu_item = new MenuItem(tftRotationMenuScreen,&tft_rotation_item_selected);
      reset_menu_item = new MenuItem(resetMenuScreen,&reset_item_selected);                // to be removed
      shutdown_menu_item = new MenuItem(shutdownMenuString, &shutdown_menu_item_selected);
      brightness_menu_item = new MenuItem(brightnessMenuString, &brightness_menu_item_selected);

      
      ms.get_root_menu().add_item(back_menu_item);
      ms.get_root_menu().add_item(info_menu_item);
      ms.get_root_menu().add_item(config_menu_item);       
      ms.get_root_menu().add_item(tft_rotation_menu_item);       
      ms.get_root_menu().add_item(reset_menu_item);       
      ms.get_root_menu().add_menu(profile_menu_item);
      ms.get_root_menu().add_item(shutdown_menu_item);
      ms.get_root_menu().add_item(brightness_menu_item);
      profile_menu_item->add_item(back_menu_item);
      profile_menu_item->add_item(save_profile_menu_item);

      hookLangToMenu();
      
      //quarter1xPos = 

      // glowing colors declarations
      //gui->glowingColors.add(&glowingGoldColor);
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      gui->alphaColors.add(&GUI::glowingGoldColor);      
            
      gui->alphaColors.add(&networkIconGlowingRedColor);
      gui->alphaColors.add(&apIconGlowingRedColor);
      gui->alphaColors.add(&configIconGlowingRedColor);

      gui->rainbowColors.add(&rainbowColor);
      gui->alphaColors.add(&rainbowColor);

      minHeapSize = maxHeapSize = ESP.getFreeHeap();

      setAPandNetIcons();
      setBatIcon();     

      // BY DEFAULT, WE DO NOT STARTUP, ONLY DISPLAY BATTERY LEVEL AND SWITCH OFF                 

      bool wizard = false;

      switch(hwm->startupState){
        case RESET_STARTUP:
          // put reset code here
          goToResetScreenAndSleep("RESET");
        break;
        case BATTERY_STARTUP:
          menuTopText = *lang.getString("Press >1s to startup"); menuBottomLabel = emptyString;
          goSleepCountdownStart = hwm->bootTime;
          goToBigBatteryIconScreenAndSleep();         
        break;
        case NO_FS_STARTUP:
          menuTopText = *lang.getString("File System");
          menuBottomLabel = emptyString;
          byeIcon = &iconX;
          // as the FS is not ready, there is no icon to display and no fonts as well so it will be display by default font
          screen.id = SCREEN_BYE; screen.state = STATE_BYE_MODAL_MESSAGE;
        break;
        case WIZARD_STARTUP:          
          persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_WIZARD);
          persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_LANG_STEP1);
          // no break, we continue normal startup
        case NORMAL_STARTUP:
          // perform all application initialization
          bootScreen();                
          // we init banks and then go to banks or resume wizard
          if (initBanks()) restoreScreen();
        break;
      }
    }

    // EVENTS HANDLING

    void onOffCallback(Event* event){
      ButtonEvent* btnEvt = (ButtonEvent*) event->data;
      switch(btnEvt->id){
        case IntervalButton::BUTTON_DOUBLE_CLIC:
          
        break;
        case IntervalButton::BUTTON_HOLD:
          
        break;
        case IntervalButton::BUTTON_LONG_CLIC:
          menuSwitchCallback(btnEvt->buttonId,btnEvt->time);
        break;
        case IntervalButton::BUTTON_RELEASE_HOLD:
          
        break;
        case IntervalButton::BUTTON_SIMPLE_CLIC:
          simpleClicCallBack(btnEvt->buttonId,btnEvt->time);
        break;
      }
      // propagate the event
      fireEvent(event);      
    }

    void buttonCallBack(Event* event) {
      ButtonEvent* btnEvt = (ButtonEvent*) event->data;             
      switch(btnEvt->id){
        case IntervalButton::BUTTON_DOUBLE_CLIC:
          doubleClicCallBack(btnEvt->buttonId,btnEvt->time);
        break;
        case IntervalButton::BUTTON_HOLD:
          holdCallback(btnEvt->buttonId,btnEvt->time);
        break;
        case IntervalButton::BUTTON_LONG_CLIC:
          longClicCallBack(btnEvt->buttonId,btnEvt->time);
        break;
        case IntervalButton::BUTTON_RELEASE_HOLD:
          releaseHoldCallback(btnEvt->buttonId,btnEvt->time);
        break;
        case IntervalButton::BUTTON_SIMPLE_CLIC:
          simpleClicCallBack(btnEvt->buttonId,btnEvt->time);
        break;
      }
      // propagate the event
      fireEvent(event);
    }


    //////////////////////////////////////////////////////////////
    ////                BUTTONS CALLBACKS                  ///////
    //////////////////////////////////////////////////////////////
    void menuSwitchCallback(uint8_t buttonID, unsigned long time){
      if (screen.id == SCREEN_MENU){   //set GUI
        restoreScreen();
      }else{
        restoreMenu();
      } 
    }

    void simpleClicCallBack(uint8_t buttonID, unsigned long time){
      switch(screen.id){
        case SCREEN_STARTUP:

        break;
        case SCREEN_WIZARD:
          
        break;
        case SCREEN_MENU:
          handleMenuButton(buttonID);   
        break;
        case SCREEN_BANK_SWITCH:
          handleSwitchScreen(buttonID);
        break;
        case SCREEN_PROMPT:

        break;
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_SEARCHING_CONNECTION:

            break;
            case STATE_UTILS_INFO:
              restoreMenu();  // any button calls the method
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
              restoreScreen();
            break;
            case STATE_UTILS_BRIGHTNESS:
              handleClicBrightnessButton(buttonID);    
            break;            
          }
        break;
        case SCREEN_BANK:
          //DEBUG_PRINT("\nsimpleClicCallBack: %i",buttonID);
          moduleSimpleClic(buttonID);      
        break;
      }
    }

    void longClicCallBack(uint8_t buttonID, unsigned long time){
      switch(screen.id){
        case SCREEN_STARTUP:

        break;
        case SCREEN_WIZARD:

        break;
        case SCREEN_MENU:
          handleMenuButton(buttonID);
        break;
        case SCREEN_PROMPT:

        break;
        case SCREEN_BANK_SWITCH:
          handleSwitchScreen(buttonID);
        break;    
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_SEARCHING_CONNECTION:

            break;
            case STATE_UTILS_INFO:
              restoreMenu();  // any button calls the method
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
              restoreScreen();
            break;
            case STATE_UTILS_BRIGHTNESS:
              handleClicBrightnessButton(buttonID);    
            break;            
          }
        break;
        case SCREEN_BANK:
          //DEBUG_PRINT("\nlongClicCallBack: %i",buttonID);    
          moduleLongClic(buttonID);      
        break;
      }  
    }

    void doubleClicCallBack(uint8_t buttonID, unsigned long time){
      switch(screen.id){
        case SCREEN_STARTUP:

        break;
        case SCREEN_WIZARD:

        break;
        case SCREEN_MENU:
        
        break;
        case SCREEN_PROMPT:

        break;
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_SEARCHING_CONNECTION:

            break;
            case STATE_UTILS_INFO:

            break;
          }
        break;
        case SCREEN_BANK:
          //DEBUG_PRINT("\ndoubleClicCallBack: %i",buttonID);
          broadCast("{\"mixerIP\":\"192.168.199.199\"}");
        break;
      }  
    }

    void releaseHoldCallback(uint8_t buttonID, unsigned long time){
      switch(screen.id){
        case SCREEN_STARTUP:

        break;
        case SCREEN_WIZARD:

        break;
        case SCREEN_MENU:
          handleMenuButton(buttonID);
        break;
        case SCREEN_PROMPT:

        break;
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_SEARCHING_CONNECTION:

            break;
            case STATE_UTILS_INFO:
              restoreMenu();  // any button calls the method
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
              restoreScreen();
            break;
            case STATE_UTILS_BRIGHTNESS:
              // the screen is not refreshed during the HOLD workflow to avoid flickering, so we do it when we release the button
              //saveBrightness();    // uncomment this one and comment the next line in case you want to manage hold            
              handleClicBrightnessButton(buttonID);  
            break;
          }
        break;
        case SCREEN_BANK:
          //DEBUG_PRINT("\nreleaseHoldCallback: %i",buttonID);
        break;
      }  
    }

    void holdCallback(uint8_t buttonID, unsigned long time){
      switch(screen.id){
        case SCREEN_STARTUP:

        break;
        case SCREEN_WIZARD:

        break;
        case SCREEN_MENU:
        
        break;
        case SCREEN_PROMPT:

        break;
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_BRIGHTNESS:
              #define BRIGHTNESS_HOLD_INCREMENT 0.003
              //changeBrightness(buttonID,BRIGHTNESS_HOLD_INCREMENT); // create too muchdrawback with the transparency management
            break;
          }
        break;
        case SCREEN_BANK:
          moduleHoldClic(buttonID, time);  
        break;
      }  
    }


  //////////////////////////////////////////////////////////////
  ////                BUTTONS CALLBACKS UTILITIES        ///////
  //////////////////////////////////////////////////////////////
  void handleSwitchScreen(uint8_t buttonID){
    //DEBUG_PRINT("\nhandleSwitchScreen");
    if (buttonID == hwm->buttons[MIDDLEB]->getID() ){    
      bankMenu->select();    
      setBankMenuItems();
    }else{
      if (buttonID == hwm->buttons[RIGHTB]->getID()) bankMenu->next(true);
      else bankMenu->prev(true);
      menuSwitchBankFullRefresh();
    }   
  }

  void saveBrightness(){
    triggerRedraw();
    persistants.putFloat(PERSISTANT_BRIGHTNESS,GUI::screenBrightness);    
  }

  void handleClicBrightnessButton(uint8_t buttonID){
    #define STEP_BRIGHTNESS_INCREMENT BRIGHTNESS_MAX/10
    if (buttonID == hwm->buttons[MIDDLEB]->getID() || buttonID == hwm->onOff.getID()){
      restoreMenu();
    }else{
      changeBrightness(buttonID,STEP_BRIGHTNESS_INCREMENT);
      // we refresh screen and save after each single press
      saveBrightness();
    }      
  }

  void changeBrightness(uint8_t buttonID, float increment){
    if (buttonID == hwm->buttons[LEFTB]->getID()){
      if (GUI::screenBrightness > BRIGHTNESS_MIN) GUI::screenBrightness -= increment;
      if (GUI::screenBrightness < BRIGHTNESS_MIN) GUI::screenBrightness = BRIGHTNESS_MIN;
    }else if (buttonID == hwm->buttons[RIGHTB]->getID()){
      if (GUI::screenBrightness < BRIGHTNESS_MAX) GUI::screenBrightness += increment;
      if (GUI::screenBrightness > BRIGHTNESS_MAX) GUI::screenBrightness = BRIGHTNESS_MAX;
    }    
  }

  void handleMenuButton(uint8_t buttonID){
    if (buttonID == hwm->buttons[MIDDLEB]->getID() || buttonID == hwm->onOff.getID()){
      ms.select();
      setRootMenuItems(&ms);
    }else{
      if (buttonID == hwm->buttons[LEFTB]->getID()) ms.prev(true);
      else ms.next(true);
      menuSystemFullRefresh();
    }      
  }

  void moduleSimpleClic(uint8_t buttonID){
    // perform the behaviour defined in the config file
    // at the moment, this is hard coded
    if (buttonID == hwm->buttons[MIDDLEB]->getID()){
      if (!stripAction(IntervalButton::BUTTON_SIMPLE_CLIC)){
        
        // code if execution fails: example: popup a message with the error
      }
    }
    else{
      stripScroll(buttonID);
    }
  }


  void moduleHoldClic(uint8_t buttonID, unsigned long time){
    // perform the behaviour defined in the config file
    // at the moment, this is hard coded
    stripChangeValue(buttonID, time, IntervalButton::BUTTON_HOLD);
  }

  void moduleLongClic(uint8_t buttonID){
    if (buttonID == hwm->buttons[MIDDLEB]->getID()){
      switchBank();
      //LogUDP::sendLog("moduleLongClic",hwm->network->udp);
    }else if(buttonID == hwm->buttons[LEFTB]->getID()){    
      //LogUDP::sendLog("testOSC",hwm->network->udp);
    }else{
      //testHttpRequest();
      //LogUDP::sendLog("testHTTP",hwm->network->udp);
    }
  }





    void setScreenAndButtonRotation(){
      if (persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO) == INVERTED_SCREEN_YES){
        //swap button
        uint8_t oldLeftID = hwm->buttons[LEFTB]->getID();
        hwm->buttons[LEFTB]->setID(hwm->buttons[RIGHTB]->getID());
        hwm->buttons[RIGHTB]->setID(oldLeftID);
      }else{
        // by default we keep the button as they were initialized by the HWM
        // however, in case user has rotated from the menu, we have to swap button
        if (swapButton){
          uint8_t oldRightID = hwm->buttons[RIGHTB]->getID();
          hwm->buttons[RIGHTB]->setID(hwm->buttons[LEFTB]->getID());
          hwm->buttons[LEFTB]->setID(oldRightID);
        }
      }  
      GUI::tft->setRotation(persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO));        
    }

    void refreshWiFiSignalIcon(){
      int8_t signalStrength = abs(WiFi.RSSI());
      if (signalStrength >=90) networkIcon = ICON_WIFI_STRENGTH_0;
      else if (signalStrength >=75) networkIcon = ICON_WIFI_STRENGTH_1;
      else if (signalStrength >=45) networkIcon = ICON_WIFI_STRENGTH_2;
      else networkIcon = ICON_WIFI_STRENGTH_3;      
      //DEBUG_PRINT("\nrssi: %d",signalStrength);
    }

    String serializeBanks(){
      JsonDocument bankDoc;
      //DynamicJsonDocument bankDoc(10000); 
      for (uint8_t i=0; i<banks.size(); i++){
        //DynamicJsonDocument jsonBank(JSON_BANK_SIZE); 
        JsonDocument jsonBank;
        IntervalBank* bank = banks.get(i);
        String stringBank = bank->serialize();
        DeserializationError error = deserializeJson(jsonBank, stringBank);
        jsonBank.shrinkToFit();
        if (error){
          
          return "{}";
        }else{
          bankDoc[bank->name] = jsonBank;
        }       
      }      
      return bankDoc.as<String>();
    }


/*
// Fonction pour remplir la section "toto"
void addTotoToDocument(JsonDocument& doc) {
    JsonObject toto = doc.createNestedObject("toto");
    toto["key1"] = "value1"; // Clé-valeur dans "toto"
    toto["key2"] = 1234;    // Une autre clé-valeur dans "toto"
}

// Fonction pour remplir la section "tutu"
void addTutuToDocument(JsonDocument& doc) {
    JsonObject tutu = doc.createNestedObject("tutu");
    tutu["keyA"] = "valueA"; // Clé-valeur dans "tutu"
    tutu["keyB"] = true;     // Une autre clé-valeur dans "tutu"
}

void setup() {
    Serial.begin(115200);

    // Création d'un document JSON
    JsonDocument jsonDoc;

    // Ajout de la section "toto"
    addTotoToDocument(jsonDoc);

    // Ajout de la section "tutu"
    addTutuToDocument(jsonDoc);

    // Sérialisation et affichage du document JSON
    serializeJson(jsonDoc, Serial);
    Serial.println();
}

*/

    void getCommonConfig(JsonDocument& doc) {
      String tempJsonString;
      system.serialize(&tempJsonString);  
      doc["system"]=tempJsonString;
      doc["volatile"]["langs"][0] = "francais"; doc["volatile"]["langs"][1] = "english";       
      //doc["userConfig"] = userJsonConfigCallback();  TOTO
    }

    void serialize(String* returnedJsonStringData){
      // this build the full serialization of the application
      
      JsonDocument doc;      
      String tempJsonString;

      getCommonConfig(doc);

      JsonArray skel = doc["skeletons"].to<JsonArray>();
      stripFactory.serialize(&skel);

      tempJsonString="";
      shortcuts.serialize(&tempJsonString);
      doc["shortcutsDef"] = tempJsonString;

      JsonArray ds = doc["datasets"].to<JsonArray>();  
      dataRepository.serialize(&ds);         

      tempJsonString = serializeBanks();      
      bankCfg.setValue(C_BANKS,tempJsonString);

      tempJsonString = "";
      bankCfg.serialize(&tempJsonString);
      doc["config"]=tempJsonString;
          
      serializeJson(doc, *returnedJsonStringData);  
      removeJsonQuote(returnedJsonStringData);
    }

    void restoreScreen(){
      screen.id = persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
      screen.state = persistants.getUChar(PERSISTANT_SCREEN_STATE,0);  
      // prevents wrong branching
      if (screen.id == SCREEN_BANK && screen.state > banks.size()-1) screen.state = 0; 
      if (screen.id == SCREEN_WIZARD && screen.state > STATE_WIZARD_USER_STEP4) screen.state = STATE_WIZARD_LANG_STEP1;       
      // branch the first available strip 
      if (focusedStrip == nullptr && screen.id == SCREEN_BANK){
        IntervalBank* bank = banks.get(screen.state);                
        focusedStrip = bank->strips[0];
      }
    }

    void restoreMenu(){
      screen.id = SCREEN_MENU;
    }

    String getIconFromString(const char* item){
      return String((char)item[0]);
    }

    String getNameFromString(const char* item){
      return String(item).substring(2);
    } 

    void buildLangMenu(){
      if (langMenu != nullptr) delete langMenu;
      langMenu = new Menu(langMenuString,menu_selected); 

      populateFileList("lang","/",&countryList,String(ICON_LANG)+"-",false);     // this populate the list of countries with the icon (prefix = k-) and without extension

      // populate the menu with all language found      
      for(int i = 0; i < countryList.size(); i++){
        const char* countryIconAndName = countryList.get(i)->c_str();
        langMenu->add_item(new MenuItem(countryIconAndName, loadLangFromMenu_item_selected));                
        if (debug){
          //DEBUG_PRINT("\nadded menu entry: %s",countryList.get(i)->c_str());           
        }
      }      
    }

    void hookLangToMenu(){
      buildLangMenu();
      langMenu->add_item(back_menu_item);
      ms.get_root_menu().add_menu(langMenu);              
    }


    enum{
      BINARY_INSTALLFAIL,
      BINARY_INSTALLED,
      NOBINARY_INSTALLED
    };

     void cleanFolder(String path, String fType) {
      File root = LittleFS.open(path);
      if (!root || !root.isDirectory()) {
        ////DEBUG_PRINT("\nInvalid directory: %s", path.c_str());
        return;
      }
    
      File file = root.openNextFile();
      while (file) {
        if (!file.isDirectory()) {
          String filename = file.name();
          // Extraire le nom de fichier sans le chemin si nécessaire
          String shortName = filename;
          if (filename.startsWith(path)) {
            shortName = filename.substring(path.length());
            if (shortName.startsWith("/")) shortName = shortName.substring(1);
          }
    
          if (shortName.endsWith(fType) || fType == "*") {
            String fullPath = path + "/" + shortName;
            if (LittleFS.remove(fullPath)) {
              ////DEBUG_PRINT("\nDeleted %s", fullPath.c_str());
            } else {
              ////DEBUG_PRINT("\nFailed to delete %s", fullPath.c_str());
            }
          }
        }
        file = root.openNextFile();
      }
    }
       
   
    bool unpackFile(String fileName) {
      TarGzUnpacker TARGZUnpacker;
      
      bool success = false;
      TARGZUnpacker.setLoggerCallback(BaseUnpacker::targzPrintLoggerCallback);
      TARGZUnpacker.setTarStatusProgressCallback(tarStatusProgressCallback ); // print the filenames as they're expanded
      
      if (fileName.endsWith(".fsar")) {
        //DEBUG_PRINT("\nstarting tar expansion of: %s",fileName.c_str());
        TARGZUnpacker.setTarProgressCallback(unpackProgressCallback ); // prints the untarring progress for each individual file    
        if (TARGZUnpacker.tarExpander(LittleFS, fileName.c_str(), LittleFS, "/") ) success = true;
      }
      if (! success){
        //progressBarLabel = i18n[EXPANDING_MSG]+" "+i18n[ERROR_MSG]+": "+String(TARGZUnpacker.tarGzGetError());
      }else{

      } 
      return success;
    }

    bool processArchive(){
      bool success = true;
      
      /*TJpgDec.drawFsJpg(160-40, 0, "/FIRMWARE.jpeg");
      upperInfoPopup(i18n[EXPANDING_MSG],TFT_BLACK,TFT_WHITE);
      lowerInfoPopup("waiting",TFT_BLACK,TFT_WHITE);*/
      
      bool unpackOk = unpackFile(firmwareUpdateFilename);
      if (unpackOk){

        uint8_t binStatus = checkAndInstallBinary();
        switch (binStatus){
          case BINARY_INSTALLFAIL:
            //utilitySequence = SEQ_ACTIONFAIL;
            success = false;
            // progressBarLabel contains the error message from the archive
          break;          
          case BINARY_INSTALLED:
            /*progressBarLabel = i18n[RESTART_MSG];
            utilitySequence = SEQ_ACTIONREBOOT;*/
          break;
          default: //NOBINARY_INSTALLED 
            /*progressBarLabel = i18n[DATA_LOADED_MSG];
            utilitySequence = SEQ_ACTIONSUCCESS;*/
          break;
        }
      }else{
        //utilitySequence = SEQ_ACTIONFAIL;        
      }
      cleanFolder("/", ".fsar"); cleanFolder("/", ".bin.gz"); cleanFolder("/", ".fsaz");  
      return success;
    }

    uint8_t checkAndInstallBinary() {
      uint8_t binaryInstallStatus = NOBINARY_INSTALLED;
      //DEBUG_PRINT("\nNow checking if a bin need to be installed\n");
      // we have to rename each file from tmp to the root and check if we have a binary in the pack to install
      // then we have to clean /tmp
      File root = LittleFS.open("/");
      String firmwareFilename = "";
      
      if (!root || !root.isDirectory()) {
        DEBUG_PRINT("\nErreur : Impossible d'ouvrir le dossier racine");
        return BINARY_INSTALLFAIL;
      } else {
        File file = root.openNextFile();
        while (file) {
          if (!file.isDirectory()) {
            String filename = file.name();
            if (filename.endsWith(".bin.gz") /*|| filename.endsWith(".bin")*/) {
              firmwareFilename = filename;
              DEBUG_PRINT("\nbinary update found: %s", firmwareFilename.c_str());
            }
          }
          file = root.openNextFile();
        }
      }

      if (firmwareFilename != "") {
        DEBUG_PRINT("\nPeforming binary update:%s", firmwareFilename.c_str());

/*        utilitySequence = SEQ_PROCESSING;
        progressBarLabel = i18n[UPDATE_MSG];
        restoreScreen(UTILITY_SCREEN);
        upperInfoPopup(i18n[INSTALL_MSG],TFT_BLACK,TFT_WHITE);
        lowerInfoPopup(firmwareFilename,TFT_BLACK,TFT_WHITE);  /// firmware filename = you could change it to just "firmware" to avoid showing the filename.bin.gz
*/        

        GzUnpacker GZUnpacker;

        GZUnpacker.setGzProgressCallback( unpackProgressCallback/*BaseUnpacker::defaultProgressCallback*/ );
        GZUnpacker.setLoggerCallback( BaseUnpacker::targzPrintLoggerCallback  );
        int success =  GZUnpacker.gzUpdater(tarGzFS, firmwareFilename.c_str(), 0,/*don't restart after update*/false );
        if( !success ) {
          //progressBarLabel = i18n[ERROR_MSG]+ " " +i18n[UPDATE_MSG]+ " ("+String(GZUnpacker.tarGzGetError())+")";
          binaryInstallStatus = BINARY_INSTALLFAIL;
        }else{          
          //progressBarLabel = i18n[UPDATE_MSG]+ " "+ i18n[SUCCESS_MSG];
          binaryInstallStatus = BINARY_INSTALLED;     
        }        
        //LOG_MESSAGE("firmware", progressBarLabel.c_str(), firmwareFilename.c_str());      
        if (success){
          //logger->info();
        }else{
          //logger->error();
        }
      }
      return binaryInstallStatus;
    }


    //------------------
    //
    // MANAGE WEB APPLICATION
    //
    //------------------     
    void handleWeb(void * parameter){

      //unsigned long lastActivity = millis();

      Network* configMedia = hwm->configMedia;

      vTaskDelay(1);    //crash wifi otherwise

      if (xSemaphoreTake(configMedia->networkBusy, pdMS_TO_TICKS(100)) == pdTRUE){     
        if (configMedia->refreshed){
          if (configMedia->state == CONNECTED){
            setWSServer();   // the network has refreshed (could be media, IP...) hence we rebuild the WSS
          }else{
            webAppConnected = false;            
          }
        }        
        if (configMedia->state == CONNECTED){    
          if(webServer != nullptr){                       
            // then webserver
            if (configMedia->media == CABLE_MEDIA){
              EthernetClient newEthClient = ((EthernetServer*)configMedia->server)->available();                                            
              if (newEthClient.connected()) {
                //ets_printf("\n\tEth ---> ");
                webServer->process(&newEthClient);
                newEthClient.stop();              
                //ets_printf(" STOP");
              }             
            }else{
              WiFiClient newWiFiClient = ((WiFiServer *)configMedia->server)->available();
              bool stop = false;
              if (newWiFiClient.connected()) {
                taskYIELD();
                //ets_printf("\n\tWiFi ---> ");
                webServer->process(&newWiFiClient);
                newWiFiClient.stop();              
                //ets_printf(" STOP");
              }   
            }

            wss->listen();  //sockets                    

          }
        }
        xSemaphoreGive(configMedia->networkBusy);  
      }    
    }

    void jumpToMenu(const char* menuTitle){
      // we branch to the lang menu
      ms.reset();             
      for (int i = 0; i < ms.get_root_menu().get_num_components(); ++i) {
        const MenuComponent* m = ms.get_current_menu()->get_current_component();
        if (strcmp(menuTitle,m->get_name())!=0) ms.next();
        else{ms.select();break;}
      }                       
      restoreMenu();         
    }

    void triggerRedraw(){
      // this forces the GUI to be rebuild at next refresh
      DEBUG_PRINT("\ntriggerRedraw()");
      screen.focus = screen.focus - 1;      
    }

    bool refresh(bool force=false){
      yield();
      
      refreshed = force;


      //------------------
      //
      // MANAGE NETWORK
      //
      //------------------     
      hwm->refresh();      

      if (hwm->network->refreshed){ // network changed its statusset
        
        setNetworkIcon();

        // inform all that network has changed (it could be the network cable pluged/unplug while in AP)
        broadCast(parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivityConfig\"}"));  // inform webclient               could be done via ws.send as well
        // ============>>> CALLBACK APPLICATION
        evt.type = EVENT_NETWORK_CHANGED_STATE;
        evt.data = &hwm->network->state;
        fireEvent(&evt);
        //userNetworkChangeStateCallback(hwm->network->state);

        if (hwm->network->state == CONNECTED){         
          // network has refreshed so we shift to appropriate configMedia
          if (hwm->network->media == WIFI_MEDIA){
            //we set last used Wifi SSID (id) and save config, hence keep WIFI_MEDIA credential
            //DEBUG_PRINT("\n------> SAVING SSID");
            system.setValue(S_LAST_USED_SSID,hwm->network->SSID);
            system.save(); 
            /* add this to the possible SSID "ssid": {
            "intervalmix": "intervalpop74",
            "infrafast": "Mf9k8iie2",
            "ssid3": "pass3"
            */
          }
          
          if (!webAppConnected){
            // there is no connection to any application, therefore, we prefer to use the network as the one to configure the application
            //if (debug) DEBUG_PRINT("\n\tsetting the configMedia to WiFiEth");
            hwm->configMedia = hwm->network;    // since the normal network gets reconnected,

          }else{

          }
        }else{
          // we disconnected , so we give back the priority on the WiFI connection list to  the last connected WiFi 
          //hwm->setWiFi(&wiFiList,*system.getString(S_LAST_USED_SSID)); 
        }
      }  

      if (hwm->ap != nullptr){
        if (hwm->ap->refreshed) setAPandNetIcons();
      }

      // real time signal strength display
      if (hwm->network->state == CONNECTED && hwm->network->media == WIFI_MEDIA) refreshWiFiSignalIcon();
    
      //------------------
      //
      // MANAGE WEBAPP
      //
      //------------------
      handleWeb(nullptr);

      //------------------
      //
      // MANAGE UTILITIES
      //
      //------------------

      // BATTERY LEVEL CHECK
      if (0/*hwm->battery.refreshed*/){        
        setBatIcon();
        if (hwm->battery.vBat<VBAT_CRIT && hwm->battery.batteryState != BAT_CHARGING){
          menuTopText = *lang.getString("Low Battery");
          menuBottomLabel = emptyString;
          goSleepCountdownStart = millis();
          goToBigBatteryIconScreenAndSleep();
        }          
      }

      //------------------
      //
      // MANAGE INTERFACE
      //
      //------------------

      for (const auto& descriptor : interfaceRepo.descriptors){
        
        DeviceInterface* interface = descriptor.interface;

        JsonDocument doc;
        JsonObject input = doc.to<JsonObject>();

        if (interface->monitor(input)) broadCast(interface->serialize());
        doc.shrinkToFit();
        
        //

        // we only have a currenntStrip valid when current screen is a bank
        if (screen.id == SCREEN_BANK){
          Interval::Bank* bank = banks.get(screen.state);          
          if (bank != nullptr){         
            if (focusedStrip != nullptr){
              if (bank->currentLayout == LAYOUT_ZOOM){
                if (focusedStrip->update(input))  triggerRedraw();;    // traite le strip avec focus/ full screen donc full update
              }else{
                for (const auto& strip : bank->strips) {
                  // traite tous les strips de la fenetre courante, c'esta dire ceux ayant la meme fenetre que le strip ayant le focus
                  if (strip->win == focusedStrip->win)
                    if (strip->update(input))  triggerRedraw();;
                }
              } 
            }
          }   
        }                      
      }

  /*if (mixerServer.mixerInterface->interfaceState == CONNECTED){
    OSCMixerInterface* interface = (OSCMixerInterface*)mixerServer.mixerInterface;
    OSCValues* data = &interface->data;
    if (data->incoming == true){
      Interval::Bank* bank = app.banks.get(screen.state);   
      if (bank != nullptr){         
        if (app.focusedStrip != nullptr){
          if (bank->currentLayout == LAYOUT_ZOOM){
            app.focusedStrip->update(data);    // traite uniquement le strip avec focus
          }else{
            for (const auto& strip : bank->strips) {
              // traite tous les strips de la fenetre courante, c'esta dire ceux ayant la meme fenetre que le strip ayant le focus
              if (strip->win == app.focusedStrip->win) strip->update(data);
            }
          } 
        }
      } 
    }
  }*/



      //------------------
      //
      // MANAGE GUI OBJECT
      //
      //------------------
      if (screen.value != oldScreen.value){
        refreshed = true;
        // we changed the screen, therefore we rebuild the screen
        // first we acknowledge the update so it is not done te next round
        //if (debug) DEBUG_PRINT("\n%s changed from screen.id:%i.state:%i.focus:%i to screen.id:%i.state:%i.focus:%i",id.c_str(),oldScreen.id,oldScreen.state,oldScreen.focus, screen.id,screen.state,screen.focus);        
        // first clean up all component of the screen
        //for(int i = 0; i < gui->screenGUIs.size()-1; i++) delete(gui->screenGUIs.shift());
        while (gui->screenGUIs.size()>0){
          //DEBUG_PRINT("\nremaining to unstack: %i",gui->screenGUIs.size());
          Widget* widget = gui->screenGUIs.shift();
          //DEBUG_PRINT("\nobject: %s ",widget->id.c_str());
          delete(widget);
          widget = nullptr;
        }
        //gui->screenGUIs.clear();

        // then retrive the screen builder and its overlay by ID           
        for(int i = 0; i < screenBuilders.size(); i++){
          screenBuilder* builder = screenBuilders.get(i);
          if (builder->id == screen.id){
            builder->screenCallback(screen.state);
            builder->overlayCallback(screen.state);
            break;
          }
        }
        #ifdef DEBUG
          // DEBUG INDICATORS OVERLAY
          gui->add(new Label(&gui->framePerSec,Align::Right, &rightScreen, &GUI::three,16,&GUI::defaultFcolor,&GUI::defaultBcolor));    
          gui->add(new Label(&heapSize,Align::Left,&GUI::zero,&GUI::three,16,&GUI::defaultFcolor,&GUI::defaultBcolor));              
        #endif

        //DEBUG_PRINT("\nnew screen widget size: %i",gui->screenGUIs.size());
        if (gui->transparency) gui->clearScreenBuffer();
        oldScreen.value = screen.value;      
        // these are the only screens that can be recall after startup
      }
      gui->refresh(force);
      // top bar
      //pendingBarWidth = 50+gui->getSinVal(100); // min size : 20, max : 40;

      //------------------
      //
      // MANAGE APPLICATION FLOW
      //
      //------------------

      switch(screen.id){
        case SCREEN_WIZARD:
          switch (screen.state){
            case STATE_WIZARD_LANG_STEP1:{
              jumpToMenu(langMenuString);   // jump to the lang menu
              screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);    
              persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_CONNECT_CONFIG_STEP2);    
            }break;
            case STATE_WIZARD_CONNECT_CONFIG_STEP2:
              handleConfigConnection();
              persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);    
              persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_CONNECT_NETWORK_STEP3); 
            break;
            case STATE_WIZARD_CONNECT_NETWORK_STEP3:
              if (!webAppConnected) screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              if (hwm->network->state == CONNECTED) screen.state = STATE_WIZARD_USER_STEP4;
            break;  
            case STATE_WIZARD_USER_STEP4:
              if (!webAppConnected) screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              if (hwm->network->state == DISCONNECTED) screen.state = STATE_WIZARD_CONNECT_NETWORK_STEP3;
              userWizardstepFlow();
            break;
          }
        break;
        case SCREEN_UTILS:
          switch(screen.state){
            case STATE_UTILS_CONNECTCONFIGMEDIA:                            
              if (hwm->configMedia->refreshed){
                handleConfigConnection();                 
              } 
            break;
            case STATE_UTILS_CONNECTWEBAPP:              
              if (hwm->configMedia->refreshed){
                handleConfigConnection();
              }
              if (webAppConnected == true){ // when web app connect we go back to normal screen
                restoreScreen();    // go back to caller. can be from bank/menu or wizard
              }
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
              if (hwm->configMedia->refreshed){
                handleConfigConnection();
              }
            break;
          }
        break;
        case SCREEN_MENU:          
          if (refreshed) menuSystemFullRefresh(); 
        break;
        case SCREEN_BANK_SWITCH:          
          if (refreshed) menuSwitchBankFullRefresh(); 
        break;
        case SCREEN_BYE:
          switch(screen.state){ 
            case STATE_BYE_GOSLEEP_MESSAGE:
              if (millis()-goSleepCountdownStart>BAT_REFRESH_RATE*BAT_NUM_READINGS*2) goDeepSleep(); // we bootSprite two full cycle of battery voltage acquisition before shuting down
            break;
            case STATE_BYE_MODAL_MESSAGE: 
              // we do not shutdown, we leave it on so we can go to the menu for instance to perform some action like update or ftp ...            
            break;
          }
        break;
      }
      //------------------
      //
      // MANAGE DEBUGGING 
      //
      //------------------
      uint32_t mem = ESP.getFreeHeap();
      if (mem>maxHeapSize) maxHeapSize = mem;
      if (mem<minHeapSize) minHeapSize = mem;
      heapSize = String(minHeapSize)+"/"+String(mem)+"/"+String(maxHeapSize);

      return refreshed;
    }

    void broadCast(String msg){
        if (webAppConnected && hwm->configMedia->state == CONNECTED){
          wss->broadcast(WebSocketDataType::TEXT, msg.c_str(), msg.length());  
          //DEBUG_PRINT("\n\n\t\t\t * * * * broadCast(%s) * * * ",msg.c_str());
        }
    }

    void reset(){      
      nvs_flash_erase(); // erase the NVS partition and...
      nvs_flash_init(); // initialize the NVS partition.        
      // next: delete conf file   
    }


    #define WEB_STOP false
    #define WEB_START true

    void setWSServer(){

      if (wss != nullptr){
        //wss->shutdown();  // need the patch here: https://github.com/skaarj1989/mWebSockets/discussions/67#
        delete wss; 
      }
   
      if (hwm->configMedia->media == CABLE_MEDIA) wss = new EthernetWebSocketServer{};
      else wss = new WiFiWebSocketServer{};

      wss->onConnection(socketConnectionCallback);    
      wss->begin();

    }

    TaskHandle_t webTaskHandle = nullptr;

    static void webTaskLoop(void* parameter) {
        ApplicationManager* self = static_cast<ApplicationManager*>(parameter);
        while (true) {
          self->handleWeb(nullptr);
        }
    }

    // Méthode pour gérer la tâche en fonction de l'état
    void webTaskhandle(bool state) {
        if (state) {
            if (webTaskHandle == nullptr) {
                // Créer une nouvelle tâche si elle n'existe pas déjà
                xTaskCreatePinnedToCore(
                    webTaskLoop,       // Méthode de la tâche
                    "webserver",         // Nom de la tâche
                    4096*2,             // Taille de la pile
                    this,             // Paramètre passé à la tâche
                    1,                // Priorité
                    &webTaskHandle,      // Handle de la tâche
                    PRO_CPU_NUM       // CPU pour exécuter la tâche APP_CPU_NUM = 1 = loop()
                );
                //DEBUG_PRINT("\nwebTaskhandle start");
            } else {
                //DEBUG_PRINT("\nwebTaskhandle est déjà en cours.");
            }
        } else {
            if (webTaskHandle != nullptr) {
                // Arrêter et supprimer la tâche
                vTaskDelete(webTaskHandle);
                webTaskHandle = nullptr;
                //DEBUG_PRINT("\nwebTaskhandle STOPPED.");
            } else {
                //DEBUG_PRINT("\nNo webstaskhandle to stop");
            }
        }
    }

    bool startWebApp(){
      if (hwm->configMedia != nullptr){
        if (hwm->configMedia->state == CONNECTED){
          if (webServer == nullptr){          
            //start webserver            
            webServer = new Application();
            //webServer->setTimeout(1000);
            // inventory zipped file
            /*populateFileList("gz","/webapp",&webGZFiles,"/",false);              
            for (uint8_t i=0;i<webGZFiles.size();i++){          
              webServer->get(webGZFiles.get(i)->c_str(), &gzFileServer);
              DEBUG_PRINT("\n ...... REGISTERING WEB GZ FILE: %s",webGZFiles.get(i)->c_str());
            }*/

            //populateFileList("*","/webapp",&webFiles,"/");  // prefill, maybe usefull if LittleFS is too slow

            // any other file
            webServer->use(commonFileServerWrapper);
            setWSServer();
            hwm->configIP = "http://"+hwm->configMedia->IP.toString();

            //webTaskhandle(WEB_START);

            /* Initialize mDNS => doesnt work for ethernet so we give up and keep IP address
            if (debug) DEBUG_PRINT("\nstarting mdns: %s",bankCfg.getString(S_AP_NAME)->c_str());
            if (!MDNS.begin(*bankCfg.getString(S_AP_NAME))) {   // Set the hostname to "esp32.local"
              if (debug) DEBUG_PRINT("Error setting up MDNS responder!");
            } */                       
          }
          return true;
        }
        return false;
      }
      return false;
    }
    
    #define MIME_TYPES_MAX 15
    /*void commonFileServer2(Request &req, Response &res){
      
      const char * path = req.path();
    
      String mimeTypes[MIME_TYPES_MAX][2] ={
        {"html",  "text/html"},
        {"js",    "application/javascript"},
        {"json",  "application/json"},
        {"jpeg",  "image/jpeg"},
        {"jpg",   "image/jpeg"},
        {"gif",   "image/gif"},
        {"png",   "image/png"},  
        {"txt",   "text/plain"},  
        {"htm",   "text/html"},
        {"xml",   "application/xml"},  
        {"css",   "text/css"},  
        {"gz",    "application/gzip"},  
        {"zip",   "application/zip"},
        {"ttf",   "font/ttf"},
        {"svg",   "image/svg+xml"}
      };
      String mimeType = mimeTypes[0][1];  //default to text/html
      for (uint8_t i=0;i<MIME_TYPES_MAX;i++){
        if (String(path).endsWith(mimeTypes[i][0])) mimeType = mimeTypes[i][1];
      }
      res.set("Content-Type", mimeType.c_str());

      Serial.printf("\n\tserving: %s <%s>",path, mimeType.c_str());
 
      File file;

      if (strcmp(path, "/") == 0) file = LittleFS.open("/webapp/indexnozip.html");
      else file = LittleFS.open("/webapp"+String(path));
      
      while (file.available()) res.write(file.read());      

      res.end();           
      file.close();
    }*/

        String mimeTypes[MIME_TYPES_MAX][2] ={
          {"html",  "text/html"},
          {"js",    "application/javascript"},
          {"json",  "application/json"},
          {"jpeg",  "image/jpeg"},
          {"jpg",   "image/jpeg"},
          {"gif",   "image/gif"},
          {"png",   "image/png"},  
          {"txt",   "text/plain"},  
          {"htm",   "text/html"},
          {"xml",   "application/xml"},  
          {"css",   "text/css"},  
          {"gz",    "application/gzip"},  
          {"zip",   "application/zip"},
          {"ttf",   "font/ttf"},
          {"svg",   "image/svg+xml"}
        };


    /*bool webFileExists(String fileName){
      //ets_printf("\nwebFileExists(%s)",fileName.c_str());
      for (int i = 0; i < webFiles.size(); i++) {
        //ets_printf("\n\t<> %s", webFiles.get(i)->c_str());
        if (*webFiles.get(i) == fileName) return true; // Libérer la mémoire des objets String
      }      
      return false;
    }*/

    /*void commonFileServer2(Request &req, Response &res){      
      httpServing = true;
      //if (req.method() != Request::GET) return;
      String path = req.path();

      ets_printf("\t %s <---\t",path.c_str());

      if(path=="/post/"){ 
        
        httpPost(req,res);

      }else{

        if(path == "/"){
          path = "/index.html.gz"; //path = "/index.html";      INDEX is the only exception were we process the gz here because we remap / to index
          #ifdef USE_NETWORK
            if (persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK) == SCREEN_WIZARD) path = "/wizard.html.gz"; //path = "/wizard.html";
          #endif
        }               

        bool fileCheckedYet = false;

        if (webFileExists(String(path)+".gz")){
          path = path+".gz";
          fileCheckedYet = true;
        }
        
        String filenameRealExtension = path;

        if (filenameRealExtension.endsWith(".gz")){
          filenameRealExtension.replace(".gz","");
          res.set("Content-Encoding", "gzip");
        }

        if (!fileCheckedYet){
          if (!webFileExists(path)){
            res.sendStatus(404);   //send(404, "text/plain", "Not found");    
            res.set("Content-Type", "text/html");
            res.print(path.c_str());
            res.end();          
            return;
          }
        }

        String fullPath = "/webapp"+String(path);   
        File file = LittleFS.open(fullPath);
        if (file.isDirectory()) return;


        String mimeType = mimeTypes[0][1];
        for (uint8_t i=0;i<MIME_TYPES_MAX;i++){
          if (filenameRealExtension.endsWith(mimeTypes[i][0])) mimeType = mimeTypes[i][1];
        }

        uint32_t free = ESP.getFreeHeap();

        //if (debug) DEBUG_PRINT("\t\t\t\t REAL EXTENSION: \t\t\tt%s \t\t\tMIME:%s",filenameRealExtension.c_str(),mimeType.c_str());
        ets_printf("\n\t\t<%s> [%d] ->%s : ",mimeType.c_str(), free, fullPath.c_str() );        
        res.set("Content-Type", mimeType.c_str());

       
        res.set("Access-Control-Allow-Origin","*");

        //if (hwm->network->media == WIFI_MEDIA) while (file.available()){ res.write(file.read()); vTaskDelay(1);}
        //else
          sendFileInChunks(&file,&res);
        res.end();
        httpServing = false;
      }
    }*/

    void commonFileServer(Request &req, Response &res){      
      httpServing = true;
      //if (req.method() != Request::GET) return;
      String path = req.path();

      if(path=="/post/"){ 
        httpPost(req,res);
      }else{
        if(path == "/"){
          path = "/index.html.gz"; //path = "/index.html";      INDEX is the only exception were we process the gz here because we remap / to index
          #ifdef USE_NETWORK
            if (persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK) == SCREEN_WIZARD) path = "/wizard.html.gz"; //path = "/wizard.html";
          #endif
        }               

        bool fileCheckedYet = false;

        //ets_printf("\tFS Exist1\t");
        if (LittleFS.exists("/webapp"+String(path)+".gz")){
          path = path+".gz";
          fileCheckedYet = true;
        }
        
        String filenameRealExtension = path;

        if (filenameRealExtension.endsWith(".gz")){
          filenameRealExtension.replace(".gz","");
          res.set("Content-Encoding", "gzip");
        }

        String fullPath = "/webapp"+String(path);   

        if (!fileCheckedYet){
          //ets_printf("\tFS Exist2\t");
          if (!LittleFS.exists(fullPath)){
            res.sendStatus(404);   //send(404, "text/plain", "Not found");    
            res.set("Content-Type", "text/html");
            res.print(fullPath.c_str());
            res.end();          
            return;
          }
        }
        
        File file = LittleFS.open(fullPath);
        if (file.isDirectory()) return;
        
        String mimeType = mimeTypes[0][1];
        for (uint8_t i=0;i<MIME_TYPES_MAX;i++){
          if (filenameRealExtension.endsWith(mimeTypes[i][0])) mimeType = mimeTypes[i][1];
        }

        uint32_t free = ESP.getFreeHeap();

        //if (debug) DEBUG_PRINT("\t\t\t\t REAL EXTENSION: \t\t\tt%s \t\t\tMIME:%s",filenameRealExtension.c_str(),mimeType.c_str());
        //ets_printf("\t\t<%s> [%d] ->%s : ",mimeType.c_str(), free, fullPath.c_str() );        
        res.set("Content-Type", mimeType.c_str());

        /*byte buffer[33] = "Access-Control-Allow-Origin: *\r\n";
        res.beginHeaders();        
          res.write(buffer,33);         // does not work
        res.endHeaders();*/
        
        res.set("Access-Control-Allow-Origin","*");

        /*if (hwm->network->media == WIFI_MEDIA) while (file.available()){ res.write(file.read()); vTaskDelay(1);}
        else */
        sendFileInChunks(&file,&res);
        res.end();
        httpServing = false;
      }
    }


    void sendFileInChunks(File* file, Response* res) {      
      size_t chunkedDataSize = 64; // with Ethernet we have to keep it low otherwise it doesnt work
            
      if (hwm->network->media == WIFI_MEDIA){
        chunkedDataSize = file->size();  // to try to send it once
        if (chunkedDataSize>TCP_SND_BUF) chunkedDataSize = TCP_SND_BUF;   // size  of the TCP buffer as a maximum
        chunkedDataSize = 8;
      }
      
      byte *buffer = nullptr; // Pointeur pour la mémoire allouée dynamiquement
      // Allouer dynamiquement de la mémoire pour stocker les données
      buffer = new (std::nothrow) byte[chunkedDataSize]; // +1 pour le caractère null

      int bytesRead;
      // Lire et envoyer des paquets de 256 octets jusqu'à ce que tout le fichier soit envoyé
      while ((bytesRead = file->read(buffer, chunkedDataSize)) > 0) {
        res->write(buffer, bytesRead);  // Écrire le tampon complet dans le client
        //DEBUG_PRINT( "%d,",bytesRead);  
        taskYIELD();
      }

      free(buffer);

      file->close();
    }    

    void render(Menu const& menu) const {}
    void render_menu_item(MenuItem const& menu_item) const {}
    void render_back_menu_item(BackMenuItem const& menu_item) const {}
    void render_numeric_menu_item(NumericMenuItem const& menu_item) const {}
    void render_menu(Menu const& menu) const {}  

    void menuSystemFullRefresh(){  
      //DEBUG_PRINT("\nmenuSystemFullRefresh");
      setRootMenuItems(&ms);
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      menuTopText = *lang.getString(" "); // empty message
    }

    void setRootMenuItems(MenuSystem* m){
      //ms.get_current_menu()->get_current_component(
      Menu const* menu = m->get_current_menu();
      uint8_t numItem = menu->get_num_components();  

      MenuComponent const* cp_m_comp;
      uint8_t selection = 0;

      cp_m_comp = menu->get_current_component();
      const char* menuString = cp_m_comp->get_name();
      menuItemIcon[1] = getIconFromString(menuString);
      selection = menu->get_current_component_num();    

      //DEBUG_PRINT("\nsetRootMenuItems name:%s numItem:%i currentComponentName: %s, menuIcon:[%s]",menu->get_name(),numItem,menuString, menuItemIcon[1]);

      menuBottomLabel = *lang.getString(getNameFromString(menuString));
      if (numItem == 1){
          menuItemIcon[0] = " ";
          menuItemIcon[2] = " ";
      }else if (numItem == 2){
        if (selection == numItem-1){ 
          // left = right = first item
          menuItemIcon[0] = getIconFromString(menu->get_menu_component(0)->get_name());
          menuItemIcon[2] = getIconFromString(menu->get_menu_component(0)->get_name());
        }else{
          // left = right = second ite,
          menuItemIcon[0] = getIconFromString(menu->get_menu_component(1)->get_name());
          menuItemIcon[2] = getIconFromString(menu->get_menu_component(1)->get_name());
          
        }
      }else if (numItem > 2){
        if (selection == numItem-1){ // last item
          // left icon = -1, right icon = first item
          menuItemIcon[0] = getIconFromString(menu->get_menu_component(selection-1)->get_name());
          menuItemIcon[2] = getIconFromString(menu->get_menu_component(0)->get_name());
        }else if (selection == 0){ // first item
          // left icon = last item, right icon = +1
          menuItemIcon[0] = getIconFromString(menu->get_menu_component(numItem-1)->get_name());
          menuItemIcon[2] = getIconFromString(menu->get_menu_component(selection+1)->get_name());
        }else{ // any item in between
          // normal case, we take the -1 and +1 item
          menuItemIcon[0] = getIconFromString(menu->get_menu_component(selection-1)->get_name());
          menuItemIcon[2] = getIconFromString(menu->get_menu_component(selection+1)->get_name());
        }  
      }
    }

    void setBankMenuItems(){
      Menu const* menu = bankMenu->get_current_menu();
      uint8_t numItem = menu->get_num_components();  

      MenuComponent const* cp_m_comp;
      cp_m_comp = menu->get_current_component();
      const char* menuString = cp_m_comp->get_name();
    
      uint8_t selection = menu->get_current_component_num();    
      IntervalBank* bank = banks.get(selection);      

      if (bank != nullptr){
        menuBottomLabel = bank->name;
        menuItemIcon[1] = bank->icon;

        if (selection == numItem-1){ // last item
          // left icon = -1, right icon = first item
          menuItemIcon[0] = banks.get(selection-1)->icon;
          menuItemIcon[2] = banks.get(0)->icon;
        }else if (selection == 0){ // first item
          // left icon = last item, right icon = +1
          menuItemIcon[0] = banks.get(numItem-1)->icon;
          menuItemIcon[2] = banks.get(selection+1)->icon;
        }else{ // any item in between
          // normal case, we take the -1 and +1 item
          menuItemIcon[0] = banks.get(selection-1)->icon;
          menuItemIcon[2] = banks.get(selection+1)->icon;
        }  
      }else{
        menuBottomLabel = *lang.getString("No bank");
        menuItemIcon[1] = ICON_QUIT;        
        menuItemIcon[0] = " ";        
        menuItemIcon[2] = " ";        
      }      
    }

    void menuSwitchBankFullRefresh(){  
      //DEBUG_PRINT("\nmenuSystemFullRefresh");
      setBankMenuItems();
      //widgets[MENU_ICON_MIDDLE].widget->setFcolor(glowingGoldColor.value); // restore color
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      menuTopText = *lang.getString(" "); // empty message
    }

    void switchBank(){
      // if we have one bank or two we shift directly      
      if (banks.size() == 2) screen.state = (screen.state + 1) % banks.size(); // <=2
      // otherwise we call the menu
      else if (banks.size()>2) screen.id = SCREEN_BANK_SWITCH;      
      //else banks are empty, we do nothing
    }

    /*void testHttpRequest(){

      String srv = "82.65.4.93"; String path="/footswitch.php?check=1.2";
      DEBUG_PRINT("\nhttp.get(%s,%s)",srv.c_str(),path.c_str());
      int err = hwm->network->http->get(srv.c_str(), path.c_str());
      if (err == 0){
        DEBUG_PRINT("\nstartedRequest ok");
        err = hwm->network->http->responseStatusCode();
        if (err >= 0) {
          DEBUG_PRINT("\nGot status code: %i",err);
          // Usually you'd check that the response code is 200 or a similar "success" code (200-299) before carrying on, but we'll print out whatever response we get
          err = hwm->network->http->skipResponseHeaders();
          if (err >= 0){
            int bodyLen = hwm->network->http->contentLength();
            DEBUG_PRINT("\nLength: %i", bodyLen);
            DEBUG_PRINT("\nBody:\n");
            // Now we've got to the body, so we can print it out
            unsigned long timeoutStart = millis();
            char c;
            // Whilst we haven't timed out & haven't reached the end of the body
            while ( (hwm->network->http->connected() || hwm->network->http->available()) && ((millis() - timeoutStart) < 30*1000) ){
                if (hwm->network->http->available()){
                    c = hwm->network->http->read();
                    DEBUG_PRINT("%c",c);
                    bodyLen--;
                    // We read something, reset the timeout counter
                    timeoutStart = millis();
                }else{
                    // We haven't got any data, so let's pause to allow some to arrive
                    delay(1000);
                }
            }
          }else{
            DEBUG_PRINT("\nFailed to skip response headers: %i",err);
          }
        }else{    
          DEBUG_PRINT("\nGetting response failed: %i",err);
        }
      }else{
        DEBUG_PRINT("Connect failed: %i",err);
      }
      hwm->network->http->stop();  
    }*/

    ///////////////////////////////////////////////////////////////////////

    //  APPLICATION & WORKFLOW MANAGEMENT METHODS

    ///////////////////////////////////////////////////////////////////////
    bool stripScroll(uint8_t direction) {
        
        IntervalBank* bank = banks.get(screen.state);
        
        if (bank->strips.size()<=1){
          //DEBUG_PRINT("\n stripscroll: only one strip,exit");
          return false; 
        }

        bank->oldSelector = bank->selector;

        if (direction == hwm->buttons[RIGHTB]->getID()) {
          if (bank->selector == bank->strips.size()) bank->selector = 1;
          else bank->selector++;
        } else if (direction == hwm->buttons[LEFTB]->getID()){
          if (bank->selector == 1) bank->selector = bank->strips.size();
          else bank->selector--;
        }

        jumpToStrip(bank->selector,bank->name);

        //DEBUG_PRINT("\t scroll(%i/%i):",bank->selector,bank->nav.window);
        
        return true;     
    }


/*
      // check that there is at least one slot available min this bank otherwise, we will have an infinite recursive call
      uint8_t nEmpty = 0;
      for (; nEmpty <BANK_MAX_STRIPS; nEmpty++) if (bank->get(nEmpty) != nullptr) break;
      if (nEmpty < BANK_MAX_STRIPS) {

        if (direction == hwm->buttons[RIGHTB]->getID()) {
          bank->selector++;
          if (bank->selector == STRIP_PER_WINDOW) {
            bank->selector = 0;
            bank->window++;
            if (bank->window == bank->getMaxWindow()) bank->window = 0;
            changedWindow = true;
          }
        } else if (direction == hwm->buttons[LEFTB]->getID()){
          if (bank->selector == 0) {
            if (bank->window == 0) bank->window = bank->getMaxWindow() - 1; 
            else bank->window--;
            bank->selector = STRIP_PER_WINDOW-1;
            changedWindow = true;
          } else bank->selector--;
        }else return false;

        forceChangedWindow = changedWindow;
        //we now check if slot is empty so we skip it
        bank->stripIndex = (bank->window * STRIP_PER_WINDOW) + bank->selector;
        screen.focus = bank->stripIndex ;
        if (bank->get(bank->stripIndex) == nullptr) {
          int8_t before = bank->window;
          DEBUG_PRINT("\t skip(%i) ",bank->stripIndex);
          //manage the case when an empty slot is the first or last of a window... it means we have to force the refresh as it will change the winodow
          changedWindow = stripScroll(direction, bank);
          int8_t after = bank->window;
          if (before != after) changedWindow = true;
        }
      }
      if (forceChangedWindow) changedWindow=true;

      // final check: if we only have one windows, any flag set above to change windows becomes invalid...
      if (bank->getMaxWindow() == 1) changedWindow = false;

      DEBUG_PRINT("\nbank->stripIndex:%i changedwindow:%i",bank->stripIndex, changedWindow);*/

    bool stripAction(uint8_t trigger) {    
      IntervalBank* bank = banks.get(screen.state);
      if (bank == nullptr) return false;
      else return bank->getStrip(bank->selector)->actionCallback(trigger);      
    }

    void stripChangeValue(uint8_t direction, unsigned long time, uint8_t trigger){
      
      IntervalBank* bank = banks.get(screen.state);    
      if (bank == nullptr) return;

      if (direction == hwm->buttons[RIGHTB]->getID()){ bank->getStrip(bank->selector)->increaseCallback(time, trigger);}      
      else if (direction == hwm->buttons[LEFTB]->getID()){ bank->getStrip(bank->selector)->decreaseCallback(time, trigger);}      

    }

    bool jumpToStrip(uint8_t id, const String& bankName) {

      // get the current window and bank, before jumping to the strip
      int8_t previousWindow = -1 ;
      IntervalBank* currentBank = banks.get(screen.state);        
      if (currentBank != nullptr){
        Strip* previousStrip = currentBank->getStrip(currentBank->oldSelector);
        if (previousStrip != nullptr) previousWindow = previousStrip->win;
      }      
      // Jump to the bank (make sure the returned pointer is valid)
      IntervalBank* targetBank = jumpToBank(bankName);
      if (targetBank == nullptr) return false;

      // Get the destination strip
      Strip* strip = targetBank->getStrip(id);
      if (strip == nullptr) return false;  // Early exit if the strip is not found

      // Set the selector to the strip's index
      targetBank->selector = strip->index;
      //highlight strip
      focus(strip,targetBank);

      // Force screen redraw if we changed the window or the bank
      //DEBUG_PRINT("\n WINDOW previous=%d current=%d",previousWindow, strip->win);      
      if (previousWindow != strip->win || currentBank != targetBank){
        triggerRedraw(); // force refresh 
        //DEBUG_PRINT("\nFORCE REFRESH!!!!!");
      }

      return true;
    }

    void focus(Strip* strip, IntervalBank* bank){
      // give focus to the strip
      for (const auto& winstrip : bank->strips) winstrip->focus(false);
      strip->focus(true);    // full brightness
      focusedStrip = strip;    
      if (stripHeader != nullptr) stripHeader->setStrip(focusedStrip);    // this trigger the update of the stripheaderwidget to the next GUI refresh
    }

    IntervalBank* jumpToBank(String bankName){
      uint8_t bankIndex = 0;
      IntervalBank* bank = getBank(bankName,&bankIndex);
      if (bank == nullptr) return nullptr;  // Early exit if the bank is not found

      persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
      persistants.putUChar(PERSISTANT_SCREEN_STATE,bankIndex);
      restoreScreen();      
      
      return bank;        
    }

    bool updatePositions(IntervalBank* bank, JsonArray positions){
      bool success = false;
      //"positions":[{"x":0,"y":0,"id":0},{"x":0,"y":1,"id":1},{"x":0,"y":2,"id":2}]}}        
      //uint8_t window = jsonInput["param"]["win"].as<int>();         
      // extract "positions"
      // Parcourir chaque objet dans le tableau
      //DEBUG_PRINT("\n Now updating selectors");
      for (JsonObject position : positions) {
        int x = position[C_X]; // extraire la valeur de x
        int y = position[C_Y]; // extraire la valeur de y
        int win = position[C_WIN]; // extraire la valeur de y
        Strip* strip = bank->getStrip(x,y,win);
        if (strip != nullptr){
          //DEBUG_PRINT("\n\t updateStrip index #%i",strip->index);
          strip->index = position[C_INDEX]; // extraire la valeur de selector
          //DEBUG_PRINT(" to #%i",strip->index);
          success = true; 
        }
      }   
      // in case of reschuffle, GUI may try to render a strip which is not existing anymore or that has changed      
      if (bank->getStrip(bank->selector) == nullptr) bank->resetStripNav();      
      return success;
    }

    /////////////////////////////////////////////////////////////////////////////////////

    #define BAD_BANK_MSG "Bad bank"
    #define BAD_STRIP_MSG "Bad strip"
    #define ERRDEL_STRIP_MSG "Delete strip"

    #define RET_UNDEFINEDCALLBACK   "undefinedCallBack"
    #define RET_SAVEBUTTON          "setSaveButton"
    #define RET_COMMANDERROR        "commandError"
    #define RET_MODALMESSAGE        "modalMessage"

    String getConnectivityState(){
      return hwm->network->serialize();
    }

    String parseJsonInput(String input){

      JsonDocument jsonInput;
      deserializeJson(jsonInput, input);

      jsonAnswer.callback = "undefinedCallBack";
      jsonAnswer.value = jsonInput.as<String>();

      //DEBUG_PRINT("\nparseJsonInput(%s)",input.c_str());
      
      String command = jsonInput["command"];

      if (command == "ping"){        
        // socket heartbeat            
        jsonAnswer.callback="pong";
        jsonAnswer.value=userJsonPongCallback();
      }else if (command == "retrieve"){

        String param = jsonInput["param"];
        jsonAnswer.callback = param;

        if(param == "fullConfig"){
          serialize(&jsonAnswer.value);
        }else if (param == "wizardConfig"){
          //jsonAnswer.value = "{\"volatile\":{\"langs\":[\"english\",\"francais\"]},\"system\":{\"configNames\":[\"anto\",\"thomas\",\"laurent\"],\"pushDelay\":\"550\",\"dClicReactivity\":\"0\",\"faderSpeed\":\"150\",\"profile\":\"default.conf\",\"apName\":\"footswitch\",\"lastSSID\":\"intervalmix\",\"lang\":\"francais\",\"ssid\":{\"intervalmix\":\"intervalpop74\",\"infrafast\":\"Mf9k8iie2\",\"ssid3\":\"pass3\"}}}";
          JsonDocument doc;
          getCommonConfig(doc);
          serializeJson(doc, jsonAnswer.value);  
          removeJsonQuote(&jsonAnswer.value);
          doc.shrinkToFit();
        }else if (param == "connectivityConfig"){
          jsonAnswer.value = getConnectivityState();
        }else if (param == "configBanks"){
          JsonDocument doc;
          
          JsonArray ds = doc["datasets"].to<JsonArray>();  
          dataRepository.serialize(&ds);         

          String banksString = serializeBanks();
          doc["banks"] = banksString;

          serializeJson(doc,jsonAnswer.value);
          removeJsonQuote(&jsonAnswer.value); 
          doc.shrinkToFit();
        }else jsonAnswer.callback = RET_UNDEFINEDCALLBACK;


      }else if (command == "set"){
        //{"command":"set","param":{"section":"system","id":"pushDelay","value":"3"}}
        String section = jsonInput["param"]["section"];
        String id = jsonInput["param"]["id"];
        String value = jsonInput["param"]["value"];
        //DEBUG_PRINT("\t => section:%s id:%s value:%s",section.c_str(),id.c_str(),value.c_str());
        bool success;        
        if (section == "system"){
          success = system.setValue(id,value);
          if (success){
            applySystemSettings();      // ===>  propagate config values by giving reference to conf object using parameters instead of using local variable
            system.save();
          }
        }else if (section == "config"){
          success = bankCfg.setValue(id,value);
          if (success) loadConfig();
        }
        if (success){
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value ="1";
        }
      }else if (command == "scanWifi"){        // Create the parrallel scan task
        xTaskCreate(
          scanNetworksTaskWrapper,  // Function to implement the task
          "ScanNetworks",     // Name of the task
          4096,              // Stack size in words
          NULL,               // Task input parameter
          1,                  // Priority of the task
          NULL                // Task handle
        );
        jsonAnswer.callback = "wifiScanStart";
        jsonAnswer.value = "";        
      }else if (command == "stopWifi"){
        hwm->network->stopWiFi();
        jsonAnswer.callback = RET_SAVEBUTTON;
        jsonAnswer.value ="1";
      }else if (command == "setWifi"){
        String ssid = jsonInput["param"]["ssid"].as<String>();
        String pwd = jsonInput["param"]["password"].as<String>();
        // indicate which WiFi to connect next
        // the appmanager refresh will take care of adding it to the valid json wifi ssid list  if it connects        
        hwm->network->setSSID(ssid,pwd);        
        if (hwm->network->media != CABLE_MEDIA){
          jsonAnswer.callback = "wifiConnectTimeout"; // we give a try to connect wifi by setting up the timeout
          jsonAnswer.value = hwm->network->wiFiTimeout;
        }
        // if media is ethernet, we just feedback save button
        else{
          jsonAnswer.callback = RET_MODALMESSAGE;
          jsonAnswer.value ="wifiSaved";
        }

      }else if (command == "setStrip"){          
        //{"command":"setStrip","param":{"bank":"Test","strip":{"type":"chGain","size":"4x1","color":4,"icon":"6","properties":["/ch/3/config/name"],"position":{"win":0,"x":"0","y":"0"}},"positions":[{"x":0,"y":0,"id":0},{"x":0,"y":1,"id":1},{"x":0,"y":2,"id":2}]}}
        //get the bank that need to be modified
        jsonAnswer.callback=RET_COMMANDERROR;
        //DEBUG_PRINT("\nsetStrip bankName:%s, strip:%s",jsonInput["param"]["bank"].as<String>().c_str(),jsonInput["param"].as<String>().c_str());        
        uint8_t bankIndex;
        IntervalBank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);  // get the bank and its index
        if (bank != nullptr){          
          uint8_t window = jsonInput["param"]["strip"]["position"]["win"];            
          uint8_t x = jsonInput["param"]["strip"]["position"]["x"];     
          uint8_t y = jsonInput["param"]["strip"]["position"]["y"];
          //DEBUG_PRINT("\nChecking if strip is existing: ");     
          Strip* strip = bank->getStrip(x,y,window);
          bool success = true;
          if (strip != nullptr){
            //DEBUG_PRINT(" YES, deleting it");
            success = bank->trash(strip); // if strip is existing we first delete it
          }  
          if (success){
            JsonObject jData = jsonInput["param"]["strip"];            
            strip = bank->addStrip(jData);
        
          }else strip = nullptr;
          if (strip != nullptr){
            updatePositions(bank,jsonInput["param"][C_POSITIONS]);
            // put the focus on the strip
            uint8_t selector = strip->index;
            jumpToStrip(selector,bank->name);    //jsonAnswer should be : {"restoreStrip":{"bank":"BANK j3","strip":{"type":"text","selector":"7","x":"3","y":"0","size":"1x4","color":3,"icon":"Z","properties":["/text/"]}}}         
            triggerRedraw();    // force refresh as jumpToStrip only refresh if you change window or bank, but here we may update a strip which is the currently displayed one
            jsonAnswer.callback = RET_SAVEBUTTON;
            jsonAnswer.value = "1";  
          }else{
            jsonAnswer.value=*lang.getString("Strip setup error");
          }
        }else{
            jsonAnswer.value=*lang.getString(BAD_BANK_MSG);
        }                
      }else if (command == "trashStrip"){        
        //{"command":"trashStrip","param":{"bank":"Test","position":{"win":0,"x":"0","y":"1"},"positions":[{"x":0,"y":0,"id":0},{"x":0,"y":2,"id":1}]}}
        uint8_t bankIndex;
        IntervalBank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);
        jsonAnswer.callback = RET_COMMANDERROR;
        if (bank != nullptr){                     
          uint8_t window = jsonInput["param"]["position"]["win"];           
          uint8_t x = jsonInput["param"]["position"]["x"];     
          uint8_t y = jsonInput["param"]["position"]["y"];     
          Strip* stripToDelete = bank->getStrip(x,y,window);
          uint8_t indexToDelete = stripToDelete->index;
          Strip* currentStrip = bank->getStrip(bank->selector);
          if (stripToDelete != nullptr){          
              if (bank->trash(stripToDelete)){   
                  updatePositions(bank,jsonInput["param"][C_POSITIONS]);              
                  jsonAnswer.callback = RET_SAVEBUTTON;
                  jsonAnswer.value = "1";   
                  // we check if we delete the strip which is currently focused
                  if (indexToDelete == currentStrip->index){                   
                    //DEBUG_PRINT("\nDELETED FOCUSED STRIP");
                    bank->resetStripNav();
                  }else{
                    // since updatePositions has shifted the index, the selector points to a strip that is not the one which was preivously displayed, we restore it
                    bank->selector = currentStrip->index;                 
                    //bank->nav.window = currentStrip->win;
                  }
                  triggerRedraw(); // force refresh 
              }else{
                   jsonAnswer.value=*lang.getString(ERRDEL_STRIP_MSG); 
              }
          }else{
            jsonAnswer.value=*lang.getString(BAD_STRIP_MSG);
          }
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }  
        //DEBUG_PRINT("\n<TrashStrip end>");
      }else if (command =="moveStrip"){
        //{"command":"moveStrip","param":{"bank":"Test2","x1":0,"y1":1,"x2":0,"y2":2,"win":0,"positions":[{"x":0,"y":0,"id":0},{"x":0,"y":2,"id":1}]}}
        // use position here instead of x1, y1 
        uint8_t bankIndex;
        IntervalBank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);
        jsonAnswer.callback = RET_COMMANDERROR;
        if (bank != nullptr){                                         
          /*uint8_t x = jsonInput["param"]["x1"];
          uint8_t y = jsonInput["param"]["y1"]; 
          uint8_t window = jsonInput["param"][C_WIN];
          Strip* strip = bank->getStrip(x,y,window);*/

          uint8_t id = jsonInput["param"][C_INDEX];
          Strip* strip = bank->getStrip(id);

          if (strip != nullptr){      
            ///WARNING THIS IS THE DESTINATION POSITION!!!!!! 
            strip->x = jsonInput["param"]["x2"];      
            strip->y = jsonInput["param"]["y2"];  
            updatePositions(bank,jsonInput["param"][C_POSITIONS]);  
            jsonAnswer.callback = RET_SAVEBUTTON;
            jsonAnswer.value = "1";              
            triggerRedraw(); // force refresh            
          }else{
            jsonAnswer.value=*lang.getString(BAD_STRIP_MSG);
          }
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }           
      }else if (command =="trashBank"){
        //{"command":"trashBank","param":{"name":"BANK j2"}}
        String bankName = jsonInput["param"]["name"];
        //DEBUG_PRINT("\ntrashBank %s",bankName);
        uint8_t bankIndex;
        IntervalBank* bankToDelete = getBank(bankName,&bankIndex);  // get the bank and its index
        IntervalBank* currentBank = banks.get(screen.state);

        if (bankToDelete != nullptr){
          banks.remove(bankIndex);  // this will shift bank index but we have saved the one currently displayed
          delete bankToDelete;
          if (bankIndex == screen.state || screen.id == SCREEN_BANK_SWITCH){

            //DEBUG_PRINT("\nDELETE CURRENT BANK");

            if (banks.size()>0){
              jumpToBank(banks.get(0)->name);  // go to first bank in case we delete the current displayed or we are showing menu switch
            }else{
              //DEBUG_PRINT("\nLAST BANK WAS TRASHED");
              
            }
          }else{
            getBank(currentBank->name,&screen.state); // restore the bank
          }
          hookBankToMenu();   // rebuild the bank switch menu
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";            
          triggerRedraw(); // force refresh           
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }

      }else if (command == "newBank"){
        //{"command":"newBank","param":{"newName":{}}}
        bool ret = false;
        String jsonBankstring = jsonInput["param"];
        IntervalBank* bank = deserializeBanks(&jsonBankstring);
        if (bank != nullptr){
          ret = true;
          jumpToBank(bank->name);
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";            
        }else jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
      }else if (command == "renameBank"){
        //{"command":"renameBank","param":{"oldname":"BANK 1","newname":"BANK 2"}}
        String oldName = jsonInput["param"]["oldname"];
        String newName = jsonInput["param"]["newname"];
        uint8_t bankIndex;
        IntervalBank* bank = getBank(oldName,&bankIndex);  // get the bank and its index
        if (bank != nullptr){
          bank->name = newName;
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";
           triggerRedraw(); // force refresh the bank screen or the bank switch
        }else jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
      }else if (command == "save"){     
        String fullConfig;
        serialize(&fullConfig);
        bool ret = bankCfg.save();
        ret &=  system.save();   
        userConfig.save();   
        if (ret){
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "0";  
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
          jsonAnswer.value = *lang.getString("Save error");
        }      
      }else{
        //DEBUG_PRINT("\n\t\t\t\t\tCalling userParseJsonInputCallback...");
        JsonObject j = jsonInput.to<JsonObject>();
        if (!userParseJsonInputCallback(j,&jsonAnswer)){

          /*JsonUserEvent je;
          je.jsonAnswer = &jsonAnswer;
          je.jsonInput = jsonInput;

          evt.type = EVENT_PARSE_JSONINPUT;
          evt.data = &je;

          if (!fireEvent(&evt)){*/
            jsonAnswer.callback = RET_UNDEFINEDCALLBACK;
            jsonAnswer.value = jsonInput.as<String>();
          //}
      
      
        }
      }


      JsonDocument jsonOutput;
      jsonOutput[jsonAnswer.callback] = jsonAnswer.value;

      String jsonString;
      serializeJson(jsonOutput,jsonString);
      removeJsonQuote(&jsonString);

      //DEBUG_PRINT("\nbuildJsonOuput: %s",jsonString.c_str());
      jsonOutput.shrinkToFit();
      return jsonString;

    }    
    
    void scanNetworksTask(void * parameter) {
      #define MAX_NUMBER_OF_NETWORK_TO_REPORT 10     
      //DynamicJsonDocument doc(60+50*MAX_NUMBER_OF_NETWORK_TO_REPORT);
      JsonDocument doc;
      //JsonArray scannedWifi = doc.createNestedArray();
      JsonArray scannedWifi = doc["scannedWifi"].to<JsonArray>();           
      //DEBUG_PRINT("\nScanning for WiFi networks...");
      int numberOfNetworks = WiFi.scanNetworks();
      //DEBUG_PRINT("Scan complete");
      for (int i = 0; i < numberOfNetworks && i<MAX_NUMBER_OF_NETWORK_TO_REPORT; i++) {
        //DEBUG_PRINT("\nNetwork name: %s",WiFi.SSID(i).c_str());
        scannedWifi.add(WiFi.SSID(i));

        String ssid = WiFi.SSID(i);         // Nom du réseau (SSID)
        int32_t rssi = WiFi.RSSI(i);        // Puissance du signal (RSSI)
        int32_t channel = WiFi.channel(i);  // Canal utilisé
        bool isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
  
        // Affichage des infos du réseau
        DEBUG_PRINT("\nRéseau #%d :", i + 1);
        DEBUG_PRINT("\n\tSSID\t\t\t\t: %s", ssid.c_str());
        DEBUG_PRINT("\n\tRSSI\t\t\t\t: %d dBm", rssi);
        DEBUG_PRINT("\n\tCanal\t\t\t\t: %d", channel);
        DEBUG_PRINT("\n\tSécurisé\t\t\t\t: %s\n", isEncrypted ? "Oui" : "Non");

      }     
      String data;
      WiFi.scanDelete();
      serializeJson(doc,data);
      doc.shrinkToFit();
      broadCast(data);
    }


    IntervalBank* getBank(const String& name, uint8_t* index) {
        if (banks.size()<1) return nullptr;  // Prevents accessing an empty list

        for (uint8_t i = 0; i < banks.size(); i++) {  
            IntervalBank* bank = banks.get(i);
            if (bank && bank->name == name) {  // Ensure bank is valid before using it
                *index = i;
                return bank;
            }
        }
        
        return nullptr;  // Not found
    }

  private:
    HardwareManager* hwm;
    GUIManager* gui;
    LinkedList<screenBuilder*> screenBuilders = LinkedList<screenBuilder*>();      

    JsonAnswer jsonAnswer;

    JsonList wiFiList;    
         
    String defaultSysConf = "{"
    //"\""+String(S_CONFIGNAMES)+"\":[\"anto\",\"thomas\",\"laurent\"]\","
      "\""+String(S_PUSHDELAY)+"\":\"852\","
      "\""+String(S_DCLICREACTIVITY)+"\":\"251\","
      "\""+String(S_CONFIG_FILE)+"\":\""DEFAULT_CONFIG_FILE_NAME"\"," 
      "\""+String(S_AP_NAME)+"\":\"footswitch\","
      "\""+String(S_LAST_USED_SSID)+"\":\"intervalmix1\","
      "\""+String(S_LANG)+"\":\"francais\","
      "\""+String(S_FADERSPEED)+"\":\"157\","
      "\""+String(S_SSID)+"\":{\"intervalmix1\":\"intervalpop74\",\"infrafast\":\"Mf9k8iie2\",\"intervalmix\":\"intervalpop74\",\"IpTch\":\"jt2p9ug1\"},"
      "\""+String(S_BUTTON_ASSIGN)+"\":{\"left\":[3,1,5,8],\"right\":[2,1,4,9],\"tap\":[6,7,8,10]},"
      "\""+String(S_SHORTCUTS)+"\":{\"shortcut1\":{\"type\":\"jump\",\"properties\":[\"/strip/id/second~module:artNet/toto\"]},\"shortcut2\":{\"type\":\"macro\",\"properties\":[\"/action/FF\",\"/macro/name/colors\"]},\"shortcut3\":{\"type\":\"advanced\",\"properties\":[\"/command//ch/level/03~55\"]}}"
    "}";

    String defaultBanksConf = "{\"banks\":{\"bank1\":{\"possibleLayout\":0,\"currentLayout\":0,\"icon\":3,\"strips\":[{\"type\":\"text\",\"color\":1,\"icon\":5,\"size\":\"2x2\",\"position\":{\"x\":1,\"y\":1,\"id\":3,\"win\":2},\"properties\":[\"/text/change-me\"]},{\"type\":\"text\",\"color\":5,\"icon\":5,\"size\":\"2x2\",\"position\":{\"x\":1,\"y\":1,\"id\":2,\"win\":1},\"properties\":[\"/text/hello\"]},{\"type\":\"text\",\"color\":3,\"icon\":5,\"size\":\"2x2\",\"position\":{\"x\":1,\"y\":1,\"id\":1,\"win\":0},\"properties\":[\"/text/world\"]}]}}}";
    String defaultUserConf = "{}";

    ScreenUnion oldScreen;    

    LinkedList<String*> countryList = LinkedList<String*>();   
    //LinkedList<String*> webGZFiles = LinkedList<String*>();   
    //LinkedList<String*> webFiles = LinkedList<String*>();   

    IWebSocketServer *wss{ nullptr };        

    uint32_t minHeapSize,maxHeapSize;
    String heapSize;    

    ///////////////////////////////////////////////////////////////////////

    void httpPost(Request &req, Response &res){
      
      constexpr size_t MAX_COMMAND_SIZE = 2048; // Limite maximale pour éviter un abus de mémoire
      char *name = nullptr; // Pointeur pour la mémoire allouée dynamiquement

      #define OPTION_SIZE 64
      char options[OPTION_SIZE];    //options is not used (the fetch from client puts everything in the name as a json)


      // Allouer dynamiquement de la mémoire pour stocker les données
      name = new (std::nothrow) char[MAX_COMMAND_SIZE]; // +1 pour le caractère null
      if (!name) {
          
          res.sendStatus(500); // Internal Server Error
          res.print("{\"error\":\"Memory allocation failed\"}");
          res.end();
          return;
      }


      res.set("Content-Type", "application/json");


      unsigned long t0 = millis();
      #define POST_TIME_OUT 5000

      while (req.left() && millis()-t0<POST_TIME_OUT) {        
        req.form(name, MAX_COMMAND_SIZE, options, OPTION_SIZE);                
        
        // Vérification de débordement et de validité
        if (strlen(name) >= MAX_COMMAND_SIZE || strlen(options) >= OPTION_SIZE) {
            
            res.sendStatus(500); // Internal Server Error
            res.print("{\"error\": \"Parameters too long\"}");
            continue; // Ignorer cette itération
        }        
        
        if (debug){
          //DEBUG_PRINT("\n\n\t httpPost <%s>:<%s>\n\n",name,options);
        } 


        String jsonAnswer = parseJsonInput(name);        

        // version compressée
        ByteBuffer compressedBuffer;
        size_t chunkedDataSize = bufferToGz(&jsonAnswer,&compressedBuffer);
        res.set("Content-Encoding", "gzip"); // Indique que la réponse est en gzip
        #define PACKET_SIZE 64 // Taille d'un paquet (64 octets)
        uint8_t packet[PACKET_SIZE];             // Crée un buffer temporaire de 64 octets               
        while (compressedBuffer.available()) {
          size_t bytesToRead = min(PACKET_SIZE, compressedBuffer.available()); // Détermine le nombre d'octets à lire
          compressedBuffer.readBytes((char*)packet, bytesToRead);
          res.write(packet, bytesToRead);
          yield(); // 
        }

        /*
        // avoid using the ByteBuffer class => 
          uint8_t* dstBuf = NULL;
          size_t dstBufLen = LZPacker::compress( srcBuf, srcBufLen, &dstBuf );
          // do something with dstBuf
          free(dstBuf);        
        */

        // versionnon  compressée
        /*        
        size_t chunkSize = 64;
        size_t totalSize = jsonAnswer.length();
        size_t sentSize = 0;
        while (sentSize < totalSize) {
            size_t remainingSize = totalSize - sentSize;
            size_t currentChunkSize = (remainingSize > chunkSize) ? chunkSize : remainingSize;
            res.write((uint8_t*)(jsonAnswer.c_str() + sentSize), currentChunkSize);
            sentSize += currentChunkSize;
            yield();
        }*/



      }  
      res.end();
      delete[] name;
    } 


    void populateFileList(String extension, String folder, LinkedList<String*> *list, String prefix="", bool withExtension=true) {
      //we first clean all items from the list

      while (list->size()>0){
        String* s = list->shift();
        delete(s);
      }

      File root = LittleFS.open(folder);
      //DEBUG_PRINT("\n\nListing: .%s in %S",extension.c_str(),folder.c_str());
      File file = root.openNextFile();
      while (file){
        if (!file.isDirectory()){
          String fullFileName = file.name();
          if (fullFileName.endsWith(extension) || extension == "*") {
            String* filename = new String(prefix);
            // if we don't want the extension, we remmove it          
            if (!withExtension) *filename += fullFileName.substring(0, fullFileName.lastIndexOf('.'));
            else *filename += fullFileName; // k is the icon ID
            list->add(filename);
            //DEBUG_PRINT("\n\t %s",filename->c_str());          
          }
        }
        file = root.openNextFile();
        yield();
      }
    }

    MenuItem* back_menu_item;
    MenuItem* info_menu_item;
    MenuItem* config_menu_item;
    Menu* profile_menu_item;
    MenuItem* save_profile_menu_item;
    MenuItem* tft_rotation_menu_item;
    MenuItem* reset_menu_item;   
    MenuItem* shutdown_menu_item; 
    MenuItem* brightness_menu_item; 

    void hookBankToMenu(){
      if (bankMenu != nullptr) delete bankMenu;
      bankMenu = new MenuSystem(*this); 

      if (banks.size()>1){                      
        // populate the menu with all banks
        for(int i = 0; i < banks.size(); i++){
          const char* bankName = banks.get(i)->name.c_str();
          bankMenu->get_root_menu().add_item(new MenuItem(bankName, &switchToBank_item_selected));                
          if (debug){
            //DEBUG_PRINT("\nadded bank switch item entry: %s",bankName);           
          }
        }
      }else{
        //DEBUG_PRINT("\nBank menu list: %s",banks.get(0)->name.c_str());  
        if (banks.size() == 1) bankMenu->get_root_menu().add_item(new MenuItem(banks.get(0)->name.c_str(), &switchToBank_item_selected));               
      }
    }    

};

#endif

