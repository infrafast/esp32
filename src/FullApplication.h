// the stripHeader class which is added in interval buildModuleScreen crashes probably because of lack of proper initialisation of its internal strip*
// I deactivated all creation of pending bar (ex. in overlay and in interval build module screen) as it seems to generate crash null pointer
// I deactivated refresh of USB and mixerServer in loop to stablize multitask and debug semphore 
// be careful with the disconnect fonction of MixingStation which (line 1282) is supposed to wait the http client to finish its task before closing connection and its destructor being called

// debugging https://docs.platformio.org/en/stable/tutorials/espressif32/arduino_debugging_unit_testing.html

// make sure to change config.hpp of mWebsocket 
//constexpr uint16_t kBufferMaxSize{512};

#define USE_NETWORK
//#define DEBUG_OSC
//#define USE_FTP

//#define TFT_BGCOLOR   TFT_DARKGREY

#include "Interval.h"
#include "mixerServer.h"
#include <StringConcat.h>

#if defined(USE_FTP) && defined(USE_NETWORK)
  #include "ESP-FTP-Server-Lib.h"
  #include "FTPFilesystem.h"
  FTPServer ftp;
#endif

//////////////////////////////////////////////////////////////////
/////////////////////                         ////////////////////
/////////////////////    STRIP DEFINITIONS    ////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
#define ICON_SENDRETURN_STRIP "="
#define ICON_CHGAIN_STRIP     "5"


#define STRIP_XAIR_GROUP    "xair"
#define STRIP_UTILS_GROUP   "utils"

#define ADDRESS_MACRONAME   "/macro/name/%s"
#define ADDRESS_ACTION      "/action/%s"
#define ADDRESS_IP          "/ip/%s"
#define ADDRESS_PORT        "/port/%s"
#define ADDRESS_MESSAGE     "/message/%s"

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    GLOBAL VARIABLES     /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////
String  previousInterfaceSubState = "";

String demoIP = "";
uint16_t demoPort=10023;

String iconMixer = "!";  
String iconMS = ">"; //ICON_MIXING_STATION;
String iconDisconnected = ICON_FOOTSWITCH;
uint16_t iconMixerColor = GUI::red;
AlteredColor mixerGlowingColor = AlteredColor{.baseColor = GUI::red, .value =  &iconMixerColor};

MixerServer mixerServer;

StripSkeleton* stripSendSkel;
StripSkeleton* stripOSCSkel;

ByteBuffer userFontBuffer;

OpenFontRender myOfr = OpenFontRender();

//float tV = 95.0;
int16_t mixerY = TFT_HEIGHT - 120;
float dataProgress = 0.0;
#define PROGRESS_BAR_HEIGHT 20
int16_t progressBarY = PROGRESS_BAR_HEIGHT; //app.overlayIconY - PROGRESS_BAR_HEIGHT;

int8_t previousInterfaceState = DISCONNECTED;

String dbUnit = "dB";

///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////    CLASS DEFINITIONS    /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////

class MyIcon : public Font{
  public:
    static OpenFontRender* ofr; 
    
    MyIcon(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness, bool d=false):Font(v, a, posX, posY, s, fC , bC , _brightness, d){
      id = concatStrings("Icon", *posX, "x", *posY, "~", *v);
      //if (debug) DEBUG_PRINT("\n new %s",id);
    };
    
  protected:
    virtual OpenFontRender* getOfr(){return ofr;}
};

OpenFontRender* MyIcon::ofr = &myOfr;

class StripMixer{
  public:

    MixerServer* mixerServer = nullptr;

    StripMixer(MixerServer* ms):mixerServer(ms){
    }

