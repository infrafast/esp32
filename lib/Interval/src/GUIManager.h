/**
 * @file GUIManager.h
 * @brief Builds and refreshes screen widgets using the configured rendering mode.
 */

#ifndef GUIManager_h
#define GUIManager_h

// https://forum.arduino.cc/t/tft-full-frame-buffer-possible-spi-dma/851348/6
//           // Select the frame buffer for graphics write (for 2 colour ePaper and DMA toggle buffer)
// Returns a pointer to the Sprite frame buffer
//void*    frameBuffer(int8_t f);

#include <TFT_eSPI.h>
//#define SPIFFS LittleFS           // this is to force TJpg_Decoder to use LittleFS, compile conflict for esp32
#include <TJpg_Decoder.h>
#include <LinkedList.h>

#include "GUIComponents.h"

#include "fonts.h"
//#define TTF_FONT NotoSans_Bold
#define TTF_FONT GeneratedFont

/**
 * @brief Maximum time spent rendering widgets in one application-loop pass.
 *
 * A short budget keeps input and MIDI processing responsive. The GUI continues
 * from the next widget on the following pass, so a complete screen may span
 * several passes. Override this value from the build flags when a different
 * display/latency trade-off is required.
 */
#ifndef GUI_REFRESH_BUDGET_US
  #define GUI_REFRESH_BUDGET_US 5000UL
#endif

/** @brief Minimum runtime GUI budget exposed by the development diagnostic menu. */
#define GUI_REFRESH_BUDGET_MIN_US 5000UL
/** @brief Maximum runtime GUI budget exposed by the development diagnostic menu. */
#define GUI_REFRESH_BUDGET_MAX_US 50000UL
/** @brief Runtime GUI budget increment used by the development diagnostic menu. */
#define GUI_REFRESH_BUDGET_STEP_US 5000UL

/**
 * @brief Page id, page state and redraw marker sharing one comparison value.
 */
union ScreenUnion{
  uint32_t value;
  struct {
    uint8_t id ;
    uint8_t state ;
    uint8_t focus;
    uint8_t reserved;
  };
}screen;

//#define USE_DMA

typedef void (*GUIBuilderCallBack)(uint8_t);

uint8_t renderLoop=0;
unsigned long roundTrip=0;
/** @brief Fixed buffers for optional render-loop diagnostics; no heap allocation per widget. */
char count[8] = "", count2[24] = "";

#ifdef USE_DMA
  uint16_t  dmaBuffer1[16*16]; ///< Toggle buffer for 16*16 MCU block, 512bytes
  uint16_t  dmaBuffer2[16*16]; ///< Toggle buffer for 16*16 MCU block, 512bytes
  uint16_t* dmaBufferPtr = dmaBuffer1;
  bool dmaBufferSel = 0;
#endif


/*bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
  if ( y >= TFT_HEIGHT ) return 0;
  GUI::tft->pushImage(x, y, w, h, bitmap);
  return 1;
}*/

/**
 * @brief This next function will be called during decoding of the jpeg file to render each
 * 16x16 or 8x8 image tile (Minimum Coding Unit) to the TFT.
 */
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= TFT_HEIGHT ) return 0;

  // STM32F767 processor takes 43ms just to decode (and not draw) jpeg (-Os compile option)
  // Total time to decode and also draw to TFT:
  // SPI 54MHz=71ms, with DMA 50ms, 71-43 = 28ms spent drawing, so DMA is complete before next MCU block is ready
  // Apparent performance benefit of DMA = 71/50 = 42%, 50 - 43 = 7ms lost elsewhere
  // SPI 27MHz=95ms, with DMA 52ms. 95-43 = 52ms spent drawing, so DMA is *just* complete before next MCU block is ready!
  // Apparent performance benefit of DMA = 95/52 = 83%, 52 - 43 = 9ms lost elsewhere
#ifdef USE_DMA
  // Double buffering is used, the bitmap is copied to the buffer by pushImageDMA() the
  // bitmap can then be updated by the jpeg decoder while DMA is in progress
  if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
  else dmaBufferPtr = dmaBuffer1;
  dmaBufferSel = !dmaBufferSel; // Toggle buffer selection
  //  pushImageDMA() will clip the image block at screen boundaries before initiating DMA
  GUI::tft->pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr); // Initiate DMA - blocking only if last DMA is not complete
  // The DMA transfer of image block to the TFT is now in progress...
