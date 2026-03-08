#ifndef STRIPSDEF_h
#define STRIPSDEF_h

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

class StripSkeleton: public SerializableParameters{
  public:
    unsigned char icon;
    StripBuilderCallback constructor;
    //JsonObject variable;
    using SerializableParameters::serialize;  // 🔴 réexpose String serialize()

    StripSkeleton(String id, String g, StripBuilderCallback b, unsigned char i='!');

    // 🚩 Redéfinition : on enrichit simplement le JsonObject
    void serialize(JsonObject& json) override {
        // Appelle la version de base
        SerializableParameters::serialize(json);
        // Ajoute la donnée spécifique
        json[C_ICON] = icon;
    }

};


class StripFactory{
  public:
    std::vector<StripSkeleton*> stripSkeletonRepo;

    bool add(StripSkeleton* ss){
      stripSkeletonRepo.push_back(ss);
      return true;
    }

    void remove(StripSkeleton* ss) {
      for (auto it = stripSkeletonRepo.begin(); it != stripSkeletonRepo.end(); ++it) {
        if ((*it)->name == ss->name) {
          stripSkeletonRepo.erase(it);
          break;
        }
      }
    }

    Strip* build(String stripID/*, String values*/){
      Strip* strip = nullptr;
      StripSkeleton* skel = getStripSkeleton(stripID);
      if (skel != nullptr){
        strip = skel->constructor(/*values*/);
      }
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
        StripSkeleton* skeleton = stripSkeletonRepo[i];
        String stripName = skeleton->name;
        jsonSkeletons->add(skeleton->serialize());
      }
    }

  private:
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

StripSkeleton::StripSkeleton(String id, String g, StripBuilderCallback c, unsigned char i):SerializableParameters(id,g, &dataRepo),constructor(c){
  stripFactory.add(this);
}