    bool retrieveInterface(StripState* state){

      bool changedState = false;

      MixerInterface* mixerInterface = mixerServer->mixerInterface;
      uint8_t interfaceState = mixerInterface->interfaceState;


      if (mixerServer->state == CONNECTED){
        if (interfaceState == CONNECTED){
          state->progress = 100.0;
          state->code = STRIP_ONLINE;          
        }else{
          state->code = STRIP_CONNECTING;
          state->progress = (float)mixerInterface->progress()*1.0;
          // here you can specialize the message based on interface type , example: could be the interface name, etc...          
          if (interfaceState == SYNCING_SOFTWARE){
            state->subStateLabel = &mixerInterface->model;             
          }else if (interfaceState == SYNCING_DATA){
            state->subStateLabel = &mixerServer->name;
          }else{
            state->code = STRIP_UNDEF_STATE;
          }
        }
      }else if (mixerServer->offline == true){
        state->progress = 100.0;
        state->code = STRIP_ONLINE;
      }else{  //mixerServer DISCONNECTED
        state->progress = 0.0;
        if (hwm.network->state == CONNECTED){
          state->code = STRIP_PENDING;
          // here you can specialize the message based on interface type or icon to indicate which mixer we are querying
          state->subStateLabel = app.userLang.getString("Pending mixer connection...");
        }else{
          state->code = STRIP_DISCONNECTED;
        }        
      }

      state->stateLabel = app.userLang.getString(stateNames[interfaceState]);

      if (mixerServer->refreshed){
        changedState = true;
        JsonDocument json;
        //{{mixerState:{"interfaceState":{"mixerName":"XR16-Interval","mixerPort":3000,"mixerConnected":1,"mixerModel":"XR16"},"serverState":{"interface":"MixingStation","serverIP":"82.65.4.93","serverPort":8080}},"mixerDemo":{"ip":"82.85.52.72","port":10023}}
        json[MIXER_STATE_KEY] = mixerServer->getMixerState();        
        String jsonString;
        serializeJson(json, jsonString);
        app.broadCast(jsonString);                 
      }

      if (hwm.network->refreshed) changedState = true;

      return changedState;

    }


    bool match2AddressIDtoDropDownID(const char* address,const char* pattern){
      // this method is called when we have to decompose the ids embedded into an address according to a provided pattern
      // upon reception of an OSC frame
      if (sscanf(address, pattern,&id1,&id2) != 2) return false;
      if (mixerServer->mixerInterface->idStart == 0){ id1 += 1; id2 += 1;}    // we have to make ID consistent, they should all start at 1
      return true;
    }

    bool match1AddressIDtoDropDownID(const char* address,const char* pattern){
      if (sscanf(address, pattern,&id1) != 1) return false;
      if (mixerServer->mixerInterface->idStart == 0){ id1 += 1;}    // we have to make ID consistent, they should all start at 1
      return true;
    }

    uint8_t dropDownIDtoInterfaceID(uint8_t id){
      // this return the value starting either at 0 or one depending on the mixerInterface (mixing station starts at 0 so drop downvalue would need to be decreased by 1)
      return id-(1-mixerServer->mixerInterface->idStart);
    }

  protected:
    int id1=0, id2=0; // these are the values extracted from the address
    // example
    // /fxrtn/03/mix/06/on
    // v1 = 03
    // v2 = 06
};

class OSCTestStrip : public TextStrip, public StripMixer{
  #define STRIP_OSCTEST "oscTest"
  public:

    OSCTestStrip(StripSkeleton* s,GUIManager* g,MixerServer* _mi,bool debug=false):TextStrip(s,g, debug),StripMixer(_mi){
      Address* portAddress = s->getDataSetAddress(1);
      Address* msgAddress = s->getDataSetAddress(2);
      if (portAddress != nullptr){
        portPropertyAddress = portAddress->pattern;
      }
      if (msgAddress != nullptr){
        msgPropertyAddress = msgAddress->pattern;
      }
      message = *app.lang.getString("undefined");
      port = 0;
    }

