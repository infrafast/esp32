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

#define ICON_SENDRETURN_STRIP "="
#define ICON_CHGAIN_STRIP     "5"


#define STRIP_XAIR_GROUP "xair"
#define STRIP_UTILS_GROUP "utils"

#define ADDRESS_MACRONAME "/macro/name/%s"
#define ADDRESS_ACTION    "/action/%s"
#define ADDRESS_IP  "/ip/%s"
#define ADDRESS_PORT "/port/%s"
#define ADDRESS_MESSAGE "/message/%s"

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

class MyIcon : public Font{
  public:
    static OpenFontRender* ofr; 
    
    MyIcon(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=false):Font(v, a, posX, posY, s, fC , bC , d){
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
      topLabels = new Top2Toggles(&gui->screenGUIs,&message, &portString, &on, &on,&frontColor,&backColor );          
      gui->add(topLabels);
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
  

  protected:
    String unitString="U";
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

    virtual void render(uint8_t shape){

      
       
      /*RingMeter* ringMeter1 = new RingMeter(&value, -1000,1000, 1, &app.quarter1xPos,&y, 32, &dBstring,&frontColor,&GUI::transparentColor,false);
      gui->add(ringMeter1);          

      RingMeter* ringMeter2 = new RingMeter(&value2, -1000,1000, 0, &app.quarter2xPos,&y, 32, &testUnitString,&GUI::blue,&GUI::transparentColor,false);
      gui->add(ringMeter2);          */

      //RingMeter* ringMeter3 = new RingMeter(&value, valueMin,valueMax, 1, &app.third3xPos,&y, 64, &testUnitString,GUI::glowingGoldColor.value,&GUI::transparentColor,false);
             

    }    

    /*virtual float getIncrement(unsigned long time){                  
      if (!Strip::progressiveDelay(time)) return true;
      float increment = 1;      
      if (time>2000) increment = (valueMax-valueMin) * 0.005;
      DEBUG_PRINT("\t INC: %.3f", increment);      
      return increment;
    }*/

  private:

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

DynamicDataSet macroNames = DynamicDataSet(ADDRESS_MACRONAME,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented
DynamicDataSet actionPlay = DynamicDataSet(ADDRESS_ACTION,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented

DynamicDataSet ipDS = DynamicDataSet(ADDRESS_IP,INPUTTEXT,&localProvider); 
DynamicDataSet portDS = DynamicDataSet(ADDRESS_PORT,INPUTTEXT,&localProvider); 
DynamicDataSet msgDS = DynamicDataSet(ADDRESS_MESSAGE,INPUTTEXT,&localProvider); 

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

String demoIP = "";
uint16_t demoPort=10023;

String mixerDemoJsonString(){
    JsonDocument json;

    json["demoPort"] = demoPort;
    json["demoIP"] = demoIP;

    String jsonString;
    serializeJson(json,jsonString);

    return jsonString;        
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

// overload the app.parseJsonInput to process input coming from user.html
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

String userPongCallback(){
  String pong = "";
  JsonDocument doc;
  // we send server connection attempts status only if network is connected, otherwise we polute the webapp with useless information
  if (hwm.network->state == CONNECTED && mixerServer.state == DISCONNECTED){
    //{"serverAttempt":"Trying 192.168.0.212 / 10026"}}>
    String serverAttempt = *app.lang.getString("Trying")+" "+mixerServer.serverIP.toString()+" "+mixerServer.behringerUDPPort+"/"+String(mixerServer.msUDPPort)+"/"+String(mixerServer.msAPIPort);
    doc["serverAttempt"] = serverAttempt;
  }
  serializeJson(doc,pong);
  //DEBUG_PRINT("\n\n\t\t\t userPongCallback -------------------------- %s\n\n",pong.c_str());  
  return pong;
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

void usertNetChangeStateCallback(bool state){
  DEBUG_PRINT("\nusertNetworkChangeStateCallback(%i)",state);
  //QueryingDemoMixer(hwm.network->internet);  
}

void userWizardGUI(void){
    //DEBUG_PRINT("\t User wizard");
    GUI::cls();
    gui.add(new Label(app.lang.getString("Waiting mixer"), Align::Center,&app.midScreenHoriz, &app.midScreenVertical,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));   
}

void userWizardStepFlow(){
  //DEBUG_PRINT("\ndefaultWizardStepFlow(%i)"); 
  if (mixerServer.state == true) app.gotoWizardEnd();
}

void userInternetCallBack(bool internet){
  DEBUG_PRINT("\n\n\n\t\t\t * * * * USER INTERNET CALLBACK %s * * * *",internet?"OK":"fail"); 
  // check updates

  // check remote instruction

  // check mixer demo online
  QueryingDemoMixer(internet);
  // check demoMixer
}

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

void userBootScreen(){
  Icon bootSprite = Icon(&app.intervalIcon,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*1.2,app.byeIconColor,&GUI::defaultBcolor,false);
  bootSprite.refresh(true);
  bootSprite.sprite->pushSprite(bootSprite.getX(),bootSprite.getY(),TFT_TRANSPARENT);      
}

//float tV = 95.0;
int16_t mixerY = TFT_HEIGHT - 120;
String interfaceStateMessage = "";
float dataProgress = 0.0;
#define PROGRESS_BAR_HEIGHT 20
int16_t progressBarY = PROGRESS_BAR_HEIGHT; //app.overlayIconY - PROGRESS_BAR_HEIGHT;

void userBankScreenBuilder(int subScreen){
  
  uint8_t interfaceState = mixerServer.mixerInterface->interfaceState;

  if (mixerServer.mixerInterface->interfaceSubState == "backgroundSync") app.rollingIcon(true);

  if (hwm.network->state == DISCONNECTED && mixerServer.offline == false){
    app.disabledBankRendering = true;
    buildNetworkScreen(ICON_FOOTSWITCH,*app.lang.getString("Scanning network"));
  }else{
    if (interfaceState == CONNECTED || mixerServer.offline == true/*|| interfaceState == SYNCING_DATA*/){
      app.disabledBankRendering = false; // normal strips and bank display
    }else{
      app.disabledBankRendering = true;

      GUI::defaultBcolor = TFT_BGCOLOR;
      GUI::defaultFcolor = TFT_FCOLOR;
      GUI::cls();

      interfaceStateMessage = *app.lang.getString(stateNames[interfaceState]);
      /*if (interfaceState == MIXER_UNREACHABLE) interfaceStateMessage += " " + String(mixerServer.serverUDPPort);
      else if (interfaceState == SOFTWARE_UNREACHABLE) interfaceStateMessage += " " + String(mixerServer.msAPIPort);*/  

      if (mixerServer.state == DISCONNECTED) gui.add(new Icon(&iconDisconnected,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,&iconMixerColor,&GUI::defaultBcolor));
      else if (interfaceState == SYNCING_SOFTWARE){
        gui.add(new MyIcon(&iconMS,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,&iconMixerColor,&GUI::defaultBcolor));
        //gui.add(new Arc(&mixerServer.mixerInterface->syncSoftwareProgress, 0.0, 100.0, &app.midScreenHoriz, &mixerY, 100, &GUI::green, &GUI::transparentColor,50));
        gui.add (new ProgressBarH(&mixerServer.mixerInterface->syncSoftwareProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &progressBarY /*y*/, TFT_WIDTH /*w*/, PROGRESS_BAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::green /*fc*/ , &GUI::defaultBcolor/*bc*/, false));                  
      }else{                        
        if (interfaceState == SYNCING_DATA){
          gui.add(new MyIcon(&iconMixer,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,&iconMixerColor,&GUI::defaultBcolor));
          //gui.add(new Arc(&dataProgress, 0.0, 100.0, &app.midScreenHoriz, &mixerY, 100, &GUI::green, &GUI::transparentColor,50));
          gui.add (new ProgressBarH(&dataProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &progressBarY /*y*/, TFT_WIDTH /*w*/, PROGRESS_BAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::green /*fc*/ , &GUI::defaultBcolor/*bc*/, false));                    
        }else{
          if (mixerServer.mixerInterface->model == MIXING_STATION_MODEL) gui.add(new MyIcon(&iconMS,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,&iconMixerColor,&GUI::defaultBcolor));
          else gui.add(new MyIcon(&iconMixer,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,&iconMixerColor,&GUI::defaultBcolor));        
        }      
      }
      gui.add(new Label(&interfaceStateMessage, Align::Center,&app.midScreenHoriz, &app.menuTitleY,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));

    }
  }
}

//#define DEFAULT_USER_CONFIG "{\"interface\":\"Offline\",\"serverIP\":\"0.0.0.0\",\"serverPort\":3000,\"serverName\":\"auto\",\"mixerPort\":8080,\"mixerModel\":\"MixingStation\",\"autoConnectModel\":-1,\"autoConnectIP\":\"192.168.0.1\"}"

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
      usertNetChangeStateCallback(state);
    }break;
    case EventDispatcher::EVENT_INTERNET_PING:{
      bool online = (bool*)event->data;
      userInternetCallBack(online);
    }break;
  } 
}

void setup() {

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


  DEBUG_PRINT("\n\n\n=====================================================================");
  DEBUG_PRINT("\n============================ SETUP END ==============================");
  DEBUG_PRINT("\n=====================================================================\n\n");  
  /*if (psramFound()) {
      DEBUG_PRINT("\nPSRAM activée !\n");
  } else {
      DEBUG_PRINT("\nPSRAM non trouvée !\n");
  }*/

  size_t dataSize = 0;
  dataSize = readGzFileToBuffer("/webapp/user.ttf.gz",&userFontBuffer);
  if (dataSize > 0){ // uncompress succeed
    if (myOfr.loadFont(userFontBuffer.buffer, dataSize)) { // this consumes 20992 bytes and take ~2ms      
      DEBUG_PRINT("\nUser Initialise error with compressed font"); 
    }
  }

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
}

void loop() {

  app.refresh();
  mixerServer.refresh();

  static uint8_t previousInterfaceState = DISCONNECTED;
  static String  previousInterfaceSubState = "";

  uint8_t interfaceState = mixerServer.mixerInterface->interfaceState;

  if (interfaceState != previousInterfaceState || mixerServer.refreshed || hwm.network->refreshed  ){   
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
