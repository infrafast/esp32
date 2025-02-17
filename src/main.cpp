// debugging https://docs.platformio.org/en/stable/tutorials/espressif32/arduino_debugging_unit_testing.html

// make sure to change config.hpp of mWebsocket 
//constexpr uint16_t kBufferMaxSize{512};

#define USE_NETWORK
//#define TFT_BGCOLOR   TFT_DARKGREY
//#define TFT_FCOLOR    TFT_SILVER

#include "Interval.h"
#include "mixerServer.h"

#define USE_FTP

#if defined(USE_FTP) && defined(USE_NETWORK)
  #include "ESP-FTP-Server-Lib.h"
  #include "FTPFilesystem.h"
  FTPServer ftp;
#endif

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    STRIP DEFINITIONS    /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
#define ICON_SENDRETURN_STRIP "="
#define ICON_CHGAIN_STRIP     "5"


#define STRIP_XAIR_GROUP "xair"
#define STRIP_UTILS_GROUP "utils"

#define ADDRESS_MACRONAME "/macro/name/%s"
#define ADDRESS_ACTION    "/action/%s"
#define ADDRESS_IP  "/ip/%s"
#define ADDRESS_PORT "/port/%s"
#define ADDRESS_MESSAGE "/message/%s"

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    GLOBAL VARIABLES     /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
String demoIP = "";
uint16_t demoPort=10023;

String iconMixer = "!";  
String iconMS = ">"; //ICON_MIXING_STATION;
String iconDisconnected = ICON_FOOTSWITCH;
uint16_t iconMixerColor = GUI::red;
AlteredColor mixerGlowingColor = AlteredColor{.baseColor = GUI::red, .value =  &iconMixerColor};

MixerServer mixerServer(&app.dataRepository);
Config userConfig = Config();

StripSkeleton* stripSendSkel;
StripSkeleton* stripOSCSkel;

ByteBuffer userFontBuffer;

OpenFontRender myOfr = OpenFontRender();

I18n i18n = I18n();

int16_t netIconY = (TFT_HEIGHT/2)-(ICON_MAX_SIZE/2.3), bottomText = TFT_HEIGHT-OVERLAY_ICON_SIZE-(CONFIG_TEXTAREA_MESSAGE_SIZE/2), belowIconText = bottomText-CONFIG_TEXTAREA_MESSAGE_SIZE, aboveBottomText = bottomText-7;

//float tV = 95.0;
int16_t mixerY = TFT_HEIGHT - 120;
float dataProgress = 0.0;
#define PROGRESS_BAR_HEIGHT 20
int16_t progressBarY = PROGRESS_BAR_HEIGHT; //app.overlayIconY - PROGRESS_BAR_HEIGHT;

float mixerPendingBarWidth = 100.0;

int8_t previousInterfaceState = DISCONNECTED;


///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    CLASS DEFINITIONS    /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////

class MyIcon : public Font{
  public:
    static OpenFontRender* ofr; 
    
    MyIcon(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, uint16_t _lumi=255, bool d=false):Font(v, a, posX, posY, s, fC , bC , _lumi, d){
      id = "Icon"+String(*posX)+"x"+String(*posY)+"~"+*v;   
      //if (debug) DEBUG_PRINT("\n new %s",id);
    };
    
  protected:
    virtual OpenFontRender* getOfr(){return ofr;}
};

OpenFontRender* MyIcon::ofr = &myOfr;

class StripMixer{
  public:
    StripMixer(MixerServer* ms) : mixerServer(ms){}

  protected:
    MixerServer* mixerServer;
};


class OSCTestStrip : public TextStrip, public StripMixer{
  #define STRIP_OSCTEST "oscTest"
  public:

    OSCTestStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):TextStrip(s,g, debug),StripMixer(ms){
      portPropertyAddress = s->dataSetAddresses.get(1);
      msgPropertyAddress =  s->dataSetAddresses.get(2);
      message = *app.lang.getString("undefined");
      port = 0;
    }

    virtual void deserializeProperty(String property)  {  
      TextStrip::deserializeProperty(property);
      // text contains the property 0 which is the IP address
      ip.fromString(text);

      // property = /port/3000
      // portProperty = /port/%s
      // portString = 3000
      char buffer[50];            
      
      if (sscanf(property.c_str(), portPropertyAddress.c_str(), buffer) == 1){
        portString = String(buffer);
        port = portString.toInt();
      }else if (sscanf(property.c_str(), msgPropertyAddress.c_str(), buffer) == 1){
        message = String(buffer);
      }     
    }

    virtual void serializeProperties(){
      TextStrip::serializeProperties();
      
      char* buf = (char*)malloc(portPropertyAddress.length()*sizeof(char*));
      sprintf(buf,portPropertyAddress.c_str(),String(port).c_str());
      properties.add(String(buf));
      delete buf;

      buf = (char*)malloc(msgPropertyAddress.length()*sizeof(char*));
      sprintf(buf,msgPropertyAddress.c_str(),message.c_str());
      properties.add(String(buf));
      delete buf;            
    }
    
    virtual bool actionCallback(uint8_t trigger){
      JsonDocument command;
      command["msg"] = message;
      command["ip"] = text;
      command["port"] = port;
      return mixerServer->mixerInterface->sendCommand("testMixingOSC",command);      
      return true;
    }

    virtual void render(uint8_t shape){
      TextStrip::render(shape);
      topLabels = new Top2Toggles(&gui->screenGUIs,&message, &portString, &on, &on,&frontColor,&backColor,&GUI::defaultFcolor, &GUI::silver,lumi );          
      gui->add(topLabels);
      DEBUG_PRINT("\nOSCTestStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY);      
    }

  private:
    bool on = false;

    String portPropertyAddress;
    String msgPropertyAddress;

    Top2Toggles* topLabels;
    
    int port; String portString;
    IPAddress ip;
    String message;
};

class SendStrip : public ThreeSelectStrip, public StripMixer{
  public:

    SendStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):ThreeSelectStrip(s,g, debug),StripMixer(ms){
    }

    ~SendStrip(){};
  
    virtual bool refresh(bool force=false){
      refreshed = force;
      ThreeSelectStrip::refresh(force);
      return refreshed;
    }
  
    virtual void render(uint8_t shape) override{      

      uint16_t h = 0, w = 0;

      switch (shape){
        case SHAPE_FULL:{
          h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
          w = WIDGET4X4W;
          renderY += h/2; 
          gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, lumi));
        }break;
        case SHAPE_1X4:{
          w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
          h = WIDGET1X4H;
          renderX += w/2;           
          gui->add(new ProgressBarV(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, lumi));
        }break;
        case SHAPE_2X2:{
          h = WIDGET2X2H/2;   // height of the bar is 2 time less big than the full height of the cell          
          uint8_t widthOffset = WIDGET2X2H/10;
          w = WIDGET2X2W-widthOffset;
          gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, lumi));
        }break;
        case SHAPE_4X1:{
          h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
          w = WIDGET4X1W;
          gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, lumi));
        }break;
      }      
      labelX = renderX+w/2;
      labelY = renderY+h;

      gui->add(new Label(&value, Align::Center,&labelX,&labelY,h*1.5,&GUI::white,&GUI::defaultBcolor, lumi));

      DEBUG_PRINT("\nsendStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

  protected:
    String unitString="U";
    int16_t labelX = 0;
    int16_t labelY = 0;

};


bool setChBusMute(bool mute,uint8_t chID, uint8_t busID){
      JsonDocument command;
      command["ch"] = chID;
      command["bus"] = busID;
      command["on"] = !mute;
      return mixerServer.mixerInterface->sendCommand("setChBusMute",command);
}

class Ch2BusStrip : public SendStrip{
  #define STRIP_CH2BUS "ch2bus"

  public:

    Ch2BusStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):SendStrip(s,g,ms,debug){      
      unitString = "dB";
      valueMin = 0; valueMax = 100;
    }

    virtual bool actionCallback(uint8_t trigger){
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      //DEBUG_PRINT("\nCh2BusStrip actionCallback");
      toggle1 = !toggle1;
      return setChBusMute(toggle1,select2ID,select3ID);     

    }

    virtual bool refresh(bool force=false){
      refreshed = force;
      SendStrip::refresh(force);
      // this code should only be called if select2ID != oldSelect2ID
      //...
      // put then the code specific here to treatment, for the value treatment like OSC frame to read the value
      return refreshed;
    }
};