    virtual bool update(JsonObject& data){      
      return retrieveInterface(&state);
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
      JsonDocument doc;

      JsonObject command = doc.to<JsonObject>();

      JsonObject osc = command[EXEC_TESTOSC].to<JsonObject>();
      osc[EXEC_FRAME] = message;
      osc[EXEC_IP] = text;
      osc[EXEC_PORT] = port;

      return mixerServer->mixerInterface->execute(command);  
    }

    
    virtual void composeWidgets(){
      //any surcharge here
      TextStrip::composeWidgets();
    }

    virtual const String* getTopLabel(){return &message;}
    virtual const String* getBottomLabel(){return &portString;}      

  private:
    bool on = false;

    String portPropertyAddress;
    String msgPropertyAddress;
   
    int port=0; String portString="port";
    IPAddress ip;
    String message="message";
};

class SendStrip : public ThreeSelectStrip, public StripMixer{
  public:

    SendStrip(StripSkeleton* s,GUIManager* g,MixerServer* _mi,bool debug=false):ThreeSelectStrip(s,g, debug),StripMixer(_mi){}

    ~SendStrip(){};
  
    virtual void composeWidgets() override{      

      uint16_t h = 0, w = 0;

      if (size == SHAPE4X4){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W;
        renderY += h/2; 
        gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE1X4){
        w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET1X4H;
        renderX += w/2;           
        gui->add(new ProgressBarV(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE2X2){
        h = WIDGET2X2H/2;   // height of the bar is 2 time less big than the full height of the cell          
        uint8_t widthOffset = WIDGET2X2H/10;
        w = WIDGET2X2W-widthOffset;
        gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));

      }
      else if (size == SHAPE4X1){
        h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
        w = WIDGET4X1W;
        gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }

      labelX = renderX+w/2;
      labelY = renderY+h;

      //gui->add(new NumericLabel(&value,1, Align::Center,&labelX,&labelY,h*1.5,&GUI::white,&GUI::defaultBcolor, &brightness, "dB"));      

      //DEBUG_PRINT("\nsendStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

  protected:
    String unitString="U";
    int16_t labelX = 0;
    int16_t labelY = 0;

};

class Ch2BusStrip : public SendStrip{
  #define STRIP_CH2BUS "ch2bus"

  public:

    Ch2BusStrip(StripSkeleton* s,GUIManager* g,MixerServer* mi,bool debug=false):SendStrip(s,g,mi,debug){      
      unitString = "dB";
      //valueMin = 0; valueMax = 1;
    }

    virtual bool actionCallback(uint8_t trigger){
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      DEBUG_PRINT("\nCh2BusStrip actionCallback");
      toggle1 = !toggle1; 
      
      //const MIDIAddress noteAddress {MIDI_Notes::C[4], Channel_1};
      // Send Note On for current position at full velocity (127) on channel 1.
      //MIDI_NOTE_ON(noteAddress, 127);

      //Ch Faders	1	CC	0-15	0-127	Input Channels 1-16
      // Ch Mutes	2	CC	0-15	0 127	Input Channels 1-16
      //void controlChange(uint8_t controller, uint8_t value, uint8_t channel = (uint8_t)0U)
      MIDI_USB_CC(1,1,60);
      MIDI_USB_NOTE_ON(90, 127);      
      MIDI_RTP_ETH.sendNoteOn(40, 55, 1);
      MIDI_RTP_ETH.sendControlChange(1,60,1);

      //hwm.network->RTPmidi.sendNoteOn(60, 100, 1);  // C4
      delay(200);
      // Send Note Off for previous note.
      MIDI_USB_NOTE_OFF(90);
      MIDI_RTP_ETH.sendNoteOff(40, 55, 1);
 
      //hwm.network->RTPmidi.sendNoteOn(60, 0, 1);  // C4

      char formattedCommandString[50];
      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHBUSMUTE];
      uint8_t chID = dropDownIDtoInterfaceID(select2ID);
      uint8_t busID = dropDownIDtoInterfaceID(select3ID);  
      sprintf(formattedCommandString,addressTemplate.c_str(),chID,busID);

