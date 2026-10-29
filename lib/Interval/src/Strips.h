#ifndef STRIPSDEF_h
#define STRIPSDEF_h

/**
 * @file Strips.h
 * @brief Defines the controls placed inside a bank.
 *
 * A strip combines a device action, saved settings and a visual representation.
 * For example, it can control a channel level or send a MIDI message.
 * Skeletons describe the available strip types; the factory creates them when
 * loading a bank. Concrete strips provide the device-specific actions and widgets.
 */
#include "Interval.h"
#include "GUIComponents.h"
#include <vector>

//using IntervalButton = Interval::Button;

//-------------------------

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

#define MAXNUMBER_OF_SKELPROPERTY 10

/**
 * @brief Description of a strip type for the factory and configuration UI.
 *
 * Its name is the persistent "type" key, its group identifies compatible
 * interfaces, and its ordered addresses describe configurable properties.
 * It describes a type, not an individual strip placed in a bank.
 */
class StripSkeleton: public SerializableParameters{
  public:
    unsigned char icon;
    StripBuilderCallback constructor;
    //JsonObject variable;
    using SerializableParameters::serialize;  // Keep the inherited String serialize() overload available.

    /** @brief Initialize StripSkeleton with the supplied configuration and bindings. */
    StripSkeleton(String id, String g, StripBuilderCallback b, unsigned char i='!');

    /**
     * @brief Extend the common editor description with an icon.
     */
    void serialize(JsonObject& json) override {
        // Export the common fields first.
        SerializableParameters::serialize(json);
        // Add strip-specific metadata.
        json[C_ICON] = icon;
    }

};


/**
 * @brief Finds a registered type and calls its constructor callback.
 *
 * Borrows skeleton pointers; callers own the strips returned by build().
 * Construction and loading settings are separate: Bank::addStrip() does both.
 * Keys must stay stable so saved banks can still reconstruct their strips.
 */
class StripFactory{
  public:
    std::vector<StripSkeleton*> stripSkeletonRepo;

    /**
     * @brief Register a type without checking duplicate names. Lookups use the first match.
     */
    bool add(StripSkeleton* ss){
      stripSkeletonRepo.push_back(ss);
      return true;
    }

    /** @brief Remove the matching entry from the registry.
     *
     * Remove the first entry with the same name; does not delete the skeleton.
     */
    void remove(StripSkeleton* ss) {
      for (auto it = stripSkeletonRepo.begin(); it != stripSkeletonRepo.end(); ++it) {
        if ((*it)->name == ss->name) {
          stripSkeletonRepo.erase(it);
          break;
        }
      }
    }

    /** @brief Create a strip from its type key, or nullptr for an unknown type. */
    Strip* build(String stripID/*, String values*/){
      Strip* strip = nullptr;
      StripSkeleton* skel = getStripSkeleton(stripID);
      if (skel != nullptr){
        strip = skel->constructor(/*values*/);
      }
      return strip;
    }

    /** @brief Return the registered strip-type description as JSON text. */
    String getDefinition(String stripID){
      String stringDef = "{\"null\":0}";
      StripSkeleton* skel = getStripSkeleton(stripID);
      if(skel != nullptr) stringDef = skel->serialize();
      return stringDef;
    }

    /**
     * @brief Export type descriptions as JSON strings for the web configuration.
     */
    void serialize(JsonArray* jsonSkeletons){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* skeleton = stripSkeletonRepo[i];
        String stripName = skeleton->name;
        jsonSkeletons->add(skeleton->serialize());
      }
    }

  private:
    /** @brief Find the first registered strip type with the requested name. */
    StripSkeleton* getStripSkeleton(String stripID){
      for (uint8_t i=0;i<stripSkeletonRepo.size();i++){
        StripSkeleton* s = stripSkeletonRepo[i];        
        if (stripID == s->name){
          return s;
        } 
      }
      return nullptr;
    }
}stripFactory;

/** @brief Initialize StripSkeleton with the supplied configuration and bindings.
 *
 * Register on construction. The icon argument is currently not assigned to
 * the icon member here; registration itself does not initialize that field.
 */
StripSkeleton::StripSkeleton(String id, String g, StripBuilderCallback c, unsigned char i):SerializableParameters(id,g, &dataRepo),constructor(c){
  stripFactory.add(this);
}

/* HELPERS BUTTON */
// ----- Helpers -----
/** @brief Clamp a normalized value to the inclusive 0..1 range. */
float clamp01(float v) { return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; }

/** @brief Evaluate one coordinate of a cubic Bezier curve from zero to one. */
float bez1D(float t, float a1, float a2) {
  float omt = 1.0f - t;
  return (3.0f * omt * omt * t * a1) + (3.0f * omt * t * t * a2) + (t * t * t);
}

/** @brief Evaluate the derivative used to solve a cubic Bezier time coordinate. */
float bez1D_d(float t, float a1, float a2) {
  float omt = 1.0f - t;
  return 3.0f * omt * omt * a1
       + 6.0f * omt * t * (a2 - a1)
       + 3.0f * t * t * (1.0f - a2);
}

/**
 * @brief Solve for y given x on cubic-bezier(x1,y1,x2,y2) (CSS-like)
 * Convert elapsed progress x into eased progress y. A Bezier curve is defined
 * by a parameter t, so first solve x(t)=x with up to six Newton iterations,
 * then ten bisection refinements. Evaluate y(t) and clamp the result to 0..1.
 * Intended for easing curves whose x coordinates give a monotonic time axis.
 */