#define CHGAINDB_MIN  -12
#define CHGAINDB_MAX  60
class ChGainStrip : public TwoSelectStrip, public StripMixer{
  #define STRIP_CHGAIN "chGain"

  public:
    ChGainStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):TwoSelectStrip(s,g,debug),StripMixer(ms){
      valueMin = CHGAINDB_MIN;
      valueMax = CHGAINDB_MAX;
      value = (valueMin + valueMax)/2;
      label = "gain";
      //icon = ICON_CHGAIN_STRIP;
    }

    ~ChGainStrip(){};

    virtual float increaseCallBack(unsigned long time,uint8_t trigger){
      float delta = Strip::increaseCallBack(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger){
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    bool setChGain(){
      JsonDocument param;
      param["ch"] = select2ID;
      param["value"] = value;
      return mixerServer->mixerInterface->sendCommand("setChGain",param);      
    }

    virtual bool actionCallback(uint8_t trigger){
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      /*char addr[100];

      //select2ID = chID

      //sprintf(addr,mixerServer->chNames.address.c_str(),chID);
      String val = "name"+String(random(1000));
      DEBUG_PRINT("\n %s %s",addr,val.c_str());
    
      //chNames.values.set(chID-1, val);
      //mixerServer->chNames.setValue(chID-1,val);
      //app.broadCast("{\"updateDataSets\":[{\""+String(addr)+"\":\""+val+"\"}]}");

      refresh();*/
      return true;
    }
       
    virtual bool refresh(bool force=false){
      refreshed = force;
      TwoSelectStrip::refresh(force);
      return refreshed;
    };    

    /*virtual float getIncrement(unsigned long time){                  
      if (!Strip::progressiveDelay(time)) return true;
      float increment = 1;      
      if (time>2000) increment = (valueMax-valueMin) * 0.005;
      DEBUG_PRINT("\t INC: %.3f", increment);      
      return increment;
    }*/

  private:
    String unitString = "dB";

};

class LocalDataProvider: public DataProvider{ // temp while shortcut factory is not implemented
  public :
    
    #define ADDRESS_ACTION_SIZE 5

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_MACRONAME) return 0;
      else if (addrr == ADDRESS_ACTION) return ADDRESS_ACTION_SIZE;
      else return 1;
    }
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_MACRONAME) return &getMacroName;
      else if (addrr = ADDRESS_ACTION) return &actionNames[index];         //javascript local function callback
      else return &voidStatic;
    }


  private:
    String actionNames[ADDRESS_ACTION_SIZE] = {
      "play",
      "pause",
      "stop",
      "FF",
      "RW"      
    };

    String getMacroName = "getMacroName";

} localProvider; // temp while shortcut factory is not implemented;


///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    DATA DEFINITIONS    /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////


