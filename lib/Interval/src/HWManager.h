#include <LinkedList.h>
#include <spi.h>
#include <LittleFS.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "lwip/api.h"

//#include "ping/ping_sock.h"
//#include <ArduinoMDNS.h>

//#define ICMPPING_ASYNCH_ENABLE 1

//#include <EthernetICMP.h>
//#include <ESP32Ping.h>

#include <EthernetUdp.h>
#include "HWComponents.h"

#include <driver/rtc_io.h>

#ifdef DEBUG
  #define MIDI_USB_NOTE_ON(arg1,arg2) DEBUG_PRINT("\nMIDI_USB_NOTE_ON(%d,%d)",arg1,arg2)
  #define MIDI_USB_NOTE_OFF(arg1) DEBUG_PRINT("\nMIDI_USB_NOTE_ON(%d,%d)",arg1)
  #define MIDI_USB_BEGIN DEBUG_PRINT("\nMIDI_USB_BEGIN");
#else
  #include <midiusb.h>
  #include <USB.h>
  MIDIusb USBmidi;

  #define MIDI_USB_NOTE_ON(arg1,arg2) USBmidi.noteON(arg1,arg2)
  #define MIDI_USB_NOTE_OFF(arg1) USBmidi.noteOFF(arg1)

  #define MIDI_USB_BEGIN USBmidi.setBaseEP(2); USBmidi.begin();

#endif



/*#include <Control_Surface.h>


// This class defines how to send and receive MIDI data using your custom
// backend.
struct MyUSBMIDIBackend {
  MIDIusb midi;
  // USB MIDI packages are 4 bytes.
  using MIDIUSBPacket_t = AH::Array<uint8_t, 4>;
  // This method is optional. It is called once upon initialization.
  void begin() { midi.setBaseEP(2); midi.begin(); }
  // Read a single packet. Return a packet of all zeros if there is no packet.
  MIDIUSBPacket_t read() { return MIDIUSBPacket_t {}; }
  // Write a single packet to the output buffer.
  void write(MIDIUSBPacket_t p) { 
    tud_midi_packet_write(p.data); //4bytes
  }
  // Transmit the output buffer immediately.
  void sendNow() { midi.flush(); }
  // Should the sendNow method be called after each message? Return false if
  // your backend can send the data in the output buffer asynchronously and
  // automatically. Return true if that's not the case, and an explicit call to
  // sendNow is required.
  bool preferImmediateSend() { return false; }
};


// This class defines how to send and receive MIDI data using your custom
// backend.
struct MyUSBMIDIBackend2 {
  // USB MIDI packages are 4 bytes.
  using MIDIUSBPacket_t = AH::Array<uint8_t, 4>;
  // This method is optional. It is called once upon initialization.
  void begin() { Serial.begin(115200); }
  // Read a single packet. Return a packet of all zeros if there is no packet.
  MIDIUSBPacket_t read() { return MIDIUSBPacket_t {}; }
  // Write a single packet to the output buffer.
  void write(MIDIUSBPacket_t p) { Serial << AH::HexDump(p.data) << endl; }
  // Transmit the output buffer immediately.
  void sendNow() { Serial.flush(); }
  // Should the sendNow method be called after each message? Return false if
  // your backend can send the data in the output buffer asynchronously and
  // automatically. Return true if that's not the case, and an explicit call to
  // sendNow is required.
  bool preferImmediateSend() { return false; }
};


 
// This is the actual MIDI interface that makes use of the backend defined above.
struct MyUSBMIDI_Interface : GenericUSBMIDI_Interface<MyUSBMIDIBackend> {
  MyUSBMIDI_Interface() = default;
  using MIDIUSBPacket_t = MyUSBMIDIBackend::MIDIUSBPacket_t;
};

MyUSBMIDI_Interface midiZob;
//HairlessMIDI_Interface midiZob;
*/


//////////////////////////////////////////////
#define ANALOGB_PIN       (gpio_num_t)D0 //GPIO_NUM_1  
#define BATLEVEL_PIN      (gpio_num_t)D1 //GPIO_NUM_2
#define ONOFF_PIN         (gpio_num_t)D2 //GPIO_NUM_3 
//                                        D3 =     
//                                        D4 =
#define POWERRAIL_EN_PIN  (gpio_num_t)D4 //GPIO_NUM_5
#define ETH_CS_PIN        (gpio_num_t)D5 //GPIO_NUM_6 
//                                        D6 =
#define RST_PIN           (gpio_num_t)D7 //GPIO_NUM_44

enum{
  LEFTB,
  RIGHTB,
  MIDDLEB,
  MAXB  
};

enum{
  OFF,
  ON
};

#define ON_OFF_TRIGGER      LOW

#include <TFT_eSPI.h>             // Graphics and font library for ST7735 driver chip  lib 2.3.61

volatile unsigned long debounceTimer = 0;
volatile unsigned int delayTime = 200;
volatile unsigned int delayRepeatTime = 500;
unsigned long lastInterrupt;

void OnWiFiEvent(WiFiEvent_t event);