#else
  // Non-DMA blocking alternative
  GUI::tft->pushImage(x, y, w, h, bitmap);  // Blocking, so only returns when image block is drawn
#endif
  // Return 1 to decode next block.
  return 1;
}

/**
 * @brief Pairs a page id with content and overlay construction callbacks.
 */
struct screenBuilder{
  uint8_t id;
  GUIBuilderCallBack screenCallback;
  GUIBuilderCallBack overlayCallback;
};

//EmptyWidget emptyWidget;

/**
 * @brief Owns displayed widgets and coordinates their rendering and color animations.
 *
 * With transparency enabled, widgets compose into a screen-sized buffer. Otherwise
 * changed sprites are sent directly to the display. Widgets are owned by the screen
 * list and deleted by purge(); application data bound to them stays borrowed.
 */
class GUIManager : public GenericComponent {  
  
  public:
    bool transparency;
    int sinI = 0; 
    uint32_t guiRefreshBudgetUs = GUI_REFRESH_BUDGET_US;
    #ifdef DEBUG
      String loopPerformance;       ///< Debug text containing average loop rate and period jitter.
      String guiBudgetTitle = "GUI BUDGET"; ///< Development-only budget page title.
      String guiRefreshBudgetText;   ///< Debug text containing the current GUI budget.
    #endif
    TFT_eSprite screenBuffer = TFT_eSprite(GUI::tft);  
    LinkedList<Widget*> screenGUIs = LinkedList<Widget*>();
    OpenFontRender labelOfr = OpenFontRender();
    OpenFontRender iconOfr = OpenFontRender();
    OpenFontRender userIconOfr = OpenFontRender();
    bool userIconFontLoaded = false;

    LinkedList<AlteredColor*> glowingColors = LinkedList<AlteredColor*>();
    LinkedList<AlteredColor*> alphaColors = LinkedList<AlteredColor*>();
    LinkedList<AlteredColor*> rainbowColors = LinkedList<AlteredColor*>();

    uint8_t sinusWave;

    /** @brief Initialize GUIManager with the supplied configuration and bindings. */
    GUIManager(bool t=false,bool debug=false):GenericComponent(debug){
      transparency = t;
      id="GUIManager";      
      TJpgDec.setCallback(tft_output);              
    }   

    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual ~GUIManager(){}

    /** @brief Prepare this object for use with its current configuration. */
    virtual void init() {      
      GenericComponent::init();
      
      /*labelOfr.setDebugLevel( OFR_INFO | OFR_ERROR);
      iconOfr.setDebugLevel( OFR_INFO | OFR_ERROR);*/

      if (labelOfr.loadFont(TTF_FONT, sizeof(TTF_FONT))) { // this consumes 20992 bytes and take ~2ms
        DEBUG_ERROR("\nLabel Initialise error"); 
      }
      
      /*
      // Open the font file
      File fontFile = LittleFS.open("/webapp/interval.ttf", "r");
      if (!fontFile) {
        DEBUG_PRINT("Failed to open icon file");
      }
      // Read font file contents into a buffer
      size_t dataSize = fontFile.size();
      uint8_t* fontBuffer = (uint8_t*)malloc(dataSize);
      fontFile.read(fontBuffer, dataSize);
      fontFile.close();*/

      size_t dataSize = 0;

      #define INTERVAL_FONT_PATH "/webapp/interval.ttf.gz"

      dataSize = readGzFileToBuffer(INTERVAL_FONT_PATH,&fontBuffer); 

      if (dataSize > 0){ // uncompress succeed
        //DEBUG_PRINT("\ndataSize: %i, sizeof(buffer):%i sizeof(TTF_ICON):%i",dataSize,sizeof(fontBuffer),sizeof(TTF_ICON));
        if (iconOfr.loadFont(fontBuffer.buffer, dataSize)) { // this consumes 20992 bytes and take ~2ms      
          //if (iconOfr.loadFont(TTF_ICON, sizeof(TTF_ICON))) {
          DEBUG_ERROR("\nIcon Initialise error with compressed font"); 
        }
      }else{
        /*if (iconOfr.loadFont(TTF_FONT, sizeof(TTF_FONT))) { //  use the default font as icon
        //if (iconOfr.loadFont(TTF_ICON, sizeof(TTF_ICON))) {
          if (debug) DEBUG_PRINT("\nIcon Initialise error with default font"); 
        }*/
       iconOfr = labelOfr;
      }

      // Optional application-owned icon font, kept separate from system icons.
      #define USER_ICON_FONT_PATH "/webapp/user.ttf.gz"
      size_t userIconDataSize = readGzFileToBuffer(USER_ICON_FONT_PATH,&userIconFontBuffer);
      if (userIconDataSize > 0) {
        userIconFontLoaded = !userIconOfr.loadFont(userIconFontBuffer.buffer, userIconDataSize);
        if (!userIconFontLoaded) DEBUG_ERROR("\nUser icon font initialise error");
      }

      

      //free(fontBuffer);

      widgetIndex = 0;

      if (transparency){
        //we create the screen buffer on which we will stack future transparent sprite
        screenBuffer.setColorDepth(BIT_DEPTH);
        screenBuffer.createSprite(TFT_WIDTH,TFT_HEIGHT);                
        //screenBuffer.setSwapBytes(true);        
      } 

      lastFXmillis = millis();
      lastLoopMetricsUs = micros();

    }

