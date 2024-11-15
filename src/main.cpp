// debugging https://docs.platformio.org/en/stable/tutorials/espressif32/arduino_debugging_unit_testing.html

// make sure to change config.hpp of mWebsocket 
//constexpr uint16_t kBufferMaxSize{512};

//  increaseCallback (hold switch) also call selectStrip, check that

#define USE_NETWORK
//#define TFT_BGCOLOR   TFT_DARKGREY
//#define TFT_FCOLOR    TFT_SILVER

#include "Interval.h"
#include "mixerServer.h"

#if defined(DEBUG) && defined(USE_NETWORK)
  #include "ESP-FTP-Server-Lib.h"
  #include "FTPFilesystem.h"
  FTPServer ftp;
#endif

#define STRIP_XAIR_GROUP "xair"

class StripMixer{
  public:
    StripMixer(MixerServer* ms) : mixerServer(ms){}

  protected:
    MixerServer* mixerServer;
};

class SendStrip : public ThreeSelectStrip, public StripMixer{
  public:

    SendStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):ThreeSelectStrip(s,g, &sendName, &returnName, debug),StripMixer(ms){}

    ~SendStrip(){};
  
    virtual bool refresh(bool force=false){
      refreshed = force;
      ThreeSelectStrip::refresh(force);
      return refreshed;
    }

    virtual void buildWidgets(uint8_t layout){
      this->layout = layout;
      switch(layout){

        case LAYOUT_QUATUOR:

        case LAYOUT_ICON:

        case LAYOUT_DETAILS: 
        default:
       
          topLabels = new Top2Toggles(&gui->screenGUIs,&sendName, &returnName, &sendMute, &returnMute,&frontColor,&backColor );          
          gui->add(topLabels);

          // add the SliderWidget here based on value
          // value float

        break;
      }      
    }    

  protected:
    bool sendMute = false, returnMute = false;
    String sendName = "sendName";
    String returnName ="returnName";
    float value;
    Top2Toggles* topLabels;
};

class Ch2BusStrip : public SendStrip{
  #define STRIP_CH2BUS "ch2bus"

  public:

    Ch2BusStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):SendStrip(s,g,ms,debug){
      select2PropertyAddress = s->dataSetAddresses.get(0);
      select3PropertyAddress = s->dataSetAddresses.get(1);
    }

    virtual bool actionCallback(){
      //mute/unmute
      DEBUG_PRINT("\nCh2BusStrip actionCallback");
      sendMute = !sendMute;
      return true;
    }


    virtual bool refresh(bool force=false){
      refreshed = force;
      SendStrip::refresh(force);
      // this code should only be called if select2ID != oldSelect2ID
      sendName = skeleton->getValue(select2PropertyAddress,select2ID-1);
      returnName = skeleton->getValue(select3PropertyAddress,select3ID-1);
      // put then the code specific here to treatment, for the value treatment like OSC frame to read the value
      return refreshed;
    }
};



#define CHGAINDB_MIN  -12
#define CHGAINDB_MAX  60
class ChGainStrip : public TwoSelectStrip, public StripMixer{
  #define STRIP_CHGAIN "chGain"

  public:
    ChGainStrip(StripSkeleton* s,GUIManager* g,MixerServer* ms,bool debug=false):TwoSelectStrip(s,g, &chName,debug),StripMixer(ms){
      select2PropertyAddress = ADDRESS_CHNAME;
      valueMin = CHGAINDB_MIN;
      valueMax = CHGAINDB_MAX;
      value = (valueMin + valueMax)/2;
      //icon = ICON_CHGAIN_STRIP;
    }

    ~ChGainStrip(){};

    virtual bool actionCallback(){
      char addr[100];

      uint8_t chID = 2;

      //sprintf(addr,mixerServer->chNames.address.c_str(),chID);
      String val = "name"+String(random(1000));
      DEBUG_PRINT("\n %s %s",addr,val.c_str());
    
      //chNames.values.set(chID-1, val);
      //mixerServer->chNames.setValue(chID-1,val);
      app.broadCast("{\"updateDataSets\":[{\""+String(addr)+"\":\""+val+"\"}]}");
      refresh();

      return false;
    }
       