enum{
  SYSTEM_OFF,
  BATTERY_STARTUP,
  NORMAL_STARTUP,
  WIZARD_STARTUP,  
  RESET_STARTUP,  
  NO_FS_STARTUP,  
};

#define SHUTDOWN_PUSH_DELAY 1500    // how long to press the button to switchoff

class HardwareManager : public GenericComponent {
public:

    TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

    uint8_t startupState = SYSTEM_OFF;

    LadderButton *buttons[3];
    //DigitalButton *buttons[3];
    DigitalButton onOff = DigitalButton(ONOFF_PIN,INPUT_PULLUP,ON_OFF_TRIGGER,true,SHUTDOWN_PUSH_DELAY,DEFAULT_DCLIC_DELAY);
    DigitalOut powerRail = DigitalOut(POWERRAIL_EN_PIN,OFF);
    BatteryMonitor battery = BatteryMonitor(BATLEVEL_PIN,false);
    AnalogIn analogButtonIn = AnalogIn(ANALOGB_PIN,20,false); //20% threshold
    AccessPoint* ap;

    Network *network = nullptr;
    Network *configMedia = nullptr;    

    String configIP;

    StdFunctionCallbackVoidBool userInternetCallback;

    unsigned long bootTime = 0;     // how long it took to boot (means when powerRail is on)

    HardwareManager(bool debug=true):GenericComponent(debug){
      id="HardwareManager";
         
      #ifdef DEBUG
  
      #else
        #define SERIAL_SPEED 31250
      #endif           

      #ifdef USE_NETWORK
        network = new WiFiEth(ETH_CS_PIN,RST_PIN,true,true);
        configMedia = network;
      #else
        network = new NoNetwork(true);
        configMedia = network;
      #endif
      hwComponents.add(network);      

      hwComponents.add(&analogButtonIn);    // to be added before creation of the button so init is called first

      buttons[LEFTB] = new LadderButton(&analogButtonIn, HIGH,false);       // virtual ID 20
      buttons[MIDDLEB] = new LadderButton(&analogButtonIn, HIGH,false);     // virtual ID 21
      buttons[RIGHTB] = new LadderButton(&analogButtonIn, HIGH,false);      // virtual ID 22      
      for (uint8_t i=0;i<3;i++) hwComponents.add(buttons[i]);

      //onOff = new DigitalButton(ONOFF_PIN,&onOff_button_pushDelay,&default_button_dClicdelay,INPUT_PULLDOWN,ON_OFF_TRIGGER,false);
      onOff.id="onOff";      
      powerRail.id = "powerRail";
      hwComponents.add(&onOff);

      WiFi.onEvent(OnWiFiEvent);        
      hwComponents.add(&battery);    
    }
    
    /*
    virtual ~HardwareManager(){
        delete button
        delete network
        but who cares?    
    }
    */
    void wakeUp(uint16_t pressTime){
      #define RESET_DELAY 10000
      #define RESET_GRACE_PERIOD RESET_DELAY/3
      pinMode(BUILTIN_LED, OUTPUT);
      ledcSetup(0, 5000, 8);        // chhanel 0, freq 5000, resolution 8 bit
      ledcAttachPin(BUILTIN_LED, 0);   
      ledcWrite(0,255);

      startupState = SYSTEM_OFF;
      onOff.usesLed = false;
      if (onOff.state == ON_OFF_TRIGGER){
        delay(DEBOUCE_TIME);        
        while(onOff.state == ON_OFF_TRIGGER && startupState == SYSTEM_OFF) {          
          bool released = onOff.refresh();
          if (released){
            if (onOff.pushedTime>RESET_GRACE_PERIOD ){ 
              startupState = WIZARD_STARTUP;
              //if (debug) DEBUG_PRINT("\nlong press");
            }else if (onOff.pushedTime>pressTime ){ 
              startupState = NORMAL_STARTUP;
              //if (debug) DEBUG_PRINT("\nnormal press");
            }else{
              startupState = BATTERY_STARTUP;
              //if (debug) DEBUG_PRINT("\nshort press");
            }
          }else{
            if (onOff.pushedTime>RESET_DELAY){
              startupState = RESET_STARTUP;
            }else if (onOff.pushedTime>RESET_GRACE_PERIOD){

              if (toggle == 0) toggle = 255; 
              else toggle = 0;
              ledcWrite(0,toggle);              
              delay(map(RESET_DELAY-onOff.pushedTime,0,RESET_DELAY-RESET_GRACE_PERIOD,0,200));

            }else if (onOff.pushedTime<RESET_GRACE_PERIOD && onOff.pushedTime>pressTime){
              ledcWrite(0,0); //led on
            }
          }
        }
        ledcWrite(0,255);   //led off
        onOff.usesLed = true;
      }else{
        goDeepSleep();
      }
    }
    