float bezYforX(float x, float x1, float y1, float x2, float y2) {
  x = clamp01(x);

  float t = x; // seed
  for (int i = 0; i < 6; i++) {
    float bx = bez1D(t, x1, x2);
    float dx = bez1D_d(t, x1, x2);
    float err = bx - x;
    if (fabsf(err) < 1e-5f) break;
    if (fabsf(dx) < 1e-6f) break;
    t -= err / dx;
    t = clamp01(t);
  }

  // quick bisection fallback/cleanup
  float lo = 0.0f, hi = 1.0f;
  for (int i = 0; i < 10; i++) {
    float bx = bez1D(t, x1, x2);
    if (bx < x) lo = t; else hi = t;
    t = 0.5f * (lo + hi);
  }

  return clamp01(bez1D(t, y1, y2));
}


//////////////////// STRIP MANAGEMENT ////////////////////////

#define LAYOUT_ZOOM     0b00000001
#define LAYOUT_DETAILS  0b00000010

#define SHAPE4X4 "4x4"
#define SHAPE4X1 "4x1"
#define SHAPE1X4 "1x4"
#define SHAPE2X2 "2x2"

float oneHundedFloat = 100.0;

/**
 * @brief Base behavior shared by strips: interface events, button value changes,
 * configuration loading/saving and widget placement.
 * A bank owns the strip; the strip borrows its skeleton and device interface.
 * Widgets added to gui are managed by the GUI. Their pointers into strip values,
 * labels and colors require the strip and its backing data to remain alive.
 */
class Strip: public EventListener{  
  #define JSON_STRIP_PROPERTY_SIZE 100
  #define  JSON_STRIP_TEMPLATE_SIZE 300
  public:
    TFT_eSprite* sprite = nullptr;

    /** @brief Return the name used to identify this listener in logs. */
    virtual const char* getListenerName()  override{
      return getTopLabel()->c_str();
    }    

    /**
     * @brief flag to indicate if position is out of range
     */
    bool renderError;
    //bool redraw = false;

    bool* toggle1 = &GUI::on;      ///< mute off
    bool* toggle2 =  &GUI::on;      ///< mute off
    
    float innerValue = 0.5f;  ///< default value that need to be overriden by derived strip
    // Defaults to local storage; derived strips may bind shared device data.
    // This pointer and the toggle pointers do not transfer ownership.
    float* value = &innerValue; 
    
    
    //bool updated = false;
    uint16_t frontColor, backColor;    

    String iconInterfaceConnect = ICON_FOOTSWITCH;

    String type="undef";

    String interfaceName = "interfaceName"; //
    DeviceInterface* interface = nullptr;

    // Visible means included in the displayed bank view; focused means selected.
    // Several strips may be visible in details mode, but selection is separate.
    bool visible = false;
    bool focused = false;

    /**
     * @brief Logical placement: x/y on the four-by-four grid, zero-based window,
     * and one-based strip id used by bank selection (not a vector offset).
     */
    uint8_t x, y, win, index;
    
    /**
     * @brief top left corner position on TFT screen
     */
    int16_t topLeftX, topLeftY;
    
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    float brightness = BRIGHTNESS_NOFOCUS;

    /** @brief Initialize Strip with the supplied configuration and bindings. */
    Strip(StripSkeleton* s){
      
      skeleton = s;
      //*value = (valueMax-valueMin)/2;
      //id = skeleton->stripID;      
      
      cfg.clickStepRatio = 0.03f;     // Fraction of the full range used for a click.
      
      cfg.durationMs = 3000;          // Hold duration from the captured value to either limit.
      cfg.steps = 0;                  // No time steps; enable them for stepped movement.

      
      //cfg.type = EasingType::LINEAR;  // Constant progress.      
      //cfg.type = EasingType::EASE;    // Gentle acceleration and deceleration.      
      //cfg.type = EasingType::EASE_IN; // Start slowly, then accelerate.     
      cfg.type = EasingType::EASE_IN_OUT; // Start slowly, accelerate, then slow down near the limit.
      
      //cfg.type = EasingType::CUBIC; // Custom curve with a fast initial response.    
      //cfg.x1 = 0.15f; cfg.y1 = 0.90f;
      //cfg.x2 = 0.20f; cfg.y2 = 1.00f;
            
      //cfg.type = EasingType::CUBIC;  // Combine a custom curve with stepped progress.
      //cfg.x1 = 0.25f; cfg.y1 = 0.10f;
      //cfg.x2 = 0.25f; cfg.y2 = 1.00f;
      //cfg.steps = 20;   // Twenty time steps over the movement.
      

    }    

    /**
     * @brief Detach from the current interface. Derived classes release any additional
     * subscriptions or shared-value reservations they created.
     */
    virtual ~Strip(){
      if (interface != nullptr){
        /*interface->stripCount -= 1;
        if (interface->stripCount<0) interface->stripCount = 0; */
        interface->unregisterEventListener(this);
      }
    }    

    // --------------------------------------------------------------------------
    // EVENT HANDLING
    // -------------------------------------------------------------------------- 


    /**
     * @brief Only interface-message events are decoded here. Copy their JSON payload,
     * use its first element as the action code (-1 when absent), then let the
     * concrete strip interpret it. The base processAction() does nothing.
     */
    virtual void onEvent(const Event& evt) override{

      if (evt.type != EVENT_INTERFACE_MESSAGE || evt.data == nullptr) return;

      // handle events from the mixer (e.g. to update strip state on external changes not made through the interface)
      JsonDocument evtData = evt.data != nullptr ? *((JsonDocument*)evt.data) : JsonDocument();
      int action = evtData[0] | -1;
      
      processAction(action,evtData);

    } 

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void processAction(int action,JsonDocument& data){} // do nothing by default


