#ifndef APPManager_h
#define APPManager_h

#include <Preferences.h>
#include "nvs_flash.h" 
#include <LinkedList.h>
#include <ArduinoJson.h>
#include <Preferences.h>
//#include <esp32FOTA.hpp>

#include <aWOT.h>                 // awOT lasselukkari/aWOT@^3.5.0 wifi+eth server
                                  // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
#include <MenuSystem.h>           // https://github.com/jonblack/arduino-menusystem                        
#include <LittleFS.h>

#include <WebSocketServer.hpp>    // https://github.com/skaarj1989/mWebSockets#meta     // use a local repository with SOCKETBUFFER SIZE CHANGED one because of https://github.com/skaarj1989/mWebSockets/issues/72
using namespace net;
using WiFiWebSocketServer = WebSocketServer<WiFiServer, WiFiClient, 1>;
using EthernetWebSocketServer = WebSocketServer<EthernetServer, EthernetClient, 1>;
void socketConnectionCallback(IWebSocket &ws);

///////////////////// WIDGET LOCATION & SIZE ////////////////////////
#define OVERLAY_ICON_SIZE TFT_WIDTH/20

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

///////////////////// CONFIG PARAMETERS ////////////////////////

#define C_CURRENTLAYOUT     "currentLayout"
#define C_POSSIBLELAYOUT    "possibleLayout"
#define C_BANKS             "banks"
  #define C_STRIPS          "strips"
    #define C_PROPERTIES    "properties"
    #define C_STRIPTYPE     "type"
    #define C_SELECTOR      "selector"
    #define C_X             "x"
    #define C_Y             "y"
    #define C_WIN           "win"
    #define C_SIZE          "size"
#define C_ICON              "icon"
#define C_COLOR             "color"

///////////////////// VOLATILE PARAMETERS ///////////////////////
#define V_USE_NETWORK       "myNetworkIP"

// C to C++ class function wrapper

void commonFileServerWrapper(Request &req, Response &res);  
void gzFileServer(Request &req, Response &res);
void scanNetworksTaskWrapper(void * parameter);
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
  STATE_WIZARD_USER_STEP3,
  STATE_WIZARD_END_STEP4,
  STATE_UTILS_SEARCHING_CONNECTION,
  STATE_UTILS_INFO,
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

typedef bool (*ParseJsonInputCallback)(JsonDocument,JsonAnswer*);
typedef void (*NetworkChangeStateCallback)(bool state);

bool defaultParseJsonInputCallback(JsonDocument jsonDoc,JsonAnswer* jsonAnswer){
  String jsonInputString;
  serializeJson(jsonDoc, jsonInputString);
  DEBUG_PRINT("\ndefaultParseJsonInputCallback(%s)",jsonInputString.c_str());
  return false;
}

String defaultPongCallback(){
  return "";
}

String defaultUserJsonConfigCallback(){
  return "";
}

void defaultWizardScreenBuilder();
void defaultBootScreen();

void defaultNetworkChangeStateCallback(bool state){
  DEBUG_PRINT("\ndefaultNetworkChangeStateCallback(%i)",state);  
}

///////////////////// BUTTONS CALLBACKS ////////////////////////
void simpleClicCallBack(uint8_t buttonID, unsigned long time);
void longClicCallBack(uint8_t buttonID, unsigned long time);
void doubleClicCallBack(uint8_t buttonID, unsigned long time);
void releaseHoldCallback(uint8_t buttonID, unsigned long time);
void holdCallback(uint8_t buttonID, unsigned long time);

void menuSwitchCallback(uint8_t buttonID, unsigned long time);


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

void handleConfigConnection();  // 


///////////////////// DATA MANAGEMENT ////////////////////////
#include <vector>

class DataProvider{
  public :
    virtual uint16_t size(String addrr) = 0;
    virtual String getValue(String addrr, uint16_t index) = 0;
};

class DataSet{
  public:
    String address;
    String object;
    DataProvider* dataProvider;    

    ~DataSet(){
      //DEBUG_PRINT("\n\t~DataSet(%s)",address.c_str());
    }

    virtual uint16_t size(){
      return dataProvider->size(address);
    }

    virtual String getValue(uint16_t i){
      return dataProvider->getValue(address,i);
    }

    DataSet(String addr,String obj, DataProvider* dp): dataProvider(dp), address(addr), object(obj) {}

    String serialize(){

      JsonDocument doc;

      JsonArray jsonValues = doc[address].to<JsonArray>();
      uint16_t n = size();
      for (uint8_t i=0; i< n; i++) jsonValues.add(getValue(i));
      String data;
      serializeJson(doc,data);
      //DEBUG_PRINT("\n\n\tdataset->serialize: %s\n\n", data.c_str());
      return data;
    }      
};

class DataRepository{
  public:
    DataRepository(){}

    DataSet* getDataSet(String address){
      for (uint8_t i=0;i<dataSets.size();i++){
        DataSet* ds = dataSets.get(i);        
        if (address == ds->address){          
          return ds;
        } 
      }
      return nullptr;
    }

    bool addDataSet(DataSet* ds){
      dataSets.add(ds);
      /*String object = ds->object;
      String address = ds->address;      */
      return true;
    }

    void removeDataSet(DataSet* ds){
      //DEBUG_PRINT("\nremoveDataSet data set ",ds->address);
      for (uint16_t i=0;i<dataSets.size();i++){
        //DEBUG_PRINT("...%s",ds->address.c_str());
        if (dataSets.get(i)->address == ds->address){            
          dataSets.remove(i);
          //DEBUG_PRINT("removed");
          break;
        }else{
          //DEBUG_PRINT("keep");                  
        }
      }      
    }


    void serialize(JsonArray* jsonDataSets){
      //DEBUG_PRINT("\n<<<<<<<<<<<<< SERIALIZATION OF DATASETS FROM DataRepository, qty: %i >>>>>>>>>>>>>>>>>>>",dataSets.size() );
      for (uint8_t i=0;i<dataSets.size();i++){        
        //DEBUG_PRINT("\ngetting next data set: ");
        DataSet* ds = dataSets.get(i);
        //DEBUG_PRINT("name: %s size:%i address:%s",ds->dataProvider->name.c_str(),ds->size(),ds->address.c_str());
        jsonDataSets->add(ds->serialize());
      }
    }

  private:
    LinkedList<DataSet*> dataSets = LinkedList<DataSet*>();
};


class StripSkeleton;
///////////////////// STRIP MANAGEMENT ////////////////////////
#define INPUTSELECT "dropdown"
#define INPUTTEXT   "freetext"

#define LAYOUT_DETAILS  0b00000001
#define LAYOUT_ICON     0b00000100
#define LAYOUT_QUATUOR  0b00010000

class Strip : public GenericComponent{  
  #define JSON_STRIP_PROPERTY_SIZE 100
  #define  JSON_STRIP_TEMPLATE_SIZE 300
  public:
    TFT_eSprite* sprite = nullptr;
    uint8_t selector;
    uint8_t x, y, win;
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    GUIManager* gui;

    Strip(StripSkeleton* s, GUIManager* g, bool debug=false):GenericComponent(debug){
      skeleton = s;
      gui = g;
      value = (valueMax-valueMin)/2;
      //id = skeleton->stripID;
    }    

    ~Strip(){
      DEBUG_PRINT("\n~Strip(): %s",id.c_str());
    }

    virtual bool refresh(bool force=false){
      frontColor = colorMap[colorID].fColor;
      backColor = colorMap[colorID].bColor;      
      return refreshed;
    }

