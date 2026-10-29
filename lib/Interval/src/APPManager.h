#ifndef APPManager_h
#define APPManager_h

/**
 * @file APPManager.h
 * @brief Coordinates the footswitch's everyday operation.
 *
 * Connects the buttons, screens, banks and device interfaces, and handles
 * configuration from the web application.
 *
 * For example, it sends a button press to the selected strip, opens the bank
 * menu, or saves changes received from the browser.
 *
 * Call init() at startup, then refresh() from the main loop to keep the
 * hardware, interfaces and display up to date.
 */

#include "nvs_flash.h"
#include <LinkedList.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

#include "interfaces/i_utils.h"
#include "interfaces/i_midi.h"
#include "interfaces/i_dmx.h"
#include "interfaces/i_osc.h"


//////////////////// WIDGET LOCATION & SIZE ////////////////////////
#define OVERLAY_ICON_SIZE TFT_HEIGHT/15


#include "strips/s_midi.h"
#include "strips/s_utils.h"
#include "strips/s_osc.h"

#include "Banks.h"

#include "DataManagement.h"

//#include <esp32FOTA.hpp>

#include <aWOT.h>                 // awOT lasselukkari/aWOT@^3.5.0 wifi+eth server
                                  // apply patch https://github.com/arduino-libraries/Ethernet/pull/107/files
#include <MenuSystem.h>           // https://github.com/jonblack/arduino-menusystem
#include <LittleFS.h>

#include <WebSocketServer.hpp>    // https://github.com/skaarj1989/mWebSockets#meta     // use a local repository with SOCKETBUFFER SIZE CHANGED one because of https://github.com/skaarj1989/mWebSockets/issues/72
using namespace net;

/** @brief Attach handlers to a newly connected browser WebSocket. */
void socketConnectionCallback(IWebSocket &ws);


//////////////////// VOLATILE PARAMETERS ///////////////////////
//#define V_USE_NETWORK       "myNetworkIP"

/**
 * @brief C to C++ class function wrapper
 */
void commonFileServerWrapper(Request &req, Response &res);
/** @brief Bridge the callback/task entry to the responsible application or transport instance. */
void handleWebTaskWrapper(void * parameter);
//void gzFileServer(Request &req, Response &res);
/** @brief Bridge the callback/task entry to the responsible application or transport instance. */
void scanNetworksTaskWrapper(void * parameter);         // parrallel task
/** @brief Bridge the callback/task entry to the responsible application or transport instance. */
void webServerTaskWrapper(void * parameter);            // parrallel task
/** @brief Bridge the callback/task entry to the responsible application or transport instance. */
void WiFiRTPDiscoveryTask(void* parameter);             // parrallel task
/** @brief Application extension point for the final setup workflow. */
void defaultWizardStepFlow();

/**
 * @brief Build and animate a temporary progress page without selecting a new screen.
 *
 * Purge current widgets and call buildGUI() directly. screen.id/state remain
 * unchanged, allowing stopGUIThread() to request their normal reconstruction.
 */
void startGUIThread(uint8_t, uint8_t);
/**
 * @brief Stop temporary progress rendering and invalidate the normal page.
 *
 * Notify the GUI task and call triggerRedraw(); briefly wait for task exit
 * before the regular application loop resumes rendering the selected screen.
 */
void stopGUIThread();

/**
 * @brief Find the highest numeric archive version newer than the running firmware.
 *
 * Read the server directory listing and select valid numeric .tgz filenames.
 * Versions are compared as floating-point numbers, not semantic version tuples.
 * This function returns the selected URL/version; the caller opens the download.
 * @param baseUrl URL of the HTML archive listing.
 * @param currentVersion Running firmware version converted to a number.
 * @param outHttp HTTP client used to fetch the listing; stopped after reading it.
 * @param outRemoteVersion Selected version text when a newer file is found.
 * @param outFileUrl Absolute URL of the selected archive.
 * @return 1 for a newer archive, 0 if none is found, or a negative error code.
 */
int8_t findNewerRemoteFWUpdate(const String& baseUrl,
                               const String& currentVersion,
                               HttpClient& outHttp,
                               String& outRemoteVersion,
                               String& outFileUrl);
/** @brief Bridge the callback/task entry to the responsible application or transport instance. */
void expandTGZstreamToFSTask(void* parameter);
/**
 * @brief Extract an update package and advance to the firmware-file check.
 *
 * Notify the browser, extract archive files, and release the extractor and
 * parameters. Delete the stream only when params->ownsStream is true. Success
 * selects CHECK_BINARY; failure opens an error prompt. Clear the download flag
 * before returning so later update requests can show the appropriate screen.
 */
bool expandTGZstreamToFS(StreamParams* params);
/**
 * @brief Decompress and flash firmware from the supplied stream.
 *
 * Mark flashing in progress, configure progress callbacks and invoke the gzip
 * updater. Success opens an OK prompt leading to restart; failure shows Abort.
 * The helper deletes params and deletes its stream only when ownsStream is true.
 */
bool installGZBinaryFromStream(StreamParams* params);
/** @brief Update the progress displayed during file transfer or flashing. */
void updaterProgressCallback( uint8_t progress );

/**
 * @brief Flash an extracted firmware file when present, otherwise restore the screen.
 *
 * Open the compressed firmware, switch to single-file progress and start its
 * installer task. The installer owns the file stream after successful handoff.
 */
void checkAndInstallBinaryFromFile();

/** @brief Delete the requested file if it exists. */
bool checkAndDeleteFile(const char* filename) ;

#define VOID_JSON_ITEM "\"void\": 0"

//////////////////// SCREEN APPLICATION MANAGEMENT ////////////////////////

/**
 * @brief Screen selection (ScreenUnion is defined in GUIManager.h):
 * - id chooses the main page, such as banks, menus or utilities.
 * - state gives that page its context: a bank index or a workflow step.
 * - focus is also part of change detection. triggerRedraw() changes it to
 *   rebuild the same page; it is not the selected strip's id.
 * - reserved has no page-selection role here, but belongs to the compared value.
 *
 * A sub-screen is not a separate object or a stack of pages: it is simply a
 * different state interpreted by the main page's builder and workflow.
 * For example, SCREEN_UTILS + STATE_UTILS_INFO shows device information;
 * SCREEN_UTILS + STATE_UTILS_BRIGHTNESS uses the same page family for brightness.
 * SCREEN_BANK + state 1 instead means the second bank in the banks vector.
 *
 * Callers set id/state directly. No screen-change event is required: refresh()
 * compares the combined value against oldScreen and rebuilds when different.
 * Multiple changes before that comparison result in the final selection only.
 */
/**
 * @brief Main pages. Keep their numeric order: persisted screen ids use these values.
 */
enum{
  SCREEN_STARTUP,       ///< Startup logo.
  SCREEN_WIZARD,        ///< First-time setup and application-specific setup.
  SCREEN_MENU,          ///< Main settings menu; MenuSystem tracks its own selection.
  SCREEN_BANK_SWITCH,   ///< Bank-selection menu; separate from the bank being played.
  SCREEN_PROMPT,        ///< Message and up to three choices from promptParams.
  SCREEN_UTILS,         ///< Information, connectivity, brightness and update workflows.
  SCREEN_BYE,           ///< Status/error message or battery display before sleeping.
  SCREEN_BANK,          ///< Play controls; state is a zero-based bank index.
  SCREEN_MAX            // Page count, not a selectable page.
};

// Workflow steps. Meaning depends on screen.id; these are not bank indices.
// Some values are action requests rather than lasting visible pages.
// Declaring a state here does not by itself provide a builder or handler.
enum{
  // Setup: choose language, connect the browser, connect the network, user setup.
  STATE_WIZARD_LANG_STEP1,
  STATE_WIZARD_CONNECT_CONFIG_STEP2,
  STATE_WIZARD_CONNECT_NETWORK_STEP3,
  STATE_WIZARD_USER_STEP4,

  // Utility displays and the stages of opening browser configuration.
  STATE_UTILS_SEARCHING_CONNECTION,
  STATE_UTILS_INFO,
  STATE_UTILS_BRIGHTNESS,
  STATE_UTILS_CONNECTCONFIGMEDIA,       ///< Establish a configuration connection.
  STATE_UTILS_CONNECTWEBAPP,            ///< Connection available; wait for the browser.
  STATE_UTILS_WEBAPPCONNECTED,           ///< Browser session is connected.

  // Update actions and progress screens.
  STATE_UTILS_DOWNLOAD_FROM_URL,        ///< Start downloading/extracting the package.
  STATE_UTILS_UPDATEFW_FROM_FILEUPLOAD,
  STATE_UTILS_DISABLE_SWUPDATE_PROMPT,  ///< Save the preference, then restore the page.
  STATE_UTILS_SHOW_PROGRESS_SINGLE,     ///< Display one transfer/flash progress value.
  STATE_UTILS_RESTORE_SCREEN,           ///< Return to the saved bank or wizard.
  STATE_UTILS_RESTART_DEVICE,           ///< Stop interfaces and restart the ESP32.
  STATE_UTILS_SHUTDOWN,

  // Both goodbye states use the same visual builder, with different workflows.
  STATE_BYE_MODAL_MESSAGE,              ///< Show a message and shut down interfaces.
  STATE_BYE_GOSLEEP_MESSAGE,            ///< Wait for the battery display, then sleep.

  STATE_UTILS_TRIGGER_FWUPDATE_PROMPT,  ///< Prepare the update question.
  STATE_UTILS_CHECK_BINARY,             ///< Install extracted firmware if present.
  STATE_UTILS_INSTALLING_BINARY,
  STATE_UTILS_SHOW_PROGRESS_MULTIPLE,   ///< Display archive/file progress.
  STATE_UTILS_BLINK_ICON_TEXT
};


/** @brief Build the startup page or overlay from the supplied page state. */
void startupScreenBuilder(uint8_t id);
/** @brief Build the wizard page or overlay from the supplied page state. */
void wizardScreenBuilder(uint8_t id);
/** @brief Build the menu page or overlay from the supplied page state. */
void menuScreenBuilder(uint8_t id);
/** @brief Build the bank switch page or overlay from the supplied page state. */
void bankSwitchScreenBuilder(uint8_t id);
/** @brief Build the prompt page or overlay from the supplied page state. */
void promptScreenBuiler(uint8_t id);
/** @brief Build the utils page or overlay from the supplied page state. */
void utilsScreenBuilder(uint8_t id);
/** @brief Build the bye page or overlay from the supplied page state. */
void byeScreenBuilder(uint8_t id);
/** @brief Build the bank page or overlay from the supplied page state. */
void bankScreenBuilder(uint8_t id);
/** @brief Build the void overlay page or overlay from the supplied page state. */
void voidOverlay(uint8_t id);

/** @brief Build the base page or overlay from the supplied page state. */
void baseOverLayBuilder(uint8_t id);
/** @brief Build the bank page or overlay from the supplied page state. */
void bankOverlayBuilder(uint8_t id);

// Each page has a content builder and an overlay builder. Most share status
// icons; banks use a dedicated overlay and the goodbye page has none.
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

/**
 * @brief Reply sent to the browser: callback names the action, value holds its data.
 */
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
/** @brief Leave an unknown browser command unhandled for the default reply.
 *
 * typedef void (*NetworkChangeStateCallback)(bool state);
 */
bool defaultParseJsonInputCallback(JsonObject& jsonObject,JsonAnswer* jsonAnswer){
  DEBUG_INFO("defaultParseJsonInputCallback: %s",jsonObject["command"].as<String>().c_str());
  return false;
}

/** @brief Default callback returning an empty string. */
String defaultStringReturn(){return "";};
/** @brief Default callback with no action. */
void defaultVoidReturn(){};
/** @brief Default integer callback with no action. */
void defaultVoidInt(int i){};
/** @brief Default two-integer callback with no action. */
void defaultVoidDoubleInt(int i,int j){};
/** @brief Placeholder hook; no active operation in this implementation. */
void defaultVoidBool(bool state){}
/** @brief Build the default wizard page or overlay from the supplied page state. */
void defaultWizardScreenBuilder();
/** @brief Application extension point for the final setup workflow. */
void defaultWizardStepFlow();

/** @brief Open the device information page. */
void info_item_selected(MenuComponent* p_menu_component);
/** @brief Open browser configuration using the available network connection. */
void config_menu_item_selected(MenuComponent* p_menu_component);
/** @brief Go back within the menu, or restore the saved application page. */
void back_menu_item_selected(MenuComponent* p_menu_component);
/** @brief Profile-save menu hook; currently has no active body. */
void save_profile_item_selected(MenuComponent* p_menu_component);
/** @brief Load the language chosen from the menu. */
void loadLangFromMenu_item_selected(MenuComponent* p_menu_component);
/** @brief Resolve the selected bank name and open that bank screen. */
void switchToBank_item_selected(MenuComponent* p_menu_component);
/** @brief Save the opposite screen rotation and update button orientation. */
void tft_rotation_item_selected(MenuComponent* p_menu_component);
/** @brief Clear system settings and open the reset/sleep message. */
void reset_item_selected(MenuComponent* p_menu_component);
/** @brief Shut down interfaces and enter deep sleep. */
void shutdown_menu_item_selected(MenuComponent* p_menu_component);
/** @brief Handle entry into a submenu. */
void menu_selected(MenuComponent* p_menu_component);
/** @brief Open the brightness adjustment page. */
void brightness_menu_item_selected(MenuComponent* p_menu_component);
/** @brief Open the firmware update prompt without an automatic timeout. */
void fmwUpdate_menu_item_selected(MenuComponent* p_menu_component);

/** @brief Choose the configuration-connection page or show the connected browser session. */
void handleConfigConnection();  //


#include "DataManagement.h"

/**
 * @brief Shows the selected strip's labels, toggles and colors in the screen header.
 */
class StripHeaderWidget: public Top2Toggles{
  public:
    Strip* strip = nullptr;
    /**
     * @brief Initialize StripHeaderWidget and its configured member bindings.
     */
    StripHeaderWidget(Strip* s, bool debug=false): strip(s),
        /*Top2Toggles(s->getTopLabel(),s->getBottomLabel(),&s->toggle1,&s->toggle2,&s->frontColor,&s->backColor,
                    &GUI::defaultFcolor, &GUI::silver, &GUI::fullBrightness, debug)*/
        Top2Toggles(&NoStrip,&emptyString,&GUI::off,&GUI::off,&GUI::defaultFcolor,&GUI::defaultBcolor,
                    &GUI::defaultFcolor, &GUI::silver, &GUI::fullBrightness, debug)

    {
      //if (strip) setValues();
    }


    /**
     * @brief Refresh the header when the selected strip changes or a redraw is requested.
     */
    virtual bool refresh(bool force=false){

      //if (strip == nullptr) return false;

      refreshed = force;

      if (oldStrip != strip || /*strip->updated*/ force){
        //strip->updated = false; // we acknowledge the strip change
        oldStrip = strip;
        refreshed = true;
        if (strip != nullptr) setStripPointer(strip);
      }

      Top2Toggles::refresh(refreshed);

      return refreshed;
    }

    /** @brief Bind the header to the selected strip data. */
    void setStripPointer(Strip* s) {
      //DEBUG_PRINT("\nsetStripPointer(");
      if (s == nullptr){
        //DEBUG_PRINT("nullptr)");
        return;
      }
      else strip = s;

      setPointers(
        strip->getTopLabel(),
        strip->getBottomLabel(),
        strip->toggle1,
        strip->toggle2,
        &strip->frontColor,
        &strip->backColor
      );
      //DEBUG_PRINT("%p)",strip);
    }

  private:
    Strip* oldStrip = nullptr;

    String NoStrip = "No Strip";



};

#define MACRO_NUMBER_MAX      10
#define ADDRESS_COMMAND    "/command/%s"
#define ADDRESS_STIPID      "/strip/id/%s"
#define ADDRESS_ACTION        "/action/%s"
  #define ADDRESS_ACTION_SIZE 5
#define ADDRESS_MACRONAME     "/macro/name/%s"


#define SHORTCUT_GROUP "shortcut"
/**
 * @brief Provides the choices used to configure jump, macro and custom-command shortcuts.
 */
class Shortcuts: public DataProvider{
  public:

    ////////////// SHORTCUTS ////////////////////
    SerializableParameters jumpToSD = SerializableParameters("jump", SHORTCUT_GROUP, &dataRepo);
    SerializableParameters macroSD = SerializableParameters("macro", SHORTCUT_GROUP, &dataRepo);
    SerializableParameters advancedSD = SerializableParameters("advanced", SHORTCUT_GROUP, &dataRepo);


