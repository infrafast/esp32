/**
 * @file HWManager.h
 * @brief Initializes hardware and manages startup, power and display settings.
 */

#ifndef HWMManager_h
#define HWMManager_h

#include <LinkedList.h>
#include <SPI.h>
#include <LittleFS.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "lwip/api.h"

#include <OSCMessage.h>           //https://github.com/CNMAT/OSC  lib 1.3.5

#include <EthernetUdp.h>
#include "HWComponents.h"

#include <Preferences.h>

#include <driver/rtc_io.h>

#include <USB.h>
#ifdef PROD
  // ESP32TinyUSB gates MIDIusb on ESP-IDF configuration macros. Include the
  // selected board sdkconfig before its header so the XIAO ESP32-S3 TinyUSB
  // MIDI class is visible when compiling the production environment.
  #include <sdkconfig.h>
  #include <tusb.h>
  #define INTERVAL_USE_NATIVE_TINYUSB_MIDI 1
  #include <midiusb.h>     // TinyUSB hardware transport (production only)
  #undef INTERVAL_USE_NATIVE_TINYUSB_MIDI
#endif


/////////////////////////////////////////////
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

/** @brief Forward Wi-Fi state changes to the application event system. */
void OnWiFiEvent(WiFiEvent_t event);
/** @brief Render the hardware startup status display. */
void bootScreen(uint8_t);


// Runtime shutdown: holding the power button this long requests normal
// shutdown while the application is already running.
constexpr uint32_t SHUTDOWN_PUSH_DELAY = 1500;
// The duration is measured from the debounced press transition in
// Button::processRefresh(). Keep the normal-start threshold short enough for
// a practical press while leaving the longer CDC/reset windows unchanged.
// A shorter press is interpreted as BATTERY_STARTUP.
constexpr uint32_t NORMAL_STARTUP_PRESS_MS = 500;
// Time spent showing the battery screen before entering deep sleep. This is
// deliberately independent from the battery sampling/filtering cadence.
constexpr uint32_t BATTERY_STARTUP_SCREEN_MS = 5000;
// Time spent showing the reset/restore screen before entering deep sleep.
constexpr uint32_t RESET_STARTUP_SCREEN_MS = 5000;
// Holding beyond this grace window and then releasing requests a reboot into
// the USB CDC/firmware-update startup mode.
constexpr uint32_t RESET_DELAY_MS = 8000;
// Derived threshold separating the CDC request window from the immediate
// reset window. The button must be released before RESET_DELAY_MS to request
// CDC; holding until RESET_DELAY_MS triggers reset immediately.
constexpr uint32_t RESET_GRACE_PERIOD_MS = RESET_DELAY_MS / 3;
// Initial debounce delay used before the startup button sampling loop begins.
// Extra settling time applied only before the startup-button decision loop.
// Generic press/release debounce timings live in HWComponents::Button.
constexpr uint32_t STARTUP_BUTTON_DEBOUNCE_MS = 20;

#define INVERTED_SCREEN_NO 1
#define INVERTED_SCREEN_YES 3

/**
 * @brief Coordinates hardware initialization, power, buttons and network components.
 *
 * Starts the peripheral power rail, initializes components and TFT in the selected
 * order, mounts LittleFS and chooses the startup USB mode. Power-off prepares pins
 * and saved settings before deep sleep. Application screens are managed separately.
 */
class HardwareManager : public GenericComponent {
public:

    Preferences persistants;
    bool swapButton = false;


    TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

    uint8_t startupState = NORMAL_STARTUP;

    LinkedList<HWComponent*> hwComponents = LinkedList<HWComponent*>(); 

    LadderButton *buttons[3];
    //DigitalButton *buttons[3];
    DigitalButton onOff = DigitalButton(ONOFF_PIN,INPUT_PULLUP,ON_OFF_TRIGGER,false,SHUTDOWN_PUSH_DELAY,DEFAULT_DCLIC_DELAY);
    DigitalOut powerRail = DigitalOut(POWERRAIL_EN_PIN,OFF);
    BatteryMonitor battery = BatteryMonitor(BATLEVEL_PIN,false);
    AnalogIn analogButtonIn = AnalogIn(ANALOGB_PIN,20,false); //20% threshold
    AccessPoint* ap;

    Network *network = nullptr;
    Network *configMedia = nullptr;    
  
    #ifdef PROD
      MIDIusb* tinyMIDIusb = nullptr;
    #endif

    String configURL="0.0.0.0";

    unsigned long bootTime = 0;     ///< how long it took to boot (means when powerRail is on)