    /** @brief Draw a JPEG file from LittleFS at the requested position. */
    void drawJpg(int16_t x, int16_t y, String file){
      TJpgDec.drawFsJpg(x, y,file,LittleFS);
      //TJpgDec.drawJpg()
    }

    /** @brief Request clearing the composed screen buffer. */
    void clearScreenBuffer(){
      clearBuffer = true;
      //screenBuffer.fillSprite(TFT_BGCOLOR); 
      DEBUG_INFO("clearScreenBuffer true");
    }

    /**
     * @brief Refresh widget sprites and compose or send the changed display content.
     *
     * In transparency mode, force widget redraws and compose them into screenBuffer;
     * otherwise send changed sprites directly to the TFT. Rotated sprites use a
     * temporary bounding-box buffer with centered pivots. Consumed widget sprites
     * are deleted after transfer. Work is time-sliced using the 50 ms loop budget.
     * Color animations are updated periodically after widget processing.
     */
    bool refresh(bool force=false) {  
      
      // when using transparency, sprite need to be fully redrawn, hence we force the refresh
      force |= transparency;
      refreshed = force;

      renderLoop = 0;

      unsigned long t0 = micros(), t1;

      do{

        // we have reached the end so we loop
        if (widgetIndex >= screenGUIs.size()){
          widgetIndex = 0;
          if (transparency) clearScreenBuffer();
        }

        //get the next widget and porcess it
        Widget* widget = screenGUIs.get(widgetIndex);
        if (!widget){
          //we have to protect the GUI manager in case widget queue is empty
          //DEBUG_PRINT("\n!%i-no widget",widgetIndex);
          return false;
        }

        //For some reason DMA requires that a MISO pin is defined. If you do not have a spare then you can allocate it to be the same as the TFT_CS line and it will work, e.g. in your case add the following to the setup file:
        //#define TFT_MISO 17
        //https://github.com/Bodmer/TFT_eSPI/discussions/2233


        if (widget->refresh(force)){
          refreshed = true;
          TFT_eSprite* sprite = widget->sprite;
          if (sprite != nullptr){
            // when using transparency, widget sprite are pushed to the screen buffer instead of the screen directly
            
            if (widget->angle == 0) {
              // Pas de rotation → affichage direct (x,y = coin haut-gauche)
              if (transparency)
                sprite->pushToSprite(&screenBuffer, widget->getX(), widget->getY(), TFT_BGCOLOR);
              else
                sprite->pushSprite(widget->getX(), widget->getY(), TFT_TRANSPARENT);

            } else {
              // Rotation → sprite intermédiaire 16 bits, pivot centré
              const int w = sprite->width();
              const int h = sprite->height();
              const float a = deg2rad((float)widget->angle);

              const float ca = cosf(a);
              const float sa = sinf(a);

              // Bounding box après rotation (arrondi vers le haut pour éviter la coupe de bords)
              const int rotW = (int)ceilf(fabsf(w * ca) + fabsf(h * sa));
              const int rotH = (int)ceilf(fabsf(w * sa) + fabsf(h * ca));

              // Sprite intermédiaire 16 bits
              TFT_eSprite rotatedSprite(GUI::tft);
              rotatedSprite.setColorDepth(16);
              rotatedSprite.createSprite(rotW, rotH);
              rotatedSprite.fillSprite(TFT_TRANSPARENT);

              // Pivots centrés
              sprite->setPivot(w / 2, h / 2);
              rotatedSprite.setPivot(rotW / 2, rotH / 2);

              // Rotation : on pousse "sprite" dans "rotatedSprite"
              sprite->pushRotated(&rotatedSprite, widget->angle, TFT_TRANSPARENT);

              // ✅ Correction pour afficher le coin haut-gauche à getX/getY
              const int offsetX = (rotW - w) / 2;
              const int offsetY = (rotH - h) / 2;

              const int dstX = widget->getX() - offsetX;
              const int dstY = widget->getY() - offsetY;     

              if (transparency) {
                // Vers screenBuffer (coin haut-gauche calculé)
                rotatedSprite.pushToSprite(&screenBuffer, dstX, dstY, TFT_BGCOLOR);
              } else {
                // Directement vers l’écran
                rotatedSprite.pushSprite(dstX, dstY, TFT_TRANSPARENT);
              }

              rotatedSprite.deleteSprite(); // libère la RAM
            }


            
            delete sprite;        
            widget->sprite = nullptr;            
          }
        }

        // the screenBuffer is display when we reached the end of the widget queue
        if (widgetIndex++ == screenGUIs.size()-1 && transparency){        
          pushBuffer = true;
          //screenBuffer.pushSprite(0,0);        
        }    

        t1 = micros(); 
        roundTrip = t1-t0;

        snprintf(count, sizeof(count), "~%u~", ++renderLoop);
        snprintf(count2, sizeof(count2), "%luuS", roundTrip);

      }while(roundTrip<guiRefreshBudgetUs && widgetIndex != screenGUIs.size()-1);

      if (debug){
        if (roundTrip>guiRefreshBudgetUs){
          //DEBUG_PRINT("\ncause TRIP" );
        }else{
          //DEBUG_PRINT("\ncause SIZE");        
        }
        //DEBUG_PRINT("\t\trtrip:%i , rloop:%i, windex:%i/%i",roundTrip,renderLoop,widgetIndex,screenGUIs.size());
      }
   
      /**
       * @brief Update animated colors at a bounded rate.
       *
       * Widgets consume these shared values during their next refresh; this
       * block does not redraw the display by itself.
       */
      const unsigned long now = millis();
      if (now - lastFXmillis > 35){  // 35ms min to refresh
        increment = increment+1 % 255;
        sinI = (sinI + 10) % 360;  
        sinusWave = getSinVal(254); // range 254
        const uint16_t currentRainbow = GUI::rainbow(increment);
        for(int i = 0; i < glowingColors.size(); i++) {
          AlteredColor* color = glowingColors.get(i);
          *color->value = GUI::luminance(color->baseColor,sinusWave);
        }
        for(int i = 0; i < rainbowColors.size(); i++) {
          rainbowColors.get(i)->baseColor = currentRainbow;
        }
        for(int i = 0; i < alphaColors.size(); i++) {
          AlteredColor* color = alphaColors.get(i);
          *color->value = GUI::tft->alphaBlend(sinusWave, color->baseColor, GUI::defaultBcolor);
        }
        lastFXmillis = now;
      }      
      #ifdef DEBUG
        updateLoopMetrics();
      #endif

      return refreshed;
    }