/* HELPERS BUTTON */
// ----- Helpers -----
float clamp01(float v) { return (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v; }

float bez1D(float t, float a1, float a2) {
  float omt = 1.0f - t;
  return (3.0f * omt * omt * t * a1) + (3.0f * omt * t * t * a2) + (t * t * t);
}

float bez1D_d(float t, float a1, float a2) {
  float omt = 1.0f - t;
  return 3.0f * omt * omt * a1
       + 6.0f * omt * t * (a2 - a1)
       + 3.0f * t * t * (1.0f - a2);
}

// Solve for y given x on cubic-bezier(x1,y1,x2,y2) (CSS-like)
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


///////////////////// STRIP MANAGEMENT ////////////////////////

#define LAYOUT_ZOOM     0b00000001
#define LAYOUT_DETAILS  0b00000010

#define SHAPE4X4 "4x4"
#define SHAPE4X1 "4x1"
#define SHAPE1X4 "1x4"
#define SHAPE2X2 "2x2"

float oneHundedFloat = 100.0;

class Strip: public EventListener{  
  #define JSON_STRIP_PROPERTY_SIZE 100
  #define  JSON_STRIP_TEMPLATE_SIZE 300
  public:
    TFT_eSprite* sprite = nullptr;

    virtual const char* getListenerName()  override{
      return getTopLabel()->c_str();
    }    

    // flag to indicate if position is out of range
    bool renderError;
    //bool redraw = false;

    bool toggle1 = false;
    bool toggle2 = false;
    //bool updated = false;
    uint16_t frontColor, backColor;    

    String iconInterfaceConnect = ICON_FOOTSWITCH;

    String type="undef";

    String interfaceName = "interfaceName"; //
    DeviceInterface* interface = nullptr;

    bool visible = false;

    // logical position
    uint8_t x, y, win, index;
    
    // top left corner position on TFT screen
    int16_t topX, topY;
    
    String size;
    unsigned char icon = 0x21;
    uint8_t colorID=0; 

    //GUIManager* gui;

    float brightness = BRIGHTNESS_NOFOCUS;

    Strip(StripSkeleton* s){
      
      skeleton = s;
      value = (valueMax-valueMin)/2;
      //id = skeleton->stripID;      
      
      cfg.clickStepRatio = 0.03f;     // (en % de l’amplitude totale), seulement utile en simple clic
      
      cfg.durationMs = 3000;          // temps pour aller de value → valueMin ou valueMax en HOLD
      cfg.steps = 0;                  // pas de quantification, permet de faire un effet cranté ou bien limiter la quantité de ddonnée envoyée.

      
      //cfg.type = EasingType::LINEAR;  // Progression constante. Idéal pour valeurs techniques (fréquence, gain fin, ratio).      
      //cfg.type = EasingType::EASE;    // Accélère légèrement puis ralentit doucement.Très bon compromis pour paramètres généraux.      
      //cfg.type = EasingType::EASE_IN; //Démarre doucement, accélère progressivement. Bien pour réglages “sensibles” au début (ex: volume bas).     
      cfg.type = EasingType::EASE_IN_OUT; // Démarre doucement → accélère → ralentit vers la fin. Le plus “premium” visuellement.
      
      //cfg.type = EasingType::CUBIC; //Permet des comportements plus dynamiques. 🔥 Version “Punchy rapide”. Accélération forte puis ralentissement marqué.    
      //cfg.x1 = 0.15f; cfg.y1 = 0.90f;
      //cfg.x2 = 0.20f; cfg.y2 = 1.00f;
            
      //cfg.type = EasingType::CUBIC;  // + STEPS Simule un comportement proche d’un encodeur cranté.
      //cfg.x1 = 0.25f; cfg.y1 = 0.10f;
      //cfg.x2 = 0.25f; cfg.y2 = 1.00f;
      //cfg.steps = 20;   // 20 paliers sur toute la course
      

    }    

    virtual ~Strip(){
      if (interface != nullptr){
        /*interface->stripCount -= 1;
        if (interface->stripCount<0) interface->stripCount = 0; */
        interface->unregisterEventListener(this);
      }
    }    

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

    struct EasingConfig {
      EasingType type = EasingType::EASE_IN_OUT;

      // Time to reach target (min/max) while holding
      uint16_t durationMs = 1500;

      // For CUBIC only
      float x1 = 0.25f, y1 = 0.10f, x2 = 0.25f, y2 = 1.00f;

      // Optional quantization (0 = off)
      uint8_t steps = 0;

      // Step for SIMPLE_CLIC etc (ratio of amplitude)
      float clickStepRatio = 0.05f;

      // Throttle period for HOLD updates (ms)
      // 16ms ~ 60Hz, 20ms ~ 50Hz, 33ms ~ 30Hz
      uint16_t throttleMs = 50;

      // Optional: do not emit updates smaller than this (absolute value units)
      float minDeltaToApply = 0.0f; // e.g. 0.001f
    };

    EasingConfig cfg;

    // HOLD state (remplace holdStart + time logic)
    // état HOLD
    bool holdArmed = false;
    int8_t holdDir = 0;
    float holdStartValue = 0.0f;
    unsigned long holdStartMillis = 0;

    unsigned long lastHoldUpdate = 0; // for throttle


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

    float computeHoldValue(unsigned long timeMs, float start, float target, const EasingConfig& c) {
      if (c.durationMs == 0) return target;
      float p = (float)timeMs / (float)c.durationMs;
      float e = applyEasing(p, c);
      return start + (target - start) * e;
    }

    bool throttleHold(){
      if (cfg.throttleMs == 0) return true;
      unsigned long now = millis();
      if (lastHoldUpdate == 0 || (now - lastHoldUpdate) >= cfg.throttleMs){
        lastHoldUpdate = now;
        return true;
      }
      return false;
    }

    float applyProgressive(int8_t dir, uint8_t trigger){
      if (dir != 1 && dir != -1) return 0.0f;

      // CLIC (ou autre) => pas fixe
      if (trigger != BUTTON_HOLD){
        float amp = (valueMax - valueMin);
        if (amp <= 0.0f) return 0.0f;

        float step = amp * cfg.clickStepRatio;
        if (step <= 0.0f) return 0.0f;

        float newValue = value + (float)dir * step;
        if (newValue > valueMax) newValue = valueMax;
        if (newValue < valueMin) newValue = valueMin;

        float delta = newValue - value;
        if (delta == 0.0f) return 0.0f;

        value = newValue;
        return fabsf(delta); // delta positif comme ton code
      }

      // HOLD : on arme au début (ou si changement de sens)
      unsigned long now = millis();
      if (!holdArmed || holdDir != dir){
        holdArmed = true;
        holdDir = dir;
        holdStartValue = value;
        holdStartMillis = now;
        lastHoldUpdate = 0; // update immédiat
      }

      // anti “rebonds” : si tu veux ignorer les tout premiers ms
      if ((now - holdStartMillis) < DEBOUCE_TIME) {
        return 0.0f;
      }

      if (!throttleHold()) return 0.0f;

      // elapsed = temps réel écoulé depuis début du HOLD
      unsigned long elapsed = now - holdStartMillis;

      float target = (dir > 0) ? valueMax : valueMin;

      float p = (cfg.durationMs == 0) ? 1.0f : ((float)elapsed / (float)cfg.durationMs);
      float e = applyEasing(p, cfg);
      float newValue = holdStartValue + (target - holdStartValue) * e;

      if (newValue > valueMax) newValue = valueMax;
      if (newValue < valueMin) newValue = valueMin;

      float delta = newValue - value;
      if (delta == 0.0f) return 0.0f;

      if (cfg.minDeltaToApply > 0.0f && fabsf(delta) < cfg.minDeltaToApply) {
        return 0.0f;
      }

      value = newValue;
      return fabsf(delta);
    }

    void onButtonReleased(){
        holdArmed = false;
        holdDir = 0;
        lastHoldUpdate = 0;
    }

      virtual float increaseCallback(unsigned long /*time*/, uint8_t trigger){
        return applyProgressive(+1, trigger);
      }

      virtual float decreaseCallback(unsigned long /*time*/, uint8_t trigger){
        return applyProgressive(-1, trigger);
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
      
      if (trigger == BUTTON_HOLD) waitBetween2update = (TIME_MAX_PROGRESSIVITY-time)/5;
      else waitBetween2update = 50 ; // 50ms 

      unsigned long delaySoFar = millis() - lastCall;

      if (delaySoFar < waitBetween2update) return false;  
      lastCall = millis();

      return true;    
    }

    virtual uint8_t getNumberOfProperties(){
      return skeleton->parameterAddresses.size();      
    }

    virtual bool actionCallback(uint8_t trigger)=0;
    virtual void serializeProperties()=0;
    virtual void deserializeProperty(String s)=0;        
    virtual void queryInterface() = 0; // this method is the one which is used to force poll data from the interface, 
    // it will usually drop a command to the interface which outcome be lately read by the process() method
    // is called from the focus() method so to force data to be updated in case some whre missed
    virtual const String* getTopLabel()=0;
    virtual const String* getBottomLabel()=0;
    
    // update is called by process() if data are availabe and must therefore implement the code that uses what is returned by read() by casting it to the right data structure    
    // it assign the GUI values the values that were acquired or processed in the process() method
    // basedon its internal state, it will set the redraw flag 
    //virtual void update() = 0;  
      
    // this code is requesting the strip to update its state (values, etc...) using data from the interface
    // process() is called from main app on all strip: the strip decide then if he has to do the query (for instance if it is focused) as       
    // some strip may do some backgroud tasks (like playing sample or executing a macro), other will only do this if they have focus         
    void process(){ 
      if (!interface) return;
      //if (interface->available() > 0) update();
    }


    void focus(bool f=true){
      //DEBUG_PRINT("\nStrip(%d)",index);
      if (f){
        brightness = BRIGHTNESS_MAX;
        //DEBUG_PRINT(" has focus");
        visible = true;
      }else{
        brightness = BRIGHTNESS_NOFOCUS;
        //DEBUG_PRINT(" lost focus");
      }
    }

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

      //interface->stripCount++;              // et signale qu'il utilise l'interface
      interface->registerEventListener(this); // cela permettra d'annuler le comptage des strips ci dessus


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

    virtual void pushToGUI(uint8_t layout){
      
      visible = true;
      focus(false);
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
            
      if (!determineXY(layout)){
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

    bool determineXY(uint8_t layout){      

      //DEBUG_PRINT("\nstrip[%d] %s topLeft=",index,type.c_str());
      // retrieve the corner of the strip where we can start drawing, which is topX, topY
      if (layout == LAYOUT_ZOOM || size == SHAPE4X4){
        topX = 0;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);
        areaW = WIDGET4X4W;
        areaH = WIDGET4X4H;        
      }
      else if (size == SHAPE1X4){
        topX = x * WIDGET1X4W;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2);        
        areaW = WIDGET1X4H;   
        areaH = WIDGET4X1H*0.6; // some margin to avoid text being rendered with overlap on top2label  
      }
      else if (size == SHAPE4X1){
        topX = 0;
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET4X1H) ;        
        areaW = WIDGET4X1W;
        areaH = WIDGET4X1H;        
      }
      else if (size == SHAPE2X2){
        topX = (x * WIDGET2X2W/2);
        topY = WIDGET_PADDING + (TOP2WIDGET_SIZE*2) + (y*WIDGET2X2H/2);
        areaW = WIDGET2X2W;
        areaH = WIDGET2X2H;
    
      }else{
        return false;
      }         

      centerX = topX + areaW / 2;       //middle
      centerY = topY + (areaH/2) -2;       //middle

      // fix position according to some widget size specificity, but only when layout is DETAILS:
      if (layout == LAYOUT_DETAILS){
        if (size == SHAPE4X1) simpleTextY -= WIDGET4X1H/2-3;     // fix vertical location for flat strip
        else if (size == SHAPE1X4) {
          // special treatment for vertical widget where text is rotated
          centerX = topX + WIDGET1X4W/2;   // exactly middle
          centerY = topY + WIDGET1X4H/2.5; // we want middle but for some reason we have to give it some margin
        }
      }

      //DEBUG_PRINT("(%i,%i)",topX,topY);
      return true;
    }

    #define SIMPLE_TEXT_MAX_LEN 20
    #define MIN_SIMPLETEXT_SIZE 40
    void pushSimpleText(uint8_t layout, const String* textOriginal) {
      // texte limité par SIMPLE_TEXT_MAX_LEN
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

      // Réduire la taille jusqu'à tenir, ou jusqu'à MIN
      while (fontSize > MIN_SIMPLETEXT_SIZE) {
        float charWidth = 0.4f * fontSize;
        float estimatedWidth = charWidth * len;

        if (estimatedWidth <= areaW) break;  // ça rentre, on s'arrête
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

    void pushIFState4x4(){
      if (!interface) return;
      gui.add(new Label(&interface->state.label, Align::Center,&GUI::midScreenHoriz, &GUI::topTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::defaultBcolor));    
      gui.add(new Led(&GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20, &interface->state.led));      
      gui.add(new Arc(&interface->state.progress, 0,1.0 , &GUI::vquarter1xPos, &GUI::midScreenVertical, SIZE_20+10, &GUI::gold, &GUI::transparentColor,0));
      gui.add(new Icon(interface->getIcon(),Align::Right,&GUI::rightMenuIconXpos, &GUI::iconY,MEDIUM_ICON_SIZE,&GUI::gold,&GUI::defaultBcolor));   
      gui.add(new TextArea(&interface->state.details,TFT_WIDTH/2.3 ,Align::Left,Align::Center,&GUI::midScreenHoriz,&GUI::iconY,CONFIG_TEXTAREA_MESSAGE_SIZE,&GUI::defaultFcolor,&GUI::defaultBcolor));           
      gui.add(new Label(lang.getString("Double clic on/off to toggle offline"), Align::Center,&GUI::midScreenHoriz, &GUI::belowBottomTexty,SIZE_14,40,&GUI::green,&GUI::transparentColor));          
      gui.add(new Label(&interface->state.subState, Align::Center,&GUI::midScreenHoriz, &GUI::bottomTextY,CONFIG_TEXTAREA_MESSAGE_SIZE,32, &GUI::gold,&GUI::defaultBcolor));
    }   

    void pushIFState1x4(){      
      IFledX = topX + WIDGET1X4W / 1.25;
      IFledY = topY + WIDGET1X4H / 2.5;

      IFiconX = topX + WIDGET1X4W / 1.05;
      IFiconY = topY + WIDGET1X4H / 1.8;      

      pushSmallLedandIcon();
    }      


    void pushIFState2x2(){      
      IFledX = topX + WIDGET2X2W / 2.5;
      IFledY = topY + SIZE_20;

      IFiconX = topX + WIDGET2X2W / 1.4;
      IFiconY = topY + SIZE_10 ;

      pushSmallLedandIcon();
    }     
    
    void pushIFState4x1(){      
      IFledX = GUI::leftMenuIconXpos+3;
      IFledY = topY + WIDGET4X1H/2;

      IFiconX = TFT_WIDTH - SIZE_26/2;
      IFiconY = topY + SIZE_26/4;

      pushSmallLedandIcon();
    }     

    void pushSmallLedandIcon(){
      if (!interface) return;
      gui.add(new Led(&IFledX, &IFledY, SIZE_10, &interface->state.led,&brightness));      
      gui.add(new Arc(&interface->state.progress, 0,1.0 , &IFledX, &IFledY, SIZE_16, &GUI::gold, &GUI::transparentColor,0,&brightness));
      gui.add(new Icon(interface->getIcon(),Align::Right,&IFiconX, &IFiconY,SIZE_26,&GUI::gold,&GUI::transparentColor,&brightness));    
    }

    virtual void composeWidgets(uint8_t layout){
      drawWidgetsFrames(layout);
    } 

    void drawWidgetsFrames(uint8_t layout){

      if (interface != nullptr) return;
      
      if (interface->state.enabled) frameColor = GUI::luminance(TFT_GOLD,127);     // dark frame
      else frameColor = TFT_RED;    // indicate offline

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        // having a frame when we are 4x4 is useless unless we want to indicate interface is offline
          gui.add(new Canva(&topX, &topY, WIDGET4X4W, WIDGET4X4H,4, &frameColor, &GUI::transparentColor,&brightness));  
      }
      else if ( size == SHAPE1X4) gui.add(new Canva(&topX, &topY, WIDGET1X4W, WIDGET1X4H, 4,&frameColor, &GUI::transparentColor,&brightness));
      else if (size == SHAPE2X2) gui.add(new Canva(&topX, &topY, WIDGET2X2W, WIDGET2X2H,4, &frameColor, &GUI::transparentColor,&brightness));  
      else if (size == SHAPE4X1) gui.add(new Canva(&topX, &topY, WIDGET4X1W, WIDGET4X1H,4, &frameColor, &GUI::transparentColor,&brightness));      
    }

    JsonArray properties;
    StripSkeleton* skeleton;
    //LinkedList<Widget*> widgetHandles = LinkedList<Widget*>();
    float value = 0.0, valueMin = 0, valueMax = 1;

    int16_t simpleTextX = 0, simpleTextY = 0;

    int16_t centerX = 0, centerY = 0;

    uint16_t areaW = 0, areaH = 0;

    int16_t IFledX = 0, IFledY = 0;
    int16_t IFiconX = 0, IFiconY = 0;

  protected:
    unsigned long lastCall;     
    uint16_t frameColor = TFT_GOLD;
    //Label* defaultLabel = nullptr;

};

class TwoSelectStrip : public Strip{

  public: 
    TwoSelectStrip(StripSkeleton* s, bool debug=false):Strip(s){
      Address* select2Address = s->getDataSetAddress(0);
      if (select2Address != nullptr){
        select2PropertyAddress = select2Address->pattern;
      }
    }

    void serializeProperties(){
      char* buf = (char*)malloc(select2PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select2PropertyAddress.c_str(),select2ID);
      properties.add(String(buf));
      free(buf);

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){      
      Strip::pushToGUI(layout);
    }    

    void deserializeProperty(String property){
      if (!interface) return;
      if (sscanf(property.c_str(), select2PropertyAddress.c_str(), &select2ID)){
        select2Content = skeleton->getValuePtr(interface->getProviderNameForData(),select2PropertyAddress,select2ID);
      }
    }  

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return select2Content;}     
 
  protected:
    Top1Toggle1Label* topLabels;
    uint8_t select2ID=1; 
    const String *select2Content;
    String select2PropertyAddress; 
    String label = "~";    
};