    /** @brief Initialize HardwareManager with the supplied configuration and bindings. */
    HardwareManager(bool debug=true):GenericComponent(debug){
      id="HardwareManager";
         
      #ifdef DEBUG
  
      #else
        #define SERIAL_SPEED 31250
      #endif           

      #ifdef USE_NETWORK
        #ifdef DEBUG
          network = new WiFiEth(ETH_CS_PIN,RST_PIN,true,true);
        #else
          network = new WiFiEth(ETH_CS_PIN,RST_PIN,true,false);
        #endif
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
    /**
     * @brief Interpret the startup button press and select the hardware mode.
     *
     * The selected value is stored in @c startupState and consumed later by
     * the hardware/application initialization path:
     *
     * - @c SYSTEM_OFF: no startup request; the device returns to deep sleep.
     * - @c BATTERY_STARTUP: show the battery level, then return to sleep.
     * - @c NORMAL_STARTUP: continue with the normal application startup.
     * - @c SYSTEM_RESTART: internal one-shot flag used after a software restart
     *   to bypass the physical startup-button requirement.
     * - @c RESTART_REQUEST_AS_SUB_CDC: internal hand-off flag.  It is written
     *   before rebooting after the CDC-selection hold window.  It means
     *   "the next boot must enter CDC", not "CDC is active now".  On the next
     *   boot it is converted to @c USB_CDC_AND_FWUPDATE_STARTUP and then
     *   cleared.  This two-step state is needed because the requested mode
     *   must survive the ESP restart while the current boot is still ending.
     * - @c USB_CDC_AND_FWUPDATE_STARTUP: operational state for the boot that
     *   actually starts the USB CDC/firmware-update path.  It is the result of
     *   consuming @c RESTART_REQUEST_AS_SUB_CDC, not a second button gesture.
     * - @c RESET_STARTUP: enter the reset/restore startup flow.
     * - @c NO_FS_STARTUP: report that LittleFS could not be mounted.
     *
     * @note In DEV/DEBUG builds this method still detects the selected state,
     * but @c HardwareManager::init() later forces USB CDC for diagnostics.
     * Therefore states such as @c BATTERY_STARTUP and @c RESET_STARTUP may be
     * bypassed after this method returns.  PROD preserves the selected state.
     *
     * @param pressTime Minimum press duration required for normal startup.
     */
    void wakeUp(uint16_t pressTime){
      pinMode(BUILTIN_LED, OUTPUT);
      ledcSetup(0, 5000, 8);        // chhanel 0, freq 5000, resolution 8 bit
      ledcAttachPin(BUILTIN_LED, 0);   
      ledcWrite(0,255);

      startupState = persistants.getUChar(PERSISTANT_STARTUP,SYSTEM_OFF);

      onOff.usesLed = false;
      if (startupState == SYSTEM_RESTART){        
        // if system was flag for restart (after a SW update for instance, we go directly to normal startup without having to press a button
        persistants.putUChar(PERSISTANT_STARTUP,SYSTEM_OFF);  // we set the startup mode so that next time it will be requested to press the button
        startupState = NORMAL_STARTUP;
      }else if(startupState == RESTART_REQUEST_AS_SUB_CDC){
        startupState = USB_CDC_AND_FWUPDATE_STARTUP;         
        persistants.putUChar(PERSISTANT_STARTUP,SYSTEM_OFF);  // we set the startup mode so that next time it will be requested to press the button
      }
      // SYSTEM_OFF 
      else if (onOff.state == ON_OFF_TRIGGER){
        // else a button has to be pressed to startup
        delay(STARTUP_BUTTON_DEBOUNCE_MS);
        while(startupState == SYSTEM_OFF) {
          unsigned long releasedDuration = 0;
          const bool released = onOff.consumeRelease(releasedDuration);
          if (released){
            // The action for a normal, short or CDC startup is committed when
            // the button is released.  A hold longer than the CDC window is
            // handled below, while the button is still pressed.
            if (releasedDuration>RESET_GRACE_PERIOD_MS ){ 
              // A release between the grace threshold and the reset threshold
              // requests a reboot into the USB CDC/firmware-update startup.
              persistants.putUChar(PERSISTANT_STARTUP,RESTART_REQUEST_AS_SUB_CDC);
              delay(11);
              // put the code here that would deactivate the USBMIDI and reset the USB interface to jtag
              // Serial.begin(115200); doesnt work
              // tud_disconnect() / reconnect // doesnt work
              ESP.restart();
            }else if (releasedDuration>pressTime ){ 
              // The configured normal-start threshold has been exceeded.
              startupState = NORMAL_STARTUP;
              //if (debug) DEBUG_PRINT("\nnormal press");
            }else{
              // A short press only requests a battery display before sleep.
              startupState = BATTERY_STARTUP;
              //if (debug) DEBUG_PRINT("\nshort press ");
            }
            break;
          }else if (onOff.state != ON_OFF_TRIGGER) {
            // The sampler has observed the release, but the latched event has
            // not reached this context yet. Give it one short scheduling slot
            // instead of leaving startup with SYSTEM_OFF.
            delay(1);
          }else{
            if (onOff.pushedTime>RESET_DELAY_MS){
              // Ten seconds of continuous hold enters reset startup
              // immediately.  Breaking here is intentional: waiting for a
              // release would route the same press through the CDC branch.
              startupState = RESET_STARTUP;
              break;
            }else if (onOff.pushedTime>RESET_GRACE_PERIOD_MS){
              // Between the CDC grace threshold and ten seconds, flash the
              // LED to indicate that releasing now will request CDC startup.

              if (toggle == 0) toggle = 255; 
              else toggle = 0;
              ledcWrite(0,toggle);              
              delay(map(RESET_DELAY_MS-onOff.pushedTime,0,RESET_DELAY_MS-RESET_GRACE_PERIOD_MS,0,200));

            }else if (onOff.pushedTime<RESET_GRACE_PERIOD_MS && onOff.pushedTime>pressTime){
              // The normal startup window is acknowledged by a steady LED.
              ledcWrite(0,0); //led on
            }
          }
        }
      }else{
        // else we do not startup but go back to sleep
        goDeepSleep();
      }
      ledcWrite(0,255);   //led off
      onOff.usesLed = true;      
    }

    /**
     * @brief Stop all button LED activity before entering deep sleep.
     *
     * Button sampling runs on a separate FreeRTOS task and normally uses LEDC
     * channel 0 to show press progress. During shutdown that task must no longer
     * be allowed to change the LED brightness. The XIAO status LED is active-low,
     * therefore a duty cycle of 255 and a HIGH GPIO level both mean "off".
     */
    void stopStatusLed(){
      onOff.usesLed = false;
      for (uint8_t i = 0; i < MAXB; ++i){
        if (buttons[i] != nullptr) buttons[i]->usesLed = false;
      }

      ledcWrite(0, 255);
      ledcDetachPin(BUILTIN_LED);
      pinMode(BUILTIN_LED, OUTPUT);
      digitalWrite(BUILTIN_LED, HIGH);
    }
    
    /** @brief Apply the saved display rotation and matching button orientation. */
    void setScreenAndButtonRotation(){
      if (persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO) == INVERTED_SCREEN_YES){
        //swap button
        uint8_t oldLeftID = buttons[LEFTB]->getID();
        buttons[LEFTB]->setID(buttons[RIGHTB]->getID());
        buttons[RIGHTB]->setID(oldLeftID);
      }else{
        // by default we keep the button as they were initialized by the HWM
        // however, in case user has rotated from the menu, we have to swap button
        if (swapButton){
          uint8_t oldRightID = buttons[RIGHTB]->getID();
          buttons[RIGHTB]->setID(buttons[LEFTB]->getID());
          buttons[LEFTB]->setID(oldRightID);
        }
      }  
      tft.setRotation(persistants.getUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO));        
    }

    /** @brief Select the USB serial configuration for this startup. */
    void useCDC(){
      // activate the serial communication and deactivate MIDI
      useUSBMidi = false;
      #ifdef DEBUG
        debugMon.begin(115200);
      #endif
      USB.enableDFU();   // allow the interface to be seen as jtag and upload fw but still do not show the seriall log          
      USB.begin();  // required to make the serial usb line working, but DO NOT PUT USB.begin() when using MIDI          
      // Give the USB Serial/JTAG host a short opportunity to attach.  This is
      // deliberately bounded: it is not a boot prerequisite and cannot
      // recreate the previous indefinite wait caused by Serial.flush().
      const unsigned long start = millis();
      while ((millis() - start < 1000) && !debugMon) {
        delay(1);
      }
    }


    /** @brief Select the USB MIDI configuration for this startup. */
    void useMIDI(){
      useUSBMidi = true;        // used to display appropriate icon for battery charging or cable

      #ifdef PROD

      /*
      USB.onEvent(usbEventCallback);
      //USBSerial.onEvent(usbEventCallback);
      MidiUSB.onEvent(usbEventCallback);

      MidiUSB.begin();
      //USBSerial.begin();
      USB.begin();
      */
    
      //USB.VID(0x6666);
      //USB.PID(0x1234);
      //USB.serialNumber("FS-1000");
      //USB.firmwareVersion(0x0000);
      //USB.usbVersion(0x0200);
      /*USB.usbClass(TUSB_CLASS_AUDIO);
      USB.usbSubClass(0x00);
      USB.usbProtocol(0x00);
      USB.usbAttributes(0x80);*/
      //USB.usbPower(100);
      //tinyusb_add_string_descriptor("MIDIUSB_ESP32_STR_DESC");

      //tinyMIDIusb.begin();            // as long as we don't call these 2 begins, the USB is not visible        
      
      // with tinyUSB we initialize the USB midi stack, then MIDI HELPERS from Lathoud MIDIUSB will work
      tinyMIDIusb = new MIDIusb;
      tinyMIDIusb->setBaseEP(2);
      tinyMIDIusb->begin();
      #endif
    }

    /** @brief Initialize the TFT using the board reset, pin and driver settings. */
    void initTFT(){
      tft.init();
      tft.fillScreen(TFT_BLACK);
      tft.setSwapBytes(true);
      setScreenAndButtonRotation();
    }

    /**
     * @brief Initialize power, inputs, storage, display and the startup USB mode.
     *
     * Read persistent preferences and the startup button first, then enable the
     * peripheral rail. The historical order initializes components, including
     * Ethernet, before the TFT. Mount LittleFS before
     * application data loads. Debug builds select CDC; production selects CDC or MIDI
     * from the startup mode. Start background power-button/display handling last.
     */
    virtual void init() {
      GenericComponent::init();            
      
      persistants.begin("interval", false);     

      // init device that do not need power but are required for startup
      onOff.init();    
      powerRail.init();        
      // get the power
      wakeUp(NORMAL_STARTUP_PRESS_MS);

      // Start the debug console early enough to capture hardware init.
      #ifdef DEBUG
        debugMon.begin(115200);
      #endif

      setPowerRail(ON);
      for(int i = 0; i < hwComponents.size(); i++) hwComponents.get(i)->init();
      initTFT();

      USB.productName("FOOTSWITCH");
      USB.manufacturerName("Interval");
      USB.serialNumber("FS-1000");   // pas vide

      if (!LittleFS.begin()){
        startupState = NO_FS_STARTUP;
      }

      bootScreen(startupState); 

      #ifdef DEBUG
        // Development builds always expose CDC for diagnostics, regardless
        // of the startup-button duration selected above.
        // Consequently, modes such as BATTERY_STARTUP and RESET_STARTUP are
        // bypassed after this point in DEV; production keeps the state chosen
        // by the startup button and selects CDC or MIDI accordingly.
        //serialMutex = xSemaphoreCreateMutex();             
        startupState = USB_CDC_AND_FWUPDATE_STARTUP;
        useCDC();
      #else
        // Production builds preserve the startup mode selected by the button:
        // CDC for firmware-update startup, MIDI otherwise.
        if (startupState == USB_CDC_AND_FWUPDATE_STARTUP){
          useCDC();
        }else{        
          useMIDI();
        }
      #endif

      

      // the below code is equivalent to setting RST , which also works for W5500
      /*pinMode(D7, OUTPUT);
      digitalWrite(D7,LOW);
      delay(100);
      digitalWrite(D7,HIGH);
      delay(100);*/
   
      xTaskCreatePinnedToCore(xTaskBackground, "OnOFFandGUI", 4096, NULL, 1, NULL, 0);   // prio was 3 
      
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
    }

    /**
     * @brief Start configuration access-point mode and select it as configMedia.
     *
     * Pause normal Wi-Fi attempts, create the AP only once, then initialize it and
     * make it the connection used for browser configuration.
     */
    void startAP(const String* apName){
      // All normal network attempts have failed. Stop the STA side before
      // starting the dedicated configuration AP so both objects cannot drive
      // the shared Wi-Fi state at the same time.
      network->stopWiFi();
      if (ap == nullptr){
        ap = new AccessPoint(apName, true); 
        hwComponents.add(ap);
      }
      ap->init();         
      configMedia = ap;
    }
          

    /** @brief Refresh this component and report whether its displayed state changed. */
    bool refresh(bool force=false) {  
      refreshed = force;     
      for(int i = 0; i < hwComponents.size(); i++){
        refreshed |= hwComponents.get(i)->refresh(force);
      }

      //Control_Surface.loop();      
      
      return refreshed;
    }

    /** @brief Return the display object used by the hardware manager. */
    TFT_eSPI* getDisplay(){
      return &tft;
    }

    /** @brief Prepare hardware pins before entering deep sleep. */
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


    /** @brief Switch power to the external hardware rail. */
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

    /** @brief Provide the saved network choices and preferred SSID. */
    void setWiFi(JsonList* wiFiList, String ssid) {      
      if (network != nullptr) network->setWiFi(wiFiList,ssid);
    }

    /** @brief Apply click/hold timing preferences to the hardware buttons. */
    void setButtonReactivity(uint16_t push, uint16_t dClic){
      for (uint8_t b=0;b<MAXB;b++){
        buttons[b]->doubleClickGap = dClic;
        buttons[b]->push_reactivity = push;
      }
    }

 
  private:
    #ifdef DEBUG
      setup_t user;
    #endif
    uint8_t toggle = 0;
      
};

#endif