    bool deserialize(String values){
      bool ret = false;
      DEBUG_PRINT("\n\t deserialize STRIP(%s)",values.c_str());
      uint8_t n = getNumberOfProperties();
      DEBUG_PRINT("\t getNumberOfProperties():%i",n);      
      //DynamicJsonDocument jsonStrip(350+JSON_STRIP_PROPERTY_SIZE*n);          
      JsonDocument jsonStrip;
      DeserializationError error = deserializeJson(jsonStrip, values);
      if (error){
        DEBUG_PRINT("\n\tStrip <%s> deserialization error:%s",id.c_str(),error.f_str());
      }else{      
        id = jsonStrip[C_STRIPTYPE].as<String>();
        DEBUG_PRINT("\t<%s> properties:",id.c_str());   /////////////////// CONTINUE DEBUG
        // set index; 
        selector = jsonStrip[C_SELECTOR].as<int>();//.as<int>();
        x = jsonStrip["position"][C_X].as<int>();
        y = jsonStrip["position"][C_Y].as<int>();
        win = jsonStrip["position"][C_WIN].as<int>();
        size = jsonStrip[C_SIZE].as<String>();
        colorID = jsonStrip[C_COLOR];//.as<int>();
        icon = jsonStrip[C_ICON];//.as<unsigned char>();
        DEBUG_PRINT("\tselector=%i, colorID=%i, icon=%c",selector,colorID,icon);
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
      //DEBUG_PRINT("\nserialize %s=>",id.c_str());
      
      JsonDocument doc;
      doc[C_STRIPTYPE] = id;  // this is a text
      doc[C_SELECTOR] = selector;
      doc[C_COLOR] = colorID;
      doc[C_ICON] = icon;
      doc["position"][C_X] = x;
      doc["position"][C_Y] = y;
      doc[C_SIZE] = size;
      doc["position"][C_WIN] = win;
      
      properties = doc[C_PROPERTIES].to<JsonArray>();
      serializeProperties();
      String data;
      serializeJson(doc, data);
      //DEBUG_PRINT("%s",data.c_str());
      return data;
    }

    /*virtual void wipeWidgets(){
      DEBUG_PRINT("\nwipe %s, number of widgets:%i",id.c_str(),widgetHandles.size());
      int16_t xMin=TFT_WIDTH, xMax=0, yMin=TFT_HEIGHT, yMax=0;

      while (widgetHandles.size()>0){          
          
          Widget* widget = widgetHandles.shift();
          
          if (widget->getX()<xMin) xMin = widget->getX();
          if (widget->getX()+widget->width > xMax) xMax = widget->getX()+widget->width;
          if (widget->getY()<yMin) yMin = widget->getY();
          if (widget->getY()+widget->height > yMax) yMax = widget->getY()+widget->height;      

          //DEBUG_PRINT("\tcls %s  (%i,%i,h:%i,w:%i)  minMax=>(%i,%i,w:%i,h:%i)",widget->id.c_str(),widget->getX(),widget->getY(),widget->width,widget->height,xMin,xMax-xMin,yMin,yMax-yMin);

          GUI::tft->fillRect(xMin,yMin,xMax-xMin,yMax-yMin,GUI::defaultBcolor);

          //DEBUG_PRINT("\nremove gui->remove(%s) before:%i",widget->id.c_str(),gui->screenGUIs.size());
          gui->remove(widget);          
          delete(widget);       
          //DEBUG_PRINT("\tafter:%i",gui->screenGUIs.size());        

      }
    }*/

    virtual bool progressiveDelay(unsigned long time){
      uint16_t TIME_MAX_PROGRESSIVITY = 3000;
      if (valueMax-valueMin<=20) TIME_MAX_PROGRESSIVITY = 5000;
      else if (valueMax-valueMin>500) TIME_MAX_PROGRESSIVITY = 1000;

      //we start counting the time between two calls when we have called the first time, means during debounce duration
      if (time<DEBOUCE_TIME){
        lastCall = 0;      
        // first call we start timer

      }
      if (time>=TIME_MAX_PROGRESSIVITY) time = TIME_MAX_PROGRESSIVITY;

      unsigned long waitBetween2update = (TIME_MAX_PROGRESSIVITY-time)/5;
      unsigned long delaySoFar = millis() - lastCall;

      //DEBUG_PRINT("\nprogressiveDelay(%d) - %u/%u -",time, delaySoFar, waitBetween2update);          

      if (delaySoFar < waitBetween2update) return false;  
      lastCall = millis();
 
      return true;    
    }

    /*virtual bool increaseCallBack(unsigned long time) = 0;
    virtual bool decreaseCallback(unsigned long time) = 0;
    virtual float getIncrement(unsigned long time)=0;*/

    virtual float getIncrement(unsigned long time){
      float increment;
      //value = roundHalf(value);
      if (time<2000){
        increment = 0.1;
      }else{
        float diff = valueMax - value;
        float percentageToTarget = map(diff,0,valueMax-valueMin,100,1)/2000.0;  // 3000 seems to be a good compromise between delay and precision, 2000 react quicker but skip some decimal
        increment = diff*percentageToTarget;          
        //DEBUG_PRINT("\t => diff: %.3f, percentageToTarget: %.3f",diff, percentageToTarget);        
      }
      //DEBUG_PRINT("\t value: %.3f, increment: %.4f, ",value,increment);
      return increment;
    }    

    virtual bool increaseCallBack(unsigned long time){            
      if (!Strip::progressiveDelay(time)) return true;
      value += getIncrement(time);

      if (value > valueMax) value = valueMax;
      //value = truncateToOneDecimal(value);
      
      DEBUG_PRINT("\nincreaseCallBack: %s, time:%s",String(value).c_str(),String(time).c_str());

      return true;      
    }

    virtual bool decreaseCallback(unsigned long time){
      if (!Strip::progressiveDelay(time)) return true;      
      value -= getIncrement(time);
      

      if (value < valueMin) value = valueMin;

      //value = truncateToOneDecimal(value);

      DEBUG_PRINT("\ndecreaseCallback: %.2f",value);

      return true;      
    }

    virtual uint8_t getNumberOfProperties()=0;
    virtual bool actionCallback()=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;    
    virtual void buildWidgets(uint8_t layout)=0;
    

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;
  protected:
    uint16_t frontColor, backColor;    
    unsigned long lastCall;     
    uint8_t layout = LAYOUT_DETAILS;   
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

    String getValue(String address, uint8_t index){
      DataSet* ds = dataRepo->getDataSet(address);
      if (ds != nullptr) return ds->getValue(index);
      else return "null";
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
      DEBUG_PRINT("\nskeleton->serialize: %s",data.c_str());
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
        
    TextStrip(StripSkeleton* s,GUIManager* g,bool debug=false):Strip(s,g,debug){
      //icon = ICON_TEXT_STRIP;
    }

    ~TextStrip(){
    }

    virtual bool actionCallback(){return true;}

    virtual void buildWidgets(uint8_t layout){
      switch(layout){
        case LAYOUT_QUATUOR:
        break;
        case LAYOUT_ICON:
        break;
        case LAYOUT_DETAILS: 
        default:
          if (debug) DEBUG_PRINT("\n%s:buildWidgets Label(%s)",id.c_str(),value.c_str());
          Label* label = new Label(&value, Align::Center,&x,&y,100,&frontColor,&backColor,false);
          //widgetHandles.add(label);          
          gui->add(label);
        break;
      }
    }

    virtual void deserializeProperty(String property)  {      
      value = property;
      //if (debug){DEBUG_PRINT("\tvalue=%s",value.c_str());}
      /*
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
        //DEBUG_PRINT("\t%s=%i",select2PropertyAddress.c_str(), select2ID);      
      } */ 

    }

    void serializeProperties(){
      properties.add(value);    
    }

    virtual bool refresh(bool force=false){
      refreshed = force;
      Strip::refresh(force);
      return refreshed;
    };

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
    }

    virtual bool increaseCallBack(unsigned long time){return true;}
    virtual bool decreaseCallback(unsigned long time){return true;}
    virtual float getIncrement(unsigned long time){return 0.0;}

    private:
      String value="";
      int16_t x=GUI::width()/2, y=GUI::height()/2;       
};

class TwoSelectStrip : public Strip{

  public: 
    TwoSelectStrip(StripSkeleton* s,GUIManager* g,String *select2String, bool debug=false):Strip(s,g,debug){
      select2Content = select2String;
    }

    void serializeProperties(){
      char* buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select2PropertyAddress.c_str(),select2ID);
      properties.add(String(buf));
      delete buf;

    }

    void deserializeProperty(String property){
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
        //DEBUG_PRINT("\t%s=%i",select2PropertyAddress.c_str(), select2ID);      
      }    
    }  

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
    }
 
  protected:
    uint8_t select2ID=1; 
    String *select2Content;
    String select2PropertyAddress;
};


class ThreeSelectStrip: public TwoSelectStrip{
  public:
    ThreeSelectStrip(StripSkeleton* s,GUIManager* g, String *select2String, String *select3String, bool debug=false):TwoSelectStrip(s,g,select2String,debug){
      select3Content = select3String;
    }
  
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      char* buf = (char*)malloc(select3PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select3PropertyAddress.c_str(),select3ID);
      properties.add(String(buf));
      delete buf;

    }

    void deserializeProperty(String property){
      TwoSelectStrip::deserializeProperty(property);
      if (sscanf(property.c_str(), select3PropertyAddress.c_str(), &select3ID)){
        //DEBUG_PRINT("\t%s=%i",select3PropertyAddress.c_str(), select3ID);      
      }  

    }  

    virtual uint8_t getNumberOfProperties(){
      return skeleton->numberOfProperties;      
    }

  protected:
    uint8_t select3ID=1; 
    String *select3Content;
    String select3PropertyAddress;

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
      DEBUG_PRINT("\nbuild(%s)",stripID.c_str());     
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
      //DEBUG_PRINT("\ngetStripSkeleton(%s)",stripID.c_str());
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* s = stripSkeletonRepo.get(i);        
        //DEBUG_PRINT("==%s?",s->stripID.c_str());
        if (stripID == s->stripID){
          //DEBUG_PRINT(": MATCH!");
          return s;
        } 
      }
      return nullptr;
    }
};

///////////////////// BANKS MANAGEMENT ////////////////////////

union bankUnion{
  uint16_t value;
  struct {
    uint8_t window ;
    uint8_t selector;   // which strips under focus in the 4x window
  };
};


class Bank{  
  #define BANK_MAXWINDOW 3
  #define STRIP_PER_WINDOW 4
  #define BANK_MAX_STRIPS BANK_MAXWINDOW*STRIP_PER_WINDOW

  public:
    bankUnion nav;
 
    //uint8_t numberOfStrips = 0;
    //uint8_t maxWindow = 0;

    String name;
    unsigned char icon=' ';

    uint8_t possibleLayout;
    uint8_t currentLayout;
    
    LinkedList<Strip*> strips;

    Bank(String n, String value, StripFactory* sf){ 
      name = n;
      //Strip* s = nullptr;
      //for (size_t i = 0; i < BANK_MAX_STRIPS; ++i) strips[i] = nullptr; //strips.add(s); // strips.push_back(nullptr);
      stripFactory = sf;
      deserialize(value);
    }

    ~Bank(){
      DEBUG_PRINT("\n~Bank: %s",name.c_str());
      for (uint8_t i=0; i<strips.size(); i++){
        if (strips[i] != nullptr ) delete strips[i];
      }      
    }

    void init(){
      DEBUG_PRINT("\t%s->init()",name.c_str());
      //set stripIndex to the first available strip
      nav.selector = 0; nav.window = 0;
      uint8_t i=0;
      while(i<strips.size()){
        DEBUG_PRINT("\t<*>");
        if (strips[i] != nullptr){
          nav.selector = strips[i]->selector;
          nav.window = strips[i]->win;
          break;
        }
        i++;
      }
      DEBUG_PRINT("\n¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨¨-------___________ %s selector:%i, window: %i",name.c_str(),nav.selector,nav.window);
    }