DynamicDataSet macroNames = DynamicDataSet(ADDRESS_MACRONAME,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented
DynamicDataSet actionPlay = DynamicDataSet(ADDRESS_ACTION,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented

DynamicDataSet ipDS = DynamicDataSet(ADDRESS_IP,INPUTTEXT,&localProvider); 
DynamicDataSet portDS = DynamicDataSet(ADDRESS_PORT,INPUTTEXT,&localProvider); 
DynamicDataSet msgDS = DynamicDataSet(ADDRESS_MESSAGE,INPUTTEXT,&localProvider); 

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                               /////////////////////
/////////////////////    STRIP CONSTRUCTOR CALLBACKS DEFINITIONS    /////////////////////
/////////////////////                                               ////////////////////
////////////////////////////////////////////////////////////////////////////////////////


Strip* testOSCStripConstructorCallback(){
  return new OSCTestStrip(stripOSCSkel,&gui,&mixerServer);
}

Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tch2busStripConstructorCallback()");
  return new Ch2BusStrip(stripSendSkel,&gui,&mixerServer);
}

StripSkeleton* stripChGainSkel;
Strip* gain2chStripConstructorCallback(){
  //DEBUG_PRINT("\n\tgain2chStripConstructorCallback()");
  return new ChGainStrip(stripChGainSkel,&gui,&mixerServer);
}

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    UTILITIES            /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
String mixerDemoJsonString(){
  JsonDocument json;

  json["demoPort"] = demoPort;
  json["demoIP"] = demoIP;

  String jsonString;
  serializeJson(json,jsonString);

  return jsonString;        
}

void QueryingDemoMixer(bool internet){
  demoIP = "";
  if (internet){
    DEBUG_PRINT("\nQuerying demo mixer");    
    IPAddress demoServer(82,65,4,93);
    OSCInterface osc(hwm.network);
    //osc.setClient(demoServer,10023);

    OSCValues data;
    data.expectedStr = "/xinfo";

    if(osc.blockingPing(demoServer,10023,&data,hwm.network->WANlatency)){
        demoIP = "82.65.4.93";                // should be retrieved by an http request
        DEBUG_PRINT("\tDemo Mixer responded");    
    }else{
      DEBUG_PRINT("\tDemo Mixer didn't respond");    
    }
  }
  String jsonString;
  JsonDocument json;
  json[MIXER_DEMO_KEY] = mixerDemoJsonString();
  serializeJson(json,jsonString);
  removeJsonQuote(&jsonString);
  app.broadCast(jsonString); // refresh application demoMixer dropdown  
}

String getMixersState(){
  String jsonString;
  JsonDocument json;
  //{{mixerState:{"interfaceState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}},"mixerDemo":{"ip":"82.85.52.72","port":10023}}
  json[MIXER_STATE_KEY] = mixerServer.getMixerState();
  json[MIXER_DEMO_KEY] = mixerDemoJsonString();
  serializeJson(json,jsonString);
  removeJsonQuote(&jsonString);
  return jsonString; 
}

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    USER CALLBACKS       /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////


/////////////////////    USER.HTML CALLBACKS       /////////////////////
#define USER_CONFIG_REQ "userConfig"
#define RET_RELOAD_BANKS "reloadBanks"
bool userHTMLJsonInput(JsonDocument jsonDoc,JsonAnswer* jsonAnswer){
  bool success = false;
  if (jsonDoc["command"] == MIXER_STATE_KEY ){
    jsonAnswer->value = getMixersState();
    jsonAnswer->callback = MIXER_STATE_KEY;
    success = true;
    // ok remote
  }else if (jsonDoc["command"] == MIXER_SET_KEY ){
    //"setMixer","param":{"serverIP":"192.168.0.200","serverPort":"10024","mixerPort":"8080","serverModel":"XR16","autoConnectModel":"-1","autoConnectIP":"192.168.0.1"}
    mixerServer.serverIP.fromString(jsonDoc["param"][SERVER_IP_KEY].as<String>());
    mixerServer.serverUDPPort = jsonDoc["param"][SERVER_PORT_KEY];
    mixerServer.msAPIPort = jsonDoc["param"][MIXER_PORT_KEY];

    userConfig.setValue(SERVER_IP_KEY,mixerServer.serverIP.toString());    
    userConfig.setValue(SERVER_PORT_KEY,String(mixerServer.serverUDPPort));
    userConfig.setValue(SERVER_MODEL_KEY,jsonDoc["param"][SERVER_MODEL_KEY]);

    userConfig.setValue(MIXER_PORT_KEY,String(mixerServer.msAPIPort));     
    userConfig.setValue(MIXER_AUTOMODEL_KEY,jsonDoc["param"][MIXER_AUTOMODEL_KEY]);
    userConfig.setValue(MIXER_AUTOIP_KEY,jsonDoc["param"][MIXER_AUTOIP_KEY]);


    String serverConnection = jsonDoc["param"][SERVER_SELECTED_KEY];
    if (serverConnection == "offline"){
      mixerServer.offline = true;
      jsonAnswer->callback = RET_RELOAD_BANKS;  // when offline we reload the banks to adapt the number of channel to the selected mixer
      jsonAnswer->value = "";      
    }else{
      mixerServer.offline = false;    // do not reload bank, this will be done if interface is connected 
      jsonAnswer->callback = RET_SAVEBUTTON;
      jsonAnswer->value ="1";
    }  

    success = true;
    mixerServer.disconnect(); // this will force trying to reconnect with new parameters
    // force GUI rebuild
  }else if (jsonDoc["command"] == USER_CONFIG_REQ){
    jsonAnswer->callback = USER_CONFIG_REQ;
    JsonDocument doc;
    doc["mixers"] =  getMixersState();
    //doc["interfaces"] = mixerServer.getInterfaces();
    String jsonString;
    serializeJson(doc,jsonString);
    removeJsonQuote(&jsonString);    
    jsonAnswer->value = jsonString;// TO BE ADAPATED
    success = true;
  }
  return success;
}

/////////////////////    PONG CALLBACKS       /////////////////////
String userPongCallback(){
  String pong = "";
  JsonDocument doc;
  // we send server connection attempts status only if network is connected, otherwise we polute the webapp with useless information
  if (hwm.network->state == CONNECTED && mixerServer.state == DISCONNECTED){
    //{"serverAttempt":"Trying 192.168.0.212 / 10026"}}>
    String serverAttempt = *i18n.getString("Trying")+" "+mixerServer.serverIP.toString()+" "+mixerServer.behringerUDPPort+"/"+String(mixerServer.msUDPPort)+"/"+String(mixerServer.msAPIPort);
    doc["serverAttempt"] = serverAttempt;
  }
  serializeJson(doc,pong);
  //DEBUG_PRINT("\n\n\t\t\t userPongCallback -------------------------- %s\n\n",pong.c_str());  
  return pong;
}

/////////////////////    NETWORK       /////////////////////
void netChangeState(bool state){
  DEBUG_PRINT("\nusertNetworkChangeStateCallback(%i)",state);  
  // in case we were offline but footswitch connects a network, we give the possibility to connect again or offer user to confirm he wants to stay offline
  if (mixerServer.offline == true && hwm.network->state == CONNECTED) mixerServer.offline = false;  
}

void internetChangeState(bool internet){
  DEBUG_PRINT("\n\n\n\t\t\t * * * * USER INTERNET CALLBACK %s * * * *",internet?"OK":"fail"); 
  // check updates

  // check remote instruction

  // check mixer demo online
  QueryingDemoMixer(internet);
  // check demoMixer
}

/////////////////////    EVENT HANDLING       /////////////////////
void networkEvent(WiFiEvent_t event){
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_READY");
    break;
    case ARDUINO_EVENT_WIFI_STA_START:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_START");
    break;    
    case ARDUINO_EVENT_WIFI_STA_STOP:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_CONNECTED");
    break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_GOT_IP");
    break;
    case ARDUINO_EVENT_WIFI_AP_STOP: //SYSTEM_EVENT_STA_STOP:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: //SYSTEM_EVENT_STA_DISCONNECTED:
      DEBUG_PRINT("~");
    break;
    case ARDUINO_EVENT_WIFI_AP_START: //SYSTEM_EVENT_AP_START:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_START");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: //SYSTEM_EVENT_AP_STACONNECTED:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STACONNECTED");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: //SYSTEM_EVENT_AP_STADISCONNECTED:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STADISCONNECTED");
      //hwm.ap->state = DISCONNECTED;
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED: //SYSTEM_EVENT_AP_STAIPASSIGNED:
      //hwm.ap->state = CONNECTED;
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED");
    break;
    case ARDUINO_EVENT_ETH_START:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_START");
    break;
    case ARDUINO_EVENT_ETH_STOP:
      DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_ETH_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_LOST_IP");
    break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_ETH_CONNECTED");
    break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_DISCONNECTED");
    break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_GOT_IP");
    break;
    default: 
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: %i",event);
    break;
  }
}