    /** @brief Convert degrees to radians for rotated sprite placement. */
    float deg2rad(float deg) { return deg * 0.017453292519943295f; }

    /** @brief Scale the current sine-wave phase into the requested color-animation range. */
    uint16_t getSinVal(uint16_t range){
      return sin( (float)sinI * 6.28 / 360.0) * (range/2) + range/2;  
    }

    #ifdef DEBUG
    /**
     * @brief Update the loop-rate and period-jitter diagnostic once per second.
     *
     * The sample period is measured between completed GUI-manager refresh calls,
     * which correspond to completed application-loop passes in the normal app.
     * The displayed rate is the average loop frequency; jitter is the standard
     * deviation of the measured loop periods, in milliseconds.
     */
    void updateLoopMetrics(){
      const uint32_t nowUs = micros();
      if (lastLoopSampleUs != 0) {
        const uint32_t periodUs = nowUs - lastLoopSampleUs;
        loopPeriodSumUs += periodUs;
        loopPeriodSquareSumUs += static_cast<uint64_t>(periodUs) * periodUs;
        loopPeriodSamples++;
      }
      lastLoopSampleUs = nowUs;

      // Keep the hot path integer-only.  The floating-point statistics below
      // are evaluated once per reporting interval, never once per loop.
      if (nowUs - lastLoopMetricsUs < 1000000UL || loopPeriodSamples == 0) return;

      const double meanPeriodUs = static_cast<double>(loopPeriodSumUs) / loopPeriodSamples;
      double varianceUs2 = static_cast<double>(loopPeriodSquareSumUs) / loopPeriodSamples
                         - meanPeriodUs * meanPeriodUs;
      if (varianceUs2 < 0.0) varianceUs2 = 0.0;

      const uint32_t loopsPerSecond = (meanPeriodUs > 0.0)
                                    ? static_cast<uint32_t>(1000000.0 / meanPeriodUs + 0.5)
                                    : 0;
      const uint32_t jitterMs = static_cast<uint32_t>(sqrt(varianceUs2) / 1000.0 + 0.5);
      loopPerformance = String(loopsPerSecond) + "/s +/-" + String(jitterMs) + "ms";

      loopPeriodSumUs = 0;
      loopPeriodSquareSumUs = 0;
      loopPeriodSamples = 0;
      lastLoopMetricsUs = nowUs;
    }