    /*uint8_t getMaxWindow(){
      //determine how many strips are in use ... means if there are free strip,in between (value nullptr) we skip but as soon as we found one not 0 we set it to the current index
      uint8_t numberOfBusyStrip = 0;
      DEBUG_PRINT("\n%s numberOfBusySlot=",name.c_str());
      for (uint8_t i = 0; i < BANK_MAX_STRIPS; i++) {
        if (strips[i] != nullptr) {
          numberOfBusyStrip = i;
          DEBUG_PRINT(" %i",numberOfBusyStrip);
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
        DEBUG_PRINT("\nBank <%s> deserialization error:%s",name.c_str(), error.f_str());
      }else{
        //DEBUG_PRINT("\n\nbank <%s>:<%s>",name.c_str(),value.c_str());

        possibleLayout = bank[C_POSSIBLELAYOUT];
        currentLayout = bank[C_CURRENTLAYOUT];
        icon = bank[C_ICON].as<unsigned char>();
        
        DEBUG_PRINT("\npossible layout:%i, currentLayout:%i, icon:%c",possibleLayout,currentLayout,icon);
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

    bool insertStrip(JsonObject jsonStrip){

    }

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
          DEBUG_PRINT("\n\t\taddStrip=============>%s",jsonStripString.c_str());
          strips.add(strip);
        }  
      }
      return strip;
    }

    /*void add(Strip* strip, uint8_t index){
      if (index<BANK_MAX_STRIPS){

        strips[index] = strip;
      }
    }*/

    Strip* selectStrip(uint8_t selector, uint8_t window){
      DEBUG_PRINT("\n    selectStrip(%i,%i)",selector,window);
      Strip* strip = nullptr;
      // retrieve strips at given position
      uint8_t i = 0;
      while (i<strips.size()){
        DEBUG_PRINT("\n\t\tStrip[%i]->selector=%i ->win=%i",i,strips[i]->selector,strips[i]->win);
        if (strips[i]->selector == selector && strips[i]->win == window){
          DEBUG_PRINT(" MATCH");
          strip = strips[i];
          break;
        }
        i++;
      }
      return strip;
    }

    Strip* getStrip(uint8_t x, uint8_t y, uint8_t window){
      DEBUG_PRINT("\n  getStrip(%i,%i,%i)",x,y,window);
      Strip* strip = nullptr;
      // retrieve strip at given position
      uint8_t i = 0;
      while (i<strips.size()){
        DEBUG_PRINT("\n\t\tStrip[%i]->x=%i ->y:%i ->win=%i",i,strips[i]->x,strips[i]->y,strips[i]->win);
        if (strips[i]->x == x && strips[i]->y == y && strips[i]->win == window){
          DEBUG_PRINT(" MATCH");
          strip = strips[i];
          break;
        }
        i++;
      }
      return strip;
    }