    virtual bool refresh(bool force=false){
      refreshed = force;
      TwoSelectStrip::refresh(force);
      chName = skeleton->getValue(ADDRESS_CHNAME,select2ID-1);
      return refreshed;
    };    

    virtual void buildWidgets(uint8_t layout){
      switch(layout){
        case LAYOUT_QUATUOR:
        break;
        case LAYOUT_ICON:
        break;
        case LAYOUT_DETAILS: 
        default:
       
          Top1Toggle1Label* topLabels = new Top1Toggle1Label(&gui->screenGUIs,&chName, &gainString, &chMute,&frontColor,&backColor );          
          gui->add(topLabels);

          RingMeter* ringMeter1 = new RingMeter(&value, -1000,1000, 1, &app.quarter1xPos,&y, 32, &dBstring,&frontColor,&GUI::transparentColor,false);
          gui->add(ringMeter1);          

          RingMeter* ringMeter2 = new RingMeter(&value2, -1000,1000, 0, &app.quarter2xPos,&y, 32, &testUnitString,&GUI::blue,&GUI::transparentColor,false);
          gui->add(ringMeter2);          

          //RingMeter* ringMeter3 = new RingMeter(&value, valueMin,valueMax, 1, &app.third3xPos,&y, 64, &testUnitString,GUI::glowingGoldColor.value,&GUI::transparentColor,false);
          RingMeter* ringMeter3 = new RingMeter(&value, valueMin,valueMax, 1, &app.third3xPos,&y, 64, &testUnitString,&GUI::red,&GUI::transparentColor,false);
          gui->add(ringMeter3);               


        break;
      }      
    }    

    /*virtual float getIncrement(unsigned long time){                  
      if (!Strip::progressiveDelay(time)) return true;
      float increment = 1;      
      if (time>2000) increment = (valueMax-valueMin) * 0.005;
      DEBUG_PRINT("\t INC: %.3f", increment);      
      return increment;
    }*/

  private:
    float value2 = 0;
    String chName = "chName";
    String gainString = "Gain";
    String dBstring = "Hz";
    String testUnitString = "gate";
    bool chMute = false;
    int16_t y = TFT_HEIGHT-WIDGET_BIG_WIDTH-OVERLAY_ICON_SIZE*2;
};


MixerServer mixerServer(&app.dataRepository,hwm.network);

StripSkeleton* stripSendSkel;
Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tch2busStripConstructorCallback()");
  return new Ch2BusStrip(stripSendSkel,&gui,&mixerServer,true);
}