    DynamicDataSet dsStripIDs = DynamicDataSet(new DropDownAddress(ADDRESS_STIPID),this);
    DynamicDataSet dsDropDownActions = DynamicDataSet(new DropDownAddress(ADDRESS_ACTION),this);
    DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this);
    DynamicDataSet dsCommand = DynamicDataSet(new FreeTextAddress(ADDRESS_COMMAND, "command", REGEX_STRING) ,this);

    /** @brief Initialize Shortcuts with the supplied configuration and bindings. */
    Shortcuts(String name):DataProvider(name){
      // *********************************
      // SHORTCUTS
      // *********************************
      //*buffer = "[{\"jump\":\"shortcut\",\"properties\":[{\"/strip/id/%s\":[\"dropdown\"]}]},{\"macro\":\"shortcut\",\"properties\":[{\"/action/%s\":[\"dropdown\"]},{\"/macro/name/%s\":[\"dropdown\"]}]},{\"advanced\":\"shortcut\",\"properties\":[{\"/command/%s\":[\"freetext\"]}]}]";

      jumpToSD.registerDataSetAddress(dsStripIDs.address);
      macroSD.registerDataSetAddress(dsMacroNames.address);
      macroSD.registerDataSetAddress(dsDropDownActions.address);

      advancedSD.registerDataSetAddress(dsCommand.address);

      jsonInputRepo.add(&jumpToSD);
      jsonInputRepo.add(&macroSD);
      jsonInputRepo.add(&advancedSD);
    }

      // *********************************
      // DATASETS
      // *********************************

    /**
     * @brief Return the number of shortcut choices for the requested address.
     *
     * For strip ids, zero denotes a browser callback name rather than an array.
     */
    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_STIPID) return 0;       // 0 => means this is not an array of data but a javascript callback name
      else if (addrr == ADDRESS_ACTION) return ADDRESS_ACTION_SIZE;
      else if (addrr == ADDRESS_MACRONAME) return MACRO_NUMBER_MAX;
      DEBUG_WARN("<%s>:DataSet getSize(%s) is not implemented",dataProviderName.c_str(),addrr.c_str());
      return 0;

    }

    /**
     * @brief Return a shortcut choice or the browser callback name for strip ids.
     */
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_STIPID){
        return &getStripID;         //javascript local function callback
      }else if (addrr == ADDRESS_ACTION){
        return &dsDropDownActionsValues[index];
      }else if (addrr == ADDRESS_MACRONAME){
        return &dsMacroNamesValues[index];
      }
      DEBUG_WARN("<%s>:DataSet getValuePtr(%s,%d) is not implemented",dataProviderName.c_str(),addrr.c_str(),index);
      return &voidStatic;
    }

    private:
      /**
       * @brief javascript function that generates the data locally
       */
      String getStripID = "getStripID";

      String dsDropDownActionsValues[ADDRESS_ACTION_SIZE] = {
        "play",
        "pause",
        "stop",
        "FF",
        "RW"
      };

      String dsMacroNamesValues[MACRO_NUMBER_MAX] = {
        "macro1",
        "macro2",
        "macro3",
        "macro4",
        "macro5",
        "macro6",
        "macro7",
        "macro8",
        "macro9",
        "macro10"
      };
};


//////////////////// MAIN APPLICATION CLASS ////////////////////////

/**
 * @brief Connects hardware events to application actions and keeps the local and web UI updated.
 */
class ApplicationManager : public GenericComponent,MenuComponentRenderer,public EventDispatcher, public EventListener{
  //
  public:
  #define maxConcurrentClients 3
  int activeClients = 0;

    std::queue<EthernetClient*> clientQueue;

    std::vector<String> broadcastQueue;

    Event evt;

    //////////////////// USER CALLBACKS ////////////////////////////////////////////////////
    ParseJsonInputCallback userParseJsonInputCallback = defaultParseJsonInputCallback;
    ReturnStringCallback userJsonPongCallback = defaultStringReturn;
    VoidCallback userWizardstepFlow = defaultWizardStepFlow;
    VoidDoubleIntCallback userScreenBuilder = defaultVoidDoubleInt;

    //////////////////// STRIPS & BANKS CONTAINERS AND DEFINITION ////////////////////////
    //LinkedList<IntervalBank*> banks = LinkedList<IntervalBank*>();
    std::vector<IntervalBank*> banks; ///< Banks available to the application.

    StripHeaderWidget* stripHeader = nullptr;

    //////////////////// OVERLAY ICONS ////////////////////////

    String apIcon,
          interfaceIcon = ICON_FORBID,
          batIcon = ICON_WAIT, 
          iconReset = ICON_RESET,
          iconForbid = ICON_FORBID,
          powerIcon = ICON_POWER,
          iconX = "X",
          footSwitchIcon = ICON_FOOTSWITCH,
          intervalIcon = ICON_INTERVAL,
          batteryIcons[BAT_MAX_LEVEL]={ICON_BAT_LEVEL_0,ICON_BAT_LEVEL_1,ICON_BAT_LEVEL_2,ICON_BAT_LEVEL_3},          
          brightIcon = ICON_BRIGHTNESS,
          bigLeftIcon,
          bigRightIcon;        
    OpenFontRender* interfaceIconRenderer = &gui->iconOfr;

    uint16_t networkConnectivityIconColorIdle = TFT_RED, 
            apIconColor = TFT_RED, 
            batIconColor = TFT_GREEN, 
            bigLeftIconColor = GUI::menuActiveIconColor, 
            blinkColorBlue = TFT_BLUE,
            rainbowC,
            interfaceIconColor = TFT_GREY;
                
    String* byeIcon = &iconForbid; const uint16_t* byeIconColor = &GUI::defaultFcolor;

    float interfaceIconBrightness = GUI::systemBrightness;      ///< used to blink icon when dataProcessed

    String utilsBlinkLabel = "", utilsBlinkIcon = "";