    // --------------------------------------------------------------------------
    // BUTTON MANAGEMENT
    // -------------------------------------------------------------------------- 
    enum class EasingType : uint8_t {
      LINEAR,
      EASE,
      EASE_IN,
      EASE_IN_OUT,
      CUBIC
    };

    /**
     * @brief Controls how button presses change values.
     *
     * Short presses move by clickStepRatio of the full value range.
     * Holds move from the captured start to the chosen limit over durationMs.
     * steps quantizes time progress before easing; throttleMs limits update rate,
     * while minDeltaToApply suppresses small hold changes in value units.
     * The Strip constructor overrides defaults to 3%, 3000 ms and no steps.
     */
    struct EasingConfig {
      EasingType type = EasingType::EASE_IN_OUT;

      /**
       * @brief Time to reach target (min/max) while holding
       */
      uint16_t durationMs = 1500;

      // For CUBIC only
      float x1 = 0.25f, y1 = 0.10f, x2 = 0.25f, y2 = 1.00f;

      /**
       * @brief Optional quantization (0 = off)
       */
      uint8_t steps = 0;

      /**
       * @brief Step for SIMPLE_CLIC etc (ratio of amplitude)
       */
      float clickStepRatio = 0.05f;

      /**
       * @brief Throttle period for HOLD updates (ms)
       * 16ms ~ 60Hz, 20ms ~ 50Hz, 33ms ~ 30Hz
       */
      uint16_t throttleMs = 50;

      /**
       * @brief Optional: do not emit updates smaller than this (absolute value units)
       */
      float minDeltaToApply = 0.0f; ///< e.g. 0.001f
    };

    EasingConfig cfg;

    /**
     * @brief State captured for the current hold.
     *
     * Reset when the button is released.
     */
    bool holdArmed = false;
    int8_t holdDir = 0;
    float holdStartValue = 0.0f;
    unsigned long holdStartMillis = 0;

    unsigned long lastHoldUpdate = 0; ///< for throttle


    /**
     * @brief Clamp progress to 0..1, optionally divide time into steps, then apply the
     * selected curve. Quantized time does not imply equally spaced output values.
     */
    float applyEasing(float p, const EasingConfig& c) {
      p = clamp01(p);

      if (c.steps > 0) {
        float s = (float)c.steps;
        p = clamp01(floorf(p * s) / s);
      }

      switch (c.type) {
        case EasingType::LINEAR:      return p;
        case EasingType::EASE:        return bezYforX(p, 0.25f, 0.10f, 0.25f, 1.00f);
        case EasingType::EASE_IN:     return bezYforX(p, 0.42f, 0.00f, 1.00f, 1.00f);
        case EasingType::EASE_IN_OUT: return bezYforX(p, 0.42f, 0.00f, 0.58f, 1.00f);
        case EasingType::CUBIC:
        default:                      return bezYforX(p, c.x1, c.y1, c.x2, c.y2);
      }
    }

    /**
     * @brief Calculate a value from elapsed milliseconds without changing hold state.
     *
     * The active hold path below performs the same interpolation with throttling.
     */
    float computeHoldValue(unsigned long timeMs, float start, float target, const EasingConfig& c) {
      if (c.durationMs == 0) return target;
      float p = (float)timeMs / (float)c.durationMs;
      float e = applyEasing(p, c);
      return start + (target - start) * e;
    }

    /**
     * @brief Permit the first hold update immediately, then at most one per throttle
     * interval. This reduces repeated value writes and device traffic.
     */
    bool throttleHold(){
      if (cfg.throttleMs == 0) return true;
      unsigned long now = millis();
      if (lastHoldUpdate == 0 || (now - lastHoldUpdate) >= cfg.throttleMs){
        lastHoldUpdate = now;
        return true;
      }
      return false;
    }

    /**
     * @brief Change a value and return the absolute amount changed (zero if unchanged).
     *
     * A click adds/subtracts a fixed fraction of the range and clamps at limits.
     * A hold captures its starting value/time once, or again on direction change.
     * After debounce, elapsed wall-clock time drives easing toward min/max;
     * skipped/throttled calls therefore do not slow down the intended movement.
     * This updates memory only. Concrete callbacks must send device commands.
     * Call onButtonReleased() to avoid reusing the previous hold's start.
     * The pointer must be valid and valueMin/valueMax must describe its units.
     */
    float applyProgressive(float* fVal, int8_t dir, uint8_t trigger){
      if (dir != 1 && dir != -1) return 0.0f;

      // Non-hold triggers use a fixed increment.
      if (trigger != BUTTON_HOLD){
        float amp = (valueMax - valueMin);
        if (amp <= 0.0f) return 0.0f;

        float step = amp * cfg.clickStepRatio;
        if (step <= 0.0f) return 0.0f;

        float newValue = *fVal + (float)dir * step;
        if (newValue > valueMax) newValue = valueMax;
        if (newValue < valueMin) newValue = valueMin;

        float delta = newValue - *fVal;
        if (delta == 0.0f) return 0.0f;

        *fVal = newValue;
        return fabsf(delta); // Return the magnitude in either direction.
      }

      // Capture the start on the first hold call or a direction change.
      unsigned long now = millis();
      if (!holdArmed || holdDir != dir){
        holdArmed = true;
        holdDir = dir;
        holdStartValue = *fVal;
        holdStartMillis = now;
        lastHoldUpdate = 0; // Allow the first eligible update immediately.
      }

      // Ignore the initial debounce interval.
      if ((now - holdStartMillis) < BUTTON_DEBOUNCE_MS) {
        return 0.0f;
      }

      if (!throttleHold()) return 0.0f;

      // Use actual elapsed time since this hold started.
      unsigned long elapsed = now - holdStartMillis;

      float target = (dir > 0) ? valueMax : valueMin;

      float p = (cfg.durationMs == 0) ? 1.0f : ((float)elapsed / (float)cfg.durationMs);
      float e = applyEasing(p, cfg);
      float newValue = holdStartValue + (target - holdStartValue) * e;

      if (newValue > valueMax) newValue = valueMax;
      if (newValue < valueMin) newValue = valueMin;

      float delta = newValue - *fVal;
      if (delta == 0.0f) return 0.0f;

      if (cfg.minDeltaToApply > 0.0f && fabsf(delta) < cfg.minDeltaToApply) {
        return 0.0f;
      }

      *fVal = newValue;
      return fabsf(delta);
    }