StripSkeleton* stripChGainSkel;
Strip* gain2chStripConstructorCallback(){
  //DEBUG_PRINT("\n\tgain2chStripConstructorCallback()");
  return new ChGainStrip(stripChGainSkel,&gui,&mixerServer,true);
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
bool userHTMLJsonInput(JsonDocument jsonDoc,JsonAnswer* jsonAnswer){
  bool success = false;
  if (jsonDoc["command"] == MIXER_STATE_KEY ){
    jsonAnswer->value = getMixersState();
    jsonAnswer->callback = MIXER_STATE_KEY;
    success = true;
    // ok remote
  }else if (jsonDoc["command"] == MIXER_SET_KEY ){
    //{"command":"setMixer","param":{"mixerIP":"82.65.4.93","mixerPort":"10023"}}
    mixerServer.serverIP.fromString(jsonDoc["param"][SERVER_IP_KEY].as<String>());
    mixerServer.serverPort = jsonDoc["param"][SERVER_PORT_KEY];

    app.config.setValue(SERVER_IP_KEY,mixerServer.serverIP.toString());    
    app.config.setValue(SERVER_PORT_KEY,String(mixerServer.serverPort));
    
    jsonAnswer->callback = RET_SAVEBUTTON;
    jsonAnswer->value = "1";       
    success = true;
    mixerServer.disconnect(); // this will force trying to reconnect with new parameters
    // force GUI rebuild
  }
  return success;
}

String userPongCallback(){
  String pong = "";
  JsonDocument doc;
  if (mixerServer.mixerInterface->state == DISCONNECTED){
    //{"serverAttempt":"Trying 192.168.0.212 / 10026"}}>
    String serverAttempt = *app.lang.getString("Trying")+" "+mixerServer.serverIP.toString()+"/"+mixerServer.serverPort+" "+mixerServer.mixingStationIP.toString()+"/"+mixerServer.mixingStationPort;
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
    mixerServer.SendCmd2OSC("/xinfo",demoServer, 10023);
    delay(hwm.network->WANlatency);      
    if (mixerServer.OSCReadIn()) {
      demoIP = "82.65.4.93";
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
  //DEBUG_PRINT("\nusertNetworkChangeStateCallback(%i)",state);
  QueryingDemoMixer(hwm.network->internet);  
}

String waitingMixer = "Waiting mixerServer connection";

void userWizardGUI(void){
    //DEBUG_PRINT("\t User wizard");
    GUI::cls();
    gui.add(new Label(&waitingMixer, Align::Center,&app.midScreenHoriz, &app.midScreenVertical,MENU_TITLE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));   
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

String getJsonUserConfWrapper(){
  // add any other json object here before converting them to String  
  // so far only mixer state
  return getMixersState();
}

void userBootScreen(){
  Icon bootSprite = Icon(&app.footSwitchIcon,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE,app.byeIconColor,&GUI::defaultBcolor,false);
  bootSprite.refresh(true);
  bootSprite.sprite->pushSprite(bootSprite.getX(),bootSprite.getY(),TFT_TRANSPARENT);      
}


void setup() {

  stripSendSkel = new StripSkeleton(STRIP_CH2BUS,STRIP_XAIR_GROUP,sendStripConstructorCallback,'!');  
  stripSendSkel->registerDataSetAddress(ADDRESS_CHNAME);
  stripSendSkel->registerDataSetAddress(ADDRESS_BUSNAME);
  
  stripChGainSkel = new StripSkeleton(STRIP_CHGAIN,STRIP_XAIR_GROUP,gain2chStripConstructorCallback,'y'); 
  stripChGainSkel->registerDataSetAddress(ADDRESS_CHNAME);

  app.stripFactory.add(stripTextSkel);
  app.stripFactory.add(stripSendSkel);
  app.stripFactory.add(stripChGainSkel);

  app.userParseJsonInputCallback = userHTMLJsonInput;
  app.userJsonConfigCallback = getJsonUserConfWrapper;
  app.userNetworkChangeStateCallback = usertNetChangeStateCallback;
  app.userJsonPongCallback = userPongCallback;    
  app.userWizardScreenBuilder = userWizardGUI;
  app.userWizardstepFlow = userWizardStepFlow;
  app.setUserInternetCallback((StdFunctionCallbackVoidBool)userInternetCallBack);
  //app.bootScreen = userBootScreen;


  app.init();  
  mixerServer.init(&app.config);

  /*String data;
  app.serialize(&data);
  DEBUG_PRINT("\nAPPLICATION SERIALIZATION:<%s>",data.c_str());*/

  #if defined(DEBUG) && defined(USE_NETWORK)
    ftp.addUser("ftp", "ftp");  
    ftp.addFilesystem("LITTLEFS", &LittleFS);
    ftp.begin();
  #endif
  

  DEBUG_PRINT("\n\n\n=====================================================================");
  DEBUG_PRINT("\n============================ SETUP END ==============================");
  DEBUG_PRINT("\n=====================================================================\n\n");  
}

void loop() {

  app.refresh();
  if (mixerServer.refresh()){
    screen.focus = screen.focus - 1;    // rebuild GUI
    app.broadCast(getMixersState());
  }

  #if defined(DEBUG) && defined(USE_NETWORK)
    ftp.handle();
  #endif
}