void eventHandling(EventDispatcher::Event* event){  
  switch(event->type){
    case EventDispatcher::EVENT_DIGITAL_BUTTON:{
      ButtonEvent* btnEvt = (ButtonEvent*)event->data;
      switch(btnEvt->id){
        case Button::BUTTON_SIMPLE_CLIC:
          DEBUG_PRINT("\n\t\t *** EVENT_DIGITAL_BUTTON SIMPLE_CLIC");
          if (screen.id == SCREEN_BANK && mixerServer.state == DISCONNECTED){
            mixerServer.offline = true;
            app.broadCast(getMixersState());
          }
        break;
      }
    }break;
    case EventDispatcher::EVENT_NETWORK:{
      WiFiEvent_t* evt = (WiFiEvent_t*)event->data;
      networkEvent(*evt);
    }break;
    case EventDispatcher::EVENT_NETWORK_CHANGED_STATE:{
      bool state = (bool*)event->data;
      netChangeState(state);
    }break;
    case EventDispatcher::EVENT_INTERNET_PING:{
      bool online = (bool*)event->data;
      internetChangeState(online);
    }break;
  } 
}

/////////////////////    GUI       /////////////////////
void userWizardGUI(void){
    //DEBUG_PRINT("\t User wizard");
    GUI::cls();
    gui.add(new Label(i18n.getString("searching"), Align::Center,&app.midScreenHoriz, &app.midScreenVertical,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));   
}