      JsonDocument command;
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      osc[EXEC_ADDRESS] = formattedCommandString ;
      osc[EXEC_VALUE] = toggle1?1:0;

      return mixerServer->mixerInterface->execute(command.as<JsonObject>());        

    }

    virtual void queryState(){
      DEBUG_PRINT("\nupdate %s",type.c_str());      

      char formattedCommandString[50];
      uint8_t chID = dropDownIDtoInterfaceID(select2ID);
      uint8_t busID = dropDownIDtoInterfaceID(select3ID);  
      // we query levels and mute state to update the strip

      JsonDocument command;      
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      JsonDocument doc;

      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHBUSMUTE];
      sprintf(formattedCommandString, addressTemplate.c_str(), chID, busID);
      osc[EXEC_ADDRESS] = formattedCommandString ;
      mixerServer->mixerInterface->execute(command.as<JsonObject>());    

      addressTemplate = mixerServer->mixerInterface->addresses[ADDR_BUSMUTE];
      sprintf(formattedCommandString, addressTemplate.c_str(), busID);
      osc[EXEC_ADDRESS] = formattedCommandString ;
      mixerServer->mixerInterface->execute(command.as<JsonObject>());

      addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHBUSVALUE];
      sprintf(formattedCommandString, addressTemplate.c_str(), chID, busID);
      osc[EXEC_ADDRESS] = formattedCommandString ;      
      mixerServer->mixerInterface->execute(command.as<JsonObject>());

    }

    virtual bool update(JsonObject& data) {     
      
      String output;
      serializeJson(data, output);
      
      // here we would check if the object message is of interest to us
      if (data.containsKey(INTERFACE_NAME)) DEBUG_PRINT("\n%s::update(<%s>)",type.c_str(),output.c_str());      
      
      bool requestRedraw = retrieveInterface(&state);

      // this method implements the behaviour of the strip when receiving 
      // he will process certain frame and ignore other     

      OSCMixerInterface* interface = (OSCMixerInterface*)mixerServer->mixerInterface;
      
      if (OSCInterface::matchPattern(interface->addresses[ADDR_BUSMUTE],data[EXEC_ADDRESS])){
        if(match1AddressIDtoDropDownID(data[EXEC_ADDRESS],interface->addresses[ADDR_BUSMUTE].c_str())){ 
          if (id1 == select3ID){  // retrieved ID match the select3ID which is the bus
            toggle2 = data[EXEC_VALUE][0];
          }
        }
      }else if (OSCInterface::matchPattern(interface->addresses[ADDR_CHBUSMUTE],data[EXEC_ADDRESS])){
        if(match2AddressIDtoDropDownID(data[EXEC_ADDRESS],interface->addresses[ADDR_CHBUSMUTE].c_str())){ 
          if (id1 == select2ID && id2 == select3ID){  
            toggle1 = data[EXEC_VALUE][0];
          }
        }
      }else if (OSCInterface::matchPattern(interface->addresses[ADDR_CHBUSVALUE],data[EXEC_ADDRESS])){        
        if(match2AddressIDtoDropDownID(data[EXEC_ADDRESS],interface->addresses[ADDR_CHBUSVALUE].c_str())){         
          //DEBUG_PRINT("\n id1:%d(%d) id2:%d(%d)",id1,select2ID,id2,select3ID);
          if (id1 == select2ID && id2 == select3ID){  
            value = data[EXEC_VALUE][0];
            //DEBUG_PRINT("\t value: %.2f",value);
          }
        }        
      }/*else if (OSCInterface::matchPattern(interface->addresses[ADDR_BUSMUTE],oscValues->addressStr)){

      }*/

      return requestRedraw;

    }

    virtual float increaseCallback(unsigned long time,uint8_t trigger){
      float delta = Strip::increaseCallback(time,trigger);
      if (delta != 0.0) setChBusLevel();
      return delta;
    }    

    virtual float decreaseCallback(unsigned long time,uint8_t trigger){
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setChBusLevel();
      return delta;
    }      

    void setChBusLevel(){

      char formattedCommandString[50];
      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHBUSVALUE];
      uint8_t chID = dropDownIDtoInterfaceID(select2ID);
      uint8_t busID = dropDownIDtoInterfaceID(select3ID);  
      sprintf(formattedCommandString,addressTemplate.c_str(),chID,busID);

      JsonDocument command;
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      osc[EXEC_ADDRESS] = formattedCommandString ;
      osc[EXEC_VALUE] = value;

      mixerServer->mixerInterface->execute(command.as<JsonObject>());    
    }    
};