class ThreeSelectStrip: public TwoSelectStrip{
  public:
    ThreeSelectStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug){
      Address* select3Address = s->getDataSetAddress(1);
      if (select3Address != nullptr){
        select3PropertyAddress = select3Address->pattern;
      }
    }
  
    void serializeProperties(){
      TwoSelectStrip::serializeProperties();

      char* buf = (char*)malloc(select3PropertyAddress.length()*sizeof(char*));
      sprintf(buf,select3PropertyAddress.c_str(),select3ID);
      properties.add(String(buf));
      free(buf);

    }

    virtual void pushToGUI(uint8_t layout = LAYOUT_DETAILS){
      TwoSelectStrip::pushToGUI(layout);
    }    
    

    void deserializeProperty(String property){
      if (!interface) return;
      TwoSelectStrip::deserializeProperty(property);
      if (sscanf(property.c_str(), select3PropertyAddress.c_str(), &select3ID)) {
        select3Content = skeleton->getValuePtr(interface->getProviderNameForData(),select3PropertyAddress,select3ID-1);         
      }
    }  

    virtual const String* getTopLabel(){return select2Content;}
    virtual const String* getBottomLabel(){return select3Content;}      

  protected:
    uint8_t select3ID=1; 
    const String *select3Content;
    String select3PropertyAddress;
    Top2Toggles* topLabels;  
};


#endif