    virtual void init() {      
 
      //Serial.setDebugOutput(true);     

      GenericComponent::init();            
      // init device that do not need power but are required for startup
      onOff.init();    
      powerRail.init();        
      // get the power
      wakeUp(500); // you have to press button for more than 500ms to wake up, otherwise we do not start.    
      setPowerRail(ON);        

      #ifdef DEBUG
        //DEBUG is enabled so we don't use MIDI USB, the USB C is used for Serial printf debug
        Serial.begin(115200);
        /*unsigned long start = millis();
        while (!Serial && (millis() - start < 2000)) {
          delay(1); // petite pause pour éviter de monopoliser la CPU
        } */       
      #endif

      MIDI_USB_BEGIN;
      
      
      //Control_Surface.begin();


      //init all other devices      

      for(int i = 0; i < hwComponents.size(); i++) hwComponents.get(i)->init();

      tft.init(1);  //tft.invertDisplay(1);            
      tft.fillScreen(TFT_BLACK);

      // the below code is equivalent to setting RST , which also works for W5500
      /*pinMode(D7, OUTPUT);
      digitalWrite(D7,LOW);
      delay(100);
      digitalWrite(D7,HIGH);
      delay(100);*/
   
      xTaskCreatePinnedToCore(xTaskBackground, "backgroundHWandGUI", 4096, NULL, 3, NULL, 0);
      
      debounceTimer = millis();
      lastInterrupt = millis();

      attachInterrupt(digitalPinToInterrupt(ONOFF_PIN), [] {
        if (millis() - debounceTimer >= delayTime ) {
          debounceTimer = millis();
          if (millis() - lastInterrupt > delayRepeatTime){
            onOffPushedStartTime = millis();
            lastInterrupt = millis();
            //ets_printf("\n\n\t\tPressed");
          }
        }
      }, RISING);             

      //debugTFTSetup();      

      if (!LittleFS.begin()){
        startupState = NO_FS_STARTUP;
      }

      //tusb_init();
    
    }

    void startAP(const String* apName){
      // if we arrive here it means all networks have failed. 
      // hance just stop WiFI to avoid its loop taking time
      network->WiFiEnabled = false ; 
      if (ap == nullptr){
        ap = new AccessPoint(apName, true); 
        hwComponents.add(ap);
        //if (debug) DEBUG_PRINT("\n setting the configMedia to ap");
      }
      ap->init();
      configMedia = ap;
    }

    bool refresh(bool force=false) {  
      refreshed = force;     
      for(int i = 0; i < hwComponents.size(); i++) refreshed |= hwComponents.get(i)->refresh(force);

      //Control_Surface.loop();      
      
      return refreshed;
    }

    TFT_eSPI* getDisplay(){
      return &tft;
    }

    void pinSleep(uint8_t pin, uint8_t mode = OUTPUT){
      pinMode(pin, mode);   // Met la broche en mode entrée pour désactiver tout état précédent
      if (mode == OUTPUT) digitalWrite(pin, LOW);
      if (pin != 44 && pin != 43){
        //rtc_gpio_isolate((gpio_num_t)pin);  // Remplacer par le GPIO à isoler
        //rtc_gpio_hold_en((gpio_num_t)pin);
      } 
      // Lire l'état de la broche après la configuration
      #if DEBUG
        int pinState = digitalRead(pin);
        //DEBUG_PRINT("\n%d state after setting to LOW+Input: %d",pin,pinState);
      #endif
    }


    void setPowerRail(bool state){

      //rtc_gpio_hold_dis((gpio_num_t)TFT_DC);
      //gpio_hold_dis((gpio_num_t)TFT_DC);

/*      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);    
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TOUCHPAD);
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ULP);*/

    /* One thing to keep in mind is that when you disable the power to the sensor by turning off the transistor you would also want to be sure 
    to set the pins used for SCL and SDA as INPUT's (high-z state) so that the sensor doesn't try to draw power through some odd 
    internal wiring that might find either of those lines > 0V.      
    */      
      if (state == OFF){
        startupState = SYSTEM_OFF;          
        tft.fillScreen(TFT_BLACK);
        tft.getSPIinstance().end();        
        pinSleep(TFT_CS);
        pinSleep(TFT_DC,INPUT_PULLDOWN);
        pinSleep(TFT_MOSI);
        pinSleep(TFT_SCLK);
        pinSleep(ETH_CS_PIN);
        pinSleep(RST_PIN,INPUT_PULLDOWN);
       
        // Configure GPIOs to retain their state in deep sleep
        //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

        //for(int i = 0; i < hwComponents.size(); i++) hwComponents.get(i)->stop();

      }  
      powerRail.state = state;
      powerRail.refresh();      
      bootTime = millis();
    }

    void setWiFi(JsonList* wiFiList, String ssid) {      
      if (network != nullptr) network->setWiFi(wiFiList,ssid);
    }

    void setButtonReactivity(uint16_t push, uint16_t dClic){
      for (uint8_t b=0;b<MAXB;b++){
        buttons[b]->dClic_reactivity = dClic;
        buttons[b]->push_reactivity = push;
      }
    }

 
  private:
    #ifdef DEBUG
      setup_t user;
    #endif
    uint8_t toggle = 0;
    LinkedList<HWComponent*> hwComponents = LinkedList<HWComponent*>();     
};


