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
String count="",count2="";

#ifdef USE_DMA
  uint16_t  dmaBuffer1[16*16]; // Toggle buffer for 16*16 MCU block, 512bytes
  uint16_t  dmaBuffer2[16*16]; // Toggle buffer for 16*16 MCU block, 512bytes
  uint16_t* dmaBufferPtr = dmaBuffer1;
  bool dmaBufferSel = 0;
#endif


/*bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap){
  if ( y >= TFT_HEIGHT ) return 0;
  GUI::tft->pushImage(x, y, w, h, bitmap);
  return 1;
}*/

// This next function will be called during decoding of the jpeg file to render each
// 16x16 or 8x8 image tile (Minimum Coding Unit) to the TFT.
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

struct screenBuilder{
  uint8_t id;
  GUIBuilderCallBack screenCallback;
  GUIBuilderCallBack overlayCallback;
};

//EmptyWidget emptyWidget;

class GUIManager : public GenericComponent {  
  
  public:
    bool transparency;
    int sinI = 0; 
    String framePerSec;
    TFT_eSprite screenBuffer = TFT_eSprite(GUI::tft);  
    LinkedList<Widget*> screenGUIs = LinkedList<Widget*>();
    OpenFontRender labelOfr = OpenFontRender();
    OpenFontRender iconOfr = OpenFontRender(); 

    LinkedList<AlteredColor*> glowingColors = LinkedList<AlteredColor*>();
    LinkedList<AlteredColor*> alphaColors = LinkedList<AlteredColor*>();
    LinkedList<AlteredColor*> rainbowColors = LinkedList<AlteredColor*>();

    uint8_t sinusWave;

    GUIManager(bool t=false,bool debug=false):GenericComponent(debug){
      transparency = t;
      id="GUIManager";      
      TJpgDec.setCallback(tft_output);              
    }   

    virtual ~GUIManager(){}

    virtual void init() {      
      GenericComponent::init();
      
      /*labelOfr.setDebugLevel( OFR_INFO | OFR_ERROR);
      iconOfr.setDebugLevel( OFR_INFO | OFR_ERROR);*/

      if (labelOfr.loadFont(TTF_FONT, sizeof(TTF_FONT))) { // this consumes 20992 bytes and take ~2ms
        DEBUG_PRINT("\nLabel Initialise error"); 
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
          DEBUG_PRINT("\nIcon Initialise error with compressed font"); 
        }
      }else{
        /*if (iconOfr.loadFont(TTF_FONT, sizeof(TTF_FONT))) { //  use the default font as icon
        //if (iconOfr.loadFont(TTF_ICON, sizeof(TTF_ICON))) {
          if (debug) DEBUG_PRINT("\nIcon Initialise error with default font"); 
        }*/
       iconOfr = labelOfr;
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
      lastFPSmillis = millis();

    }

    void drawJpg(int16_t x, int16_t y, String file){
      TJpgDec.drawFsJpg(x, y,file,LittleFS);
      //TJpgDec.drawJpg()
    }

    void clearScreenBuffer(){
      clearBuffer = true;
      //screenBuffer.fillSprite(TFT_BGCOLOR); 
      DEBUG_PRINT("\nclearScreenBuffer true");
    }

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

              // Destination : widget->getX()/getY() est le PIVOT (centre à l’écran/buffer)
              const int dstX = widget->getX() - rotW / 2;
              const int dstY = widget->getY() - rotH / 2;

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

        count = "~"+String(++renderLoop)+"~";
        count2 = String(roundTrip)+"uS";

        #define RTRIP 50000UL

      }while(roundTrip<RTRIP && widgetIndex != screenGUIs.size()-1);    // we render a maximum of widget in 50ms      

      if (debug){
        if (roundTrip>RTRIP){
          //DEBUG_PRINT("\ncause TRIP" );
        }else{
          //DEBUG_PRINT("\ncause SIZE");        
        }
        //DEBUG_PRINT("\t\trtrip:%i , rloop:%i, windex:%i/%i",roundTrip,renderLoop,widgetIndex,screenGUIs.size());
      }
   
      /* FX stuff*/
      if (millis() - lastFXmillis > 35){  // 35ms min to refresh
        increment = increment+1 % 255;
        sinI = (sinI + 10) % 360;  
        sinusWave = getSinVal(254); // range 254
        for(int i = 0; i < glowingColors.size(); i++) *glowingColors.get(i)->value = GUI::luminance(glowingColors.get(i)->baseColor,sinusWave);
        for(int i = 0; i < rainbowColors.size(); i++) rainbowColors.get(i)->baseColor = GUI::rainbow(increment);
        for(int i = 0; i < alphaColors.size(); i++) *alphaColors.get(i)->value = GUI::tft->alphaBlend(sinusWave, alphaColors.get(i)->baseColor, GUI::defaultBcolor);
        lastFXmillis = millis();
      }      
      fps(1);

      return refreshed;
    }

    float deg2rad(float deg) { return deg * 0.017453292519943295f; }

    uint16_t getSinVal(uint16_t range){
      return sin( (float)sinI * 6.28 / 360.0) * (range/2) + range/2;  
    }

    int fps(const uint8_t seconds){
      unsigned long now = millis();
      int fpsVal;
      frameCount ++;
      if (now - lastFPSmillis >= seconds * 1000) {
        fpsVal = frameCount / seconds;
        framePerSec = String(fpsVal);        
        frameCount = 0;
        lastFPSmillis = now;
      }
      return fpsVal;
    }

    bool add(Widget* w){
      if (screenGUIs.add(w)) return true;
      else return false;
    }

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

    uint8_t increment = 0;

    String i18n_token;  
    
    LinkedList<Widget*> overlayGUIs = LinkedList<Widget*>();
    LinkedList<String*> i18nRepo = LinkedList<String*>();
    uint8_t widgetIndex = 0;

    unsigned long lastFPSmillis,lastFXmillis;
    unsigned long frameCount;


};

/*************************************************** IMPLEMENTATION ********************************************************/

#endif