    bool trash(Strip* strip){
      bool success = false;
      for (uint8_t i=0;i<strips.size();i++){
        if (strips[i] == strip){
          DEBUG_PRINT("\n!!!!TRASHSTRIP(%i,%i,win:%i)",strip->x,strip->y,strip->win);
          strips.remove(i);
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
      //DEBUG_PRINT("\nserialize %s=>",name.c_str());

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
      //DEBUG_PRINT("\n\tbank->serialze: <%s>\n",data.c_str());
      return data;
    }

  private:
    StripFactory* stripFactory;
    //std::vector<Strip*> strips;
    //Strip* strips[BANK_MAX_STRIPS];


};

class BankNavigator: public Widget{
  //#define BANK_NAME_SIZE TFT_WIDTH/15
  public:

    BankNavigator(Bank* b/*, LinkedList<Widget*>* w*/, bool debug):Widget(&xPos, &yPos, &fC, &bC, debug){      
      bank = b;
      //widgetList = w;
      oldNav.value = bank->nav.value - 1; //this is to force first refresh
      width = TFT_WIDTH/2; height = OVERLAY_ICON_SIZE;
    }

    virtual bool refresh(bool force=false){
      refreshed = force;
      Widget::refresh(force);      

      if (bank->nav.value != oldNav.value || bank->strips.size() != oldNumberOfStrips || id != bank->name || refreshed){
        refreshed = true;

        oldNumberOfStrips = bank->strips.size();
        id = bank->name;
        oldNav.value = bank->nav.value;        
        if (debug) DEBUG_PRINT("\n\t bank <%s> nav changed to <%i/%i>",id.c_str(),bank->nav.selector,bank->nav.window);        

     
        #define NAV_HEIGHT OVERLAY_ICON_SIZE+1
        
        #define WIN_WIDTH OVERLAY_ICON_SIZE*1.6

        #define YPADDING 1
        #define XPADDING 2

        #define SPACE_BETWEEN_WIN 3
        #define NAV_WIDTH (WIN_WIDTH*BANK_MAXWINDOW)+(SPACE_BETWEEN_WIN*BANK_MAXWINDOW)

        #define CELL_WIDTH (WIN_WIDTH)/(STRIP_PER_WINDOW)
        #define CELL_HEIGHT (NAV_HEIGHT)/(STRIP_PER_WINDOW)

        #define CANVA_VS_BANKNAME_SPACE 2 // 2 pixels

        uint16_t navFrameColor = GUI::luminance(GUI::gold,100);

        Canva* canva = new Canva(&GUI::zero,&GUI::zero,NAV_WIDTH,NAV_HEIGHT,&navFrameColor,&GUI::defaultBcolor); // +2 = space between Canva and bankname
        canva->refresh(true);

        /*for (uint8_t i = 0; i<BANK_MAXWINDOW; i++){
          canva->sprite->drawRect(i*SPACE_BETWEEN_WIN+WIN_WIDTH*i,0,WIN_WIDTH,NAV_HEIGHT,GUI::gold);
        }*/
       
        for (uint8_t i = 0; i<bank->strips.size(); i++){
          uint16_t stripCol = GUI::luminance(GUI::gold,100);
          Strip* strip = bank->strips[i];
          if (strip !=nullptr){
            
            uint8_t lumi = 100;

            if (strip->selector == bank->nav.selector && strip->win == bank->nav.window) lumi = 255;            
            stripCol = GUI::luminance(colorMap[strip->colorID].bColor,lumi);
            uint16_t stripFrame = GUI::luminance(colorMap[strip->colorID].fColor,lumi);

            uint8_t cellX = strip->x*CELL_WIDTH + (strip->win*WIN_WIDTH)+(SPACE_BETWEEN_WIN*strip->win);
            uint8_t cellY = strip->y*CELL_HEIGHT;

            // extraction de x et y dans size : 3x2
            int index = strip->size.indexOf('x');

            String xStr = strip->size.substring(0, index);
            String yStr = strip->size.substring(index + 1);

            int widgetWidth = xStr.toInt();
            int widgetHeight = yStr.toInt();

            uint8_t cellWidth = (widgetWidth*CELL_WIDTH)-XPADDING;
            uint8_t cellHeight = (widgetHeight*CELL_HEIGHT)-YPADDING;

            canva->sprite->fillRect(cellX+XPADDING,cellY+YPADDING,cellWidth,cellHeight,stripFrame);
            canva->sprite->drawRect(cellX,cellY,cellWidth+XPADDING,cellHeight+YPADDING,stripCol);

          }
        }

        
        width = canva->width+CANVA_VS_BANKNAME_SPACE;        

        //bank name
        Label* label = new Label(&bank->name,Align::Right,&GUI::zero,&GUI::zero,OVERLAY_ICON_SIZE*2,&fC,&bC,debug);
        label->refresh(true);

        width += label->width;
        height = max(label->height,canva->height);
        width = max(width,oldWidth);
        oldWidth = width;

        createSprite();  

        canva->sprite->pushToSprite(sprite,0,2);
        delete canva;

        //label->sprite->pushToSprite(sprite,canva->width,-OVERLAY_ICON_SIZE/3);    
        label->sprite->pushToSprite(sprite,canva->width+CANVA_VS_BANKNAME_SPACE,-OVERLAY_ICON_SIZE/5);    
        delete label;           

      }

      return refreshed;
    }
    
    virtual void setText(const String* text){

    }  

    virtual int16_t getX(){
      return xPos;
    }

  protected:
    Bank* bank;
    //LinkedList<Widget*>* widgetList;
    bankUnion oldNav;
    int8_t oldNumberOfStrips = 0;    
    uint16_t oldWidth=0;

    //int16_t xPos = TFT_WIDTH/2, yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE;
    int16_t xPos = OVERLAY_ICON_SIZE*2 , yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE-3;
    uint16_t fC = GUI::defaultFcolor, bC = GUI::defaultBcolor;

};

///////////////////// JSON AND SERIALIZATION  ////////////////////////

class JsonListFile : public JsonList{
  public:
      String filename;

      JsonListFile():JsonList(){};

      bool load(String f,String defaultValues=""){
        //DEBUG_PRINT("\nload(%s,<%s>) ",f.c_str(),defaultValues.c_str());
        // We first clean up existing values
        /*while (keyValues.size()>0){
          //DEBUG_PRINT("\nremaining to unstack: %i",gui->screenGUIs.size());
          KeyValue* kv = keyValues.shift();
          //DEBUG_PRINT("\nobject: %s ",widget->id.c_str());
          delete(kv);
        }*/
        bool success = false;

        if (defaultValues != "") deserialize(defaultValues);

        filename = f;
        // we try to allocate maximum memory and keep 20K free for security
        //DynamicJsonDocument doc(getMaxAlloc());
        JsonDocument doc;

        if (!LittleFS.exists(filename)){
          DEBUG_PRINT("\nfile: %s invalid, skipping by using default parameters",filename.c_str());          
        }else{
          File file = LittleFS.open(filename);
          DeserializationError error = deserializeJson(doc, file);      
          if (!error){
            jsonToKeyValues(&doc);
            success = true;
          }else{
            DEBUG_PRINT("\nJson error %s:%s",error.c_str(),filename.c_str());
          }
          doc.shrinkToFit();   
          DEBUG_PRINT("\nloaded %s", filename.c_str());     
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
            DEBUG_PRINT("\nfailed to open %s for writing",filename.c_str());
            success = false;
        }
        if(!file.print(s)){
            DEBUG_PRINT("\n %s write fail",filename.c_str());
            success = false;
        }else{
          //DEBUG_PRINT("\n saved: %s",s.c_str());
        }
        file.close();  
        return success;
      }
};

class Config : public JsonListFile{
  public:
    Config():JsonListFile(){}  

    const int getInt(String name) {
      return getString(name)->toInt();
    };
    const float getFloat(String name) {
      return getString(name)->toFloat();
    };
    const boolean getBool(String name) {
      return (*getString(name) != "0");
    };

};

class I18n : public JsonListFile{
  public:
    I18n():JsonListFile(){}

    virtual const String* getString(const String name, const String defaultValue="") {
      int8_t index;
      index = getIndex(String(name));
      if (index < 0) {
        //DEBUG_PRINT("\ntoken: %s do not exist, creating it.",name.c_str());
        addValue(name,name);  // if it doesnt exist we create it in the list
        return getString(name);
      } else {
        
        KeyValue* kv = keyValues.get(index);  
        //DEBUG_PRINT("\ntoken: %s = %s",name.c_str(),kv->value.c_str());      
        return &kv->value;
      }
    };
};

///////////////////// MAIN APPLICATION CLASS ////////////////////////

enum{
  MENU_ICON_MIDDLE,
  MENU_LABEL_TITLE,
  CONFIG_TEXTAREA_MESSAGE,
  BANK_SINGLEWIDGET,
  CONFIG_CONNECTWEBAPP,
  NUMBER_OF_WIDGETS
};

    #define ADDRESS_SIMPLETEXT "/text/%s"

class StaticText: public DataProvider{
  public :
    virtual uint16_t size(String addrr){return 1;}
    virtual String getValue(String addrr, uint16_t index){
      if (addrr == ADDRESS_SIMPLETEXT) return "hello"+String(index);
      else return "voidStatic";
    }
};

class ApplicationManager : public GenericComponent,MenuComponentRenderer{
  public:    
    Application* webServer = nullptr;
    ///////////////////// USER CALLBACKS ////////////////////////////////////////////////////
    ParseJsonInputCallback userParseJsonInputCallback = defaultParseJsonInputCallback;
    ReturnStringCallback userJsonConfigCallback = defaultUserJsonConfigCallback;
    ReturnStringCallback userJsonPongCallback = defaultPongCallback;
    NetworkChangeStateCallback userNetworkChangeStateCallback = defaultNetworkChangeStateCallback;
    VoidCallback userWizardScreenBuilder = defaultWizardScreenBuilder;
    VoidCallback userWizardstepFlow = defaultWizardStepFlow;
    VoidCallback bootScreen = defaultBootScreen;
    ///////////////////// STRIPS & BANKS CONTAINERS AND DEFINITION ////////////////////////
    LinkedList<Bank*> banks = LinkedList<Bank*>(); 
    StripFactory stripFactory;
    DataRepository dataRepository;
    
    StaticText textProvider;
    DataSet simpleText= DataSet(ADDRESS_SIMPLETEXT,INPUTTEXT,&textProvider);
    
    ///////////////////// OVERLAY ICONS ////////////////////////
    String networkIcon, apIcon;
    uint16_t networkIconColor = TFT_RED, apIconColor = TFT_RED, batIconColor = TFT_GREEN, utils1Icon1Color = GUI::menuActiveIconColor;   

    String batteryIcons[BAT_MAX_LEVEL]={ICON_BAT_LEVEL_0,ICON_BAT_LEVEL_1,ICON_BAT_LEVEL_2,ICON_BAT_LEVEL_3};
    String batIcon = ICON_WAIT; // by default we display a timeelapser while battery level is undetermined
    String* byeIcon; const uint16_t* byeIconColor = &GUI::defaultFcolor; 
    String iconReset = ICON_RESET;
    String iconForbid = ICON_FORBID;
    String powerIcon = ICON_POWER; 
    String footSwitchIcon = ICON_FOOTSWITCH;

    AlteredColor networkIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor apIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor configIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};

    
    int16_t overlayIconY = TFT_HEIGHT-OVERLAY_ICON_SIZE, apIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE*2;
    int16_t batIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE;

    ///////////////////// WIDGETS & ATTRIBUTES ////////////////////////
    #define WIDGET_SMALL_WIDTH TFT_WIDTH/8-5
    #define WIDGET_BIG_WIDTH TFT_WIDTH/5
    #define ICON_MAX_SIZE TFT_WIDTH/2

    int16_t belowTop = 10, midScreenHoriz = TFT_WIDTH/2, rightScreen = TFT_WIDTH, midScreenVertical = TFT_HEIGHT/2, bigWidgetMidY = midScreenVertical-WIDGET_BIG_WIDTH/2, iconMidY = midScreenVertical-ICON_MAX_SIZE/2 ;

    uint16_t rainbowC;
    AlteredColor rainbowColor = AlteredColor{.baseColor = TFT_WHITE, .value = &rainbowC};
    Widget* widgets[NUMBER_OF_WIDGETS]; 

    #define TFT_QUARTER TFT_WIDTH/4
    int16_t quarter1xPos = TFT_QUARTER-TFT_QUARTER/2;
    int16_t quarter2xPos = TFT_QUARTER*2-TFT_QUARTER/2;
    int16_t quarter3xPos = TFT_QUARTER*3-TFT_QUARTER/2;
    int16_t quarter4xPos = TFT_QUARTER*4-TFT_QUARTER/2;

    #define TFT_THIRD TFT_WIDTH/3
    int16_t third3xPos = TFT_THIRD*2+TFT_QUARTER/2;

    String utils1Icon1,utils1Icon2;

    ///////////////////// MENU SYSTEM ATTRIBUTES AND MESSAGE ////////////////////////
    #define MENU_ICON_SIZE TFT_WIDTH/3
    #define MENU_TITLE_SIZE MENU_ICON_SIZE/2
    #define CONFIG_TEXTAREA_MESSAGE_SIZE MENU_TITLE_SIZE/1.8

    MenuSystem ms = MenuSystem(*this);
    MenuSystem* bankMenu = nullptr;
    MenuSystem* wizardMenu = nullptr;

    Menu* langMenu = nullptr;
    Menu* confMenu = nullptr;  
    String topLabelItem=" ", vBatLabel; //menuTitle
    String menuItemIcon[3]={"","",""}; // left, mid, right
    String bottomLabelTitle=" ";

    uint16_t menuActiveIconColor;

    int16_t menuIconYpos = TFT_HEIGHT/2-MENU_ICON_SIZE/2.2, leftMenuIconXpos = midScreenHoriz-MENU_ICON_SIZE*1.4,rightMenuIconXpos = midScreenHoriz+MENU_ICON_SIZE*1.4;
    int16_t menuTitleY = TFT_HEIGHT-MENU_TITLE_SIZE*1.2, belowMenuIcon = menuIconYpos+MENU_ICON_SIZE;

    #define ICON_DOT_MAX 4
    String iconDot[ICON_DOT_MAX]={
      ICON_LEFTDOT+String(ICON_NODOT),
      ICON_RIGHTDOT+String(ICON_NODOT),
      ICON_NODOT+String(ICON_LEFTDOT),
      ICON_NODOT+String(ICON_RIGHTDOT),
    };
    String iconPending[2];

    const char* quitMenuString ={ICON_QUIT":QUIT"};
    const char* infoMenuString ={ICON_INFO":INFO"};
    const char* configMenuString ={ICON_CONFIG":CONFIGURATION"};
    const char* profileMenuString ={ICON_PROFILES":PROFILE"};
    const char* saveMenuString ={ICON_SAVE":SAVE"};
    const char* langMenuString ={ICON_LANG":LANGUAGE"};
    const char* tftRotationMenuScreen{ICON_TFT_ROTATE":ROTATE"};
    const char* resetMenuScreen{ICON_RESET":RESET"};
    const char* shutdownMenuString{ICON_POWER":OFF"};
    

    ///////////////////// UTILITIES ATTRIBUTES ////////////////////////
    I18n lang = I18n();
    Config config = Config();
    Config system = Config();
    Preferences persistants;

    unsigned long goSleepCountdownStart;

    bool webAppConnected;

    String configClientIP; // ip of the client connect to the AP during configuration    

    ApplicationManager(HardwareManager* h, GUIManager* g, bool debug=true):GenericComponent(debug){
      hwm = h;
      gui = g;
      id = "ApplicationManager";

      stripTextSkel = new StripSkeleton(STRIP_TEXT,STRIP_DEFAULT_GROUP,textStripConstructorCallback,'*');
      dataRepository.addDataSet(&simpleText);
      stripTextSkel->registerDataSetAddress(ADDRESS_SIMPLETEXT);
    }

    bool getButtonState(uint8_t B){
      return hwm->buttons[B]->refreshed;
    }

    void setUserInternetCallback(StdFunctionCallbackVoidBool userInternetCallback){
      hwm->setUserInternetCallback(userInternetCallback);
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
          //if (debug) DEBUG_PRINT("\nIcon index: %i icon:%s",vBatIconIndex,batIcon.c_str());
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

    void setNetworkIcon(){

      #ifdef USE_NETWORK
              
        if (hwm->network->state == CONNECTED){
          if (hwm->network->media == WIFI_MEDIA) refreshWiFiSignalIcon();
          else networkIcon = ICON_ETH;
          networkIconGlowingRedColor.value = &GUI::unUsedColor;
          networkIconColor = GUI::green;
        }else{
          networkIconGlowingRedColor.value = &networkIconColor;
          //DEBUG_PRINT("\n setNetworkIcon ********************************");
          if (hwm->network->ethEnabled && hwm->network->WiFiEnabled && hwm->network->getLink() == LinkOFF) networkIcon = ICON_MIXETHWIFI; // both WiFi and ETh are up but there is no cable on, we don't know who will connect first
          else if ((hwm->network->ethEnabled && !hwm->network->WiFiEnabled) || (hwm->network->getLink() == LinkON)) networkIcon = ICON_ETH;  //case when we bootSprite for a DHCP lease or have no WiFi enabled
          else if (!hwm->network->ethEnabled && hwm->network->WiFiEnabled) networkIcon = ICON_WIFI_STRENGTH_3; 
          else networkIcon = emptyString;  // when it is only wifi and ap is launched.
        }

      #else
        networkIcon = emptyString;
      #endif

    }

    void gotoWizardEnd(){
      screen.id = SCREEN_WIZARD; screen.state = STATE_WIZARD_END_STEP4;
      persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
    }

    void goToResetScreenAndSleep(String message){
      clearConfig();
      topLabelItem = *lang.getString(message);
      bottomLabelTitle = emptyString;
      byeIcon = &iconReset; byeIconColor = &GUI::red;
      goSleepCountdownStart = millis();          
      screen.id = SCREEN_BYE; screen.state = STATE_BYE_GOSLEEP_MESSAGE;         //we could create a screen dedicated for reset         
    }

    void goToBigBatteryIconScreenAndSleep(){
      byeIcon = &batIcon; byeIconColor = &batIconColor;
      screen.id = SCREEN_BYE;
      screen.state = STATE_BYE_GOSLEEP_MESSAGE;       
    }

    Bank* deserializeBanks(const String* bankJsonString){
      Bank* bank = nullptr;     
       
      //int memAlloc = JSON_BANK_SIZE * numberOfBanks;
      DEBUG_PRINT("\nCreating banks from: <%s>",bankJsonString->c_str());
      //DynamicJsonDocument jsonBanks(memAlloc);          
      JsonDocument jsonBanks;
      
      DeserializationError error = deserializeJson(jsonBanks, *bankJsonString);
      if (error){
        DEBUG_PRINT("\nBank deserialization error:%s",error.f_str());
      }else{              
        JsonObject root = jsonBanks.as<JsonObject>();      
        for (JsonPair kv : root) {
          DEBUG_PRINT("\n%s = %s",kv.key().c_str(),kv.value().as<String>().c_str());          
          bank = new Bank(kv.key().c_str(),kv.value().as<String>(),&stripFactory);
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
      DEBUG_PRINT("\n*system.getString(S_LAST_USED_SSID): %s",system.getString(S_LAST_USED_SSID)->c_str());
      hwm->setButtonReactivity(system.getInt(S_PUSHDELAY),system.getInt(S_DCLICREACTIVITY));     
      DEBUG_PRINT("\nS_PUSHDELAY: %s",String(system.getInt(S_PUSHDELAY)).c_str());
      String langFile = "/"+*system.getString(S_LANG,"ENGLISH")+".lang";
      lang.load(langFile); 
    }

    #define DEFAULT_CONFIG_FILE_NAME "default.conf"

    void applyConfigSettings(){
      // this function applies all settings from the user conf file
      String profileFile = "/"+*system.getString(S_CONFIG_FILE,DEFAULT_CONFIG_FILE_NAME);
      config.load(profileFile,defaultUserConf);  
    }  

    #define SYSTEM_FILE_NAME "/settings.sys"
    void clearConfig(){
      // delete all config files here, add the code to iterate FS on *.conf
      LittleFS.remove(DEFAULT_CONFIG_FILE_NAME);
      // delete system settings
      LittleFS.remove(SYSTEM_FILE_NAME);
      persistants.clear();      
    }

    #define CUSTOM_LOGO_FILE "/custom.jpg"

    void init(){
      GenericComponent::init();

      persistants.begin("interval", false);
      DEBUG_PRINT("\npersistants.begin()");

      if (!LittleFS.begin()){
        DEBUG_PRINT("\nFS BEGIN ISSUE");
      }

      system.load(SYSTEM_FILE_NAME,defaultSysConf);  

      applySystemSettings();
      applyConfigSettings();

      hwm->init();
      gui->init();         
      GUI::tft->setRotation(persistants.getUChar(PERSISTANT_TFT_ROTATION,1)); 

      if (debug){
        String s;
        system.serialize(&s);
        DEBUG_PRINT("\n\n serialization of system:\n%s",s.c_str());
        s="";
        config.serialize(&s);
        DEBUG_PRINT("\n\n serialization of config:\n%s",s.c_str());
        s="";
        lang.serialize(&s);
        DEBUG_PRINT("\n\n serialization of langfile:\n%s",s.c_str());        
        //DEBUG_PRINT("\nexample of langfile token: Connect:%s",lang.getString("Connect")->c_str());        
      }

      for (uint8_t i=0;i<SCREEN_MAX;i++) screenBuilders.add(&screens[i]);
            
      for (uint8_t i=0;i<MAXB;i++){
        hwm->buttons[i]->setCallBack(SIMPLE_CLIC,simpleClicCallBack);
        hwm->buttons[i]->setCallBack(DOUBLE_CLIC,doubleClicCallBack);
        hwm->buttons[i]->setCallBack(LONG_CLIC,longClicCallBack);
        hwm->buttons[i]->setCallBack(HOLD,holdCallback);
        hwm->buttons[i]->setCallBack(RELEASE_HOLD,releaseHoldCallback);
      }

      hwm->onOff.setCallBack(LONG_CLIC,menuSwitchCallback);
      hwm->onOff.setCallBack(SIMPLE_CLIC,simpleClicCallBack);

      back_menu_item = new MenuItem(quitMenuString, &back_menu_item_selected);
      info_menu_item = new MenuItem(infoMenuString, &info_item_selected);
      config_menu_item = new MenuItem(configMenuString,&config_menu_item_selected);
      profile_menu_item = new Menu(profileMenuString,&menu_selected);
      save_profile_menu_item = new MenuItem(saveMenuString,&save_profile_item_selected);
      tft_rotation_menu_item = new MenuItem(tftRotationMenuScreen,&tft_rotation_item_selected);
      reset_menu_item = new MenuItem(resetMenuScreen,&reset_item_selected);                // to be removed
      shutdown_menu_item = new MenuItem(shutdownMenuString, &shutdown_menu_item_selected);

      ms.get_root_menu().add_item(shutdown_menu_item);
      ms.get_root_menu().add_item(back_menu_item);
      ms.get_root_menu().add_item(info_menu_item);
      ms.get_root_menu().add_item(config_menu_item);       
      ms.get_root_menu().add_item(tft_rotation_menu_item);       
      ms.get_root_menu().add_item(reset_menu_item);       
      ms.get_root_menu().add_menu(profile_menu_item);
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

      switch(hwm->startupState){
        case RESET_STARTUP:
          // put reset code here
          goToResetScreenAndSleep("RESET");
          enableWizard();
        break;
        case BATTERY_STARTUP:
          topLabelItem = *lang.getString("Press >1s to startup"); bottomLabelTitle = emptyString;
          goSleepCountdownStart = hwm->bootTime;
          goToBigBatteryIconScreenAndSleep();         
        break;
        case WIZARD_STARTUP:
          enableWizard();
          // no break, we continue normal startup
        case NORMAL_STARTUP:
          // perform all application initialization
          DEBUG_PRINT("\n\n\n********");

          bootScreen();

          if (deserializeBanks(config.getString(C_BANKS)) == nullptr){
            topLabelItem = *lang.getString("Bank creation");
            bottomLabelTitle = emptyString;
            byeIconColor = &GUI::red; byeIcon = &iconForbid;
            screen.id = SCREEN_BYE; screen.state = STATE_BYE_MODAL_MESSAGE;
            DEBUG_PRINT("\n\n\n*********************** COULD NOT CREATE BANK");
          }else{
            resetToBankScreen();
          }
        break;
      }
    }

    void resetToBankScreen(){
      screen.id = persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);; //screen.id = SCREEN_BANK;        
      screen.state = persistants.getUChar(PERSISTANT_SCREEN_STATE,0);
      //screen.state is bank ID;
      if (screen.id == SCREEN_BANK && screen.state > banks.size()-1){
        screen.state = 0; // prevents branching a bank which is not existing anymore
        persistants.putUChar(PERSISTANT_SCREEN_STATE,screen.state); 
        DEBUG_PRINT("\n\n\n*********************** RESETING TO BANK: %i",screen.state );                     
      }
      oldScreen.value = screen.value - 1;      
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
        Bank* bank = banks.get(i);
        String stringBank = bank->serialize();
        DeserializationError error = deserializeJson(jsonBank, stringBank);
        if (error){
          DEBUG_PRINT("\nBank <%s> deserialization error:%s",bank->name.c_str(),error.f_str());
          return "{}";
        }else{
          bankDoc[bank->name] = jsonBank;
        }       
      }      
      return bankDoc.as<String>();
    }

    void serialize(String* returnedJsonStringData){
      // this build the full serialization of the application
      //DEBUG_PRINT("\nserialize full config");
      JsonDocument doc;
      
      String tempJsonString;

      system.serialize(&tempJsonString);  
      doc["system"]=tempJsonString;

      doc["volatile"]["langs"][0] = "francais"; doc["volatile"]["langs"][1] = "english"; 
     
      JsonArray skel = doc["skeletons"].to<JsonArray>();
      stripFactory.serialize(&skel);

      JsonArray ds = doc["datasets"].to<JsonArray>();  
      dataRepository.serialize(&ds);         

      tempJsonString = serializeBanks();
      
      config.setValue(C_BANKS,tempJsonString);

      tempJsonString = "";
      config.serialize(&tempJsonString);
      doc["config"]=tempJsonString;
          
      doc["userConfig"] = userJsonConfigCallback();

      serializeJson(doc, *returnedJsonStringData);  
      removeJsonQuote(returnedJsonStringData);
    }

    void restoreScreen(){
      screen.id = persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
      screen.state = persistants.getUChar(PERSISTANT_SCREEN_STATE,0);    
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
    

    bool refresh(bool force=false){
      yield();
      
      refreshed = force;

      //------------------
      //
      // MANAGE NETWORK
      //
      //------------------     
      hwm->refresh();      

      if (hwm->network->refreshed){ // network changed its status
        
        setNetworkIcon();

        // inform all that network has changed (it could be the network cable pluged/unplug while in AP)
        broadCast(parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivityConfig\"}"));  // inform webclient               could be done via ws.send as well
        // ============>>> CALLBACK APPLICATION
        userNetworkChangeStateCallback(hwm->network->state);

        if (hwm->network->state == CONNECTED){         
          // network has refreshed so we shift to appropriate configMedia
          if (hwm->network->media == WIFI_MEDIA){
            //we set last used Wifi SSID (id) and save config, hence keep WIFI_MEDIA credential
            DEBUG_PRINT("\n------> SAVING SSID");
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
            if (debug) DEBUG_PRINT("\n\tsetting the configMedia to WiFiEth");
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
      // MANAGE WEB APPLICATION
      //
      //------------------  

      if (hwm->configMedia->state == CONNECTED){    
        if(webAppOn){          

          if (hwm->configMedia->refreshed) setWSServer();   // the network has refreshed (could be media, IP...) hence we rebuild the WSS
          
          wss->listen();  //sockets      
          
          // then webserver
          if (hwm->configMedia->media == CABLE_MEDIA){
            EthernetClient newEthClient = ((EthernetServer*)hwm->configMedia->server)->available();                          
            // Création de la tâche "handleWebConnection" avec le client en paramètre
            /*xTaskCreate(
              handleWebConnection,          // Nom de la fonction de tâche
              "handleWebConnection",        // Nom de la tâche
              8192*2,                         // Taille de la pile (4 Ko)
              (void*)&newEthClient,               // Paramètre (pointeur vers EthernetClient)
              1,                            // Priorité de la tâche
              NULL                          // Pas besoin de handle de tâche ici
            );*/                       
            if (newEthClient.connected()) {
              DEBUG_PRINT("\nWebapp connected eth");
              webServer->process(&newEthClient);
              newEthClient.stop();
            }             
            
           
          }else{
            WiFiClient newWiFiClient = ((WiFiServer *)hwm->configMedia->server)->available();
            if (newWiFiClient.connected()) {
              //if (debug) DEBUG_PRINT("\nWebapp connected wifi");
              webServer->process(&newWiFiClient);
              newWiFiClient.stop();
            }   
          }
        }
      }else{
        //if (hwm->configMedia->refreshed) 
        webAppConnected = false;
      }
      
      // BATTERY LEVEL CHECK
      if (hwm->battery.refreshed){        
        setBatIcon();
        if (hwm->battery.vBat<VBAT_CRIT && hwm->battery.batteryState != BAT_CHARGING){
          topLabelItem = *lang.getString("Low Battery");
          bottomLabelTitle = emptyString;
          goSleepCountdownStart = millis();
          goToBigBatteryIconScreenAndSleep();
        }          
      }
   

      //------------------
      //
      // MANAGE GUI OBJECT
      //
      //------------------
      if (screen.value != oldScreen.value){
        refreshed = true;
        // we changed the screen, therefore we rebuild the screen
        // first we acknowledge the update so it is not done te next round
        if (debug) DEBUG_PRINT("\n%s changed from screen.id:%i.state:%i.focus:%i to screen.id:%i.state:%i.focus:%i",id.c_str(),oldScreen.id,oldScreen.state,oldScreen.focus, screen.id,screen.state,screen.focus);        
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
        //#ifdef DEBUG
          // DEBUG INDICATORS OVERLAY
          gui->add(new Label(&gui->framePerSec,Align::Right, &rightScreen, &GUI::zero,TFT_WIDTH/16,&GUI::defaultFcolor,&GUI::defaultBcolor,false));    
          gui->add(new Label(&heapSize,Align::Left,&GUI::zero,&GUI::zero,TFT_WIDTH/16,&GUI::defaultFcolor,&GUI::defaultBcolor,false));              
        //#endif

        //DEBUG_PRINT("\nnew screen widget size: %i",gui->screenGUIs.size());
        if (gui->transparency) gui->clearScreenBuffer();
        oldScreen.value = screen.value;      
        // these are the only screens that can be recall after startup
      }
      gui->refresh(force);

      //------------------
      //
      // MANAGE APPLICATION FLOW
      //
      //------------------

      switch(screen.id){
        case SCREEN_WIZARD:
          switch (screen.state){
            case STATE_WIZARD_LANG_STEP1:{
              // we branch to the lang menu
              ms.reset();             
              for (int i = 0; i < ms.get_root_menu().get_num_components(); ++i) {
                const MenuComponent* m = ms.get_current_menu()->get_current_component();
                if (strcmp(langMenuString,m->get_name())!=0) ms.next();
                else{ms.select();break;}
              }                       
              restoreMenu();
              screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);    
              persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_CONNECT_CONFIG_STEP2);    
            }break;
            case STATE_WIZARD_CONNECT_CONFIG_STEP2:
              handleConfigConnection();
              persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);    
              persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_USER_STEP3); 
            break;
            case STATE_WIZARD_USER_STEP3:
              if (!webAppConnected) screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              userWizardstepFlow();
            break;
            case STATE_WIZARD_END_STEP4:
              resetToBankScreen();
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
              // we do not shutdown, we leave it on so we can go to the men for instance to perform some action like update or ftp ...            
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
          DEBUG_PRINT("\n\n\t\t\t * * * * broadCast(%s) * * * ",msg.c_str());
        }
    }

    void reset(){      
      nvs_flash_erase(); // erase the NVS partition and...
      nvs_flash_init(); // initialize the NVS partition.        
      // next: delete conf file   
    }

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

    bool startWebApp(){
      webAppOn = false;
      if (hwm->configMedia != nullptr){
        if (!webAppOn && hwm->configMedia->state == CONNECTED){          
          //start webserver
          if (webServer != nullptr) delete webServer;
          webServer = new Application();
          // inventory zipped file
          populateFileList("gz","/webapp",&webGZFiles,"/",false);    
          for (uint8_t i=0;i<webGZFiles.size();i++){          
            webServer->get(webGZFiles.get(i)->c_str(), &gzFileServer);
            //DEBUG_PRINT("\n ...... REGISTERING WEB GZ FILE: %s",webGZFiles.get(i)->c_str());
          }
          // any other file
          webServer->use(commonFileServerWrapper);
          setWSServer();
          webAppOn = true;
          hwm->configIP = "http://"+hwm->configMedia->IP.toString();
          /* Initialize mDNS => doesnt work for ethernet so we give up and keep IP address
          if (debug) DEBUG_PRINT("\nstarting mdns: %s",config.getString(S_AP_NAME)->c_str());
          if (!MDNS.begin(*config.getString(S_AP_NAME))) {   // Set the hostname to "esp32.local"
            if (debug) DEBUG_PRINT("Error setting up MDNS responder!");
          } */           
        }
      }
      return webAppOn;
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

    void commonFileServer(Request &req, Response &res){
      
      //if (req.method() != Request::GET) return;
      const char * path = req.path();

      if(strcmp(path, "/post/") == 0){ 
        httpPost(req,res);
      }else{
        if(strcmp(path, "/") == 0){
          path = "/index.html.gz"; //path = "/index.html";      INDEX is the only exception were we process the gz here because we remap / to index
          #ifdef USE_NETWORK
            if (persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK) == SCREEN_WIZARD) path = "/wizard.html.gz"; //path = "/wizard.html";
          #endif
        } 
        String filenameRealExtension = path;
        
        if (filenameRealExtension.endsWith(".gz")){
          filenameRealExtension.replace(".gz","");
          res.set("Content-Encoding", "gzip");
        }

        String fullPath = "/webapp"+String(path);   
        if (!LittleFS.exists(fullPath)){
          res.sendStatus(404);   //send(404, "text/plain", "Not found");    
          res.set("Content-Type", "text/html");
          res.print(fullPath.c_str());
          res.end();          
          return;
        }
        File file = LittleFS.open(fullPath);
        if (file.isDirectory()) return;
        
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
        String mimeType = mimeTypes[0][1];
        for (uint8_t i=0;i<MIME_TYPES_MAX;i++){
          if (filenameRealExtension.endsWith(mimeTypes[i][0])) mimeType = mimeTypes[i][1];
        }

        //if (debug) DEBUG_PRINT("\t\t\t\t REAL EXTENSION: \t\t\tt%s \t\t\tMIME:%s",filenameRealExtension.c_str(),mimeType.c_str());
        DEBUG_PRINT(" serving <%s> --------------------->%s",mimeType.c_str(), fullPath.c_str() );        
        res.set("Content-Type", mimeType.c_str());
        iconPending[0] = ICON_ROLLING1;
        iconPending[1] = ICON_ROLLING2;
        
        //while (file.available()) res.write(file.read());
        sendFileInChunks(&file,&res);
        res.end();
      }
    }

    #define BUFFER_SIZE 128  // Taille du buffer en octets

    void sendFileInChunks(File* file, Response* res) {

      // set the buffer sidze according to netWorkConfig, for AP = 128 is good, for Eth, probably 1024 is best and for STA, let's do a test with 512
      // because having 128 with STA created some ERR_CONNECTION_TIMED_OUT

      byte buffer[BUFFER_SIZE];  // Tampon pour stocker les données lues
      int bytesRead;

      // Lire et envoyer des paquets de 256 octets jusqu'à ce que tout le fichier soit envoyé
      while ((bytesRead = file->read(buffer, BUFFER_SIZE)) > 0) {
        res->write(buffer, bytesRead);  // Écrire le tampon complet dans le client
      }
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
      //widgets[MENU_ICON_MIDDLE].widget->setFcolor(glowingGoldColor.value); // restore color
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      widgets[CONFIG_TEXTAREA_MESSAGE]->setText(lang.getString(" ")); // empty message
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

      bottomLabelTitle = *lang.getString(getNameFromString(menuString));
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
      Bank* bank = banks.get(selection);      
      //widgets[MENU_LABEL_TITLE]->setText(&bank->name); // restore title  
      if (bank != nullptr){
        bottomLabelTitle = bank->name;
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
        bottomLabelTitle = *lang.getString("No bank");
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
      widgets[CONFIG_TEXTAREA_MESSAGE]->setText(lang.getString(" ")); // empty message
    }

    void switchBank(){
      // if we have one bank or two we shift directly      
      if (banks.size() == 2) screen.state = (screen.state + 1) % banks.size(); // <=2
      // otherwise we call the menu
      else if (banks.size()>2) screen.id = SCREEN_BANK_SWITCH;      
      //else banks are empty, we do nothing
    }

    void testHttpRequest(){

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
    }

    void selectNextWindow(Bank* bank){
      if (bank->nav.window++ == BANK_MAXWINDOW-1) bank->nav.window = 0;      
      bank->nav.selector = 0;
    }

    void selectPreviousWindow(Bank* bank){      
      if (bank->nav.window > 0) bank->nav.window--;      
      else bank->nav.window = BANK_MAXWINDOW-1;
      bank->nav.selector = STRIP_PER_WINDOW-1;      
    }


    ///////////////////////////////////////////////////////////////////////

    //  APPLICATION & WORKFLOW MANAGEMENT METHODS

    ///////////////////////////////////////////////////////////////////////
    bool stripScroll(uint8_t direction, Bank* bank) {
        if (bank->strips.size()<=1){
          DEBUG_PRINT("\n stripscroll: only one strip,exit");
          return false; 
        }

        if (direction == hwm->buttons[RIGHTB]->getID()) {
          if (bank->nav.selector++ >= STRIP_PER_WINDOW-1) selectNextWindow(bank);
        } else if (direction == hwm->buttons[LEFTB]->getID()){
          if (bank->nav.selector == 0) selectPreviousWindow(bank);
          else bank->nav.selector--;
        }

        DEBUG_PRINT("\t scroll(%i/%i):",bank->nav.selector,bank->nav.window);

        Strip* strip = bank->selectStrip(bank->nav.selector,bank->nav.window);

        if (strip != nullptr){
          DEBUG_PRINT("FOUND");
          jumpToStrip(bank->nav.selector,bank->nav.window,bank->name);
          return true;
        } 
        else return stripScroll(direction,bank);
      
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

    bool stripAction() {    
      Bank* bank = banks.get(screen.state);
      if (bank != nullptr) return false;
      else return bank->selectStrip(bank->nav.selector,bank->nav.window)->actionCallback();      
    }

    bool stripChangeValue(uint8_t direction, unsigned long time){
      bool success;
      
      Bank* bank = banks.get(screen.state);    
      if (bank == nullptr) return false;

      if (direction == hwm->buttons[RIGHTB]->getID()){ success = bank->selectStrip(bank->nav.selector,bank->nav.window)->increaseCallBack(time);}      
      else if (direction == hwm->buttons[LEFTB]->getID()){ success = bank->selectStrip(bank->nav.selector,bank->nav.window)->decreaseCallback(time);}      

      return success;
    }

    void jumpToStrip(uint8_t selector, uint8_t window, String bankName){
      DEBUG_PRINT("\n ^^^^^^^^^jumpToStrip %s(%i,%i)",bankName.c_str(),selector,window);
      Bank* bank = jumpToBank(bankName);      
      bank->nav.selector = selector;
      bank->nav.window = window;
      //screen.focus = bank->nav.selector+(bank->nav.window*STRIP_PER_WINDOW);  // this trigger GUI rebuild
      screen.focus = screen.focus - 1;
    }

    Bank* jumpToBank(String bankName){
      uint8_t bankIndex;
      Bank* bank = getBank(bankName,&bankIndex);
      screen.id = SCREEN_BANK;
      screen.state = bankIndex;   
      DEBUG_PRINT("\njumpToBank =>%s,%d",bank->name.c_str(),bankIndex);
      return bank;        
    }

    bool updateSelectors(Bank* bank, uint8_t win, JsonArray positions){
      bool success = false;
      //"selectors":[{"x":0,"y":0,"s":0},{"x":0,"y":1,"s":1},{"x":0,"y":2,"s":2}]}}        
      //uint8_t window = jsonInput["param"]["win"].as<int>();         
      // extract "positions"
      // Parcourir chaque objet dans le tableau
      DEBUG_PRINT("\n Now updating selectors");
      for (JsonObject position : positions) {
        int x = position["x"]; // extraire la valeur de x
        int y = position["y"]; // extraire la valeur de y
        Strip* strip = bank->getStrip(x,y,win);
        if (strip != nullptr){
          DEBUG_PRINT("\t updateStripPosition from %i",strip->selector);
          strip->selector = position["s"]; // extraire la valeur de selector
          DEBUG_PRINT(" to %i",strip->selector);
          success = true; 
        }
      }   
      // in case of reschuffle, GUI may try to render a strip which is not existing anymore or that has changed      
      if (bank->selectStrip(bank->nav.selector,bank->nav.window) == nullptr) findFirstValidStrip(bank);      
      return success;
    }

    /////////////////////////////////////////////////////////////////////////////////////

    #define BAD_BANK_MSG "Bad bank"
    #define BAD_STRIP_MSG "Bad strip"
    #define ERRDEL_STRIP_MSG "Delete strip"

    #define RET_UNDEFINEDCALLBACK   "undefinedCallBack"
    #define RET_SAVEBUTTON          "setSaveButton"
    #define RET_COMMANDERROR        "commandError"

    String getConnectivityState(){
      return hwm->network->serialize();
    }

    void findFirstValidStrip(Bank* bank){
      // if that's the case, we first scroll to the next available one.          
      DEBUG_PRINT("\nfindFirstValidStrip...");
      // if scrolling do not worked , means there is only one strip, we take the first available strip;
      if (!stripScroll(hwm->buttons[RIGHTB]->getID(),bank)){
        bank->init();   // reset the nav bar
        jumpToStrip(bank->nav.selector,bank->nav.window,bank->name); 
      }      
    }

    String parseJsonInput(String input){

      JsonDocument jsonInput;
      deserializeJson(jsonInput, input);

      jsonAnswer.callback = "undefinedCallBack";
      jsonAnswer.value = jsonInput.as<String>();

      DEBUG_PRINT("\nparseJsonInput(%s)",input.c_str());
      
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
          jsonAnswer.value = "{\"volatile\":{\"langs\":[\"english\",\"francais\"]},\"system\":{\"configNames\":[\"anto\",\"thomas\",\"laurent\"],\"pushDelay\":\"550\",\"dClicReactivity\":\"0\",\"faderSpeed\":\"150\",\"profile\":\"default.conf\",\"apName\":\"footswitch\",\"lastSSID\":\"intervalmix\",\"lang\":\"francais\",\"ssid\":{\"intervalmix\":\"intervalpop74\",\"infrafast\":\"Mf9k8iie2\",\"ssid3\":\"pass3\"}}}";
        }else if (param == "connectivityConfig"){
          jsonAnswer.value = getConnectivityState();
        }else if (param == "configBanks"){
          JsonDocument doc;
          
          JsonArray ds = doc["datasets"].to<JsonArray>();  
          dataRepository.serialize(&ds);         

          String banksString = serializeBanks();
          doc["banks"] = banksString;

          serializeJson(doc,jsonAnswer.value);
          removeJsonQuote(&jsonAnswer.value); // done later on just before sending

        }else jsonAnswer.callback = RET_UNDEFINEDCALLBACK;


      }else if (command == "set"){
        //{"command":"set","param":{"section":"system","id":"pushDelay","value":"3"}}
        String section = jsonInput["param"]["section"];
        String id = jsonInput["param"]["id"];
        String value = jsonInput["param"]["value"];
        DEBUG_PRINT("\t => section:%s id:%s value:%s",section.c_str(),id.c_str(),value.c_str());
        bool success;        
        if (section == "system"){
          success = system.setValue(id,value);
          if (success){
            applySystemSettings();      // ===>  propagate config values by giving reference to conf object using parameters instead of using local variable
            system.save();
          }
        }else if (section == "config"){
          success = config.setValue(id,value);
          if (success) applyConfigSettings();
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
          jsonAnswer.value = hwm->network->getConnectTimeout();
        }
        // if media is ethernet, we just feedback save button
        else{
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value ="1";
        }

      }else if (command == "setStrip"){          
        //{"command":"setStrip","param":{"bank":"Test","strip":{"type":"chGain","size":"4x1","color":4,"icon":"6","properties":["/ch/3/config/name"],"position":{"win":0,"x":"0","y":"0"}},"selectors":[{"x":0,"y":0,"s":0},{"x":0,"y":1,"s":1},{"x":0,"y":2,"s":2}]}}
        //get the bank that need to be modified
        jsonAnswer.callback=RET_COMMANDERROR;
        DEBUG_PRINT("\nsetStrip bankName:%s, strip:%s",jsonInput["param"]["bank"].as<String>().c_str(),jsonInput["param"].as<String>().c_str());        
        uint8_t bankIndex;
        Bank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);  // get the bank and its index
        if (bank != nullptr){          
          uint8_t window = jsonInput["param"]["strip"]["position"]["win"];            
          uint8_t x = jsonInput["param"]["strip"]["position"]["x"];     
          uint8_t y = jsonInput["param"]["strip"]["position"]["y"];
          DEBUG_PRINT("\nChecking if strip is existing: ");     
          Strip* strip = bank->getStrip(x,y,window);
          bool success = true;
          if (strip != nullptr){
            DEBUG_PRINT(" YES, deleting it");
            success = bank->trash(strip); // if strip is existing we first delete it
          }  
          if (success){
            JsonObject jData = jsonInput["param"]["strip"];            
            strip = bank->addStrip(jData);
        
          }else strip = nullptr;
          if (strip != nullptr){
            updateSelectors(bank,window,jsonInput["param"]["selectors"]);
            // put the focus on the strip
            uint8_t selector = strip->selector;
            jumpToStrip(selector,window,bank->name);    //jsonAnswer should be : {"restoreStrip":{"bank":"BANK j3","strip":{"type":"text","selector":"7","x":"3","y":"0","size":"1x4","color":3,"icon":"Z","properties":["/text/"]}}}         
            jsonAnswer.callback = RET_SAVEBUTTON;
            jsonAnswer.value = "1";  
          }else{
            jsonAnswer.value=*lang.getString("Strip setup error");
          }
        }else{
            jsonAnswer.value=*lang.getString(BAD_BANK_MSG);
        }                
      }else if (command == "trashStrip"){        
        //{"command":"trashStrip","param":{"bank":"Test","position":{"win":0,"x":"0","y":"1"},"selectors":[{"x":0,"y":0,"s":0},{"x":0,"y":2,"s":1}]}}
        uint8_t bankIndex;
        Bank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);
        jsonAnswer.callback = RET_COMMANDERROR;
        if (bank != nullptr){                     
          uint8_t window = jsonInput["param"]["position"]["win"];           
          uint8_t x = jsonInput["param"]["position"]["x"];     
          uint8_t y = jsonInput["param"]["position"]["y"];     
          Strip* strip = bank->getStrip(x,y,window);
          if (strip != nullptr){          
              // we check if we delete the strip which is currently focused
              // if that's the case, we first scroll to the next available one.
              jsonAnswer.callback = RET_SAVEBUTTON;
              jsonAnswer.value = "1";                                         
              if (strip->selector == bank->nav.selector && strip->win == bank->nav.window){
                if (bank->trash(strip)){                                  
                  DEBUG_PRINT("\nDELETING FOCUSED STRIP, SCROLING...");
                  findFirstValidStrip(bank);                 
                }else jsonAnswer.value=*lang.getString(ERRDEL_STRIP_MSG);  
              }else{
                if (!bank->trash(strip)) jsonAnswer.value=*lang.getString(ERRDEL_STRIP_MSG);  
              }
              updateSelectors(bank,window,jsonInput["param"]["selectors"]);              
          }else{
            jsonAnswer.value=*lang.getString(BAD_STRIP_MSG);
          }
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }  
        DEBUG_PRINT("\n<...>");
      }else if (command =="moveStrip"){
        //{"command":"moveStrip","param":{"bank":"Test2","x1":0,"y1":1,"x2":0,"y2":2,"win":0,"selectors":[{"x":0,"y":0,"s":0},{"x":0,"y":2,"s":1}]}}
        // use position here instead of x1, y1 
        uint8_t bankIndex;
        Bank* bank = getBank(jsonInput["param"]["bank"],&bankIndex);
        jsonAnswer.callback = RET_COMMANDERROR;
        if (bank != nullptr){                     
          uint8_t x = jsonInput["param"]["x1"];
          uint8_t y = jsonInput["param"]["y1"]; 
          uint8_t window = jsonInput["param"]["win"];
          Strip* strip = bank->getStrip(x,y,window);
          if (strip != nullptr){      
            ///WARNING THIS IS THE DESTINATION POSITION!!!!!! 
            strip->x = jsonInput["param"]["x2"];      
            strip->y = jsonInput["param"]["y2"];  
            updateSelectors(bank,window,jsonInput["param"]["selectors"]);  
            jsonAnswer.callback = RET_SAVEBUTTON;
            jsonAnswer.value = "1";              
            screen.focus = screen.focus - 1; // force refresh            
          }else{
            jsonAnswer.value=*lang.getString(BAD_STRIP_MSG);
          }
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }           
      }else if (command =="trashBank"){
        //{"command":"trashBank","param":{"name":"BANK j2"}}
        String bankName = jsonInput["param"]["name"];
        DEBUG_PRINT("\ntrashBank %s",bankName);
        uint8_t bankIndex;
        Bank* bank = getBank(bankName,&bankIndex);  // get the bank and its index
        if (bank != nullptr){
          banks.remove(bankIndex);
          delete bank;
          if (bankIndex == screen.state || screen.id == SCREEN_BANK_SWITCH){
            if (banks.size()>0) jumpToBank(banks.get(0)->name);  // go to first bank in case we delete the current displayed or we are showing menu switch
            else{
              DEBUG_PRINT("\nLAST BANK WAS TRASHED");
              screen.focus = screen.focus - 1; // force refresh 
            }
          }
          hookBankToMenu();   // rebuild the bank switch menu
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";            
        }else{
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }

      }else if (command == "newBank"){
        //{"command":"newBank","param":{"newName":{}}}
        bool ret = false;
        String jsonBankstring = jsonInput["param"];
        Bank* bank = deserializeBanks(&jsonBankstring);
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
        Bank* bank = getBank(oldName,&bankIndex);  // get the bank and its index
        if (bank != nullptr){
          bank->name = newName;
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";    
        }else jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
      }else if (command == "save"){     
        String fullConfig;
        serialize(&fullConfig);
        bool ret = config.save();
        ret &=  system.save();       
        if (ret){
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "0";  
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
          jsonAnswer.value = *lang.getString("Save error");
        }      
      }else{
        DEBUG_PRINT("\n\t\t\t\t\tCalling userParseJsonInputCallback...");
        if (!userParseJsonInputCallback(jsonInput,&jsonAnswer)){
          jsonAnswer.callback = RET_UNDEFINEDCALLBACK;
          jsonAnswer.value = jsonInput.as<String>();
        }
      }


      JsonDocument jsonOutput;
      jsonOutput[jsonAnswer.callback] = jsonAnswer.value;

      String jsonString;
      serializeJson(jsonOutput,jsonString);
      removeJsonQuote(&jsonString);

      DEBUG_PRINT("\nbuildJsonOuput: %s",jsonString.c_str());
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
      }     
      String data;
      serializeJson(doc,data);
      broadCast(data);
      // Delete the task itself
      vTaskDelete(NULL);
    }


    Bank* getBank(String name, uint8_t* index){
      for ( *index = 0;*index<banks.size();(*index)++){          
        //DEBUG_PRINT("\nindex:%d=>name:%s",*index,banks.get(*index)->name.c_str());
        if (banks.get(*index)->name == name){
          //DEBUG_PRINT("\nfound bank: %s",name.c_str());
          return banks.get(*index);
        }
      }
      return nullptr;      
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
      "\""+String(S_CONFIG_FILE)+"\":\"default.conf\"," 
      "\""+String(S_AP_NAME)+"\":\"footswitch\","
      "\""+String(S_LAST_USED_SSID)+"\":\"intervalmix1\","
      "\""+String(S_LANG)+"\":\"francais\","
      "\""+String(S_FADERSPEED)+"\":\"157\","
      "\""+String(S_SSID)+"\":{\"intervalmix1\":\"intervalpop74\",\"infrafast\":\"Mf9k8iie2\",\"IphoneTch\":\"jt2p9ug1\"},"
      "\""+String(S_BUTTON_ASSIGN)+"\":{\"left\":[3,1,5,8],\"right\":[2,1,4,9],\"tap\":[6,7,8,10]}"
    "}";



    String defaultUserConf = "{"      
      "\""+String(C_BANKS)+"\":{"
        "\"Test\":{\"possibleLayout\":4,\"currentLayout\":1,\"icon\":\"=\",\"strips\":[{\"type\":\"ch2bus\",\"size\":\"4x1\",\"position\":{\"win\":0,\"x\":0,\"y\":0},\"selector\":0,\"color\":3,\"icon\":\"-\",\"properties\":[\"/ch/02/config/name\",\"/bus/02/config/name\"]},{\"type\":\"chGain\",\"selector\":1,\"size\":\"4x1\",\"position\":{\"win\":0,\"x\":0,\"y\":1},\"color\":4,\"icon\":\"6\",\"properties\":[\"/ch/03/config/name\"]},{\"type\":\"ch2bus\",\"selector\":0,\"size\":\"4x1\",\"position\":{\"win\":1,\"x\":0,\"y\":2},\"color\":1,\"icon\":\"2\",\"properties\":[\"/ch/04/config/name\",\"/bus/02/config/name\"]},{\"type\":\"ch2bus\",\"selector\":1,\"size\":\"4x1\",\"position\":{\"win\":1,\"x\":0,\"y\":3},\"color\":1,\"icon\":\"2\",\"properties\":[\"/ch/05/config/name\",\"/bus/02/config/name\"]}]}"
      "}"
    "}";

    ScreenUnion oldScreen;    

    LinkedList<String*> countryList = LinkedList<String*>();   
    LinkedList<String*> webGZFiles = LinkedList<String*>();   

    bool webAppOn = false;
    IWebSocketServer *wss{ nullptr };        

    uint32_t minHeapSize,maxHeapSize;
    String heapSize;

    ///////////////////////////////////////////////////////////////////////

    void httpPost(Request &req, Response &res){
      #define COMMAND_SIZE 500
      #define OPTION_SIZE 64
      char name[COMMAND_SIZE];
      char options[OPTION_SIZE];    //options is not used (the fetch from client puts everything in the name as a json)
      res.set("Content-Type", "application/json");
      while (req.left()) {        
        req.form(name, COMMAND_SIZE, options, OPTION_SIZE);                
        if (debug){
          //DEBUG_PRINT("\n\n\t httpPost <%s>:<%s>\n\n",name,options);
        } 
        String jsonAnswer = parseJsonInput(name);
        if (debug){
          //DEBUG_PRINT("\n\t httpres:<%s>",jsonAnswer.c_str());
        }
        res.print(jsonAnswer);
        res.end();
      }  
    }  

    void populateFileList(String extension, String folder, LinkedList<String*> *list, String prefix="", bool withExtension=true) {
      //we first clean all items from the list

      while (list->size()>0){
        String* s = list->shift();
        delete(s);
      }

      File root = LittleFS.open(folder);
      //DEBUG_PRINT("\nListing: .%s",extension.c_str());
      File file = root.openNextFile();
      while (file){
        if (!file.isDirectory()){
          String fullFileName = file.name();
          if (fullFileName.endsWith(extension)) {
            String* filename=new String(prefix);
            // if we don't want the extension, we remmove it          
            if (!withExtension) *filename += fullFileName.substring(0, fullFileName.lastIndexOf('.'));
            else *filename += fullFileName; // k is the icon ID
            list->add(filename);
            //if (debug) DEBUG_PRINT("\n\t %s",filename->c_str());          
          }
        }
        file = root.openNextFile();
      }
    }

    void enableWizard(){
      persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_WIZARD);
      persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_LANG_STEP1);      
    }

    MenuItem* back_menu_item;
    MenuItem* info_menu_item;
    MenuItem* config_menu_item;
    Menu* profile_menu_item;
    MenuItem* save_profile_menu_item;
    MenuItem* tft_rotation_menu_item;
    MenuItem* reset_menu_item;   
    MenuItem* shutdown_menu_item; 

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