    AlteredColor networkConnectivityIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &networkConnectivityIconColorIdle};
    AlteredColor apIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor configIconGlowingRedColor = AlteredColor{.baseColor = GUI::red, .value =  &GUI::unUsedColor};
    AlteredColor blinkColor = AlteredColor{.baseColor = GUI::blue, .value =  &GUI::unUsedColor};
    AlteredColor rainbowColor = AlteredColor{.baseColor = TFT_WHITE, .value = &rainbowC};

    int16_t overlayIconY = TFT_HEIGHT-OVERLAY_ICON_SIZE, 
            apIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE*2,
            batIconPosX = TFT_WIDTH-OVERLAY_ICON_SIZE,
            iconRow2 = OVERLAY_ICON_SIZE,
            rightScreen = TFT_WIDTH;

    float pendingBarWidth = TFT_WIDTH;
   
    bool disabledBankRendering = false;   ///< this is to stop rendering the bank

    //////////////////// MENU SYSTEM ATTRIBUTES AND MESSAGE ////////////////////////

    MenuSystem ms = MenuSystem(*this);
    MenuSystem* bankMenu = nullptr;
    MenuSystem* wizardMenu = nullptr;

    Menu* langMenu = nullptr;
    Menu* confMenu = nullptr;
    String menuTopText=" ", vBatLabel; ///< menuTitle

    /**
     * @brief Icon and color for one of the three visible menu positions.
     */
    struct MenuItems{
      String icon="";
      uint16_t color = GUI::menuInactiveIconColor;
    }menuItems[3];

    String menuBottomLabel=" ";

    uint16_t menuActiveIconColor;

    #define TWOFRAMEICON 2
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
    const char* fmwUpdateMenuString{ICON_FWUPDATE":FIRMWARE"};

    #define NO_BANK_MENU_ITEM "No bank"

    // Serialization
    #define ifdatasets    "ifdatasets"
    #define stripdatasets "stripdatasets"
    #define bulkconfig  "bulkConfig"

    //////////////////// UTILITIES ATTRIBUTES ////////////////////////

    BankNavigator* navBar = nullptr;

    /**
     * @brief Prompt message, button choices, destinations and optional timeout.
     *
     * Message, button choices and destinations for a prompt; zero timeout waits for input.
     */
    struct PromptParameters{
      String message;
      String labels[3];
      uint8_t targetScreens[3];
      uint8_t targetState[3];
      uint32_t timeoutMs;
    };

    PromptParameters promptParams;
    float promptRemainingTime = 0;
    float promptTimeoutMax = 1;
    unsigned long promptStartTime = 0;

    String newSWversion = "";

    String heapSize;
    String iconProgress = "";
    bool downloadSreamInprogress = false;
    bool flashBinaryInprogress = false;

     String FSstat = "";

    //////////////// INTERFACES ///////////////////
    //OSCInterface osc;
    NetworkOSC* networkOSCinterface;

    AppleMIDIInterface* appleMIDIInterface;
    USBMIDIInterface* usbMIDIInterface;
    BLEMIDIInterface* bleMIDIInterface;
    DINMIDIInterface*  dinMIDIInterface;

    ArtnetDMXInterface* artnetDMXInterface;
    ThreePinDMXInterface* threePinDMXInterface;

    unsigned long goSleepCountdownStart;
    // Duration of the current pre-sleep screen. Kept separate from the
    // battery sampling cadence so display timing can be adjusted independently.
    uint32_t goSleepDurationMs = BATTERY_STARTUP_SCREEN_MS;

    bool webAppConnected;

    WebApplicationService* webAppService = nullptr;

    String configClientIP; ///< ip of the client connect to the AP during configuration

    String firmwareUpdateFilename="";       ///< from fileStream
    String fwUpdateURL="";                   ///< from internet

    Shortcuts shortcuts = Shortcuts(SHORTCUT_GROUP);

    //////////////////////////////////////////////////////////////
    /////////////////                        /////////////////////
    /////////////////    MAIN CLASS START   //////////////////////
    /////////////////                       //////////////////////
    //////////////////////////////////////////////////////////////

    virtual const char* getDispatcherName() const override{
      return id.c_str();
    }

    /** @brief Return the name used to identify this listener in logs. */
    virtual const char* getListenerName()  override{
      return id.c_str();
    }


    /** @brief Initialize ApplicationManager with the supplied configuration and bindings. */
    ApplicationManager(HardwareManager* h, GUIManager* g, bool debug=true): GenericComponent(debug),EventDispatcher(12){
      hwm = h;
      gui = g;
      id = "ApplicationManager";

      Log::init(this);


      promptParams.message = "";
      promptParams.labels[0] = "";
      promptParams.labels[1] = "";
      promptParams.labels[2] = "";
      promptParams.targetScreens[0] = SCREEN_MENU;
      promptParams.targetScreens[1] = SCREEN_MENU;
      promptParams.targetScreens[2] = SCREEN_MENU;
      promptParams.timeoutMs = 0;
      promptTimeoutMax = 1;
      promptRemainingTime = 0;

      hwm->network->internetCallback = [this](TargetEcho* internet) {
          this->internetCallBack(internet);
      };

      /*callback = [this](Event* event) {
          this->defaultEventHandling(event);
      };*/
    }

    /** @brief Return the button refresh flag used by the application. */
    bool getButtonState(uint8_t B){
      return hwm->buttons[B]->refreshed;
    }

    /*void defaultEventHandling(EventDispatcher::Event* event){
      switch(event->type){
        case EventDispatcher::EVENT_APPLICATION:{

        }break;
      }
    }*/

    /**
     * @brief Return the focused strip in the current bank, if available.
     */
    Strip* getSelectedStrip(){
     if (screen.state >= banks.size()) return nullptr;
      IntervalBank* bank = banks[screen.state];
      if (bank == nullptr) return nullptr;
      return bank->getStrip(bank->selector);
    }


    /**
     * @brief Check once for a newer firmware version and offer the update when enabled.
     *
     * A successful internet check triggers this once per boot. A newer version
     * enables the update menu and can open an automatic prompt if allowed.
     * alreadyChecked is set before the request: reconnecting does not retry a
     * failed check during the same boot.
     */
    void checkRemoteVersion(){
      static bool alreadyChecked = false;
      //  check is only done once to avoid slowing down workflow when network disconnect and reconnect (when called by ping callback)
      if (!alreadyChecked){
        alreadyChecked = true;
        HttpClient httpClient(*hwm->network->client);
        String serverRoot = *systemCfg.getString(S_HOSTNAME,INTERVAL_DEFAULT_HOSTNAME);
        int8_t retCode = findNewerRemoteFWUpdate(
          concatStrings("http://",serverRoot,SERVER_SW_UPDATE_PATH),
          _V,
          httpClient,
          newSWversion,
          fwUpdateURL
        );
        // enable or disable the firmware update menu item accordingly
        fmw_update_menu_item->setEnabled(retCode == 1 ? true:false); // if retcode = 1, there is a newer version available
        //DEBUG_PRINT("\n================> fmw_update_menu_item(%s", retCode == 1 ? "enable)":"disable)");
        // BRANCH TO SCREEN PROMPT IF CONFIG ALLOWS IT
        if (retCode == 1){
          if (systemCfg.getBool(S_SWUPDATE_PROMPT)){
            screen.id = SCREEN_UTILS;
            screen.state = STATE_UTILS_TRIGGER_FWUPDATE_PROMPT;
            broadCast(concatStrings("{\""FW_UPDATE_URL"\":\"",fwUpdateURL,"\"}"));
          }else{
            if (screen.id == SCREEN_MENU) triggerRedraw();  // this s to update firmware menu item to be actif or disabled uppon internet check
            //DEBUG_PRINT("\nStartup update disabled in config");
          }
        }else if(retCode  == 0) {
          //DEBUG_PRINT("\n*** No firmware update ***");
        }else{
          //DEBUG_PRINT("\n*** Firmware update check error :%i ***",retCode);
        }
      }else{
        //DEBUG_PRINT("\n SW update check already done at startup");
      }

    }


    /**
     * @brief React to the internet check, then pass the result to listeners.
     */
    void internetCallBack(TargetEcho* internetEcho){
      //////////////////////// do what application manager has to do when internet is connected/disconnected
      if (internetEcho->alive){
        DEBUG_INFO("[ping]\t\t%s : internet connected\n\n",internetEcho->host.c_str());
        checkRemoteVersion();
      }else{
        DEBUG_INFO("[ping]\t\t%s : No internet connection, skipping remote version check\n\n",internetEcho->host.c_str());
      }
      //////////////////////// then propagate the event
      evt.type = EVENT_INTERNET_PING;
      evt.data = internetEcho;
      fireEvent(evt);
    }

    /**
     * @brief Choose the battery icon, warning color and voltage label.
     */
    void setBatIcon(){
      batIconColor = TFT_GREEN;
      switch(hwm->battery.batteryState){
        case BAT_USB_PLUGGED:
          batIcon = ICON_PLUG;
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
      vBatLabel = concatStrings(*lang.getString("Battery"), ": ", hwm->battery.vBat, "v");
    }

    /**
     * @brief Show whether the configuration access point is available and connected.
     */
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

    /**
     * @brief Show a steady connected color or animate the disconnected indicator.
     */
    void setNetworkConnectivityColor(){
      if (hwm->network->state == CONNECTED){
        networkConnectivityIconGlowingRedColor.value = &GUI::unUsedColor;
        networkConnectivityIconColorIdle = GUI::green;
      }else{
        networkConnectivityIconGlowingRedColor.value = &networkConnectivityIconColorIdle;
      }
    }

    /**
     * @brief Clear system settings and show a reset message before sleeping.
     */
    void goToResetScreenAndSleep(String message){
      clearConfig();
      menuTopText = *lang.getString(message);
      menuBottomLabel = emptyString;
      byeIcon = &iconReset; byeIconColor = &GUI::red;
      goSleepCountdownStart = millis();
      goSleepDurationMs = RESET_STARTUP_SCREEN_MS;
      screen.id = SCREEN_BYE; screen.state = STATE_BYE_GOSLEEP_MESSAGE;         //we could create a screen dedicated for reset
    }

    /**
     * @brief Show the battery status before the device returns to sleep.
     */
    void goToBigBatteryIconScreenAndSleep(){
      byeIcon = &batIcon; byeIconColor = &batIconColor;
      goSleepDurationMs = BATTERY_STARTUP_SCREEN_MS;
      screen.id = SCREEN_BYE;
      screen.state = STATE_BYE_GOSLEEP_MESSAGE;
    }

    /**
     * @brief Append banks from JSON and rebuild the bank menu.
     */
    IntervalBank* deserializeBanks(const String* bankJsonString){
      IntervalBank* bank = nullptr;

      JsonDocument jsonBanks;

      DeserializationError error = deserializeJson(jsonBanks, *bankJsonString);
      if (error){
        DEBUG_INFO("deserializeBanks()::%s",error.c_str());
        return nullptr;
      }else{
        JsonObject root = jsonBanks.as<JsonObject>();
        for (JsonPair kv : root) {
          bank = new IntervalBank(kv.key().c_str(),kv.value().as<String>());
          //DEBUG_PRINT("\nIntervalBank(%s,%s)",kv.key().c_str(),kv.value().as<String>().c_str());
          banks.push_back(bank);
        }
        refreshBankMenu();
      }

      if (banks.size()>0) return banks[banks.size() - 1]; // return the last bank
      else return nullptr;
    }


    /** @brief Load the firmware catalogue and the optional user catalogue. */
    bool loadLanguageCatalogues(String languageName){
      String langFile = concatStrings("/", languageName);
      langFile.toUpperCase();

      lang.clean();
      bool firmwareLoaded = lang.load(langFile + ".lang");

      // User catalogues are optional. Support both filename conventions so an
      // application can ship `FRANCAIS.i18n` or `francais.i18n` on LittleFS.
      userLang.clean();
      String userFile = langFile + ".i18n";
      if (!userLang.load(userFile)) {
        String lowerUserFile = userFile;
        lowerUserFile.toLowerCase();
        if (lowerUserFile != userFile) userLang.load(lowerUserFile);
      }

      return firmwareLoaded;
    }

    /**
     * @brief Apply saved Wi-Fi, button timing and language settings.
     */
    void applySystemSettings(){
      // this function applies all settings from the systemCfg conf file
      wiFiList.deserialize(*systemCfg.getString(S_SSID));
      hwm->setWiFi(&wiFiList,*systemCfg.getString(S_LAST_USED_SSID));

      hwm->setButtonReactivity(systemCfg.getInt(S_PUSHDELAY),systemCfg.getInt(S_DCLICREACTIVITY));

      loadLanguageCatalogues(*systemCfg.getString(S_LANG, "ENGLISH"));


    }

    #define DEFAULT_PROFILE "default"
    #define SYSTEM_FILE_NAME "/settings.sys"

    /**
     * @brief Remove system settings and reset saved display preferences.
     */
    void clearConfig(){
      // add a callback usere function userClearConfig()
      // delete all config files here, add the code to iterate FS on *.conf

      // ... remove (*.user)
      // ... remove (*.bank)

      // delete systemCfg settings
      LittleFS.remove(SYSTEM_FILE_NAME);
      hwm->persistants.clear();
      hwm->persistants.putUChar(PERSISTANT_TFT_ROTATION,INVERTED_SCREEN_NO);
      hwm->persistants.putFloat(PERSISTANT_BRIGHTNESS,GUI::systemBrightness);    //default brightness
    }

    #define CUSTOM_LOGO_FILE "/custom.jpg"


    /**
     * @brief Initialize hardware, strip types, interfaces, settings and menus, then choose the startup screen.
     */
    void init(String _defaultUserConf="{}"){
      defaultUserConf = _defaultUserConf;
      GenericComponent::init();

      hwm->init();

      #if defined(USE_FTP) && defined(USE_NETWORK)
        ftp.addUser("ftp", "ftp");
        ftp.addFilesystem("LITTLEFS", &LittleFS);
        ftp.begin();
      #endif

      Event::disableAllEventDumps();
      //Event::enableEventDump(EVENT_DIGITAL_BUTTON);
      //Event::enableEventDump(EVENT_NETWORK_CONNECTED);
      //Event::enableEventDump(EVENT_NETWORK_CONNECTION_CHANGED);
      //Event::enableEventDump(EVENT_NETWORK_DISCONNECTED);
      //Event::enableEventDump(EVENT_SOCKET_ERROR);
      //Event::enableEventDump(EVENT_SOCKET_CLOSE);
      //Event::enableEventDump(EVENT_SOCKET_MESSAGE);
      //Event::enableEventDump(EVENT_WEBAPP_STARTED);
      //Event::enableEventDump(EVENT_BATTERY_REFRESHED);
      //Event::enableEventDump(EVENT_INTERFACE_MESSAGE);      
      //Event::enableEventDump(EVENT_ANALOG_BUTTON);

      // *********************************
      // SKELETONS & DATA SETS
      // *********************************
      //
      // general purposes / test skeletons
      //
      stripTextSkel = new StripSkeleton(STRIP_TEXT,GROUP_INTERFACE_UTILS,textStripConstructorCallback,'*');
        stripTextSkel->registerDataSetAddress(systemInterface.dsText.address);

      //
      // midi skeletons
      //
      midiNoteSkel = new StripSkeleton(STRIP_MIDINOTE,GROUP_INTERFACE_MIDI,midiNoteStripConstructorCallback,'*');
        midiNoteSkel->registerDataSetAddress(dsMidiChannels.address);
        midiNoteSkel->registerDataSetAddress(dsNotesOnOff.address);
        midiNoteSkel->registerDataSetAddress(dsMidiValue1.address);
        midiNoteSkel->registerDataSetAddress(dsMidiValue2.address);

      sysexSkel = new StripSkeleton(STRIP_SYSEX,GROUP_INTERFACE_MIDI,sysexStripConstructorCallback,'*');
        dsSysexMessages = new SysexLib(new DropDownAddress(ADDRESS_SYSEX));        // this dataset is based on a gzip file therefore it needs to be created after littleFS init and reloaded each time a saveLibrary is saved
        sysexSkel->registerDataSetAddress(dsSysexMessages->address);

      //
      // OSC skeletons
      //
      oscSkel = new StripSkeleton(STRIP_OSC,GROUP_INTERFACE_OSC,oscStripConstructorCallback,'*');
        dsOSCMessages = new OSCLib(new DropDownAddress(ADDRESS_OSCMSG));        // this dataset is based on a gzip file therefore it needs to be created after littleFS init and reloaded each time a saveLibrary is saved
        oscSkel->registerDataSetAddress(dsOSCMessages->address);


      // interfaces that do not need the network have to be started manually
      usbMIDIInterface = new USBMIDIInterface(true);
      bleMIDIInterface = new BLEMIDIInterface(true);
      dinMIDIInterface = new DINMIDIInterface(true);
      // interfaces that need the network will be started and stopped by app refresh when network change state
      networkOSCinterface = new NetworkOSC("sysOSC",7777,"1.1.1.1",5000,"/info",true); // true = configurable
      // DMX
      artnetDMXInterface = new ArtnetDMXInterface(true);
      threePinDMXInterface = new ThreePinDMXInterface(true);
      // RTPMIDI
      appleMIDIInterface = new AppleMIDIInterface(true);
      // WEB APP
      webAppService = new WebApplicationService(false);    // internal web app lifecycle service
      webAppService->setWebServerHandler(commonFileServerWrapper);
      webAppService->setWebSocketHandler(socketConnectionCallback);

      // EVENT REGISTRATION
      //we tell appleMIDIInterface to listen to event raised by directoryService  and vice versa
      directoryService.registerEventListener(appleMIDIInterface);
      appleMIDIInterface->registerEventListener(&directoryService);

      std::vector<String> groupesUniques;
      /* register the interface names in the dataset of their group, without duplating the groups
          {"provider":"systemCfg","address":"/provider/Mixer/%s","values":["X+Mseries-Network(OSC)","MixingStation"]},
          this is used by the webapp to populate the available interface for a defined strip
      */
      for (const auto& interface : interfaceRepo.interfaces){

        if (!interface->init(/*this*/)){
          DEBUG_WARN("%s->init() failed or is not fully implemented",interface->interfaceName.c_str());
        }

        bool exists = false;
        for (const String& g : groupesUniques) {
          if (g == interface->group) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          groupesUniques.push_back(interface->group);
          new DynamicDataSet(new DropDownAddress(ADDRESS_PROVIDER+interface->group+"/%s"),&systemInterface);
        }

      }

      systemCfg.load(SYSTEM_FILE_NAME,defaultSysConf);
      applySystemSettings();

      userCfg.load(concatStrings(concatStrings("/", *systemCfg.getString(S_PROFILE, DEFAULT_PROFILE)), ".user"),defaultUserConf);

      #ifdef DEBUG_Z
        String s;
        systemCfg.serialize(&s);
        DEBUG_PRINT("\n\n systemCfg:\t%s [...]",s.substring(0,300).c_str());
        s="";
        userCfg.serialize(&s);
        DEBUG_PRINT("\n\n userCfg:\t%s [...]",s.substring(0,300).c_str());
        s="";
        lang.serialize(&s);
        DEBUG_PRINT("\n\n langfile:\t%s [...]",s.substring(0,300).c_str());
        DEBUG_PRINT("\nexample of langfile token: Connect:<%s>",lang.getString("Connect")->c_str());
      #endif


      GUI::systemBrightness = hwm->persistants.getFloat(PERSISTANT_BRIGHTNESS,DEFAULT_BRIGHTNESS);
      if (GUI::systemBrightness < BRIGHTNESS_MIN) GUI::systemBrightness = BRIGHTNESS_MIN;
      if (GUI::systemBrightness > BRIGHTNESS_MAX) GUI::systemBrightness = BRIGHTNESS_MAX;

      gui->init();

      for (uint8_t i=0;i<SCREEN_MAX;i++) screenBuilders.add(&screens[i]);

      ///////////////////////////////////////////////////////////////////////
      // REGISTER APPLICATION MANAGER AS AN EVENT LISTENER OF OTHERS COMPONENTS
      ///////////////////////////////////////////////////////////////////////
      for (uint8_t i=0;i<MAXB;i++){
        hwm->buttons[i]->registerEventListener(this);   // ApplicationManager listen to switch event
      }
      hwm->onOff.registerEventListener(this);           // ApplicationManager listen to onOff button event
      hwm->network->registerEventListener(this);        // ApplicationManager listen to network event

      hwm->battery.registerEventListener(this);         // ApplicationManager listen to battery state

      ///////////////////////////////////////////////////////////////////////
      // DESIGN MENU
      ///////////////////////////////////////////////////////////////////////
      back_menu_item = new MenuItem(quitMenuString, &back_menu_item_selected);
      info_menu_item = new MenuItem(infoMenuString, &info_item_selected);
      config_menu_item = new MenuItem(configMenuString,&config_menu_item_selected);
      profile_menu_item = new Menu(profileMenuString,&menu_selected);
      save_profile_menu_item = new MenuItem(saveMenuString,&save_profile_item_selected);
      tft_rotation_menu_item = new MenuItem(tftRotationMenuScreen,&tft_rotation_item_selected);
      reset_menu_item = new MenuItem(resetMenuScreen,&reset_item_selected);                // to be removed
      shutdown_menu_item = new MenuItem(shutdownMenuString, &shutdown_menu_item_selected);
      brightness_menu_item = new MenuItem(brightnessMenuString, &brightness_menu_item_selected);
      fmw_update_menu_item = new MenuItem(fmwUpdateMenuString, &fmwUpdate_menu_item_selected);
      fmw_update_menu_item->setEnabled(false); // disabled by default, enabled when internet is available

      ms.get_root_menu().add_item(back_menu_item);
      ms.get_root_menu().add_item(info_menu_item);
      ms.get_root_menu().add_item(config_menu_item);
      ms.get_root_menu().add_item(tft_rotation_menu_item);
      ms.get_root_menu().add_item(reset_menu_item);
      ms.get_root_menu().add_menu(profile_menu_item);
      ms.get_root_menu().add_item(shutdown_menu_item);
      ms.get_root_menu().add_item(brightness_menu_item);
      ms.get_root_menu().add_item(fmw_update_menu_item);
      profile_menu_item->add_item(back_menu_item);
      profile_menu_item->add_item(save_profile_menu_item);

      hookLangToMenu();

      //quarter1xPos =

      // glowing colors declarations
      //gui->glowingColors.add(&glowingGoldColor);
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      gui->alphaColors.add(&GUI::glowingGoldColor);

      blinkColor.value = &blinkColorBlue;
      gui->alphaColors.add(&blinkColor);
      gui->alphaColors.add(&networkConnectivityIconGlowingRedColor);
      gui->alphaColors.add(&apIconGlowingRedColor);
      gui->alphaColors.add(&configIconGlowingRedColor);

      gui->rainbowColors.add(&rainbowColor);
      gui->alphaColors.add(&rainbowColor);

      minHeapSize = maxHeapSize = ESP.getFreeHeap();

      // BY DEFAULT, WE DO NOT STARTUP, ONLY DISPLAY BATTERY LEVEL AND SWITCH OFF, SO WE HET THE ICON READY
      setAPIcon();
      setBatIcon();


      bool wizard = false;

      // Choose startup from the power-on conditions: reset clears settings;
      // battery-only startup shows its charge then sleeps; missing storage shows
      // an error; first-time setup opens the wizard; normal startup loads banks.
      switch(hwm->startupState){
        case RESET_STARTUP:
          // put reset code here
          goToResetScreenAndSleep("RESET");
        break;
        case BATTERY_STARTUP:
          menuTopText = *lang.getString("Press >1s to startup"); menuBottomLabel = emptyString;
          goSleepCountdownStart = hwm->bootTime;
          goSleepDurationMs = BATTERY_STARTUP_SCREEN_MS;
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
          hwm->persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_LANG_STEP1);
          startup(SCREEN_WIZARD);
        break;
        case NORMAL_STARTUP:
        case USB_CDC_AND_FWUPDATE_STARTUP:
          startup(SCREEN_BANK);
        break;
      }

      DEBUG_INFO("[APP] ***************************************************");
      DEBUG_INFO("[APP] ******************* INIT FINISHED *****************");
      DEBUG_INFO("[APP] ***************************************************\n\n");
    }

    /**
     * @brief Load the selected profile's banks and restore the saved screen.
     */
    void startup(uint8_t startupScreen){
      DEBUG_INFO("[APP] ******************* startup *****************");
      // save the screen to be called (wiuard or bank)
      hwm->persistants.putUChar(PERSISTANT_SCREEN_ID,startupScreen);
      // we init banks and then go to banks or fail screen
      String bankFilename = concatStrings(concatStrings("/", *systemCfg.getString(S_PROFILE, DEFAULT_PROFILE)), ".bank");
      if (!loadBanks(bankFilename)){
        menuTopText = *lang.getString("Bank creation");
        menuBottomLabel = emptyString;
        byeIconColor = &GUI::red; byeIcon = &iconForbid;
        screen.id = SCREEN_BYE; screen.state = STATE_BYE_MODAL_MESSAGE;
      }

      //disableUnusedInterface();      // optimize

      checkAndDeleteFile(FIRMWARE_BIN_FILENAME);  // in case there is a .bin.gz file leftover from previous firmware update
      restoreScreen();       // restore the screen that persists
      DEBUG_INFO("[APP] ******************* startup.end *****************\n\n");
    }

    /**
     * @brief Disable strip-facing interfaces that have no registered listeners.
     */
    void disableUnusedInterface(){
      // unsused interfaces are the one which are not referred by any instanciated strips
      for (const auto& interface : interfaceRepo.interfaces){
        if (interface->countRegisteredListeners()==0 && interface->hasStrip){ // only for interfaces that are exposed => means they can be configured by user
          DEBUG_INFO("%s has no strip : 🚫DISABLED",interface->interfaceName.c_str());
          //interface->switchDevice(false);  // BE CAREFULL
          interface->state.enabled = false;
        }
      }
    }

    /**
     * @brief Ask the browser to fetch the bank configuration again over HTTP.
     */
    void webAppToRefreshBanks(){
      // the banks have to be send by POST http and not socket, hence we just tells the application:
      // now you need to request the banks to be send you back by placing a postQuery
      broadCast("{\"reloadBanks\":\"\"}");
    }

    /** @brief Handle relevant incoming events for this component.
    *
    * EVENTS HANDLERS
    * Handle network, button and battery changes, then forward the event to listeners.
    * Important reactions before forwarding the event to listeners:
    * - Ethernet inserted during Wi-Fi closes the browser session and stops interfaces.
    * - A connected network saves the Wi-Fi choice and can serve configuration.
    * - Connection changes update icons; button events go to gesture handlers.
    * - Critical battery voltage opens the sleep screen unless USB power is present.
    * Unhandled events are forwarded too.
    */
   void onEvent(const Event& evt) override {
      switch (evt.type) {
        case EVENT_PLUGGED_ETH_WHILE_WIFI_CONNECTED:
          DEBUG_INFO("[EVENT] Plugged Ethernet cable while WiFi connected");
          sendTerminateMessageToWebApp("Network connection changed. Close this page");         // the browser becomes unavialble, so terminate the application
          stopInterfaces();
        break;
        case EVENT_NETWORK_CONNECTED:
          // inform all that network has changed (it could be the network cable pluged/unplug while in AP)
          if (webAppConnected) broadCast(parseJsonInput("{\"command\":\"retrieve\",\"param\":\"connectivity\"}"));  // inform webclient               could be done via ws.send as well
          
          if (hwm->network->media == WIFI_MEDIA){
            //we set last used Wifi SSID (id) and save config, hence keep WIFI_MEDIA credential
            //DEBUG_PRINT("\n------> SAVING SSID");
            systemCfg.setValue(S_LAST_USED_SSID,hwm->network->SSID);
            systemCfg.save();
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
          }

        break;
        case EVENT_NETWORK_CONNECTION_CHANGED:
          setNetworkConnectivityColor();
          setAPIcon();
        break;
        case EVENT_ANALOG_BUTTON:
          handleSwitchEvent(evt);
        break;
        case EVENT_DIGITAL_BUTTON:
          handleOnOffEvent(evt);
        break;
        case EVENT_BATTERY_REFRESHED:
          setBatIcon();
          if (hwm->battery.vBat<VBAT_CRIT && hwm->battery.batteryState != BAT_USB_PLUGGED){
            menuTopText = *lang.getString("Low Battery");
            menuBottomLabel = emptyString;
            goSleepCountdownStart = millis();
            goToBigBatteryIconScreenAndSleep();
          }
        break;
      }
      fireEvent(evt);   // propagate the event
    }

    /**
     * @brief Route a footswitch gesture to its click or hold handler.
     */
    void handleSwitchEvent(const Event& event) {
      ButtonData* btData = (ButtonData*) event.data;
      switch(btData->id){
        case BUTTON_DOUBLE_CLIC:
          doubleClicCallBack(btData->buttonId,btData->time);
        break;
        case BUTTON_HOLD:
          holdCallback(btData->buttonId,btData->time);
        break;
        case BUTTON_LONG_CLIC:
          longClicCallBack(btData->buttonId,btData->time);
        break;
        case BUTTON_RELEASE_HOLD:
          releaseHoldCallback(btData->buttonId,btData->time);
        break;
        case BUTTON_SIMPLE_CLIC:
          simpleClicCallBack(btData->buttonId,btData->time);
        break;
        case BUTTON_ENTER_HOLD:
          enterHoldCallback(btData->buttonId,btData->time);
        break;
      }
    }


    /** @brief Map power-button gestures to menu navigation and interface enable/disable.
     *
     * Use power-button gestures to open menus or toggle the selected interface.
     * Long press toggles the menu. Double press on the bank screen toggles the
     * selected strip's interface only if allowed; save the choice and mark its
     * state for notification to the browser.
     */
    void handleOnOffEvent(const Event& event){
      ButtonData* btData = (ButtonData*) event.data;
      switch(btData->id){
        case BUTTON_DOUBLE_CLIC:

          if (screen.id == SCREEN_BANK){
            // toggle interface on / off
            if (getSelectedStrip() != nullptr){
              DeviceInterface* interface = getSelectedStrip()->interface;
              bool enabled = !interface->state.enabled;
              // if interface allows it, we switch its state
              if (interface->isSwitchable()){
                getSelectedStrip()->interface->switchDevice(enabled);
                interface->state.broadcastState = true;
                interface->save();
              }else{
                DEBUG_INFO("[DCLIC]\t\tInterface %s is not switchable",interface->interfaceName.c_str());
              }
            }
          }

        break;
        case BUTTON_HOLD:

        break;
        case BUTTON_LONG_CLIC:
          //DEBUG_PRINT("\nmenuSwitchCallback");
          menuSwitchCallback(btData->buttonId,btData->time);
        break;
        case BUTTON_RELEASE_HOLD:

        break;
        case BUTTON_SIMPLE_CLIC:
          simpleClicCallBack(btData->buttonId,btData->time);
          /*char* msg = "helloworld";
          USBMIDI.sendSysEx(strlen(msg),(const byte*)msg);
          USBMIDI.sendNoteOn(123,62,1);*/
        break;
      }
    }

    /**
     * @brief Open the prepared prompt and start its timer.
     */
    void promptScreen(){
      resetPromptTimer();
      screen.id = SCREEN_PROMPT;
    }

    /**
     * @brief Prepare a message with one confirmation button.
     */
    void singlePromptScreen(String message,String label, uint8_t targetState = STATE_UTILS_RESTORE_SCREEN){
      promptParams.message = *lang.getString(message);

      promptParams.labels[0] = "";

      promptParams.labels[1] = *lang.getString(label);
      promptParams.targetScreens[1] = SCREEN_UTILS;
      promptParams.targetState[1] = targetState;        // back to caller

      promptParams.labels[2] = "";

      promptParams.timeoutMs = 0;

      promptScreen();     // branch to prompt screen
    }

    /**
     * @brief Offer the firmware update, or show its progress if already running.
     *
     * Left accepts, middle declines, right disables future automatic prompts.
     * Timeout takes the middle choice: waiting never accepts an update.
     * If download/flashing is already running, show its progress instead.
     */
    void SWupdatePromptScreen(uint16_t timeoutMs = 5000){

      if (downloadSreamInprogress){
        setCloudDownloadProgressScreen(); // if an update is already in progress, we go directly to the progress screen
      }else if (flashBinaryInprogress){
        screen.id = SCREEN_UTILS;
        screen.state = STATE_UTILS_SHOW_PROGRESS_SINGLE;
        setBinaryFlashScreen();
      }else{
        DEBUG_INFO("Preparing SW update prompt screen for v%s",newSWversion.c_str());
        // prepare software update promnpt screen
        promptParams.message = concatStrings(*lang.getString("Update to firmware")," v",newSWversion,"?");

        promptParams.labels[0] = *lang.getString("Yes");
        promptParams.targetScreens[0] = SCREEN_UTILS;  // go to utils screen to perform the update
        promptParams.targetState[0] = STATE_UTILS_DOWNLOAD_FROM_URL;

        promptParams.labels[1] = *lang.getString("No");
        promptParams.targetScreens[1] = SCREEN_UTILS;   // back to caller
        promptParams.targetState[1] = STATE_UTILS_RESTORE_SCREEN;

        promptParams.labels[2] = *lang.getString("No, never ask");
        promptParams.targetScreens[2] = SCREEN_UTILS;  // diable auto update prompt in utils screen
        promptParams.targetState[2] = STATE_UTILS_DISABLE_SWUPDATE_PROMPT;

        promptParams.timeoutMs = timeoutMs;                // 5s timeout

        promptScreen();
      }
    }


    /**
     * @brief Open the screen assigned to the chosen prompt button.
     */
    void branchFromPrompt(uint8_t target){

      uint8_t targetScreen = promptParams.targetScreens[target];

      if (targetScreen < SCREEN_MAX){
        screen.id = targetScreen;
        screen.state = promptParams.targetState[target];
      }
    }

    /** @brief Follow the configured destination of a labeled prompt button. */
    void handlePromptClick(uint8_t buttonID){
      // we only handle buttons for which a label has been set
      if (buttonID == hwm->buttons[LEFTB]->getID() && promptParams.labels[0] != "") branchFromPrompt(0);
      else if (buttonID == hwm->buttons[MIDDLEB]->getID() && promptParams.labels[1] != "") branchFromPrompt(1);
      else if (buttonID == hwm->buttons[RIGHTB]->getID() && promptParams.labels[2] != "") branchFromPrompt(2);
    }

    /** @brief Start the prepared prompt countdown from the current time. */
    void resetPromptTimer(){
      //DEBUG_PRINT("\nresetPromptTimer()");
      promptTimeoutMax = (float)promptParams.timeoutMs;
      promptRemainingTime = promptTimeoutMax;
      promptStartTime = millis();
    }

    /** @brief Update the countdown and take the middle choice when it expires.
     *
     * Update the countdown; a timed-out prompt takes its middle-button choice.
     */
    void handlePromptTimeout(){
      promptTimeoutMax = promptParams.timeoutMs > 0 ? promptParams.timeoutMs : 1;
      if (promptParams.timeoutMs == 0){
        promptRemainingTime = 0;
        return;
      }
      unsigned long elapsed = millis() - promptStartTime;
      if (elapsed >= promptParams.timeoutMs){
        branchFromPrompt(1);
      }else{
        promptRemainingTime = promptParams.timeoutMs - elapsed;
      }
    }

    /**
     * @brief Toggle between the menu and the saved application screen.
     */
    void menuSwitchCallback(uint8_t buttonID, unsigned long time){
      if (screen.id == SCREEN_MENU){   //set GUI
        restoreScreen();
      }else{
        restoreMenu();
      }
    }

    /**
     * @brief Handle a short press according to the current screen.
     */
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
          handlePromptClick(buttonID);
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

    /**
     * @brief Handle a long press according to the current screen.
     */
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

    /** @brief Handle double clicks; the bank case currently broadcasts a fixed test address.
     *
     * Handle a double press; the bank case currently sends a fixed web message.
     */
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

    /**
     * @brief Finish a hold according to the current screen.
     */
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
          if (buttonID == hwm->buttons[MIDDLEB]->getID()) 
            ;//switchBank(); //switchBankLayout();
          else 
            moduleReleaseClic(buttonID, time);
        break;
      }
    }

    /**
     * @brief Handle a continued hold, including repeated strip value changes.
     */
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


    /**
     * @brief Route the start of a hold to the bank-screen handler.
     */
    void enterHoldCallback(uint8_t buttonID, unsigned long time){
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

        break;
        case SCREEN_BANK:
          moduleEnterholdClic(buttonID, time);
        break;
      }
    }


  /**
   * @brief Browse the bank menu with side buttons and select with the middle button.
   */
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

  /**
   * @brief Save the display brightness and request a redraw.
   */
  void saveBrightness(){
    triggerRedraw();
    hwm->persistants.putFloat(PERSISTANT_BRIGHTNESS,GUI::systemBrightness);
  }

  /** @brief Adjust and save brightness with side buttons, or return to the menu. */
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

  /**
   * @brief Adjust brightness within the supported range.
   */
  void changeBrightness(uint8_t buttonID, float increment){
    if (buttonID == hwm->buttons[LEFTB]->getID()){
      if (GUI::systemBrightness > BRIGHTNESS_MIN) GUI::systemBrightness -= increment;
      if (GUI::systemBrightness < BRIGHTNESS_MIN) GUI::systemBrightness = BRIGHTNESS_MIN;
    }else if (buttonID == hwm->buttons[RIGHTB]->getID()){
      if (GUI::systemBrightness < BRIGHTNESS_MAX) GUI::systemBrightness += increment;
      if (GUI::systemBrightness > BRIGHTNESS_MAX) GUI::systemBrightness = BRIGHTNESS_MAX;
    }
  }

  /**
   * @brief Browse menu items or activate the selected item.
   */
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

  /**
   * @brief Use side buttons to select a strip and the middle button to trigger it.
   */
  void moduleSimpleClic(uint8_t buttonID){
    // perform the behaviour defined in the config file
    // at the moment, this is hard coded
    if (buttonID == hwm->buttons[MIDDLEB]->getID()){
      if (!stripAction(BUTTON_SIMPLE_CLIC)){
        DEBUG_WARN("stripAction failed");
        // code if execution fails: example: popup a message with the error
      }
    }
    else{
      stripScroll(buttonID);
    }
  }


  /**
   * @brief Toggle the current bank between details and zoom views.
   */
  void switchBankLayout(){
    if (screen.state < banks.size()){
      Interval::Bank* bank = banks[screen.state];
      if (bank != nullptr){
        if (bank->layout == LAYOUT_DETAILS) bank->layout = LAYOUT_ZOOM;
        else bank->layout = LAYOUT_DETAILS;
      }
    }
    triggerRedraw();
  }

  /**
   * @brief Send repeated value changes to the selected strip.
   */
  void moduleHoldClic(uint8_t buttonID, unsigned long time){
    // perform the behaviour defined in the config file
    // at the moment, this is hard coded
    stripChangeValue(buttonID, time, BUTTON_HOLD);
  }

  /**
   * @brief Switch banks on a middle-button hold and call the strip hold-entry hook.
   */
  void moduleEnterholdClic(uint8_t buttonID, unsigned long time){
    // perform the behaviour defined in the config file
    // at the moment, this is hard coded
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
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
            break;
            case STATE_UTILS_BRIGHTNESS:
            break;
          }
        break;
        case SCREEN_BANK:
          if (buttonID == hwm->buttons[MIDDLEB]->getID()) switchBank(); //switchBankLayout();
          //in all case, we inform the strip
          stripEnterHold(buttonID, time, BUTTON_HOLD);
        break;
      }

    
  }


  /** @brief Notify the selected strip that the held button was released. */
  void moduleReleaseClic(uint8_t buttonID, unsigned long time){
    stripReleaseHold(buttonID, time, BUTTON_HOLD);
  }

  /**
   * @brief Toggle the bank layout with the middle button, or move between strips.
   */
  void moduleLongClic(uint8_t buttonID){
    if (buttonID == hwm->buttons[MIDDLEB]->getID()){
      //switchBank();
      switchBankLayout();
    }else if(buttonID == hwm->buttons[LEFTB]->getID()){
      stripScroll(buttonID);
    }else{
      stripScroll(buttonID);
    }
  }

  /**
   * @brief Write the current banks to the profile file.
   */
  bool saveBanks(const String& bankFileName){
    bool success = false;
    // Ouvrir le fichier en écriture
    File file = LittleFS.open(bankFileName, "w");
    if (!file) {
      DEBUG_ERROR("\nError creation:%s",bankFileName.c_str());
    }else{
      if (file.print(serializeBanks()) == 0){
        DEBUG_ERROR("\nError writting to:%s",bankFileName.c_str());
        success = false;
      }
      file.close();
    }
    return success;
  }

  /**
   * @brief Load a profile's banks, using the welcome bank when its file is unavailable.
   */
  bool loadBanks(const String& bankFileName){

    String bankDefinition = "{\"Default\":{\"layout\":2,\"icon\":3,\"strips\":[{\"type\":\"text\",\"color\":2,\"icon\":5,\"size\":\"4x1\",\"interface\":\""SYSTEM_INTERFACE"\",\"position\":{\"x\":0,\"y\":0,\"id\":1,\"win\":0},\"properties\":[\"/text/Welcome\"]},{\"type\":\"text\",\"color\":3,\"icon\":5,\"size\":\"4x1\",\"interface\":\""SYSTEM_INTERFACE"\",\"position\":{\"x\":0,\"y\":1,\"id\":2,\"win\":0},\"properties\":[\"/text/Please\"]},{\"type\":\"text\",\"color\":5,\"icon\":5,\"size\":\"4x1\",\"interface\":\""SYSTEM_INTERFACE"\",\"position\":{\"x\":0,\"y\":2,\"id\":3,\"win\":0},\"properties\":[\"/text/goto\"]},{\"type\":\"text\",\"color\":7,\"icon\":5,\"size\":\"4x1\",\"interface\":\""SYSTEM_INTERFACE"\",\"position\":{\"x\":0,\"y\":3,\"id\":4,\"win\":0},\"properties\":[\"/text/configuration\"]}]}}";

    if (LittleFS.exists(bankFileName)){
      File file = LittleFS.open(bankFileName);
      if (file) {
        bankDefinition = file.readString();  // lit tout le contenu
      }else{
        DEBUG_WARN("%s open failed, using default bank definition",bankFileName.c_str());
      }
      file.close();
    }else{
      DEBUG_WARN("%s doesnt exists, using default bank definition",bankFileName.c_str());
    }

    return deserializeBanks(&bankDefinition) != nullptr ? true:false;

  }


  /**
   * @brief Build a JSON object containing each bank under its name.
   */
  String serializeBanks(){
    JsonDocument bankDoc;

    if (banks.size() ==0 ) return "{}";

    for (uint8_t i=0; i<banks.size(); i++){
      JsonDocument jsonBank;
      IntervalBank* bank = banks[i];
      if (bank == nullptr) continue;
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

    #define MAX_POST_COMMAND_SIZE 4096 // Limite maximale pour éviter un abus de mémoire, utilisé par la fonction httpPost

    /**
     * @brief Send the browser the options it needs when opening the configuration page.
     */
    void getJsonStartup(String* returnedJsonStringData){
      JsonDocument doc;
      String tempJsonString="";

      // Keep the web language selector in sync with the language files actually
      // installed in LittleFS.  The previous implementation hard-coded only
      // French and English, so the other valid firmware languages could never
      // be advertised to the web GUI even when their .lang files were present.
      std::vector<String> availableLanguages;
      populateFileList("lang", "/", &availableLanguages, "", false);
      JsonArray languages = doc.createNestedArray("langs");
      for (const String& languageFile : availableLanguages) {
        String language = languageFile;
        // Web assets use lowercase basenames (francais.json, deutsch.json,
        // ...), while firmware .lang files are conventionally uppercase.
        if (language.startsWith("/")) language.remove(0, 1);
        language.toLowerCase();
        languages.add(language);
      }
      doc["maxPOSTsize"] = MAX_POST_COMMAND_SIZE;
      doc["safeScanfSpace"] = String(SAFE_SCANF_SPACE);
      doc["sysexLibFile"] = SYSEXLIB_FILENAME;
      doc["oscLibFile"] = OSCLIB_FILENAME;
      doc["wizard"] = (hwm->persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK) == SCREEN_WIZARD)?true:false;
      doc["lang"] = *systemCfg.getString(S_LANG,"francais");
      doc[S_SWUPDATE_PROMPT] = systemCfg.getBool(S_SWUPDATE_PROMPT);
      JsonArray configs = doc.createNestedArray("configNames");
      configs.add("default"); // parse the file list and add all entries found, at the moment it's empty

      serializeJson(doc, *returnedJsonStringData);
      removeJsonQuote(returnedJsonStringData);

    }

    /**
     * @brief Collect datasets, interfaces, settings and banks for the web configuration.
     */
    void serialize(String* returnedJsonStringData){
      // this build the full serialization of the application

      JsonDocument doc;
      String tempJsonString="";

      //DEBUG_PRINT("\nserializing dataRepo");
      JsonObject ds = doc.createNestedObject(stripdatasets);
      dataRepo.serialize(ds);

      //DEBUG_PRINT("\nserializing IFconfig");
      JsonObject ifcfg = doc.createNestedObject(ifdatasets);
      IFdataRepo.serialize(ifcfg);


      //DEBUG_PRINT("\nserializing input (shortcuts & interfaces fields)");
      JsonArray ji = doc["jsonInput"].to<JsonArray>();
      jsonInputRepo.serialize(&ji);

      //DEBUG_PRINT("\nserializing interfaces");
      JsonArray interfaces = doc["interfaces"].to<JsonArray>();
      interfaceRepo.serialize(&interfaces);

      doc["connectivity"] = getConnectivityState();

      doc[FW_UPDATE_URL] = fwUpdateURL;

      //DEBUG_PRINT("\nserializing skeletons");
      JsonArray skel = doc["skeletons"].to<JsonArray>();
      stripFactory.serialize(&skel);

      //DEBUG_PRINT("\nserializing system");
      tempJsonString="";
      systemCfg.serialize(&tempJsonString);
      doc["system"]=tempJsonString;

      //DEBUG_PRINT("\nserializing bankCfg");
      tempJsonString = serializeBanks();
      doc["banks"]=tempJsonString;

      serializeJson(doc, *returnedJsonStringData);
      removeJsonQuote(returnedJsonStringData);
    }

    // Return to the saved screen, correcting unavailable bank or wizard selections.
    // Return to the saved bank or wizard step rather than a temporary menu,
    // prompt or progress page. Bank selections are saved in refresh(); the
    // wizard saves its own steps. An unavailable bank falls back to index zero.
    /** @brief Return to the saved bank or wizard step and correct unavailable selections. */
    void restoreScreen(){
      //DEBUG_PRINT("\nrestoreScreen():");
      screen.id = hwm->persistants.getUChar(PERSISTANT_SCREEN_ID,SCREEN_BANK);
      screen.state = hwm->persistants.getUChar(PERSISTANT_SCREEN_STATE,0);
      //DEBUG_PRINT("\t read: id=%d / state:%d",screen.id,screen.state);
      // prevents wrong branching
      if (screen.id == SCREEN_BANK){
        if (banks.size() == 0 || screen.state >= banks.size()) screen.state = 0;
      }
      if (screen.id == SCREEN_WIZARD && screen.state > STATE_WIZARD_USER_STEP4) screen.state = STATE_WIZARD_LANG_STEP1;
      //DEBUG_PRINT("\t => id=%d / state:%d",screen.id,screen.state);
      // branch the first available strip
      //DEBUG_PRINT("\tgetFocusedStrip():%s, id:%i",getSelectedStrip() == nullptr?"null":getSelectedStrip()->getTopLabel()->c_str(),getSelectedStrip() == nullptr?0:getSelectedStrip()->index);
    }

    /** @brief Select the main menu page. */
    void restoreMenu(){
      //DEBUG_PRINT("\nrestoreMenu()");
      screen.id = SCREEN_MENU;
    }

    /** @brief Extract the icon prefix from a menu entry label. */
    String getIconFromString(const char* item){
      return String((char)item[0]);
    }

    /** @brief Extract the text following the menu entry icon prefix. */
    String getNameFromString(const char* item){
      return String(item).substring(2);
    }

    std::vector<String> countryList;
    /**
     * @brief Build the language menu from the language files on the device.
     */
    void buildLangMenu(){
      if (langMenu != nullptr) delete langMenu;
      langMenu = new Menu(langMenuString,menu_selected);

      populateFileList("lang","/",&countryList,concatStrings(String(ICON_LANG), "-"),false);     // this populate the list of countries with the icon (prefix = k-) and without extension

      // populate the menu with all language found
      for(int i = 0; i < countryList.size(); i++){
        const char* countryIconAndName = countryList[i].c_str();
        langMenu->add_item(new MenuItem(countryIconAndName, loadLangFromMenu_item_selected));
        if (debug){
          //DEBUG_PRINT("\nadded menu entry: %s",countryList.get(i)->c_str());
        }
      }
    }

    /**
     * @brief Add the language choices and back item to the main menu.
     */
    void hookLangToMenu(){
      buildLangMenu();
      langMenu->add_item(back_menu_item);
      ms.get_root_menu().add_menu(langMenu);
    }

    /** @brief Delete matching files in the supplied folder, without recursing into subfolders.
     *
     * Delete matching files in the given folder; '*' matches all files.
     */
    void cleanFolder(String path, String fType) {
      File root = LittleFS.open(path);
      if (!root || !root.isDirectory()) {
        ///DEBUG_PRINT("\nInvalid directory: %s", path.c_str());
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
            String fullPath = concatStrings(path, "/", shortName);
            if (LittleFS.remove(fullPath)) {
              ///DEBUG_PRINT("\nDeleted %s", fullPath.c_str());
            } else {
              ///DEBUG_PRINT("\nFailed to delete %s", fullPath.c_str());
            }
          }
        }
        file = root.openNextFile();
      }
    }


    //------------------
    //
    // MANAGE WEB APPLICATION
    //
    //------------------
    /*void handleWeb(){
      if (webAppService != nullptr) webAppService->handleWeb();
    }*/

    /**
     * @brief Find a named root-menu entry, select it and open the menu screen.
     */
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

    /**
     * @brief Mark the screen as changed so the next refresh rebuilds its widgets.
     *
     * Changing focus marks the screen as different even when id/state stay the
     * same, for example after a layout change or an edit from the browser.
     */
    void triggerRedraw(){
      // this forces the GUI to be rebuild at next refresh
      //DEBUG_PRINT("\ntriggerRedraw()");
      screen.focus = screen.focus - 1;
    }

    /** @brief Select the multi-file download progress page. */
    void setCloudDownloadProgressScreen(){
      iconProgress = ICON_CLOUD_DOWNLOAD;
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_SHOW_PROGRESS_MULTIPLE;
    }

    /** @brief Set the firmware-flashing icon and progress label. */
    void setBinaryFlashScreen(){
      iconProgress = ICON_FWUPDATE;
      updateProgressLabel = *lang.getString("Flashing firmware in progress, do not interrupt!");
    }

    /**
     * @brief Build content and overlay for id, passing state to both callbacks.
     *
     * Callback argument names may say "id", but they receive the sub-screen state.
     * This creates widgets only: it does not assign screen.id/state, save the
     * destination or run the page's workflow. The normal refresh path purges
     * old widgets first. Upload progress can also call this directly through
     * startGUIThread(), temporarily showing progress without changing screen.
     */
    void buildGUI(uint8_t id, uint8_t state){
      // then retrive the screen builder and its overlay by ID
      for(int i = 0; i < screenBuilders.size(); i++){
        screenBuilder* builder = screenBuilders.get(i);
        if (builder->id == id){
          builder->screenCallback(state);
          builder->overlayCallback(state);
          break;
        }
      }
      //DEBUG_PRINT("\nScreen rebuild done");
      #ifdef DEBUG
        // DEBUG INDICATORS OVERLAY
        gui->add(new Label(&gui->framePerSec,Align::Right, &rightScreen, &GUI::three,16,5,&GUI::defaultFcolor,&GUI::defaultBcolor));
        gui->add(new Label(&heapSize,Align::Left,&GUI::zero,&GUI::three,16,24,&GUI::defaultFcolor,&GUI::defaultBcolor));
      #endif
      //DEBUG_PRINT("\nDEBUG DONE");
      //DEBUG_PRINT("\nnew screen widget size: %i",gui->screenGUIs.size());
      if (gui->transparency) gui->clearScreenBuffer();
    }

    bool shutdownDone = false;
    /**
     * @brief End all interfaces and tell the browser that the session is closing.
     */
    void shutdownInterfaces(){
      // called when system stops
      DEBUG_INFO("shuting down interfaces");
      shutdownDone = true;
      for (const auto& interface : interfaceRepo.interfaces){
        interface->end();
      }
      sendTerminateMessageToWebApp(); // tell the webapp to close
    }

    /**
     * @brief Stop interface connections when changing network media.
     */
    void stopInterfaces(){
      // called when network change its media and we need to correcly close all connection
      DEBUG_INFO("stoping down interfaces");
      shutdownDone = true;
      for (const auto& interface : interfaceRepo.interfaces){
        interface->stop();
      }
      sendTerminateMessageToWebApp(); // tell the webapp to close
    }

    /**
     * @brief Run hardware, interface, bank and display updates, then advance the current screen's workflow.
     *
     * Update order: hardware, interfaces, queued browser messages, banks,
     * screen rebuilding/rendering, then the current page's workflow.
     * A transition requested by that final workflow is rendered on the next pass.
     */
    bool refresh(bool force=false){

      yield();

      refreshed = force;

      // ********************************
      // PERFORM ALL REFRESH AND UPDATES
      // ********************************
      hwm->refresh();

      //------------------
      //
      // MANAGE NETWORK
      //
      //------------------
      if (hwm->configMedia->refreshed){
        if (hwm->configMedia->state == CONNECTED){
          // A dedicated configuration medium (for example the fallback AP)
          // is separate from hwm->network, so the regular NetworkDevice event
          // path cannot start the web service. Start it on configMedia.
          if (hwm->configMedia != hwm->network && webAppService != nullptr){
            webAppService->startForConfigMedia();
          }
          if (webAppService != nullptr) webAppService->setWSServer();
        }else{
          webAppConnected = false;
          if (hwm->configMedia != hwm->network && webAppService != nullptr){
            webAppService->handleDisconnectService();
          }
        }
      }

      //------------------
      //
      // MANAGE INTERFACE
      //
      //------------------      

      for (const auto& interface : interfaceRepo.interfaces){
        
        bool dataInOrOut = interface->update();      // return value drive the icon to indicate data being processed        

        Strip* currentStrip = getSelectedStrip();
        if (currentStrip != nullptr && currentStrip->interface == interface){               // the interface that has just been updated is the one used by the focused strip
          interfaceIconColor = QoStoColor(interface->state.QoSlevel);                                   // set color to interface QoS color
          interfaceIconBrightness = dataInOrOut?GUI::systemBrightness*0.4f:GUI::systemBrightness;                  // and blink the icon upon data receive
        }
        
        if (interface->state.broadcastState){
          broadCast(concatStrings("{\"deviceState\":", interface->serializeState(), "}")); // inform web interface
          interface->state.broadcastState = false; // acknowledged
        }
        
        if (interface->state.broadcastConfig){
          broadcastIFconfig(interface);
          interface->state.broadcastConfig = false; // acknowledged
        }

      }

      processBroadcastQueue();

      //------------------
      //
      // MANAGE BANKS STRIPS
      //
      //------------------
      for (const auto& bank : banks) {
        if (bank != nullptr && bank->update() && screen.id == SCREEN_BANK) triggerRedraw();
      }

      //------------------
      //
      // MANAGE GUI
      //
      //------------------
      // screen.value and oldScreen.value each cover four bytes (id, state,
      // focus, reserved). One comparison therefore detects a main-page change,
      // a new sub-screen/bank, or triggerRedraw() changing focus.
      // Reassigning the same values does nothing; no transition queue exists.
      //
      // Hardware/button events handled earlier in this refresh can change the
      // selection before this comparison. The workflow switch below runs after
      // rendering: transitions it requests are normally built on the next pass.
      //
      // On change: delete old widgets, build content/overlay, then copy the
      // resulting selection to oldScreen to acknowledge it. This avoids rebuilding
      // again next time unless another change occurs. Bank selection is saved
      // for restoreScreen(); temporary utility/prompt pages are not saved here.
      //
      // Without a selection change, gui->refresh(force) still updates existing
      // widgets. A changing value/icon does not necessarily need a page rebuild.
      // force requests widget rendering; it does not replace triggerRedraw().
      if (screen.value != oldScreen.value){
        refreshed = true;
        // we changed the screen, therefore we rebuild the screen
        // first we acknowledge the update so it is not done te next round
        //if (debug) DEBUG_PRINT("\n%s changed from screen.id:%i.state:%i.focus:%i to screen.id:%i.state:%i.focus:%i",id.c_str(),oldScreen.id,oldScreen.state,oldScreen.focus, screen.id,screen.state,screen.focus);
        // first clean up all component of the screen
        gui->purge();

        buildGUI(screen.id, screen.state);

        oldScreen.value = screen.value;
        // save the bank to restore in case of restoreScreen
        if (screen.id == SCREEN_BANK){
          hwm->persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_BANK);
          hwm->persistants.putUChar(PERSISTANT_SCREEN_STATE,screen.state);
        }
      }
      gui->refresh(force);

      //------------------
      //
      // MANAGE APPLICATION FLOW
      //
      //------------------

      // Setup flow: language, browser connection, network connection, then the
      // application's own setup callback. Lost connections return to the
      // corresponding earlier step so setup can resume.
      switch(screen.id){
        case SCREEN_WIZARD:
          switch (screen.state){
            case STATE_WIZARD_LANG_STEP1:{
              jumpToMenu(langMenuString);   // jump to the lang menu
              screen.state = STATE_WIZARD_CONNECT_CONFIG_STEP2;
              hwm->persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);
              hwm->persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_CONNECT_CONFIG_STEP2);
            }break;
            case STATE_WIZARD_CONNECT_CONFIG_STEP2:
              handleConfigConnection();
              hwm->persistants.putUChar(PERSISTANT_SCREEN_ID, SCREEN_WIZARD);
              hwm->persistants.putUChar(PERSISTANT_SCREEN_STATE,STATE_WIZARD_CONNECT_NETWORK_STEP3);
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
              if (hwm->configMedia->refreshed ){
                handleConfigConnection();
              }
              if (webAppConnected == true || !webAppService->state.enabled){ // when web app connect we go back to normal screen
                restoreScreen();    // go back to caller. can be from bank/menu or wizard
              }
            break;
            case STATE_UTILS_WEBAPPCONNECTED:
              if (hwm->configMedia->refreshed){
                handleConfigConnection();
              }
              if (webAppConnected == false){
                screen.id = SCREEN_UTILS;
                screen.state = STATE_UTILS_CONNECTWEBAPP;
              }  // when web app connect we go back to normal screen
            break;
            case STATE_UTILS_TRIGGER_FWUPDATE_PROMPT:
              SWupdatePromptScreen(5000);
            break;
            case STATE_UTILS_DISABLE_SWUPDATE_PROMPT:
              systemCfg.setValue(S_SWUPDATE_PROMPT,"false");
              systemCfg.save();
              restoreScreen();
            break;
            case STATE_UTILS_RESTORE_SCREEN:
              restoreScreen();
            break;
            case STATE_UTILS_RESTART_DEVICE  :
              checkAndDeleteFile(FIRMWARE_BIN_FILENAME);
              hwm->persistants.putUChar(PERSISTANT_STARTUP,SYSTEM_RESTART);
              shutdownInterfaces();
              ESP.restart();
            break;
            // Online update: open the package URL, require HTTP 200 and hand
            // the response stream to the archive worker in Interval.h.
            // Extraction success selects CHECK_BINARY: flash packaged firmware
            // if present, otherwise restore the screen. Successful flashing
            // offers a confirmation prompt whose OK action restarts the device.
            case STATE_UTILS_DOWNLOAD_FROM_URL:{

              #define NO_ERROR "ok"

              HttpClient* httpStream = nullptr;
              updateProgressLabel = "...";
              updateSWerrorString = NO_ERROR;
              UrlParts f = parseUrl(fwUpdateURL);

              if (f.ok){
                httpStream = new HttpClient(*hwm->network->client);
                int err = httpStream->startRequest(f.host.c_str(), f.port, f.path.c_str(), "GET", "ESP32HttpClient/1.0");
                if (err != 0) {
                  updateSWerrorString = concatStrings("http request ", String(err));
                }else{
                  err = httpStream->responseStatusCode();
                  if (err != 200) {
                    updateSWerrorString = concatStrings("http response ", String(err));
                    httpStream->stop();
                  }else{
                    err = httpStream->skipResponseHeaders();
                    if (err >= 0){

                      totalGzSize = httpStream->contentLength();

                      setCloudDownloadProgressScreen();

                      StreamParams* params = new StreamParams(httpStream,true); // the task owns the httpStream and will take care of deleting it after it is used

                      // The worker takes ownership of the stream and parameters
                      // only after successful creation; failure cleans them here.
                      BaseType_t taskCreated = xTaskCreate(
                        expandTGZstreamToFSTask,         // Tâche à exécuter
                        "expandTGZstreamToFSTask",       // Nom de la tâche
                        4096*2,             // Taille de la pile de la tâche
                        params,           // Paramètres de la tâche (adresse IP et callbacks)
                        2,                // Priorité
                        nullptr
                      );
                      if (taskCreated != pdPASS) {
                        updateSWerrorString = "Task creation failed";
                        delete httpStream;          // task didn't use the objects so we both delete them ourselve
                        delete params;
                        httpStream = nullptr;
                      }
                    }else{
                      updateSWerrorString = concatStrings("http skip headers error ", String(err));
                    }
                  }
                }
              }else{
                updateSWerrorString = concatStrings("URL parse error: ", fwUpdateURL);
              }

              if (updateSWerrorString != NO_ERROR){
                singlePromptScreen(concatStrings(*lang.getString("Update failed: "),updateSWerrorString),"Ok");
                if (httpStream != nullptr) delete httpStream; // task was unsuccsessul so so we delete the stream ourselves ourselve
              }

            }break;
            case STATE_UTILS_UPDATEFW_FROM_FILEUPLOAD:
              // do nothing
            break;
            case STATE_UTILS_SHOW_PROGRESS_SINGLE:
              // do nothing
            break;
            // Extraction and flashing are separate: a package may update only
            // web assets, without including a firmware binary.
            case STATE_UTILS_CHECK_BINARY:
              checkAndInstallBinaryFromFile();
            break;
            case STATE_UTILS_INFO:

              static uint32_t t = 0;
              if (millis() - t >= 500) {
                t = millis();
                // refresh file size every 500 ms
                LittleFSInfo fsInfo;
                getLittleFSInfo(fsInfo);
                FSstat = concatStrings(("FS: "),String(fsInfo.used/1024),"/",String(fsInfo.total/1024),"KB"," Free Space: ",String(fsInfo.freeSpace/1024),"KB");
              }

            break;
          }
        break;
        case SCREEN_PROMPT:
          handlePromptTimeout();
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
              if (millis()-goSleepCountdownStart>goSleepDurationMs) goDeepSleep();
            break;
            case STATE_BYE_MODAL_MESSAGE:
              // perform some action like update or ftp and shutting down properly (like interfaces...)
              if (!shutdownDone) shutdownInterfaces();
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
      heapSize = concatStrings(minHeapSize, "/", mem, "/", maxHeapSize);

      return refreshed;
    }


    /**
     * @brief Queue a message for connected browsers, or send it immediately when requested.
     */
    void broadCast(String msg, bool immediate=false){
      if (webAppConnected){
        if (immediate) _wssBrodcast(msg);
        else broadcastQueue.push_back(msg);
      }
    }

    /**
     * @brief Send queued browser messages in their original order.
     */
    void processBroadcastQueue() {
      while (!broadcastQueue.empty()) {
        String msg = broadcastQueue.front();  // lire le 1er message
        broadcastQueue.erase(broadcastQueue.begin());  // le retirer de la file
        _wssBrodcast(msg);
      }
    }

    /**
     * @brief Send a WebSocket message when the configuration connection is available.
     */
    void _wssBrodcast(String& msg){
      if (hwm->configMedia != nullptr){
        if (webAppConnected && hwm->configMedia->state == CONNECTED){
          if (webAppService != nullptr && webAppService->wss != nullptr) webAppService->wss->broadcast(WebSocketDataType::TEXT, msg.c_str(), msg.length());
          DEBUG_INFO("[BROADCAST]\t\t(%s)\n\n",msg.c_str());
        }
      }
    }

    /** @brief Reset this object to its initial state.
     *
     * Erase and reinitialize the ESP32 settings partition; files are handled separately.
     */
    void reset(){
      nvs_flash_erase(); // erase the NVS partition and...
      nvs_flash_init(); // initialize the NVS partition.
      // next: delete conf file
    }

    /**
     * @brief Save an incoming stream to the appropriate folder and report transfer progress.
     */
    bool streamToFile(Stream* stream, String filename){

      String path = "/";
      //depending on the file extension or name, we put it to the right place
      if (filename.indexOf(".json.gz") >= 0 || filename.indexOf(".html.gz") >= 0 || filename.indexOf(".ttf.gz") >= 0 || filename == "logo.jpg") path = WEBAPP_PATH"/";

      String fullPath = concatStrings(path,filename);

      File f = LittleFS.open(fullPath, FILE_WRITE);
      if (!f){
        singlePromptScreen("Can't create file","Abort");
        return false;
      }

      LittleFSInfo fsInfo;
      getLittleFSInfo(fsInfo);

      DEBUG_INFO("Free: %i, totalGBsize: %i,  fsInfo.freeSpace - totalGzSize: %i", fsInfo.freeSpace, totalGzSize, fsInfo.freeSpace - totalGzSize);

      if ( (int)((int)fsInfo.freeSpace - (int)totalGzSize) < 1024*8){
        singlePromptScreen("file is too big","Abort");
        return false;
      }

      DEBUG_INFO("streamToFile(%s) %i bytes",fullPath.c_str(),totalGzSize);
      size_t totalWritten = 0;
      uint8_t buf[512];   // 256–1024 selon ta RAM
      while (true) {
        int n = stream->readBytes(buf, sizeof(buf));
        if (n <= 0) break;

        size_t w = f.write(buf, n);
        totalWritten += w;
        delay(1);   // laisse respirer le CPU

        int percentInt = (int)((totalWritten * 100ULL) / totalGzSize);
        updaterProgressCallback(percentInt);
      }
      f.close();
      return true;
    }

    /**
     * @brief Tell the browser immediately that its session is ending.
     */
    void sendTerminateMessageToWebApp(String message = "Finished") {
      // inform web interface immediately without queuing the message for next loop
      broadCast("{\"terminate\":\"" + message + "\"}", IMMEDIATE);
      delay(100);      // let time for the message to flow
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
      else file = LittleFS.open(concatStrings("/webapp", path));

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


    /**
     * @brief Route web commands and uploads, or serve web files with compression when available.
     */
    void commonFileServer(Request &req, Response &res) {
      if (hwm->network->media == CABLE_MEDIA) hwm->network->stopAllUDP();   // because of limited IP sockets in ethernet
      if (webAppService != nullptr) webAppService->reportActivity();
      String path = req.path();

      // --- POST: endpoints spéciaux ---
      if (req.method() == Request::POST) {
        if (path == "/post/") {
          httpPost(req, res);
          return;
        }
        if (path == "/upload") {
          DEBUG_INFO("upload has been CALLED");
          String message = "Abort";
          httpUpload(req, res);
          return;
        }

        res.sendStatus(404);
        res.set("Content-Type", "text/plain");
        res.print("POST not found");
        res.end();
        return;
      }

      // --- GET: serveur de fichiers (ton code inchangé) ---
      if (path == "/") path = "/index.html";

      bool fileIsCompressed = false;

      String gzCandidate = concatStrings(WEBAPP_PATH, path, ".gz");
      if (LittleFS.exists(gzCandidate)) {
        concatString(path, ".gz");
        fileIsCompressed = true;
      }

      String filenameRealExtension = path;

      if (filenameRealExtension.endsWith(".gz")) {
        filenameRealExtension.replace(".gz", "");
        res.set("Content-Encoding", "gzip");
      }

      String fullPath = fileIsCompressed ? gzCandidate : concatStrings(WEBAPP_PATH, path);

      if (!LittleFS.exists(fullPath)) {
        res.sendStatus(404);
        res.set("Content-Type", "text/html");
        res.print(fullPath.c_str());
        res.end();
        return;
      }

      File file = LittleFS.open(fullPath);
      if (file.isDirectory()) {
        res.end();
        return;
      }

      String mimeType = mimeTypes[0][1];
      for (uint8_t i = 0; i < MIME_TYPES_MAX; i++) {
        if (filenameRealExtension.endsWith(mimeTypes[i][0])) mimeType = mimeTypes[i][1];
      }

      res.set("Content-Type", mimeType.c_str());
      res.set("Access-Control-Allow-Origin","*");

      DEBUG_INFO("[WEBAPP]\t\tserving %s",fullPath.c_str());

      sendFileInChunks(&file, &res);
      res.end();
    }

    /**
     * @brief Read an uploaded filename and route the content to an update installer or file storage.
     *
     * Route .tgz to archive extraction/check-binary, .bin.gz directly to the
     * firmware installer, and other filenames to file storage.
     * This already runs in the web task, so installers are called directly.
     * A separate GUI task keeps progress visible during the blocking transfer.
     * The request stream remains owned by the web server.
     */
    bool httpUpload(Request &req, Response &res) {
      // aWOT 3.x: headers accessibles via req.get(...) seulement si app.header(...) a été déclaré
      String ct = String(req.get("Content-Type"));  // ex: multipart/form-data; boundary=----WebKitFormBoundary...
      String message;

      String boundary;
      if (!extractBoundary(ct, boundary)) {
        res.sendStatus(202);
        res.set("Content-Type", "text/plain");
        message = "Missing multipart boundary";
        res.print(message);
        res.end();
        singlePromptScreen(message, "Abort");
        return false;
      }

      const String boundaryLine = String("--") + boundary;

      // 1) aller jusqu'à la première boundary
      String line;
      while (readLine(req, line)) {
        if (line == boundaryLine) break;
      }
      if (line != boundaryLine) {
        res.sendStatus(202);
        res.set("Content-Type", "text/plain");
        message = "Bad multipart start";
        res.print(message);
        res.end();
        singlePromptScreen(message,"Abort");
        return false;
      }

      // 2) lire headers de la part (Content-Disposition) jusqu'à ligne vide
      String cd;
      while (readLine(req, line)) {
        DEBUG_INFO("Line: %s",line.c_str());
        if (line.length() == 0) break;
        if (line.startsWith("Content-Disposition:")) cd = line;
      }

      // 3) extraire filename
      String filename = extractFilenameFromContentDisposition(cd);
      if (filename.length() == 0) {
        res.sendStatus(202);
        res.set("Content-Type", "text/plain");
        message = "Missing filename";
        res.print(message);
        res.end();
        singlePromptScreen(message,"Abort");
        return false;
      }

      // 4) what kind of file is it ?
      UploadDest dest = decideDestination(filename);

      bool processed = false;
      totalGzSize = req.left();
      StreamParams* params = new StreamParams(&req,false);         // we don't own the req stream (it is from the web server AWOT), so we will not delete it ourselves

      // we are in a thread so blocking, calling the normal workflow would have no effect
      // therefore, we use a separate animation thread which do not alter the screen id and screen state that are restored when thread stops
      if (dest == UPLOAD_DEST_TGZUPDATE) {
        iconProgress = ICON_CLOUD_DOWNLOAD;
        startGUIThread(SCREEN_UTILS,STATE_UTILS_SHOW_PROGRESS_MULTIPLE);
        processed = expandTGZstreamToFS(params);     // we call it as a normal task as we are already in a thread from aWot
      }
      else if (dest == UPLOAD_DEST_FWUPDATE){
        setBinaryFlashScreen(); // set label and icon
        startGUIThread(SCREEN_UTILS,STATE_UTILS_SHOW_PROGRESS_SINGLE);
        processed = installGZBinaryFromStream(params);
      }else{
        iconProgress = ICON_CLOUD_DOWNLOAD;
        updateProgressLabel = filename;
        startGUIThread(SCREEN_UTILS,STATE_UTILS_SHOW_PROGRESS_SINGLE);
        processed = streamToFile((Stream*)(&req),filename);
      }

      stopGUIThread();      // back to screen ID

      res.set("Content-Type", "text/plain");
      if (processed) {
        res.print("Download complete");
        //res.sendStatus(200);
      } else {
        res.print("Could not process file");
      }
      res.end();

      return processed;

    }

    typedef enum {
      UPLOAD_DEST_TGZUPDATE,
      UPLOAD_DEST_FWUPDATE,
      UPLOAD_DEST_FSFILE,
    } UploadDest;

    /**
     * @brief Choose archive installation, firmware flashing or file storage from the filename.
     */
    UploadDest decideDestination(const String &filename) {
      if (filename.endsWith(".tgz")) return UPLOAD_DEST_TGZUPDATE;
      if (filename.endsWith(".bin.gz")) return UPLOAD_DEST_FWUPDATE;
      return UPLOAD_DEST_FSFILE;
    }


    /**
     * @brief Read an upload header line, stopping at a newline, length limit or timeout.
     */
    bool readLine(Request &req, String &out, uint32_t timeoutMs = 5000, size_t maxLen = 1024) {
      out = "";
      uint32_t t0 = millis();
      while (millis() - t0 < timeoutMs) {
        while (req.available()) {
          char c = (char)req.read();
          if (c == '\r') continue;
          if (c == '\n') return true;
          out += c;
          if (out.length() >= maxLen) return true;
          delay(1);
        }
        yield();
      }
      return out.length() > 0;
    }

    /**
     * @brief Read the multipart separator from the upload's Content-Type header.
     */
    bool extractBoundary(const String &contentType, String &boundaryOut) {
      boundaryOut = "";
      int idx = contentType.indexOf("boundary=");
      if (idx < 0) return false;
      boundaryOut = contentType.substring(idx + 9);
      boundaryOut.trim();
      if (boundaryOut.startsWith("\"") && boundaryOut.endsWith("\"") && boundaryOut.length() >= 2) {
        boundaryOut = boundaryOut.substring(1, boundaryOut.length() - 1);
      }
      return boundaryOut.length() > 0;
    }

    /**
     * @brief Extract the uploaded filename and remove any client-side folder path.
     */
    String extractFilenameFromContentDisposition(const String &cd) {
      int f = cd.indexOf("filename=");
      if (f < 0) return "";
      String v = cd.substring(f + 9);
      v.trim();

      if (v.startsWith("\"")) {
        int end = v.indexOf("\"", 1);
        v = (end > 1) ? v.substring(1, end) : v.substring(1);
      } else {
        int semi = v.indexOf(';');
        if (semi > 0) v = v.substring(0, semi);
      }

      v.replace("\\", "/");
      int slash = v.lastIndexOf('/');
      if (slash >= 0) v = v.substring(slash + 1);
      return v;
    }


    /**
     * @brief Send a file using transfer sizes suited to Wi-Fi or Ethernet, then close it.
     */
    void sendFileInChunks(File* file, Response* res) {
      size_t chunkedDataSize = 64; // for ethernet: 64 is the good value, 128/256/512 = throw ERR_CONNECTION_REFUSED, file.size() = throw DECODE_ERROR

      if (hwm->configMedia->media == WIFI_MEDIA)
      { // for WiFi we try to send it at once
        chunkedDataSize = file->size();
        if (chunkedDataSize>TCP_SND_BUF) chunkedDataSize = TCP_SND_BUF;   // provided it doesnt exceed the size  of the TCP buffer
      }

      byte *buffer = nullptr; // Pointeur pour la mémoire allouée dynamiquement
      // Allouer dynamiquement de la mémoire pour stocker les données
      buffer = new (std::nothrow) byte[chunkedDataSize]; // +1 pour le caractère null

      int bytesRead;
      // Lire et envoyer des paquets jusqu'à ce que tout le fichier soit envoyé
      while ((bytesRead = file->read(buffer, chunkedDataSize)) > 0) {
        res->write(buffer, bytesRead);  // Écrire le tampon complet dans le client
        //DEBUG_PRINT( ".");
        //taskYIELD(); // ONLY IN A TASK
        //yield();     // ==> when this yield is on, it create CONNECTION_REFUSED on iMac chrome as it slows down the sending of the data
      }

      free(buffer);

      file->close();
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    void render(Menu const& menu) const {}
    /** @brief Placeholder hook; no active operation in this implementation. */
    void render_menu_item(MenuItem const& menu_item) const {}
    /** @brief Placeholder hook; no active operation in this implementation. */
    void render_back_menu_item(BackMenuItem const& menu_item) const {}
    /** @brief Placeholder hook; no active operation in this implementation. */
    void render_numeric_menu_item(NumericMenuItem const& menu_item) const {}
    /** @brief Placeholder hook; no active operation in this implementation. */
    void render_menu(Menu const& menu) const {}

    /** @brief Refresh the current menu icons and label. */
    void menuSystemFullRefresh(){
      //DEBUG_PRINT("\nmenuSystemFullRefresh");
      setRootMenuItems(&ms);
      menuTopText = *lang.getString(" "); // empty message
    }

    /**
     * @brief Prepare the selected menu item's label and the neighboring icons.
     */
    void setRootMenuItems(MenuSystem* m){
      //ms.get_current_menu()->get_current_component(
      Menu const* menu = m->get_current_menu();
      uint8_t numItem = menu->get_num_components();

      MenuComponent const* cp_m_comp;
      uint8_t selection = 0;

      cp_m_comp = menu->get_current_component();
      const char* menuString = cp_m_comp->get_name();
      menuItems[1].icon = getIconFromString(menuString);

      if (cp_m_comp->isEnabled()){
        // if menu item is enabled, we use the glowing color
        GUI::glowingGoldColor.value = &menuActiveIconColor;
        // then based on its internal state (means utcome of the select action which sets the success), we either display it red to indicate an error happens or old
        if (cp_m_comp->success) GUI::glowingGoldColor.baseColor = TFT_GOLD;
        else GUI::glowingGoldColor.baseColor = TFT_RED;
      }else{
        menuActiveIconColor = GUI::menuGreydoutColor;
        GUI::glowingGoldColor.value = &GUI::unUsedColor;
      }

      selection = menu->get_current_component_num();

      //DEBUG_PRINT("\nsetRootMenuItems name:%s numItem:%i currentComponentName: %s, menuIcon:[%s]",menu->get_name(),numItem,menuString, menuItemIcon[1]);

      menuBottomLabel = *lang.getString(getNameFromString(menuString));
      if (numItem == 1){
          menuItems[0].icon = " ";
          menuItems[2].icon = " ";
      }else if (numItem == 2){
        if (selection == numItem-1){
          // left = right = first item
          menuItems[0].icon = getIconFromString(menu->get_menu_component(0)->get_name());
          menuItems[2].icon = getIconFromString(menu->get_menu_component(0)->get_name());

          menuItems[0].color = menu->get_menu_component(0)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;
          menuItems[2].color = menu->get_menu_component(0)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;
        }else{
          // left = right = second ite,
          menuItems[0].icon = getIconFromString(menu->get_menu_component(1)->get_name());
          menuItems[2].icon = getIconFromString(menu->get_menu_component(1)->get_name());

          menuItems[0].color = menu->get_menu_component(1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
          menuItems[2].color = menu->get_menu_component(1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
        }
      }else if (numItem > 2){
        if (selection == numItem-1){ // last item
          // left icon = -1, right icon = first item
          menuItems[0].icon = getIconFromString(menu->get_menu_component(selection-1)->get_name());
          menuItems[2].icon = getIconFromString(menu->get_menu_component(0)->get_name());

          menuItems[0].color = menu->get_menu_component(selection-1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
          menuItems[2].color = menu->get_menu_component(0)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
        }else if (selection == 0){ // first item
          // left icon = last item, right icon = +1
          menuItems[0].icon = getIconFromString(menu->get_menu_component(numItem-1)->get_name());
          menuItems[2].icon = getIconFromString(menu->get_menu_component(selection+1)->get_name());

          menuItems[0].color = menu->get_menu_component(numItem-1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
          menuItems[2].color = menu->get_menu_component(selection+1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
        }else{ // any item in between
          // normal case, we take the -1 and +1 item
          menuItems[0].icon = getIconFromString(menu->get_menu_component(selection-1)->get_name());
          menuItems[2].icon = getIconFromString(menu->get_menu_component(selection+1)->get_name());

          menuItems[0].color = menu->get_menu_component(selection-1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
          menuItems[2].color = menu->get_menu_component(selection+1)->isEnabled()?GUI::menuInactiveIconColor:GUI::menuGreydoutColor;;
        }
      }
    }

    /**
     * @brief Prepare the bank name and neighboring bank icons for the selection menu.
     */
    void setBankMenuItems(){
      Menu const* menu = bankMenu->get_current_menu();
      uint8_t numItem = menu->get_num_components();

      uint8_t selection = menu->get_current_component_num();
      IntervalBank* bank = (selection < banks.size()) ? banks[selection] : nullptr;

      if (bank != nullptr){
        menuBottomLabel = bank->name;
        menuItems[1].icon = bank->icon;

        auto iconOrEmpty = [this](uint8_t index) -> String {
          if (index < banks.size()){
            IntervalBank* candidate = banks[index];
            if (candidate != nullptr) return String(candidate->icon);
          }
          return String(" ");
        };

        if (selection == numItem-1){ // last item
          // left icon = -1, right icon = first item
          menuItems[0].icon = iconOrEmpty(selection > 0 ? selection-1 : selection);
          menuItems[2].icon = iconOrEmpty(0);
        }else if (selection == 0){ // first item
          // left icon = last item, right icon = +1
          menuItems[0].icon = iconOrEmpty(numItem>0 ? numItem-1 : 0);
          menuItems[2].icon = iconOrEmpty(selection+1);
        }else{ // any item in between
          // normal case, we take the -1 and +1 item
          menuItems[0].icon = iconOrEmpty(selection-1);
          menuItems[2].icon = iconOrEmpty(selection+1);
        }
      }else{
        menuBottomLabel = *lang.getString(NO_BANK_MENU_ITEM);
        menuItems[0].icon = " ";
        menuItems[1].icon = ICON_QUIT;
        menuItems[2].icon = " ";
      }
      menuItems[0].color = GUI::menuInactiveIconColor;
      menuItems[1].color = GUI::menuInactiveIconColor;
      menuItems[2].color = GUI::menuInactiveIconColor;
    }

    /** @brief Refresh the bank-choice icons and label. */
    void menuSwitchBankFullRefresh(){
      //DEBUG_PRINT("\nmenuSystemFullRefresh");
      setBankMenuItems();
      //widgets[MENU_ICON_MIDDLE].widget->setFcolor(glowingGoldColor.value); // restore color
      GUI::glowingGoldColor.value = &menuActiveIconColor;
      menuTopText = *lang.getString(" "); // empty message
    }

    /**
     * @brief Toggle directly between two banks, or open the menu when there are more.
     */
    void switchBank(){
      // if we have one bank or two we shift directly
      if (banks.size() == 2) screen.state = (screen.state + 1) % banks.size(); // <=2
      // otherwise we call the menu
      else if (banks.size()>2) screen.id = SCREEN_BANK_SWITCH;
      //else banks are empty, we do nothing
    }

    /** @brief Move strip focus with wraparound and rebuild when the visible layout changes.
     *
     * APPLICATION & WORKFLOW MANAGEMENT METHODS
     * Move strip focus, update the header and redraw when the visible layout changes.
     * Wrap selection from last to first and vice versa. Remove focus from the
     * old strip, then focus the target so it can request its current settings.
     * Rebuild for a new window, a full-size target or zoom mode; otherwise the
     * existing widgets can show the newly selected strip.
     */
    bool stripScroll(uint8_t direction) {

      if (screen.state >= banks.size()) return false;

      IntervalBank* bank = banks[screen.state];

      if (bank == nullptr) return false;
      if (bank->strips.size()<=1) return false;

      uint8_t stripID = bank->selector;

      if (direction == hwm->buttons[RIGHTB]->getID()) {
        if (stripID == bank->strips.size()) stripID = 1;
        else stripID++;
      } else if (direction == hwm->buttons[LEFTB]->getID()){
        if (stripID == 1) stripID = bank->strips.size();
        else stripID--;
      }

      //DEBUG_PRINT("\t scroll(%i/%i):",stripID,bank->nav.window);

      Strip* targetStrip = bank->getStrip(stripID);
      if ( targetStrip == nullptr) return false;

      Strip* currentStrip = getSelectedStrip();
      if (currentStrip != nullptr) {
        currentStrip->focus(false);
      }

      bank->selector = stripID;
      targetStrip->focus(true);

      // drop command to query strip state
      //targetStrip->queryConfig();    inside focus()

      //DEBUG_PRINT("\n==> %s %p" ,targetBank->name.c_str(),targetStrip);
      if (stripHeader != nullptr) stripHeader->strip = targetStrip;

      interfaceIcon = *targetStrip->interface->getIcon();      

      // Force screen redraw if we changed the window or the bank OR if we are displaying full size strip
      if (
          targetStrip->win != currentStrip->win ||
          targetStrip->size == SHAPE4X4 ||
          bank->layout == LAYOUT_ZOOM
      ){
        triggerRedraw(); // force refresh
      }
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

    /**
     * @brief Send the action to the selected strip.
     */
    bool stripAction(uint8_t trigger) {
      if (screen.state >= banks.size()) return false;
      IntervalBank* bank = banks[screen.state];
      if (bank != nullptr){
        Strip* strip = bank->getStrip(bank->selector);
        if (strip != nullptr) return strip->actionCallback(trigger);
      }
      return false;
    }

    /**
     * @brief Send an increase or decrease request to the selected strip.
     */
    void stripChangeValue(uint8_t direction, unsigned long time, uint8_t trigger){

      if (screen.state >= banks.size()) return;
      IntervalBank* bank = banks[screen.state];
      if (bank == nullptr) return;

      Strip* strip = bank->getStrip(bank->selector);
      if (strip == nullptr) return;

      if (direction == hwm->buttons[RIGHTB]->getID()){ strip->increaseCallback(time, trigger);}
      else if (direction == hwm->buttons[LEFTB]->getID()){ strip->decreaseCallback(time, trigger);}


    }

    /** @brief Hold-entry hook; currently only emits a debug message.
     *
     * Placeholder for strip hold-entry handling; currently only logs the event.
     */
    void stripEnterHold(uint8_t direction, unsigned long time, uint8_t trigger){
      DEBUG_WARN("\n\n\n                 ENTER HOLD STRIP              \n\n\n");
    }

    /**
     * @brief Notify the selected strip that the held button was released.
     */
    void stripReleaseHold(uint8_t direction, unsigned long time, uint8_t trigger){
      DEBUG_WARN("\n\n\n                 RELEASE HOLD STRIP              \n\n\n");   
      IntervalBank* bank = banks[screen.state];
      if (bank != nullptr){
        Strip* strip = bank->getStrip(bank->selector);
        if (strip != nullptr) return strip->onButtonReleased();
      }         
    }



    /**
     * @brief Apply strip ordering received from the browser and recover an invalid selection.
     */
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
      if (bank->getStrip(bank->selector) == nullptr) bank->init();
      return success;
    }

    /**
     * @brief Schedule a restart through the utility-screen workflow.
     */
    void restart(){
      screen.id = SCREEN_UTILS;
      screen.state = STATE_UTILS_RESTART_DEVICE;
    }

    ////////////////////////////////////////////////////////////////////////////////////

    #define BAD_BANK_MSG "Bad bank"
    #define BAD_STRIP_MSG "Bad strip"
    #define ERRDEL_STRIP_MSG "Delete strip"

    #define RET_UNDPROCESSEDJSON   "undefinedCallBack"
    #define RET_SAVEBUTTON          "setSaveButton"
    #define RET_COMMANDERROR        "commandError"
    #define RET_POPUP               "commandPopup"

    /**
     * @brief Send a log message to the browser console.
     */
    void log(const char* originator, const char* message) {
        const char* logLinePattern = "{\"logWebConsole\":{\"originator\":\"%s\",\"message\":\"%s\"}}";

        // Calcul de la taille nécessaire
        size_t needed = snprintf(nullptr, 0, logLinePattern, originator, message) + 1;

        char* sendLine = (char*) malloc(needed);
        if (!sendLine) return; // sécurité si malloc échoue

        snprintf(sendLine, needed, logLinePattern, originator, message);

        broadCast(sendLine);
        free(sendLine);
    }


    /** @brief Return the current network description as JSON. */
    String getConnectivityState(){
      return hwm->network->serialize();
    }

    /** @brief Handle a browser command and return the callback/data JSON reply.
     *
     * Process a browser command and build its reply; pass unknown commands to the user callback.
     * retrieve reads configuration; bank/strip edits generally change memory
     * and mark the browser's save button. System settings and interface settings
     * have their own immediate-save paths. Explicit save writes profile banks
     * and settings; flash schedules the online update through the screen workflow.
     * Unknown commands go to userParseJsonInputCallback. Replies use a callback
     * name as the JSON key so the browser knows what to refresh.
     */
    String parseJsonInput(String input){

      JsonDocument jsonInput;
      deserializeJson(jsonInput, input);

      jsonAnswer.callback = RET_UNDPROCESSEDJSON;
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

        if (param == "startup"){
          getJsonStartup(&jsonAnswer.value);
        }else if (param == "connectivity"){
          jsonAnswer.value = getConnectivityState();
        }else if(param == "fullConfig"){
          jsonAnswer.callback = bulkconfig;
          serialize(&jsonAnswer.value);
        }else if(param == "banks"){
          refreshBanksResponse();
        }
      }else if (command == "set"){
        //{"command":"set","param":{"section":"system","id":"pushDelay","value":"3"}}
        String id = jsonInput["param"]["id"];
        String value = jsonInput["param"]["value"];
        //DEBUG_PRINT("\t => section:%s id:%s value:%s",section.c_str(),id.c_str(),value.c_str());
        bool success = systemCfg.setValue(id,value);
        if (success){
          applySystemSettings();      // ===>  propagate config values by giving reference to conf object using parameters instead of using local variable
          systemCfg.save();
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value ="0";      // auto saved
        }else{
          jsonAnswer.callback = RET_COMMANDERROR;
          jsonAnswer.value = jsonInput.as<String>();
        }
      }else if (command == "scanWifi"){        // Create the parrallel scan task
        DEBUG_INFO("[WiFi Scan]\t\tstart\n\n");
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
        if (ssid.length() == 0){
          jsonAnswer.callback = RET_COMMANDERROR;
          jsonAnswer.value = "setWifi requires an SSID";
        }else{
          // Persist the selected network immediately. This is deliberately
          // independent of the connection attempt so an AP configuration can
          // be completed now and applied by the STA at the next boot.
          const bool dedicatedConfigMedia =
            hwm->configMedia != nullptr && hwm->configMedia != hwm->network;
          if (dedicatedConfigMedia){
            hwm->network->rememberSSID(ssid,pwd);
          }else{
            hwm->network->setSSID(ssid,pwd);
          }

          String wifiCredentials;
          wiFiList.serialize(&wifiCredentials);
          const bool saved =
            systemCfg.setValue(S_SSID,wifiCredentials) &&
            systemCfg.setValue(S_LAST_USED_SSID,ssid) &&
            systemCfg.save();

          if (!saved){
            jsonAnswer.callback = RET_COMMANDERROR;
            jsonAnswer.value = "Unable to save WiFi credentials";
          }else if (dedicatedConfigMedia){
            // Keep the AP and its WebGUI session intact. The saved STA
            // configuration is applied during the next normal startup.
            jsonAnswer.callback = RET_POPUP;
            jsonAnswer.value = "wifiSaved";
          }else if (hwm->network->media != CABLE_MEDIA){
            jsonAnswer.callback = "wifiConnectTimeout"; // we give a try to connect wifi by setting up the timeout
            jsonAnswer.value = hwm->network->wiFiTimeout;
          }
          // if media is ethernet, we just feedback save button
          else{
            jsonAnswer.callback = RET_POPUP;
            jsonAnswer.value ="wifiSaved";
          }
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
            // remove focus on selected strip
            Strip* currentStrip = getSelectedStrip();
            if (currentStrip != nullptr) currentStrip->focus(false);
            // and put the focus on the new strip
            bank->selector = strip->index;
            getBank(bank->name,&screen.state);
            screen.id = SCREEN_BANK;
            triggerRedraw();
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
          if (stripToDelete != nullptr){
            uint8_t indexToDelete = stripToDelete->index;
            Strip* currentStrip = getSelectedStrip();
            if (currentStrip !=nullptr){
              if (bank->trash(stripToDelete)){

                  updatePositions(bank,jsonInput["param"][C_POSITIONS]);
                  jsonAnswer.callback = RET_SAVEBUTTON;
                  jsonAnswer.value = "1";
                  // we check if we delete the strip which is currently focused
                  if (indexToDelete == currentStrip->index){
                    DEBUG_INFO("DELETED FOCUSED STRIP");
                    restoreScreen();
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
            //WARNING THIS IS THE DESTINATION POSITION!!!!!!
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
        IntervalBank* bankToDelete = getBank(bankName, &bankIndex);  // get the bank and its index

        if (bankToDelete != nullptr) {

          // 1. Libérer l'objet AVANT d'effacer son pointeur du vecteur
          delete banks[bankIndex];
          banks[bankIndex] = nullptr;

          // 2. Retirer l'entrée du vecteur
          banks.erase(banks.begin() + bankIndex);

          // 3. Gérer l'affichage ou la banque active
          if (bankIndex == screen.state || screen.id == SCREEN_BANK_SWITCH) {
            DEBUG_INFO("DELETE CURRENT BANK, jumping to bank 0");
            screen.state = 0;
            triggerRedraw();
          } else {
            DEBUG_INFO("Bank deleted");
          }

          refreshBankMenu();   // rebuild the bank switch menu

          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "1";

        } else {
          jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
        }

      }else if (command == "newBank"){
        //{"command":"newBank","param":{"newName":{}}}
        bool ret = false;
        String jsonBankstring = jsonInput["param"];
        IntervalBank* bank = deserializeBanks(&jsonBankstring);
        if (bank != nullptr){
          ret = true;
          getBank(bank->name,&screen.state);
          triggerRedraw();
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
          refreshBankMenu();
        }else jsonAnswer.value = *lang.getString(BAD_BANK_MSG);
      }else if (command == "save"){

        String bankFileName = concatStrings(concatStrings("/", *systemCfg.getString(S_PROFILE, DEFAULT_PROFILE)), ".bank");
        bool ret = saveBanks(bankFileName);
        ret = systemCfg.save();
        ret &= userCfg.save();
        if (ret){
          jsonAnswer.callback = RET_SAVEBUTTON;
          jsonAnswer.value = "0";
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
        }
      }else if (command == "restart"){
        restart();
        jsonAnswer.callback = RET_SAVEBUTTON;
        jsonAnswer.value = "0";
      }else if (command == "flash"){
        screen.id = SCREEN_UTILS;  // go to utils screen to perform the update
        screen.state = STATE_UTILS_DOWNLOAD_FROM_URL;
        jsonAnswer.callback = "save";
        jsonAnswer.value ="0";
      }else if (command == INTERFACE_CONFIG){
        // {"command":"deviceConfig","param":{"MixingStation":{"oscIP":"999.999.999.999","oscPort":"99999"}}}
        // Accès à l'objet "param"
        JsonObject param = jsonInput["param"];
        // Récupération du premier élément de "param"
        JsonPair first = *param.begin();
        // Nom de la clé
        const char* interfaceName = first.key().c_str();
        DeviceInterface* interface = interfaceRepo.retrieve(interfaceName);

        if (interface && interface->interfaceName == interfaceName){
          // we found the right interface
          // Valeur = parameters
          JsonObject values = first.value().as<JsonObject>();
          if (interface->restore(values)){
            if (interface->save()){
              jsonAnswer.callback = RET_SAVEBUTTON;
              jsonAnswer.value ="0";
            }else{
              jsonAnswer.callback=RET_COMMANDERROR;
              jsonAnswer.value = "interface save error";
            }
          }else{
            DEBUG_INFO("Interface no change");
            interface->handleRequestRefreshState(); // to call for specific things like refreshing the exposed params list
            jsonAnswer.callback=RET_SAVEBUTTON;
            jsonAnswer.value ="0";
          }
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
          jsonAnswer.value = concatStrings(*lang.getString("Wrong Interface"), ":", interfaceName);
        }
      /*
      if (config.containsKey(INTERFACE_ENABLED)){
        bool enabled = config[INTERFACE_ENABLED];
        switchDevice(state.enabled);
      }*/
      }else if (command == "deviceEnable"){
        // {"command":"deviceEnable","param":{"MixingStation":true}}
        // Accès à l'objet "param"
        JsonObject param = jsonInput["param"];
        // Récupération du premier élément de "param"
        JsonPair first = *param.begin();
        // Nom de la clé
        const char* interfaceName = first.key().c_str();
        DeviceInterface* interface = interfaceRepo.retrieve(interfaceName);
        if (interface && interface->interfaceName == interfaceName){
          bool enabled = first.value();
          interface->switchDevice(enabled);
          interface->state.broadcastState = true;
          if (interface->save()){
            jsonAnswer.callback = RET_SAVEBUTTON;
            jsonAnswer.value ="0";
          }else{
            jsonAnswer.callback=RET_COMMANDERROR;
            jsonAnswer.value = "interface save error";
          }
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
          jsonAnswer.value = "interface not found";
        }
      }else if (command == "saveLibrary"){
        // {"command":"saveLibrary","param":{"file":"sysexlib.json","entries":{"abc":"aa bb","key2":"01bb","key3":"1ef0","ghijk":"12 34 56"}}}}
        String fileName = jsonInput["param"]["file"];
        String entries = jsonInput["param"]["entries"];

        //write the data with compression
        if (writeBufferToGzFile(concatStrings(WEBAPP_PATH, "/", fileName, ".gz"), entries.c_str()) != 0){
          bool success;
          // reload the datasets
          if (fileName == SYSEXLIB_FILENAME){
            success = dsSysexMessages->reload();
          }else if (fileName == OSCLIB_FILENAME){
            success = dsOSCMessages->reload();
          }
          if (!success){
            jsonAnswer.callback=RET_POPUP;
            jsonAnswer.value = "Data reload error";
          }else{
            // then resend the banks with updated datasets
            refreshBanksResponse();
          }
        }else{
          jsonAnswer.callback=RET_COMMANDERROR;
          jsonAnswer.value = "File Error";
        }
      }else{
        //DEBUG_PRINT("\n\t\t\t\t\tCalling userParseJsonInputCallback...");
        JsonObject j = jsonInput.as<JsonObject>();
        if (!userParseJsonInputCallback(j,&jsonAnswer)){

          /*JsonUserEvent je;
          je.jsonAnswer = &jsonAnswer;
          je.jsonInput = jsonInput;

          evt.type = EVENT_PARSE_JSONINPUT;
          evt.data = &je;

          if (!fireEvent(&evt)){*/
            jsonAnswer.callback = RET_UNDPROCESSEDJSON;
            jsonAnswer.value = jsonInput.as<String>();
          //}
        }
      }


      JsonDocument jsonOutput;
      jsonOutput[jsonAnswer.callback] = jsonAnswer.value;

      String jsonString;
      serializeJson(jsonOutput,jsonString);
      removeJsonQuote(&jsonString);

      //DEBUG_PRINT("\nbuildJsonOuput: %s[...]",jsonString.substring(0,100).c_str());
      jsonOutput.shrinkToFit();
      return jsonString;

    }
    
    /**
     * @brief Prepare updated banks and strip choices for the browser.
     */
    void refreshBanksResponse(){
      JsonDocument doc;

      /*JsonArray ds = doc[stripdatasets].to<JsonArray>();
      dataRepo.serialize(&ds); */
      JsonObject ds = doc.createNestedObject(stripdatasets);
      dataRepo.serialize(ds);


      String banksString = serializeBanks();
      doc["banks"]=banksString;

      //{bulkConfig:{$datasets,$banks}}';
      jsonAnswer.callback = bulkconfig;

      serializeJson(doc, jsonAnswer.value);
      removeJsonQuote(&jsonAnswer.value);      
    }


    /** @brief Write text to a LittleFS file and close it. */
    bool writeStringtoFile(const String &filename, String& entries) {
      File file = LittleFS.open(filename, "w");
      if (!file) return false;
      file.print(entries);
      file.close();
      return true;
    }

    /**
     * @brief Scan nearby Wi-Fi networks and broadcast up to ten network names.
     */
    void scanNetworksTask(void * parameter) {
      #define MAX_NUMBER_OF_NETWORK_TO_REPORT 10
      //DynamicJsonDocument doc(60+50*MAX_NUMBER_OF_NETWORK_TO_REPORT);
      JsonDocument doc;
      //JsonArray scannedWifi = doc.createNestedArray();
      JsonArray scannedWifi = doc["scannedWifi"].to<JsonArray>();
      //DEBUG_PRINT("\nScanning for WiFi networks...");
/*      WiFi.mode(WIFI_STA); // dixit chatgpt mais d'apres mes tests ce n'est pas necessaire
      vTaskDelay(500);*/
      int numberOfNetworks = WiFi.scanNetworks();
      DEBUG_INFO("[WiFi Scan]\t\tcomplete\n\n");
      for (int i = 0; i < numberOfNetworks && i<MAX_NUMBER_OF_NETWORK_TO_REPORT; i++) {
        //DEBUG_PRINT("\nNetwork name: %s",WiFi.SSID(i).c_str());
        scannedWifi.add(WiFi.SSID(i));
        vTaskDelay(pdMS_TO_TICKS(1));
        String ssid = WiFi.SSID(i);         // Nom du réseau (SSID)
        int32_t rssi = WiFi.RSSI(i);        // Puissance du signal (RSSI)
        int32_t channel = WiFi.channel(i);  // Canal utilisé
        bool isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

        // Affichage des infos du réseau
        /*DEBUG_PRINT("\nRéseau #%d :", i + 1);
        DEBUG_PRINT("\n\tSSID\t\t\t\t: %s", ssid.c_str());
        DEBUG_PRINT("\n\tRSSI\t\t\t\t: %d dBm", rssi);
        DEBUG_PRINT("\n\tCanal\t\t\t\t: %d", channel);
        DEBUG_PRINT("\n\tSécurisé\t\t\t: %s\n", isEncrypted ? "Oui" : "Non");
        */

      }
      String data;
      WiFi.scanDelete();
      serializeJson(doc,data);
      doc.shrinkToFit();
      broadCast(data);     // broadcasting uses a shared network resources and should be protected by a mutex
      // instead of broadcasting, just queue it so it is sent to the next processing request
    }


    /** @brief Find a borrowed bank by name and optionally return its vector position.
     *
     * Find a bank by name and optionally return its position; the returned pointer is borrowed.
     */
    IntervalBank* getBank(const String& name, uint8_t* index) {
        if (banks.empty()) return nullptr;

        for (uint8_t i = 0; i < banks.size(); i++) {
            // Avec unique_ptr on accède au pointeur via get()
            IntervalBank* bank = banks[i];
            if (bank && bank->name == name) {
                if (index) *index = i;
                return bank; // OK : pointeur non possédé, juste une vue
            }
        }
        return nullptr;
    }

    /**
     * @brief Process browser commands from POST data and send their replies as gzip.
     */
    void httpPost(Request &req, Response &res){

      //char name[MAX_POST_COMMAND_SIZE]; //
      char* name = (char*) malloc(MAX_POST_COMMAND_SIZE);

      #define OPTION_SIZE 128
      char options[OPTION_SIZE];    //options is not used (the fetch from client puts everything in the name as a json)

      res.set("Content-Type", "application/json");


      unsigned long t0 = millis();
      #define POST_TIME_OUT 5000

      while (req.left() && millis()-t0<POST_TIME_OUT) {
        if (!req.form(name, MAX_POST_COMMAND_SIZE, options, OPTION_SIZE)){
            res.sendStatus(400); // Internal Server Error
            String err = "{\"error\": \"value-pair oversize or missing\"}";
            DEBUG_WARN("\t%s",err.c_str());
            res.print(err.c_str());
            continue; // Ignorer cette itération
            //return;
        }

        // Vérification de débordement et de validité
        if (strlen(name) >= MAX_POST_COMMAND_SIZE || strlen(options) >= OPTION_SIZE) {
            res.sendStatus(500); // Internal Server Error
            String err = "{\"error\": \"Parameters too long\"}";
            DEBUG_WARN("\t%s",err.c_str());
            res.print(err.c_str());
            continue; // Ignorer cette itération
        }

        DEBUG_INFO("[HTTP-POST]\t\treceived <%s><%s>\n\n",name,options);
        String jsonAnswer = parseJsonInput(name);

        // version compressée
        ByteBuffer compressedBuffer;
        size_t chunkedDataSize = bufferToGz(&jsonAnswer,&compressedBuffer);
        res.set("Content-Encoding", "gzip"); // Indique que la réponse est en gzip
        #define PACKET_SIZE 64 // Taille d'un paquet (64 octets)
        uint8_t packet[PACKET_SIZE];             // Crée un buffer temporaire de 64 octets
        //DEBUG_PRINT("\tsending compressed data:");
        while (compressedBuffer.available()) {
          size_t bytesToRead = min(PACKET_SIZE, compressedBuffer.available()); // Détermine le nombre d'octets à lire
          compressedBuffer.readBytes((char*)packet, bytesToRead);
          res.write(packet, bytesToRead);
          //DEBUG_PRINT("%dx...",bytesToRead);
          yield(); //
        }

        /*
        // avoid using the ByteBuffer class =>
          uint8_t* dstBuf = NULL;
          size_t dstBufLen = LZPacker::compress( srcBuf, srcBufLen, &dstBuf );
          // do something with dstBuf
          free(dstBuf);
        */

        // version non  compressée
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
      free(name);
      res.end();
      //DEBUG_PRINT("\tend()");
    }

    /*void startDirectoryServiceDiscovery(){
      // SCAN FOR APPLE MIDI SESSION ON THE NETWORK
      rtpDir.clear();
      if (hwm->network->media == CABLE_MEDIA){
        // Découvre le type de service "_apple-midi" en UDP ethernet = non bloquant
        EthernetBonjour.startDiscoveringService("_apple-midi", MDNSServiceUDP, 5000); // :contentReference[oaicite:3]{index=3}
        DEBUG_PRINT("\n\n[RTP-bonjour]\t\tScan RTP-MIDI lancé...\n");
      }else{
        // Wifi = bloquant
        xTaskCreate(
          WiFiRTPDiscoveryTask,  // Function to implement the task
          "WiFiRTPDiscoveryTask",     // Name of the task
          4096,              // Stack size in words
          NULL,               // Task input parameter
          1,                  // Priority of the task
          NULL                // Task handle
        );
      }
    }*/

/*    void epDirectoryUpdated(){
      DEBUG_PRINT("\n\n[RTP]\t\tScan terminé (timeout).");
      DEBUG_PRINT("\n[RTP]\t\t%d endpoint(s) mémorisé(s):\n", rtpDir.epCount);
      for (int i = 0; i < rtpDir.epCount; i++) {
        DEBUG_PRINT("\n  #%d  %s  %s:%u\n", i+1, rtpDir.ep[i].name, rtpDir.ep[i].ip.toString().c_str(), rtpDir.ep[i].port);
      }
      // midi interface has to update its config and regenerate its state
      appleMIDIInterface->handleRefresh();
    }
*/

  private:
    HardwareManager* hwm;
    GUIManager* gui;
    LinkedList<screenBuilder*> screenBuilders = LinkedList<screenBuilder*>();

    JsonAnswer jsonAnswer;

    JsonList wiFiList;


    String defaultSysConf =
       "{\"" S_PUSHDELAY "\":\"" +String(DEFAULT_PUSH_DELAY)+ "\","
        "\"" S_DCLICREACTIVITY "\":\"250\","
        "\"" S_PROFILE "\":\"" DEFAULT_PROFILE "\","
        "\"" S_AP_NAME "\":\"" INTERVAL_PRODUCT_NAME "\","
        "\"" S_LAST_USED_SSID "\":\"intervalmix1\","
        "\"" S_LANG "\":\"francais\","
        "\"" S_HOSTNAME "\":\"" INTERVAL_DEFAULT_HOSTNAME "\","
        "\"" S_SWUPDATE_PROMPT "\":\"true\","
        "\"" S_FADERSPEED "\":\"157\","
        "\"" S_SSID "\":{\"IpTfr\":\"jt2p9ug1\",\"infrafast\":\"Mf9k8iie2\",\"intervalmix\":\"intervalpop74\",\"IpTch\":\"jt2p9ug1\"},"
        "\"" S_BUTTON_ASSIGN "\":{\"left\":[3,1,5,8],\"right\":[2,1,4,9],\"tap\":[6,7,8,10]},"
        "\"" S_SHORTCUTS "\":{\"shortcut1\":{\"type\":\"jump\",\"properties\":[]},"
                             "\"shortcut2\":{\"type\":\"macro\",\"properties\":[\"/action/FF\",\"/macro/name/colors\"]},"
                             "\"shortcut3\":{\"type\":\"advanced\",\"properties\":[\"/command//ch/level/03\"]}}"
        "}";

    String defaultUserConf = "{}";

    // Last acknowledged screen selection; compared with the global screen each refresh.
    ScreenUnion oldScreen;

    //LinkedList<String*> countryList = LinkedList<String*>();

    //LinkedList<String*> webGZFiles = LinkedList<String*>();
    //LinkedList<String*> webFiles = LinkedList<String*>();


    uint32_t minHeapSize,maxHeapSize;

    //////////////////////////////////////////////////////////////////////

    /**
     * @brief Send updated interface settings together with their configuration choices.
     *
     * Send settings together with their choices: changing interface settings
     * can also change the choices available in the browser.
     */
    void broadcastIFconfig(DeviceInterface* interface){
      //broadCast(concatStrings("{\"deviceCfg\":", interface->serializeConfig(), "}")); // inform web interface
      // when broadcasting the config you also have to broadcast the interface data sets as it is likely to have been updated
      JsonDocument doc;

      JsonObject ifds = doc.createNestedObject(ifdatasets);
      IFdataRepo.serialize(ifds);

      JsonObject device = doc.createNestedObject("device");

      JsonDocument ifJson;
      interface->serialize(ifJson);
      // copie du contenu dans "device"
      device.set(ifJson.as<JsonObject>());

      jsonAnswer.callback = bulkconfig;

      serializeJson(doc, jsonAnswer.value);
      removeJsonQuote(&jsonAnswer.value);

      JsonDocument jsonOutput;
      jsonOutput[jsonAnswer.callback] = jsonAnswer.value;

      String jsonString;
      serializeJson(jsonOutput,jsonString);
      removeJsonQuote(&jsonString);

      jsonOutput.shrinkToFit();
      broadCast(jsonString);
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
    MenuItem* fmw_update_menu_item;

    /**
     * @brief Rebuild the bank-switch menu from the current bank names.
     */
    void refreshBankMenu(){
      if (bankMenu != nullptr) delete bankMenu;
      bankMenu = new MenuSystem(*this);

      if (banks.size()>1){
        // populate the menu with all banks
        for(int i = 0; i < banks.size(); i++){
          const char* bankName = banks[i]->name.c_str();
          bankMenu->get_root_menu().add_item(new MenuItem(bankName, &switchToBank_item_selected));
          if (debug){
            //DEBUG_PRINT("\nadded bank switch item entry: %s",bankName);
          }
        }
      }else{
        if (banks.size() == 1){
          bankMenu->get_root_menu().add_item(new MenuItem(banks[0]->name.c_str(), &switchToBank_item_selected));
          DEBUG_INFO("Menu bank one item: %s",banks[0]->name.c_str());
        }else{
          bankMenu->get_root_menu().add_item(new MenuItem(lang.getString(NO_BANK_MENU_ITEM)->c_str(),&back_menu_item_selected));
          DEBUG_WARN("Empty menu bank");
        }
      }
    }

};

#endif