#define CHGAINDB_MIN  -12
#define CHGAINDB_MAX  60
class ChGainStrip : public TwoSelectStrip, public StripMixer{
  #define STRIP_CHGAIN "chGain"

  public:
    ChGainStrip(StripSkeleton* s,GUIManager* g,MixerServer* mi,bool debug=false):TwoSelectStrip(s,g,debug),StripMixer(mi){
      valueMin = CHGAINDB_MIN;
      valueMax = CHGAINDB_MAX;
      value = (valueMin + valueMax)/2;
      label = "gain";
      //icon = ICON_CHGAIN_STRIP;
    }

    ~ChGainStrip(){};

    virtual bool update(JsonObject& data) {     
      // IMPLEMENTS CODE TO PROCESS DATA HERE
      return retrieveInterface(&state);
    }

    virtual void composeWidgets() override{      

      uint16_t h = 0, w = 0;

      if (size == SHAPE4X4){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W;
        renderY += h/2; 
        gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE1X4){
        w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET1X4H;
        renderX += w/2;           
        gui->add(new ProgressBarV(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }
      else if (size == SHAPE2X2){
        w = WIDGET2X2W;   // width of the bar is 2 time less big than the full width of the cell
        h = WIDGET2X2H;
        renderX += w/2;   
        renderY += h/2;      
        RingMeter* ringMeter = new RingMeter(&value, valueMin, valueMax, 1, &renderX,&renderY, h/2, &dbUnit,&GUI::blue,&GUI::transparentColor,&brightness);
        gui->add(ringMeter);
      }
      else if (size == SHAPE4X1){
        h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
        w = WIDGET4X1W;
        gui->add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &renderX /*x*/, &renderY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }     

      //DEBUG_PRINT("\nchGainStrip renderX: %d, renderY: %d, w: %d, h: %d",renderX,renderY,w,h);

      //DEBUG_PRINT("\t\tlabelX: %d, labelY: %d",labelX,labelY);      
    }

    virtual void queryState(){
      DEBUG_PRINT("\nupdate %s",type.c_str());
    }

    virtual float increaseCallback(unsigned long time,uint8_t trigger){
      float delta = Strip::increaseCallback(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger){
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    void setChGain(){
      char formattedCommandString[50];
      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHGAIN];
      uint8_t chID = dropDownIDtoInterfaceID(select2ID);
      sprintf(formattedCommandString,addressTemplate.c_str(),chID);

      JsonDocument command;
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      osc[EXEC_ADDRESS] = formattedCommandString ;
      osc[EXEC_VALUE] = value;

      mixerServer->mixerInterface->execute(command.as<JsonObject>());  
    }

    virtual bool actionCallback(uint8_t trigger){
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      /*char addr[100];

      //select2ID = chID

      //sprintf(addr,mixerServer->chNames.address.c_str(),chID);
      String val = concatStrings("name", random(1000));
      DEBUG_PRINT("\n %s %s",addr,val.c_str());
    
      //chNames.values.set(chID-1, val);
      //mixerServer->chNames.setValue(chID-1,val);
      //app.broadCast(concatStrings("{\"updateDataSets\":[{\"", String(addr), "\":\"", val, "\"}]}"));

      refresh();*/
      return true;
    }
       
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


//////////////////////////////////////////////////////////////////
/////////////////////                         ////////////////////
/////////////////////    DATA DEFINITIONS    /////////////////////
/////////////////////                         ////////////////////
//////////////////////////////////////////////////////////////////


DynamicDataSet macroNames = DynamicDataSet(ADDRESS_MACRONAME,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented
DynamicDataSet actionPlay = DynamicDataSet(ADDRESS_ACTION,INPUTSELECT,&localProvider); // temp while shortcut factory is not implemented

DynamicDataSet ipDS = DynamicDataSet(ADDRESS_IP,INPUTTEXT,&localProvider); 
DynamicDataSet portDS = DynamicDataSet(ADDRESS_PORT,INPUTTEXT,&localProvider); 
DynamicDataSet msgDS = DynamicDataSet(ADDRESS_MESSAGE,INPUTTEXT,&localProvider); 

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                               ////////////////////
/////////////////////    STRIP CONSTRUCTOR CALLBACKS DEFINITIONS    ////////////////////
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

//////////////////////////////////////////////////////////////////
/////////////////////                         ////////////////////
/////////////////////    UTILITIES            ////////////////////
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
    OSCInterface osc("pingDemoMixer",NO_CAPTURE);
    osc.setClient(demoServer,10023);

    JsonDocument doc;
    JsonObject input = doc.to<JsonObject>();

    if(osc.blockingPing(demoServer,10023,BEHRINGER_QUERY,input, hwm.network->WANlatency)){
        demoIP = "82.65.4.93";                // should be retrieved by an http request
        DEBUG_PRINT("\tDemo Mixer responded");    
    }else{
      DEBUG_PRINT("\tDemo Mixer didn't respond");    
    }
    doc.shrinkToFit();
  }
  String jsonString;
  JsonDocument json;
  json[MIXER_DEMO_KEY] = mixerDemoJsonString();
  serializeJson(json,jsonString);
  removeJsonQuote(&jsonString);
  app.broadCast(jsonString); // refresh application demoMixer dropdown  
  json.shrinkToFit();

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
#define RET_RELOAD_BANKS "reloadBanks"
bool userHTMLJsonInput(JsonObject& jsonDoc,JsonAnswer* jsonAnswer){
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

    app.userConfig.setValue(SERVER_IP_KEY,mixerServer.serverIP.toString());    
    app.userConfig.setValue(SERVER_PORT_KEY,String(mixerServer.serverUDPPort));
    app.userConfig.setValue(SERVER_MODEL_KEY,jsonDoc["param"][SERVER_MODEL_KEY]);

    app.userConfig.setValue(MIXER_PORT_KEY,String(mixerServer.msAPIPort));     
    app.userConfig.setValue(MIXER_AUTOMODEL_KEY,jsonDoc["param"][MIXER_AUTOMODEL_KEY]);
    app.userConfig.setValue(MIXER_AUTOIP_KEY,jsonDoc["param"][MIXER_AUTOIP_KEY]);


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
    String serverAttempt = concatStrings(
        *app.userLang.getString("Trying"),
        " ",
        mixerServer.serverIP.toString(),
        " ",
        mixerServer.behringerUDPPort,
        "/",
        mixerServer.msUDPPort,
        "/",
        mixerServer.msAPIPort);
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
  // QueryingDemoMixer(internet);    // <--------------------------- CRASH WHEN NO MIXER IS CONNECTED
  
}

/////////////////////    EVENT HANDLING       /////////////////////
void networkEvent(WiFiEvent_t event){
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_READY");
    break;
    case ARDUINO_EVENT_WIFI_STA_START:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_START");
    break;    
    case ARDUINO_EVENT_WIFI_STA_STOP:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_CONNECTED");
    break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_GOT_IP");
    break;
    case ARDUINO_EVENT_WIFI_AP_STOP: //SYSTEM_EVENT_STA_STOP:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: //SYSTEM_EVENT_STA_DISCONNECTED:
      DEBUG_PRINT("~");
    break;
    case ARDUINO_EVENT_WIFI_AP_START: //SYSTEM_EVENT_AP_START:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_START");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: //SYSTEM_EVENT_AP_STACONNECTED:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STACONNECTED");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: //SYSTEM_EVENT_AP_STADISCONNECTED:
      DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STADISCONNECTED");
      //hwm.ap->state = DISCONNECTED;
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED: //SYSTEM_EVENT_AP_STAIPASSIGNED:
      //hwm.ap->state = CONNECTED;
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED");
    break;
    case ARDUINO_EVENT_ETH_START:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_START");
    break;
    case ARDUINO_EVENT_ETH_STOP:
      //DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_ETH_STOP");
    break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      //DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_WIFI_STA_LOST_IP");
    break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      //DEBUG_PRINT("\nEVENT-WIFI: ARDUINO_EVENT_ETH_CONNECTED");
    break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_DISCONNECTED");
    break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: ARDUINO_EVENT_ETH_GOT_IP");
    break;
    default: 
      //DEBUG_PRINT("\n\t\t\tEVENT-WIFI: %i",event);
    break;
  }
}

bool eventHandling(EventDispatcher::Event* event){  
  switch(event->type){
    case EventDispatcher::EVENT_DIGITAL_BUTTON:{
      ButtonEvent* btnEvt = (ButtonEvent*)event->data;
      switch(btnEvt->id){
        case Interval::Button::BUTTON_SIMPLE_CLIC:
          //DEBUG_PRINT("\n\t\t *** EVENT_DIGITAL_BUTTON SIMPLE_CLIC");
          if (mixerServer.state == DISCONNECTED && ((screen.id == SCREEN_WIZARD && screen.state == STATE_WIZARD_USER_STEP4 ) || screen.id == SCREEN_BANK )){
            //go offline
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
    /*case EventDispatcher::EVENT_PARSE_JSONINPUT:{
      JsonEvent* evt = (JsonEvent*)event->data;

      JsonDocument jsonDoc = ;
      JsonAnswer* jsonAnswer = ;
      userHTMLJsonInput();
    }break;*/
  } 
  return true;
}

/////////////////////    GUI       /////////////////////
void userWizardStepFlow(){
  if (mixerServer.state == CONNECTED || mixerServer.offline == true ){
    app.persistants.putUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
    app.restoreScreen();
  }

}

void userBootScreen(){
  Icon bootSprite = Icon(&app.intervalIcon,Align::Center,&app.midScreenHoriz, &app.iconMidY,ICON_MAX_SIZE*1.2,app.byeIconColor,&GUI::defaultBcolor);
  bootSprite.refresh(true);
  TFT_eSprite* sprite = bootSprite.sprite;
  if (sprite != nullptr) sprite->pushSprite(bootSprite.getX(),bootSprite.getY(),TFT_TRANSPARENT);      
}


void userCLS(){
  GUI::defaultBcolor = TFT_BGCOLOR;
  GUI::defaultFcolor = TFT_FCOLOR;
  GUI::cls();
}

void userScreenBuilder(int screebID, int subScreen){}

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

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
// Log when a note is pressed.
Serial.print("Note on: channel = ");
Serial.print(channel);

Serial.print(" pitch = ");
Serial.print(pitch);

Serial.print(" velocity = ");
Serial.println(velocity);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
// Log when a note is released.
Serial.print("Note off: channel = ");
Serial.print(channel);

Serial.print(" pitch = ");
Serial.print(pitch);

Serial.print(" velocity = ");
Serial.println(velocity);
}


///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////                         /////////////////////
                          void _setup()
/////////////////////                         ////////////////////
/////////////////////                         /////////////////////
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

  /*stripFactory.add(stripTextSkel);
  stripFactory.add(stripSendSkel);
  stripFactory.add(stripChGainSkel);
  stripFactory.add(stripOSCSkel);*/

  /*app.dataRepository.addDataSet(&macroNames);   
  app.dataRepository.addDataSet(&actionPlay);  
  app.dataRepository.addDataSet(&ipDS);  
  app.dataRepository.addDataSet(&portDS);  
  app.dataRepository.addDataSet(&msgDS);  */

  app.registerCallback(eventHandling);

  app.userParseJsonInputCallback = userHTMLJsonInput;  // to be migrated to event
  app.userJsonPongCallback = userPongCallback;    
  app.userWizardstepFlow = userWizardStepFlow;
  app.bootScreen = userBootScreen;
  app.userScreenBuilder = userScreenBuilder;

  app.init(DEFAULT_USER_CONFIG);  

  mixerServer.init();

  gui.alphaColors.add(&mixerGlowingColor);

  /*String data;
  app.serialize(&data);
  DEBUG_PRINT("\nAPPLICATION SERIALIZATION:<%s>",data.c_str());*/

  #if defined(USE_FTP) && defined(USE_NETWORK)
    ftp.addUser("ftp", "ftp");  
    ftp.addFilesystem("LITTLEFS", &LittleFS);
    ftp.begin();
  #endif
  
  size_t dataSize = 0;
  //myOfr.setDebugLevel( OFR_INFO | OFR_ERROR);
  dataSize = readGzFileToBuffer("/webapp/user.ttf.gz",&userFontBuffer);
  if (dataSize > 0){ // uncompress succeed
    if (myOfr.loadFont(userFontBuffer.buffer, dataSize)) { // this consumes 20992 bytes and take ~2ms      
      //DEBUG_PRINT("\nUser compressed font initialise error"); 
    }else{
      //DEBUG_PRINT("\nUser font loaded successfullly"); 
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

  
  //DEBUG_PRINT("\n\n\n=====================================================================");
  DEBUG_PRINT("\n============================ SETUP END ==============================");
  //DEBUG_PRINT("\n=====================================================================\n\n");
}

//////////////////////////////////////////////////////////////////////////////
/*
void processInterfaceSync(){
  if (mixerServer.mixerInterface->interfaceState == SYNCING_SOFTWARE){
    static uint8_t previousProgress = 254;
    app.rollingIcon(true);
    uint8_t progress = mixerServer.mixerInterface->syncSoftwareProgress;
    if (progress<100 && previousProgress!=progress){
      previousProgress = progress;
      
    }
  }else if (mixerServer.mixerInterface->interfaceState == SYNCING_DATA ){
    static uint8_t previousDataProgress = 254;
    app.rollingIcon(true);
    dataProgress = mixerServer.mixerInterface->syncDataProgress(); 
    if (dataProgress<100 && previousDataProgress!=dataProgress){ 
      previousDataProgress = dataProgress;
      app.broadCast(concatStrings("{\"syncProgress\":", dataProgress, "}"));
    }
  }

  if (previousInterfaceSubState != mixerServer.mixerInterface->interfaceSubState){
    previousInterfaceSubState = mixerServer.mixerInterface->interfaceSubState;
    if (mixerServer.mixerInterface->interfaceSubState == "backgroundSync") app.rollingIcon(true);
    else app.rollingIcon(false);
  }  
}*/

void processMixerState(){
    /*
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
    app.triggerRedraw();    // rebuild GUI, will call userBuildScreen 
    */

}



///////////////////////////////////////////////////////////////////
/////////////////////                         /////////////////////
/////////////////////                         /////////////////////
                          void _loop()
/////////////////////                         ////////////////////
/////////////////////                         /////////////////////
//////////////////////////////////////////////////////////////////
{
  app.refresh();
  
  if (screen.id == SCREEN_BANK || screen.id == SCREEN_WIZARD){  
    mixerServer.refresh();           
  }

  if (hwm.network->state == CONNECTED) MIDI_RTP_ETH.read();
  #if defined(USE_FTP) && defined(USE_NETWORK)
    ftp.handle();
  #endif
}