    /** @brief Increase the runtime GUI budget by one development-menu step. */
    void increaseGuiRefreshBudget(){
      if (guiRefreshBudgetUs < GUI_REFRESH_BUDGET_MAX_US) {
        guiRefreshBudgetUs += GUI_REFRESH_BUDGET_STEP_US;
      }
      updateGuiRefreshBudgetText();
    }

    /** @brief Decrease the runtime GUI budget by one development-menu step. */
    void decreaseGuiRefreshBudget(){
      if (guiRefreshBudgetUs > GUI_REFRESH_BUDGET_MIN_US) {
        guiRefreshBudgetUs -= GUI_REFRESH_BUDGET_STEP_US;
      }
      updateGuiRefreshBudgetText();
    }

    /** @brief Format the runtime GUI budget for the development diagnostic page. */
    void updateGuiRefreshBudgetText(){
      guiRefreshBudgetText = String(guiRefreshBudgetUs / 1000) + " ms";
    }
    #endif

    /** @brief Append an entry to this collection. */
    bool add(Widget* w){
      if (screenGUIs.add(w)) return true;
      else return false;
    }

    /**
     * @brief Delete all current screen widgets before building another page.
     *
     * Widget bindings borrow application data, so this deletes presentation objects,
     * not the strips, interfaces or values they display.
     */
    void purge(){
      while (screenGUIs.size()>0){
        //DEBUG_PRINT("\nremaining to unstack: %i",gui->screenGUIs.size());
        Widget* widget = screenGUIs.shift();
        //DEBUG_PRINT("\nobject: %s ",widget->id.c_str());
        delete(widget);
        widget = nullptr;
      }
      //gui->screenGUIs.clear();      
    }

    /** @brief Remove the matching entry from the registry. */
    void remove(Widget *w){
      for (uint8_t i=0; i<screenGUIs.size();i++){
        if (screenGUIs.get(i) == w){
          screenGUIs.remove(i);
          break;
        }
      }
    }

  private:   
    ByteBuffer fontBuffer;
    ByteBuffer userIconFontBuffer;

    uint8_t increment = 0;

    String i18n_token;  
    
    LinkedList<Widget*> overlayGUIs = LinkedList<Widget*>();
    LinkedList<String*> i18nRepo = LinkedList<String*>();
    uint8_t widgetIndex = 0;

    unsigned long lastFXmillis;
    #ifdef DEBUG
      uint32_t lastLoopSampleUs = 0;
      uint32_t lastLoopMetricsUs = 0;
      uint32_t loopPeriodSamples = 0;
      uint64_t loopPeriodSumUs = 0;
      uint64_t loopPeriodSquareSumUs = 0;
    #endif


};

/*************************************************** IMPLEMENTATION ********************************************************/

#endif