void userWizardStepFlow(){
  //DEBUG_PRINT("\ndefaultWizardStepFlow(%i)"); 
  if (mixerServer.state == true) app.gotoWizardEnd();
}

void userBootScreen(){
  Icon bootSprite = Icon(&app.intervalIcon,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*1.2,app.byeIconColor,&GUI::defaultBcolor);
  bootSprite.refresh(true);
  TFT_eSprite* sprite = bootSprite.sprite;
  if (sprite != nullptr) sprite->pushSprite(bootSprite.getX(),bootSprite.getY(),TFT_TRANSPARENT);      
}

void modalScreen(const String* topText, const String* midLabel, const String* bottomLabel){
  gui.add(new TextArea(topText, TFT_WIDTH*0.90,Align::Center,Align::Center,&app.midScreenHoriz,&app.tenPixels,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));
  gui.add(new Label(midLabel, Align::Center,&app.midScreenHoriz, &belowIconText,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::gold,&GUI::defaultBcolor));    
  gui.add(new Label(bottomLabel, Align::Center,&app.midScreenHoriz, &bottomText,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor,150));    
}

void userBankScreenBuilder(int subScreen){  
  DEBUG_PRINT("\n - - - - - - - userBankScreenBuilder - - - - -");

  uint8_t interfaceState = mixerServer.mixerInterface->interfaceState;

  if (mixerServer.mixerInterface->interfaceSubState == "backgroundSync") app.rollingIcon(true);

  if (hwm.network->state == DISCONNECTED && mixerServer.offline == false){
    app.disabledBankRendering = true;    
    
    gui.add(new Icon(&app.networkIcon,Align::Center,&app.midScreenHoriz, &netIconY,ICON_MAX_SIZE*0.9,&app.networkIconColorIdle,&GUI::defaultBcolor));    
    modalScreen(i18n.getString("Attempt"),&hwm.network->SSID,i18n.getString("offline"));
    gui.add (new ProgressBarH(&hwm.network->wiFiConnectAttemptProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &aboveBottomText /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                      

  }else{
    if (interfaceState == CONNECTED || mixerServer.offline == true/*|| interfaceState == SYNCING_DATA*/){
      app.disabledBankRendering = false; // normal strips and bank display
    }else{
      app.disabledBankRendering = true;

      GUI::defaultBcolor = TFT_BGCOLOR;
      GUI::defaultFcolor = TFT_FCOLOR;
      
      if (mixerServer.state == DISCONNECTED){

        gui.add(new MyIcon(&iconMixer,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*.9,&iconMixerColor,&GUI::defaultBcolor));   
        modalScreen(i18n.getString("offline"),i18n.getString(stateNames[interfaceState]),i18n.getString("searching"));  
        gui.add (new PendingBar(&mixerPendingBarWidth /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &aboveBottomText /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                          

      }else if (interfaceState == SYNCING_SOFTWARE){

        gui.add(new MyIcon(&iconMS,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*.9,&iconMixerColor,&GUI::defaultBcolor));   
        modalScreen(i18n.getString("offline"),i18n.getString(stateNames[interfaceState]),i18n.getString("searching"));  
        gui.add (new ProgressBarH(&mixerServer.mixerInterface->syncSoftwareProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &aboveBottomText /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                      

        //gui.add(new Arc(&mixerServer.mixerInterface->syncSoftwareProgress, 0.0, 100.0, &app.midScreenHoriz, &mixerY, 100, &GUI::green, &GUI::transparentColor,50));        
      }else{                        
        modalScreen(&emptyString,i18n.getString(stateNames[interfaceState]),&emptyString);  
        if (interfaceState == SYNCING_DATA){
          
          gui.add(new MyIcon(&iconMixer,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*.9,&iconMixerColor,&GUI::defaultBcolor));   
          gui.add (new ProgressBarH(&dataProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &aboveBottomText /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::green /*fc*/ , &GUI::defaultBcolor/*bc*/));                               

          //gui.add(new Arc(&dataProgress, 0.0, 100.0, &app.midScreenHoriz, &mixerY, 100, &GUI::green, &GUI::transparentColor,50));
        }else{
          if (mixerServer.mixerInterface->model == MIXING_STATION_MODEL) gui.add(new MyIcon(&iconMS,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*.9,&iconMixerColor,&GUI::defaultBcolor));
          else gui.add(new MyIcon(&iconMixer,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*.9,&iconMixerColor,&GUI::defaultBcolor));        
        }      
      }
    }
  }
}


///////////////////////  USER CONFIG   ////////////////////////////////////////
#define DEFAULT_USER_CONFIG "{\
  \"" INTERFACE_STATE_KEY "\":\"Offline\",\
  \"" SERVER_IP_KEY "\":\"0.0.0.0\",\
  \"" SERVER_PORT_KEY "\":3000,\
  \"" SERVER_NAME_KEY "\":\"auto\",\
  \"" MIXER_PORT_KEY "\":8080,\
  \"" SERVER_MODEL_KEY "\":\"MixingStation\",\
  \"" MIXER_AUTOMODEL_KEY "\":-1,\
  \"" MIXER_AUTOIP_KEY "\":\"192.168.0.1\"\
}"

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
                          void setup()
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
{

  stripSendSkel = new StripSkeleton(STRIP_CH2BUS,STRIP_XAIR_GROUP,sendStripConstructorCallback,'!');  
  stripSendSkel->registerDataSetAddress(ADDRESS_CHNAME);
  stripSendSkel->registerDataSetAddress(ADDRESS_BUSNAME);
  
  stripChGainSkel = new StripSkeleton(STRIP_CHGAIN,STRIP_XAIR_GROUP,gain2chStripConstructorCallback,'y'); 
  stripChGainSkel->registerDataSetAddress(ADDRESS_CHNAME);

  stripOSCSkel = new StripSkeleton(STRIP_OSCTEST,STRIP_UTILS_GROUP,testOSCStripConstructorCallback,'a');
  stripOSCSkel->registerDataSetAddress(ADDRESS_IP);
  stripOSCSkel->registerDataSetAddress(ADDRESS_PORT);
  stripOSCSkel->registerDataSetAddress(ADDRESS_MESSAGE);

  app.stripFactory.add(stripTextSkel);
  app.stripFactory.add(stripSendSkel);
  app.stripFactory.add(stripChGainSkel);
  app.stripFactory.add(stripOSCSkel);

  app.dataRepository.addDataSet(&macroNames);   
  app.dataRepository.addDataSet(&actionPlay);  
  app.dataRepository.addDataSet(&ipDS);  
  app.dataRepository.addDataSet(&portDS);  
  app.dataRepository.addDataSet(&msgDS);  

  app.registerCallback(eventHandling);

  app.userParseJsonInputCallback = userHTMLJsonInput;
  app.userJsonPongCallback = userPongCallback;    
  app.userWizardScreenBuilder = userWizardGUI;
  app.userWizardstepFlow = userWizardStepFlow;
  app.bootScreen = userBootScreen;
  app.userBankScreenBuilder = userBankScreenBuilder;

  app.init(&userConfig);
  userConfig.load("/userConf.conf",DEFAULT_USER_CONFIG);  
  mixerServer.init(&userConfig,hwm.network);

  gui.alphaColors.add(&mixerGlowingColor);

  /*String data;
  app.serialize(&data);
  DEBUG_PRINT("\nAPPLICATION SERIALIZATION:<%s>",data.c_str());*/

  #if defined(USE_FTP) && defined(USE_NETWORK)
    ftp.addUser("ftp", "ftp");  
    ftp.addFilesystem("LITTLEFS", &LittleFS);
    ftp.begin();
  #endif
  
  String configIP = *userConfig.getString(SERVER_IP_KEY); 
  DEBUG_PRINT("\n * * SERVER_IP_KEY = %s",configIP.c_str());

  size_t dataSize = 0;
  myOfr.setDebugLevel( OFR_INFO | OFR_ERROR);
  dataSize = readGzFileToBuffer("/webapp/user.ttf.gz",&userFontBuffer);
  if (dataSize > 0){ // uncompress succeed
    if (myOfr.loadFont(userFontBuffer.buffer, dataSize)) { // this consumes 20992 bytes and take ~2ms      
      DEBUG_PRINT("\nUser compressed font initialise error"); 
    }else{
      //DEBUG_PRINT("\nUser font loaded successfullly"); 
    }
  }

  String langFile = "/"+*app.system.getString(S_LANG,"ENGLISH");
  i18n.load(langFile+".i18n"); 

/*  JsonDocument interfaceDoc;
  interfaceDoc["hello"]="world";
  interfaceDoc["titi"]="toto";
  String dataToCompress;
  serializeJson(interfaceDoc,dataToCompress);      
  DEBUG_PRINT("\ndataToCompress->%s",dataToCompress.c_str());
  dataSize = writeBufferToGzFile("/data.gz",&dataToCompress);


  ByteBuffer decompressedJson;
  dataSize = 0;
  dataSize = readGzFileToBuffer("/data.gz",&decompressedJson);
  if (dataSize == 0){ // uncompress failed
    DEBUG_PRINT("\nFailed to decompress /data.gz");
    return;
  }else{
    DEBUG_PRINT("\ndecompressed data ->  %s  <-\n",decompressedJson.buffer);
  }*/
  DEBUG_PRINT("\n\n\n=====================================================================");
  DEBUG_PRINT("\n============================ SETUP END ==============================");
  DEBUG_PRINT("\n=====================================================================\n\n"); 
}

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
                          void loop()
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
{
  app.refresh();
  mixerServer.refresh();

  static String  previousInterfaceSubState = "";

  uint8_t interfaceState = mixerServer.mixerInterface->interfaceState;

  if (interfaceState != previousInterfaceState || mixerServer.refreshed || hwm.network->refreshed ){   
    
    if (mixerServer.refreshed){
      
    }else{
      DEBUG_PRINT("\n*********************** INTERFACE CHANGED STATE FROM:%s TO: %s",stateNames[previousInterfaceState].c_str(),stateNames[interfaceState].c_str());
    }

    previousInterfaceState = interfaceState;
    
    if (interfaceState == SYNCING_SOFTWARE || interfaceState == SYNCING_DATA){
      //fixed color
      mixerGlowingColor.value = &GUI::unUsedColor;
      iconMixerColor = GUI::green;
    }else{
      //glowing color
      mixerGlowingColor.value = &iconMixerColor;
            
      // interfaceState
      if (interfaceState == CONNECTING || interfaceState == IDLE)  mixerGlowingColor.baseColor = GUI::blue;
      else if (interfaceState == MIXER_UNREACHABLE)  mixerGlowingColor.baseColor = GUI::orange;
      else if (interfaceState == SOFTWARE_UNREACHABLE)  mixerGlowingColor.baseColor = GUI::orange;
      else{
        // mixer state
        if (mixerServer.state == CONNECTED){
          mixerGlowingColor.baseColor = GUI::blue;      
        }
        else mixerGlowingColor.baseColor = GUI::red;
      }
    }

    app.broadCast(getMixersState());

    if (screen.id == SCREEN_BANK) screen.focus = screen.focus - 1;    // rebuild GUI, will call userBuildScreen 
  }

  if (mixerServer.mixerInterface->interfaceState == SYNCING_SOFTWARE){
    static uint8_t previousProgress = 254;
    app.rollingIcon(true);
    uint8_t progress = mixerServer.mixerInterface->syncSoftwareProgress;
    if (progress<100 && previousProgress!=progress){
      previousProgress = progress;
      app.broadCast("{\"syncProgress\":"+String(progress)+"}");
    }
  }else if (mixerServer.mixerInterface->interfaceState == SYNCING_DATA ){
    static uint8_t previousDataProgress = 254;
    app.rollingIcon(true);
    dataProgress = mixerServer.mixerInterface->syncDataProgress();
    if (dataProgress<100 && previousDataProgress!=dataProgress){
      previousDataProgress = dataProgress;
      app.broadCast("{\"syncProgress\":"+String(dataProgress)+"}");
    }
  }

  if (previousInterfaceSubState != mixerServer.mixerInterface->interfaceSubState){
    previousInterfaceSubState = mixerServer.mixerInterface->interfaceSubState;
    if (mixerServer.mixerInterface->interfaceSubState == "backgroundSync") app.rollingIcon(true);
    else app.rollingIcon(false);
  }

  #if defined(USE_FTP) && defined(USE_NETWORK)
    ftp.handle();
  #endif
}