    /**
     * @brief End the hold so the next press captures a fresh start value and timestamp.
     */
    void onButtonReleased(){
        holdArmed = false;
        holdDir = 0;
        lastHoldUpdate = 0;
    }

    /** @brief Increase the selected value in response to the supplied button gesture.
     *
     * Default callbacks change the bound value; the time argument is not used.
     * Hold timing comes from millis(), independently of button-event timestamps.
     */
    virtual float increaseCallback(unsigned long time, uint8_t trigger){
      return applyProgressive(value, +1, trigger);
    }

    /** @brief Decrease the selected value in response to the supplied button gesture. */
    virtual float decreaseCallback(unsigned long time, uint8_t trigger){
      return applyProgressive(value,-1, trigger);
    }

    /** @brief Return the number of property descriptors in the strip skeleton. */
    virtual uint8_t getNumberOfProperties(){
      return skeleton->parameterAddresses.size();      
    }

    /** @brief Perform the configured main action for this strip.
     *
     * Extension points: implement the main action, property format and labels.
     * hasFocus() requests selection/configuration data; hasVisibility() requests
     * displayed values even when a strip is visible without being selected.
     */
    virtual bool actionCallback(uint8_t trigger)=0;
    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties()=0;
    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String s)=0;        
    /** @brief Request configuration on selection. */
    virtual void hasFocus() = 0; // this method is the one which is used to force poll data from the interface when getting focus, it retrieve the config
    /** @brief Request current values when the strip becomes visible. */
    virtual void hasVisibility() = 0; // this method is the one which is used to force poll data from the interface when it is visible, it retrieve the values 
    /**
     * @brief it will usually drop a command to the interface which outcome be lately read by the process() method
     * is called from the focus() method so to force data to be updated in case some whre missed
     */
    virtual const String* getTopLabel()=0;
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel()=0;
    
    // update is called by process() if data are availabe and must therefore implement the code that uses what is returned by read() by casting it to the right data structure    
    // it assign the GUI values the values that were acquired or processed in the process() method
    // basedon its internal state, it will set the redraw flag 
    //virtual void update() = 0;  
      
    /**
     * @brief Bank::update() invokes this hook for every strip.
     *
     * Current base implementation is a no-op after the interface check.
     * It is non-virtual: Bank::update() calling through Strip* does not dispatch
     * a derived method of the same name. Event callbacks handle incoming data.
     */
    void process(){ 
      if (!interface) return;
      //if (interface->available() > 0) update();
    }


    /**
     * @brief Selection raises brightness, marks visibility and calls hasFocus().
     *
     * Losing focus dims the strip but does not clear visible: another strip can
     * remain displayed beside the selected one in details mode.
     */
    void focus(bool f=true){
      //DEBUG_PRINT("\nStrip(%d)",index);
      if (f){
        brightness = BRIGHTNESS_MAX;
        //DEBUG_PRINT(" has focus");
        visible = true;
        focused = true;

        // drop command to query strip state
        hasFocus();        
       
      }else{
        brightness = BRIGHTNESS_NOFOCUS;
        focused = false;
        //DEBUG_PRINT(" lost focus");
      }
    }

    /** @brief Prepare this object for use with its current configuration.
     *
     * Set the bound value to half the range width. This is only a midpoint when
     * valueMin is zero; concrete strips may override initialization.
     */
    virtual bool init(){
      *value = (valueMax-valueMin)/2;               
      return true;
    }

    /**
     * @brief Restore a strip after its concrete type has been created by the factory.
     *
     * Resolve the saved interface name, subscribe to its events, then call init()
     * before restoring type, position, shape, color, icon and property strings.
     * Each property is delegated to deserializeProperty() in saved order.
     * Unknown interface or malformed JSON returns false; color ids are bounded,
     * but geometry and property validity are not fully checked here.
     * Reloading an existing strip does not detach its previous interface first.
     */
    bool deserialize(String values){

      uint8_t n = getNumberOfProperties();
      JsonDocument jsonStrip;
      DeserializationError error = deserializeJson(jsonStrip, values);
      if (error){
        DEBUG_ERROR("\nStrip::deserialize() error %s",error.c_str());
        return false;
      }

      // find the interface corresponding to the strip
      interfaceName = jsonStrip[C_INTERFACE].as<String>();//.as<int>();
      interface = interfaceRepo.retrieve(interfaceName);
      if (interface == nullptr) return false; // if it has no interface, no need to continue

      //interface->stripCount++;              // Previously tracked interface usage.
      interface->registerEventListener(this); // Receive interface events until destruction.
      init();

      type = jsonStrip[C_STRIPTYPE].as<String>();
      // set index; 
      x = jsonStrip["position"][C_X].as<int>();
      y = jsonStrip["position"][C_Y].as<int>();
      win = jsonStrip["position"][C_WIN].as<int>();
      index = jsonStrip["position"][C_INDEX].as<int>();
      size = jsonStrip[C_SIZE].as<String>();
      colorID = jsonStrip[C_COLOR];//.as<int>();      
     
      if (colorID>MAX_COLOR_ID-1) colorID = 0;
      frontColor = colorMap[colorID].fColor;
      backColor = colorMap[colorID].bColor;  

      icon = jsonStrip[C_ICON];//.as<unsigned char>();
      properties = jsonStrip[KEY_PROPERTIES];
      for (uint8_t i = 0; i< properties.size(); i++){
        deserializeProperty(properties[i].as<String>());
      }             

      return true;
    };

    /**
     * @brief Save configuration as JSON text: type, color, icon, size, interface,
     * position {x,y,id,win}, and properties supplied by serializeProperties().
     * Live device values, focus and brightness are not persisted here.
     * properties is a temporary view into this local document; derived methods
     * must populate it during this call, not retain it for later use.
     */
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
      
      properties = doc[KEY_PROPERTIES].to<JsonArray>();
      serializeProperties();
      String data;
      serializeJson(doc, data);

      return data;
    }

    /**
     * @brief Build the strip's widgets for the current bank view.
     *
     * Mark visible, remove focus and request fresh display values, then check the
     * interface/position/shape. Ready or disabled interfaces show normal controls;
     * enabled interfaces that are not ready show connection/sync status instead.
     * hasVisibility() runs before the null-interface check, so implementations
     * must tolerate missing bindings. Only x/y origins and known shapes are
     * checked here; overlap and full rectangle bounds are not validated.
     */
    virtual void pushToGUI(uint8_t layout){
      
      visible = true;
      focus(false);

      hasVisibility(); // we query values here to be sure to have the latest data from the interface before  rendering, this is important for instance for the color of the strip which may change based on the state of the interface (like for instance for a channel that is red when it is clipping) and we want to be sure to reflect this in the GUI as soon as possible when the strip is visible, even if it is not focused yet (for instance in case of a strip that is visible but not focused in details layout, we still want to reflect the color change in case of clipping for instance)

      renderError = false;

      if (!interface){
        renderError = true;
        DEBUG_ERROR("\nStrip interface %s not instanciated",interfaceName.c_str());
        pushSimpleText(layout, lang.getString("No interface"));
        return;
      }


      if (x>3 || y>3){
        renderError = true;
        DEBUG_ERROR("\nIncorrect strip location: %dx%d",x,y);
        pushSimpleText(layout, lang.getString("Strip x/y error"));
        return;
      }
            
      if (!determineTopLeft(layout)){
        renderError = true;
        DEBUG_ERROR("\nIncorrect strip size of layout: %s / %d",size.c_str(), layout);
        pushSimpleText(layout, lang.getString("Strip shape error"));
        return;
      }

      // strip size and position are correct, we therefore go ahead and render it
      drawWidgetsFrames(layout);      

      // if connected, we call the virtual function that display the strip if we are aready but also if the interface is disabled so we can still work with it offline
      if (interface->state.code == IF_READY || interface->state.enabled == false){
        composeWidgets(layout);
      }else{
        //DEBUG_PRINT("\nStrip %s is syncing",strip->type.c_str());          
        composeInterfaceState(layout);  
      }

    }

    /** @brief Convert the strip shape and logical origin into pixel bounds and anchors.
     *
     * Translate the logical shape/origin into pixel bounds and widget anchors.
     * Zoom and 4x4 use the full control area; other shapes use the grid position.
     * Vertical/flat shapes have extra text offsets. Window selection happens
     * outside this method; win is not converted into a pixel offset here.
     */
    bool determineTopLeft(uint8_t layout){      

      //DEBUG_PRINT("\nstrip[%d] %s topLeft=",index,type.c_str());
      // retrieve the corner of the strip where we can start drawing, which is topLeftX, topLeftY
      if (layout == LAYOUT_ZOOM || size == SHAPE4X4){
        topLeftX = 0;
        topLeftY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);
        areaW = WIDGET4X4W;
        areaH = WIDGET4X4H;        
      }
      else if (size == SHAPE1X4){
        topLeftX = x * WIDGET1X4W;
        topLeftY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);        
        areaW = WIDGET1X4H;   
        areaH = WIDGET4X1H*0.6; // some margin to avoid text being rendered with overlap on top2label  
      }
      else if (size == SHAPE4X1){
        topLeftX = 0;
        topLeftY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET4X1H) ;        
        areaW = WIDGET4X1W;
        areaH = WIDGET4X1H;        
      }
      else if (size == SHAPE2X2){
        topLeftX = (x * WIDGET2X2W/2);
        topLeftY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET2X2H/2);
        areaW = WIDGET2X2W;
        areaH = WIDGET2X2H;
    
      }else{
        return false;
      }         

      centerX = topLeftX + areaW / 2;       //middle
      centerY = topLeftY + (areaH/2) -2;       //middle

      // fix position according to some widget size specificity, but only when layout is DETAILS:
      if (layout == LAYOUT_DETAILS){
        if (size == SHAPE4X1) simpleTextY -= WIDGET4X1H/2-3;     // fix vertical location for flat strip
        else if (size == SHAPE1X4) {
          // special treatment for vertical widget where text is rotated
          centerX = topLeftX + WIDGET1X4W/2;   // exactly middle
          centerY = topLeftY + WIDGET1X4H/2.5; // we want middle but for some reason we have to give it some margin
        }
      }

      //DEBUG_PRINT("(%i,%i)",topLeftX,topLeftY);
      return true;
    }

    #define SIMPLE_TEXT_MAX_LEN 20
    #define MIN_SIMPLETEXT_SIZE 40
    /** @brief Fit a status label to the strip shape, rotating vertical-strip text.
     *
     * Fit a centered status label using an estimated character width. Vertical
     * strips rotate it 90 degrees; shape-specific limits shorten long labels.
     */
    void pushSimpleText(uint8_t layout, const String* textOriginal) {
      // Label length is limited below according to its shape.
      int16_t angle = 0;
      uint8_t maxLen = 28;

      int len = textOriginal->length();

     if (size == SHAPE1X4){
        angle = 90;
        maxLen = 15;        
      }else if (size == SHAPE2X2){
        maxLen = 13;        
      }

      #define FONT_SIZE_RATIO 1.5
      int fontSize = areaH * FONT_SIZE_RATIO ;  // affect the maximum size of font possible, which is the widget height

      // Reduce the font until estimated text fits or the minimum is reached.
      while (fontSize > MIN_SIMPLETEXT_SIZE) {
        float charWidth = 0.4f * fontSize;
        float estimatedWidth = charWidth * len;

        if (estimatedWidth <= areaW) break;  // The estimated width fits.
        fontSize -= 2;                    
      }

      simpleTextX = centerX;
      simpleTextY = centerY;

      // Label GUI
      Label* label = new Label(textOriginal, Align::Center, &simpleTextX, &simpleTextY,
                              fontSize, maxLen,
                              &GUI::white, &backColor, &brightness);
      label->angle = angle;
      gui.add(label);
    }


    /**
     * @brief Select a status presentation matching the strip shape: full details in
     * zoom/4x4, or a compact label, activity LED and device icon in smaller shapes.
     */
    virtual void composeInterfaceState(uint8_t layout){
      if (!interface) return;
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        pushIFState4x4();        
        
        //gui.add (new ProgressBarH(&hwm.network->networkConnectAttemptProgress /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, MODAL_SCREEN_SCROLLBAR_HEIGHT /*h*/, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/));                             
        //gui.add (new PendingBar(&oneHundedFloat /*v*/, 0.0 /*min*/,100.0 /*max*/, &GUI::zero /*x*/, &scrollBarY /*y*/, TFT_WIDTH /*w*/, 7 /*h*/, /*pingpong*/ true, 0 /*offset*/, 0 /*radius*/, &GUI::white /*fc*/ , &GUI::defaultBcolor/*bc*/, 50, 4));                                                  

      }else if (size == SHAPE1X4){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState1x4();
      }else if (size ==SHAPE2X2 ){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState2x2();
      }else if (size == SHAPE4X1){
        pushSimpleText(layout, &interface->state.subState);
        pushIFState4x1();
      }else{

      }      
    }    

    ////////////////////////////// state widgets library

    /** @brief Add the full-size interface status, progress and offline instructions. */
    void pushIFState4x4(){
      if (!interface) return;
      gui.add(new Label(&interface->state.label, Align::Center,&GUI::midScreenHoriz, &GUI::topTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::defaultBcolor));    
      gui.add(new Led(&GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20, &interface->state.led));      
      gui.add(new Arc(&interface->state.progress, 0,100.0 , &GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20+10, &GUI::gold, &GUI::transparentColor,0));
      gui.add(new Icon(interface->getIcon(),Align::Right,&GUI::rightMenuIconXpos, &GUI::iconY,MEDIUM_ICON_SIZE,&GUI::gold,&GUI::defaultBcolor,&GUI::fullBrightness,false,interface->getIconRenderer()));
      gui.add(new TextArea(&interface->state.details,TFT_WIDTH/2.3 ,Align::Left,Align::Center,&GUI::midScreenHoriz,&GUI::iconY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));           
      gui.add(new Label(lang.getString("Double clic on/off to toggle offline"), Align::Center,&GUI::midScreenHoriz, &GUI::belowBottomTexty,SIZE_14,40,&GUI::green,&GUI::transparentColor));          
      gui.add(new Label(&interface->state.subState, Align::Center,&GUI::midScreenHoriz, &GUI::bottomTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::defaultBcolor));
    }   

    /** @brief Place compact interface status widgets in a vertical strip. */
    void pushIFState1x4(){      
      IFledX = topLeftX + WIDGET1X4W / 1.25;
      IFledY = topLeftY + WIDGET1X4H / 2.5;

      IFiconX = topLeftX + WIDGET1X4W / 1.05;
      IFiconY = topLeftY + WIDGET1X4H / 1.8;      

      pushSmallLedandIcon();
    }      


    /** @brief Place compact interface status widgets in a square strip. */
    void pushIFState2x2(){      
      IFledX = topLeftX + WIDGET2X2W / 2.5;
      IFledY = topLeftY + SIZE_20;

      IFiconX = topLeftX + WIDGET2X2W / 1.4;
      IFiconY = topLeftY + SIZE_10 ;

      pushSmallLedandIcon();
    }     
    
    /** @brief Place compact interface status widgets in a horizontal strip. */
    void pushIFState4x1(){      
      IFledX = GUI::leftMenuIconXpos+3;
      IFledY = topLeftY + WIDGET4X1H/2;

      IFiconX = TFT_WIDTH - SIZE_26/2;
      IFiconY = topLeftY + SIZE_26/4;

      pushSmallLedandIcon();
    }     

    /**
     * @brief Bind widgets directly to the interface status and strip brightness so
     * their displayed values can change without rebuilding the strip.
     */
    void pushSmallLedandIcon(){
      if (!interface) return;
      gui.add(new Led(&IFledX, &IFledY, SIZE_10, &interface->state.led,&brightness));      
      gui.add(new Arc(&interface->state.progress, 0,1.0 , &IFledX, &IFledY, SIZE_16, &GUI::gold, &GUI::transparentColor,0,&brightness));
      gui.add(new Icon(interface->getIcon(),Align::Right,&IFiconX, &IFiconY,SIZE_26,&GUI::gold,&GUI::transparentColor,&brightness,false,interface->getIconRenderer()));
    }

    // Reusable value displays for concrete strips. Widgets borrow value/range,
    // color and coordinate storage; some helpers also adjust the stored anchors.
    ////////////////////////////// common widgets library
    /** @brief Add a horizontal value bar in the full-size strip area. */
    void push4x4hBar(){      
      h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
      w = WIDGET4X4W-GUI::three*2;      
      gui.add(new ProgressBarH(value /*v*/, valueMin /*min*/,valueMax /*max*/, &GUI::three /*x*/, &GUI::midScreenHoriz /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));      
    }

    /** @brief Add the vertical value bar used by a narrow strip. */
    void push1x4vBar(){
      w = WIDGET1X4W/1.9;   // width of the bar is 2 time less big than the full width of the cell
      h = WIDGET1X4H;
      topLeftX += w/2;           
      gui.add(new ProgressBarV(value /*v*/, valueMin /*min*/,valueMax /*max*/, &topLeftX /*x*/, &topLeftY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 0 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
    }

    /** @brief Add a ring meter in a square strip. */
    void push2x2RingMeter(){
      w = WIDGET2X2W;   // width of the bar is 2 time less big than the full width of the cell
      h = WIDGET2X2H;
      topLeftX += w/2;   
      topLeftY += h/2;      
      RingMeter* ringMeter = new RingMeter(value, valueMin, valueMax, 1, &topLeftX,&topLeftY, h/2, &unitString,&GUI::blue,&GUI::transparentColor,&brightness);
      gui.add(ringMeter);
    }

    /** @brief Add the value bar used by a horizontal strip. */
    void push4x1hBar(){
      h = WIDGET4X1H/2;   // height of the bar is 2 time less big than the full height of the cell
      w = WIDGET4X1W;
      gui.add(new ProgressBarH(value /*v*/, valueMin /*min*/,valueMax /*max*/, &topLeftX /*x*/, &topLeftY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
    }

    /** @brief Add a horizontal value bar in a square strip. */
    void push2x2hBar(){
      h = WIDGET2X2H/2;   // height of the bar is 2 time less big than the full height of the cell          
      uint8_t widthOffset = WIDGET2X2H/10;
      w = WIDGET2X2W-widthOffset;
      gui.add(new ProgressBarH(value /*v*/, valueMin /*min*/,valueMax /*max*/, &topLeftX /*x*/, &topLeftY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
    }

    /////////////// default rendering /////////////////////////////////

    /** @brief Default composition draws only the frame; concrete strips add controls. */
    virtual void composeWidgets(uint8_t layout){      // has to be override by derived class
      drawWidgetsFrames(layout);
    } 

    /** @brief Build strip boundary widgets using the current interface state.
     *
     * Intended to distinguish normal/offline frames. Current guard is inverted:
     * valid interfaces return immediately, while nullptr reaches a dereference.
     * This comment documents existing behavior; it does not fix the guard.
     */
    void drawWidgetsFrames(uint8_t layout){

      if (interface != nullptr) return;
      
      if (interface->state.enabled) frameColor = GUI::luminance(TFT_GOLD,127);     // dark frame
      else frameColor = TFT_RED;    // indicate offline

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        // having a frame when we are 4x4 is useless unless we want to indicate interface is offline
          gui.add(new Canva(&topLeftX, &topLeftY, WIDGET4X4W, WIDGET4X4H,4, &frameColor, &GUI::transparentColor,&brightness));  
      }
      else if ( size == SHAPE1X4) gui.add(new Canva(&topLeftX, &topLeftY, WIDGET1X4W, WIDGET1X4H, 4,&frameColor, &GUI::transparentColor,&brightness));
      else if (size == SHAPE2X2) gui.add(new Canva(&topLeftX, &topLeftY, WIDGET2X2W, WIDGET2X2H,4, &frameColor, &GUI::transparentColor,&brightness));  
      else if (size == SHAPE4X1) gui.add(new Canva(&topLeftX, &topLeftY, WIDGET4X1W, WIDGET4X1H,4, &frameColor, &GUI::transparentColor,&brightness));      
    }

    // Borrowed JSON view used only while serializing/restoring properties.
    // skeleton must outlive this strip; value bounds use the bound value's units.
    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float valueMin = 0, valueMax = 1;

    int16_t simpleTextX = 0, simpleTextY = 0;

    int16_t centerX = 0, centerY = 0;

    uint16_t areaW = 0, areaH = 0;

    int16_t IFledX = 0, IFledY = 0;
    int16_t IFiconX = 0, IFiconY = 0;

  protected:
    unsigned long lastCall;     
    uint16_t frameColor = TFT_GOLD;
    uint16_t h = 0, w = 0;    
    String unitString;

};

#define PROPERTY_BUF_CHAR_MAX 128  // to restore and serialize address string value
/**
 * @brief Base for a strip with one configured selection, using skeleton address 0.
 *
 * With a %d pattern, store a one-based id and look up its label at id-1 in the
 * chosen interface's dataset. With a text pattern, keep the extracted text locally.
 * Example: /ch/name/%d with id 3 resolves dataset element 2 for the channel label.
 * Dataset label pointers are borrowed and depend on the provider's lifetime.
 * Current formatting/parsing does not fully enforce buffer sizes or id bounds.
 */
class OneSelectStrip : public Strip{

  public: 
    /** @brief Initialize OneSelectStrip with the supplied configuration and bindings. */
    OneSelectStrip(StripSkeleton* s, bool debug=false):Strip(s){
      Address* select1Address = s->getDataSetAddress(0);
      if (select1Address != nullptr){
        select1PropertyAddress = select1Address->pattern;
      }
    }

    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties() override{
      char* buf;
      // if address contains a %d , we extract the select1ID to populate the address otherwise we use the string value      
      if (select1PropertyAddress.indexOf("%d") != -1){
        buf = (char*)malloc(select1PropertyAddress.length()*sizeof(char*));
        sprintf(buf,select1PropertyAddress.c_str(),select1ID);
        properties.add(String(buf));
        free(buf);
      }else{
        buf = (char*)malloc((select1PropertyAddress.length()+PROPERTY_BUF_CHAR_MAX)*sizeof(char*));
        snprintf(buf, sizeof(buf), select1PropertyAddress.c_str(), select1ContentPtr->c_str());    // Current sizeof(buf) is the pointer size, not the allocated buffer size.
        properties.add(String(buf));
        free(buf);        
      }
    }  

    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String property) override{
      if (!interface) return;
      // if address contains a %d , we extract the select1ID that is then used to get the corresponding string value in the related dataset
      if (select1PropertyAddress.indexOf("%d") != -1){
        if (sscanf(property.c_str(), select1PropertyAddress.c_str(), &select1ID)){
          select1ContentPtr = skeleton->getValuePtr(interface->getProviderNameForData(),select1PropertyAddress,select1ID-1);      // drop down values always starts at 1 so we have to deduect one to have the right index (arrays starting at)
        }
      }else{
        //otherwise we get the string itself directly in the address
        char buf[PROPERTY_BUF_CHAR_MAX];         
        if (sscanf(property.c_str(), select1PropertyAddress.c_str(), buf) == 1){
          select1Content = buf;
          select1ContentPtr = &select1Content;
        }         
      }
      //DEBUG_INFO("%s::deserializeProperty(%s:%s) against %s = <%s>",type.c_str(),interface->getProviderNameForData().c_str(), property.c_str(),select1PropertyAddress.c_str(), select1ContentPtr->c_str());      
    }  

    /** @brief Return the borrowed text used for the strip header top line. */
    virtual const String* getTopLabel() override{return &type;}
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel() override {return select1ContentPtr;}        // will be the selected ch   
 
  protected:
    Top1Toggle1Label* topLabels;
    uint8_t select1ID=1; 
    const String *select1ContentPtr = &select1Content;
    String select1Content = "selectContent1";
    String select1PropertyAddress; 
    String label = "~";    
};


/**
 * @brief Adds a second selection using skeleton address 1, for example a channel/bus pair.
 *
 * Save selection 1 first, then selection 2. On load, offer each property to both
 * patterns, so each selection consumes its matching address. Labels show the
 * first selection at the top and the second at the bottom.
 * Uses the same borrowed-pointer and parsing constraints as OneSelectStrip.
 */
class TwoSelectStrip: public OneSelectStrip{
  public:
    /** @brief Initialize TwoSelectStrip with the supplied configuration and bindings. */
    TwoSelectStrip(StripSkeleton* s,bool debug=false):OneSelectStrip(s,debug){
      Address* select2Address = s->getDataSetAddress(1);
      if (select2Address != nullptr){
        select2PropertyAddress = select2Address->pattern;
      }
    }
  
    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties() override{
      OneSelectStrip::serializeProperties();
      char* buf;
      // if address contains a %d , we extract the select2ID to populate the address otherwise we use the string value      
      if (select2PropertyAddress.indexOf("%d") != -1){
        buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
        sprintf(buf,select2PropertyAddress.c_str(),select2ID);
        properties.add(String(buf));
        free(buf);
      }else{
        buf = (char*)malloc((select2PropertyAddress.length()+PROPERTY_BUF_CHAR_MAX)*sizeof(char*));
        snprintf(buf, sizeof(buf), select2PropertyAddress.c_str(), select2ContentPtr->c_str());    // secured size sprintf
        properties.add(String(buf));
        free(buf);        
      }
    }  

    /** @brief Restore a type-specific setting from its saved property string. */
    virtual void deserializeProperty(String property) override{
      if (!interface) return;
      OneSelectStrip::deserializeProperty(property);
      // if address contains a %d , we extract the select2ID that is then used to get the corresponding string value in the related dataset
      if (select2PropertyAddress.indexOf("%d") != -1){
        if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
          select2ContentPtr = skeleton->getValuePtr(interface->getProviderNameForData(),select2PropertyAddress,select2ID-1); // drop down values always starts at 1 so we have to deduect one to have the right index (arrays starting at)
        }
      }else{
        //otherwise we search for the string 
        char buf[PROPERTY_BUF_CHAR_MAX];         
        if (sscanf(property.c_str(), select2PropertyAddress.c_str(), buf) == 1){
          select2Content = buf;
          select2ContentPtr = &select2Content;
        }         
      }
    }  

    /** @brief Return the borrowed text used for the strip header top line. */
    virtual const String* getTopLabel(){return select1ContentPtr;}
    /** @brief Return the borrowed text used for the strip header bottom line. */
    virtual const String* getBottomLabel(){return select2ContentPtr;}      

  protected:
    uint8_t select2ID=1; 
    const String *select2ContentPtr = &select2Content;
    String select2Content = "select2Content";
    String select2PropertyAddress;
    Top2Toggles* topLabels;  
};


#endif
