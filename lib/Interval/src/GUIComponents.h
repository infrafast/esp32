#ifndef GUIComponents_h
#define GUIComponents_h

#include <OpenFontRender.h>
#include <vector>
#include "Interval.h"

#include "iconMap.h"

#ifndef TFT_BGCOLOR
  #define TFT_BGCOLOR TFT_BLACK
#endif

#ifndef TFT_FCOLOR
  #define TFT_FCOLOR TFT_WHITE
#endif

#define DARKER_GREY TFT_DARKGREY // 0x18E3
#define TFT_NEARLYBLACK 1 // when we need to have a component close to black but not being treated as the transparency color when pushing sprite to buffer

#define BIT_DEPTH 8


typedef struct {
  uint16_t fColor;
  uint16_t bColor;
} stripColor;

typedef struct {
  uint16_t baseColor;
  uint16_t* value;
}AlteredColor;

#define TFT_GREY 0x5AEB // New colour

#define MAX_COLOR_ID 17

stripColor colorMap[MAX_COLOR_ID] = {
  {TFT_GREY, TFT_BLACK},
  {TFT_GREY, TFT_RED},
  {TFT_GREY, TFT_GREEN},
  {TFT_GREY, TFT_YELLOW},
  {TFT_GREY, TFT_BLUE},
  {TFT_GREY, TFT_MAGENTA},
  {TFT_GREY, TFT_CYAN},
  {TFT_GREY, TFT_WHITE},
  {TFT_GREY, TFT_SILVER},
  {TFT_RED, TFT_GREY},
  {TFT_GREEN, TFT_GREY},
  {TFT_YELLOW, TFT_GREY},
  {TFT_BLUE, TFT_GREY},
  {TFT_MAGENTA, TFT_GREY},
  {TFT_CYAN, TFT_GREY},
  {TFT_WHITE, TFT_GREY},
  {TFT_GREY, TFT_ORANGE},
};

/////////////////////////////////////// FIXED SIZE 
#define SIZE_10 10
#define SIZE_14 14
#define SIZE_16 16
#define SIZE_18 18
#define SIZE_20 20
#define SIZE_26 26
///////////////////////////////////////

class GUI : public GenericComponent{
public:
    static TFT_eSPI* tft;      
    static bool off;
    static bool on;
    
    // modifiable colors
    static uint16_t unUsedColor;
    static uint16_t defaultFcolor;
    static uint16_t defaultBcolor;
    static uint16_t widgetInnerColor;
    static uint16_t widgetSubLabelColor;
    static AlteredColor glowingGoldColor;    

    // const colors
    static const uint16_t transparentColor;
    static const uint16_t nearlyBlack;
    static const uint16_t green;   
    static const uint16_t silver;
    static const uint16_t red;
    static const uint16_t orange;
    static const uint16_t white;
    static const uint16_t purple;
    static const uint16_t yellow;
    static const uint16_t gold;
    static const uint16_t blue;
    static const uint16_t menuActiveIconColor;
    static const uint16_t menuInactiveIconColor;
    static const uint16_t menuGreydoutColor;
    static const uint16_t menuBackGroundColor;

    // const icon
    static const String iconMixEthWiFi;
    static const String iconEth;
    static const String iconWiFi;
    

    // default luminance
    static const float fullBrightness;
    static const float greydOutBrightness;
    static float systemBrightness;

    // const positions and size
    #ifdef TFT_WIDTH
      #undef TFT_WIDTH
    #endif
    #ifdef TFT_HEIGHT
      #undef TFT_HEIGHT
    #endif
    #define TFT_WIDTH   320   //GUI::width()
    #define TFT_HEIGHT  240   //GUI::height()

    ///////////////////////////////////////// ICON SIZE AND POSITION  ////////////////////////////
    #define ICON_MAX_SIZE TFT_WIDTH/2
    #define MEDIUM_ICON_SIZE (ICON_MAX_SIZE/1.7)
    ///////////////////////////////////////// WIDGET SIZE AND POSITION  ////////////////////////////
    #define TOP2WIDGET_SIZE (TFT_HEIGHT/7)
    #define WIDGET_BIG_WIDTH TFT_WIDTH/5
    #define WIDGET_SMALL_WIDTH TFT_WIDTH/8-5

    #define TFT_HQUARTER TFT_WIDTH/4
    #define TFT_VQUARTER TFT_HEIGHT/4
    #define TFT_HTHIRD TFT_WIDTH/3

    #define WIDGET_PADDING 4
    ///////////////////////////////////////// MENU SIZE AND POSITION ///////////////////////////////
    #define MENU_ICON_SIZE TFT_WIDTH/3
    #define MENU_TITLE_SIZE MENU_ICON_SIZE/2
    #define CONFIG_TEXTAREA_MESSAGE_SIZE MENU_TITLE_SIZE/1.8

    
    static const int16_t zero;
    static const int16_t un;
    static const int16_t three;
    static const int16_t center;
    static const int16_t y1;
    static const int16_t tenPixels;
    static const int16_t midScreenHoriz;
    static const int16_t midScreenVertical;
    static const int16_t bigWidgetMidY;
    static const int16_t iconMidY;    
    static const int16_t iconY;
    static const int16_t topTextY;
    static const int16_t bottomTextY;
    static const int16_t belowBottomTexty;
    static const int16_t leftMenuIconXpos;
    static const int16_t rightMenuIconXpos;
    static const int16_t hquarter1xPos;
    static const int16_t hquarter2xPos;
    static const int16_t hquarter3xPos;
    static const int16_t hquarter4xPos;
    static const int16_t vquarter1xPos;
    static const int16_t vquarter2xPos;
    static const int16_t vquarter3xPos;
    static const int16_t vquarter4xPos;
    static const int16_t menuIconYpos;
    static const int16_t menuTitleY;

    /***************************************************************************************
    ** Static utilities methods
    ***************************************************************************************/    
    
    static inline void cls(uint16_t color = GUI::defaultBcolor){
      if (color == TFT_TRANSPARENT) color = GUI::defaultBcolor;
      tft->fillScreen(GUI::luminance(color,GUI::systemBrightness * 255));  
      //DEBUG_PRINT("\nGUI::cls()");
    }

    static inline uint16_t width(){
      return tft->width();
    }

    static inline uint16_t height(){
      return tft->height();
    }

    /***************************************************************************************
    ** Function name:           luminance
    ** Description:             return a 16 bit colour with reduced luminance
    ***************************************************************************************/
    static inline uint16_t luminance(uint16_t color, uint8_t luminance){
      if (color == TFT_BLACK) return TFT_BLACK;
      // Extract rgb colours and stretch range to 0 - 255
      uint16_t r = (color & 0xF800) >> 8; r |= (r >> 5);
      uint16_t g = (color & 0x07E0) >> 3; g |= (g >> 6);
      uint16_t b = (color & 0x001F) << 3; b |= (b >> 5);

      b = ((b * (uint16_t)luminance + 255) >> 8) & 0x00F8;
      g = ((g * (uint16_t)luminance + 255) >> 8) & 0x00FC;
      r = ((r * (uint16_t)luminance + 255) >> 8) & 0x00F8;

      return (r << 8) | (g << 3) | (b >> 3);
    }


    /***************************************************************************************
    ** Function name:           luminance
    ** Description:             return a 16 bit colour from an RGB with reduced luminance
    ***************************************************************************************/
    static inline uint16_t luminance(uint8_t red, uint8_t green, uint8_t blue, uint8_t luminance){
      uint16_t b = (((uint16_t)blue  * (uint16_t)luminance + 255) >> 8) & 0x00F8;
      uint16_t g = (((uint16_t)green * (uint16_t)luminance + 255) >> 8) & 0x00FC;
      uint16_t r = (((uint16_t)red   * (uint16_t)luminance + 255) >> 8) & 0x00F8;

      return (r << 8) | (g << 3) | (b >> 3);
    }

    // #########################################################################
    // Return a 16 bit rainbow colour
    // #########################################################################
    static inline uint16_t rainbow(byte value)
    {
      // If 'value' is in the range 0-159 it is converted to a spectrum colour
      // from 0 = red through to 127 = blue to 159 = violet
      // Extending the range to 0-191 adds a further violet to red band
    
      value = value%192;
      
      byte red   = 0; // Red is the top 5 bits of a 16 bit colour value
      byte green = 0; // Green is the middle 6 bits, but only top 5 bits used here
      byte blue  = 0; // Blue is the bottom 5 bits

      byte sector = value >> 5;
      byte amplit = value & 0x1F;

      switch (sector)
      {
        case 0:
          red   = 0x1F;
          green = amplit; // Green ramps up
          blue  = 0;
          break;
        case 1:
          red   = 0x1F - amplit; // Red ramps down
          green = 0x1F;
          blue  = 0;
          break;
        case 2:
          red   = 0;
          green = 0x1F;
          blue  = amplit; // Blue ramps up
          break;
        case 3:
          red   = 0;
          green = 0x1F - amplit; // Green ramps down
          blue  = 0x1F;
          break;
        case 4:
          red   = amplit; // Red ramps up
          green = 0;
          blue  = 0x1F;
          break;
        case 5:
          red   = 0x1F;
          green = 0;
          blue  = 0x1F - amplit; // Blue ramps down
          break;
      }

      return red << 11 | green << 6 | blue;
    }  

    GUI(bool d=true):GenericComponent(d){};
};

// #########################################################################
// Base class
// #########################################################################
class Widget : public GUI {

  public:
    uint16_t width = 1;
    uint16_t height = 1;
    TFT_eSprite* sprite = nullptr;
    const float* brightness = &GUI::fullBrightness;
    const uint16_t *fColor = nullptr, *bColor = nullptr;
    float oldBrightness = -1.0f;
    int16_t angle = 0;

    Widget(const int16_t* xPos,
           const int16_t* yPos,
           const uint16_t* fC,
           const uint16_t* bC,
           const float* _brightness = &GUI::fullBrightness,
           bool d = false)
      : GUI(d), brightness(_brightness) {
      x = xPos;
      y = yPos;
      fColor = fC;
      bColor = bC;
    }

    Widget(const Widget&) = delete;
    Widget& operator=(const Widget&) = delete;

    virtual ~Widget() {
      destroySprite();
    }

    void setFcolor(const uint16_t* fC) {
      fColor = fC;
    }

    virtual bool refresh(bool force = false) {
      if (x == nullptr || y == nullptr || fColor == nullptr || bColor == nullptr || brightness == nullptr) {
        refreshed = false;
        return false;
      }

      bool changed = force;

      if (oldX != *x ||
          oldY != *y ||
          oldfColor != *fColor ||
          oldbColor != *bColor ||
          oldBrightness != *brightness ||
          oldWidth != width ||
          oldHeight != height) {

        oldBrightness = *brightness;
        oldX = *x;
        oldY = *y;
        oldfColor = *fColor;
        oldbColor = *bColor;
        oldWidth = width;
        oldHeight = height;
        changed = true;
      }

      refreshed = changed;
      return refreshed;
    }

    void createSprite() {
      const uint16_t color = (bColor != nullptr) ? *bColor : GUI::defaultBcolor;
      createSprite(color);
    }

    void createSprite(uint16_t color) {
      const uint16_t w = (width == 0) ? 1 : width;
      const uint16_t h = (height == 0) ? 1 : height;

      if (sprite == nullptr) {
        sprite = new TFT_eSprite(tft);
        sprite->createSprite(w, h);
        spriteWidth = w;
        spriteHeight = h;
        fillSpriteBackground(color);
        return;
      }

      if (spriteWidth != w || spriteHeight != h) {
        sprite->deleteSprite();
        sprite->createSprite(w, h);
        spriteWidth = w;
        spriteHeight = h;
        fillSpriteBackground(color);
      }
    }

    void destroySprite() {
      if (sprite != nullptr) {
        sprite->deleteSprite();
        delete sprite;
        sprite = nullptr;
      }
      spriteWidth = 0;
      spriteHeight = 0;
    }

    uint16_t getClearColor() {
      const uint16_t localBColor = (bColor != nullptr) ? *bColor : GUI::defaultBcolor;
      const float localBrightness = (brightness != nullptr) ? *brightness : GUI::fullBrightness;

      if (localBColor == TFT_TRANSPARENT) {
        return GUI::luminance(GUI::defaultBcolor, localBrightness * GUI::systemBrightness * 255);
      }
      return GUI::luminance(localBColor, localBrightness * GUI::systemBrightness * 255);
    }

    virtual int16_t getX() { return (x != nullptr) ? *x : 0; }
    virtual int16_t getY() { return (y != nullptr) ? *y : 0; }

  protected:
    const int16_t *x = nullptr, *y = nullptr;
    int16_t oldX = -1, oldY = -1;

    uint16_t oldfColor = 0xFFFF, oldbColor = 0xFFFF;

    uint16_t oldWidth = 0, oldHeight = 0;
    uint16_t spriteWidth = 0, spriteHeight = 0;

    void fillSpriteBackground(uint16_t color) {
      if (sprite == nullptr) return;

      if (color == TFT_TRANSPARENT) {
        sprite->fillSprite(TFT_TRANSPARENT);
      } else {
        const float localBrightness = (brightness != nullptr) ? *brightness : GUI::fullBrightness;
        sprite->fillSprite(GUI::luminance(color, localBrightness * GUI::systemBrightness * 255));
      }
    }
};

class Canva : public Widget{

  public:

    Canva(const int16_t* posX, const int16_t* posY, uint16_t width, uint16_t height, uint8_t _round, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness, bool d=false):Widget(posX, posY, fC, bC, _brightness,d), round(_round){      
      id = concatStrings("Canva", *posX, "x", *posY);
      this->width = width; this->height = height;
    } 

    virtual bool refresh(bool force=false){ 
      refreshed = force;
      if(Widget::refresh(force)){     
        createSprite(GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));
        uint16_t color = *fColor;
        sprite->drawRoundRect(0,0,width,height,round,GUI::luminance(color,*brightness * GUI::systemBrightness * 255));
        // background is done by filling the sprite with bColor when creating it
      }
      return refreshed;
    }

  private:  
    uint8_t round = 0;

};


/*class EmptyWidget  : public Widget{
  public:
    int16_t x1=0,y1=0;
    EmptyWidget():Widget(&x1, &y1, &GUI::defaultFcolor, &GUI::defaultBcolor, false){};
    virtual void setText(const String* text){};
};*/

// workflow
// https://boxy-svg.com/app/disk:uSMcdhhJG4 -> https://icomoon.io/app -> https://products.aspose.app/font/generator/ttf-to-ttf -> https://notisrac.github.io/FileToCArray/          ---> preview: https://tophix.com/font-tools/font-editor
//                                                                    -> https://everythingfonts.com/subsetter

#define LIMIT_TEXT_RENDERING_LENGTH 99    // on affiche pas plus de 15 caracteres sinon le texte est illisible;

class Font : public Widget {  
  public:    
    uint8_t maxLen = LIMIT_TEXT_RENDERING_LENGTH;

    Font(const String* v, Align a, const int16_t* posX, const int16_t* posY,
         uint16_t s, uint8_t _len = LIMIT_TEXT_RENDERING_LENGTH,
         const uint16_t *fC = &GUI::defaultFcolor,
         const uint16_t *bC = &GUI::defaultBcolor,
         const float* _brightness = &GUI::fullBrightness,
         bool d = true)
      : Widget(posX, posY, fC, bC, _brightness, d), value(v), maxLen(_len) {
      init(a, posX, posY, s);
    }

    Font(Align a, const int16_t* posX, const int16_t* posY,
         uint16_t s,
         const uint16_t *fC = &GUI::defaultFcolor,
         const uint16_t *bC = &GUI::defaultBcolor,
         const float* brightness = &GUI::fullBrightness,
         bool d = true)
      : Widget(posX, posY, fC, bC, brightness, d), value(nullptr) {
      init(a, posX, posY, s);
    }

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    void init(Align a, const int16_t* posX, const int16_t* posY, uint16_t s) {
      size = s;
      align = a;
      id = concatStrings("Font", *posX, "x", *posY);
    }

    virtual int16_t getX() override {
      if (x == nullptr) return 0;
      int16_t xAligned = *x;
      if (align == Align::Center) xAligned = *x - width / 2;
      if (align == Align::Right)  xAligned = *x - width;
      return xAligned;
    }

    virtual bool refresh(bool force = false) override { 
      refreshed = force;

      if (value == nullptr) {
        oldValue = "";
        oldValuePtr = nullptr;
        return Widget::refresh(force);
      }

      bool valuePointerChanged = (value != oldValuePtr);
      if (valuePointerChanged) {
        oldValuePtr = value;
      }

      if (Widget::refresh(force || valuePointerChanged) || *value != oldValue) {       
        String truncatedValue = value->substring(0, maxLen);
        if (value->length() > maxLen && maxLen >= 3) {
          truncatedValue = truncatedValue.substring(0, maxLen - 3) + "...";
        }

        FT_BBox boundaries1 = getOfr()->calculateBoundingBox(0, 0, size, Align::Left, Layout::Horizontal, oldValue.substring(0, maxLen).c_str());
        FT_BBox boundaries2 = getOfr()->calculateBoundingBox(0, 0, size, Align::Left, Layout::Horizontal, truncatedValue.c_str());

        width  = max(boundaries1.xMax, boundaries2.xMax) + 1;
        height = max(boundaries1.yMax, boundaries2.yMax) + 1;

        if (width > tft->width())   width = tft->width();
        if (height > tft->height()) height = tft->height();
        if (width == 0)  width = 1;
        if (height == 0) height = 1;

        createSprite();
        fillSpriteBackground((bColor != nullptr) ? *bColor : GUI::defaultBcolor);

        getOfr()->setFontSize(size);
        getOfr()->setDrawer(*sprite);
        getOfr()->setCursor(width / 2, 0);

        uint16_t fCol1 = GUI::luminance(*fColor, *brightness * GUI::systemBrightness * 255);
        uint16_t bCol1 = GUI::luminance(*bColor, *brightness * GUI::systemBrightness * 255);
        getOfr()->setFontColor(fCol1, bCol1);

        getOfr()->cprintf(truncatedValue.c_str());

        oldValue = *value;            
        refreshed = true;
      }
      return refreshed;
    }

  private:    
    String oldValue = "?";
    const String* oldValuePtr = nullptr;
    Align align = Align::Left;

  protected:
    const String *value = nullptr;
    uint16_t size = 0;
    virtual OpenFontRender* getOfr() = 0;
};

class Label : public Font {
  public:
    static OpenFontRender* ofr;

    Label(const String* v, Align a, const int16_t* posX, const int16_t* posY,
          uint16_t s, uint8_t _len = LIMIT_TEXT_RENDERING_LENGTH,
          const uint16_t *fC = &GUI::defaultFcolor,
          const uint16_t *bC = &GUI::defaultBcolor,
          const float* _brightness = &GUI::fullBrightness,
          bool d = false)
      : Font(v, a, posX, posY, s, _len, fC, bC, _brightness, d) {
      id = "Label";
    }

    Label(const Label&) = delete;
    Label& operator=(const Label&) = delete;

  protected:
    virtual OpenFontRender* getOfr() override { return ofr; }
};

struct LineBreakInfo
{
    int lines;
    std::vector<int> lineBreaks;
    int maxWidth;
    int maxHeight;

    LineBreakInfo()
    {}

    LineBreakInfo(int l, std::vector<int> lb, int mw)
    {
        lines = l;
        lineBreaks = lb;
        maxWidth = mw;
    }
};

class TextArea : public Widget{
  public:

    TextArea(const String* textvalue, uint16_t w, Align pAlign, Align aAlign, const int16_t* posX, const int16_t* posY, uint8_t fontSize, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool d=false):Widget(posX, posY, fC, bC, _brightness, d){
      areaAlign = aAlign;
      phraseAlign = pAlign;
      text = textvalue;
      size = fontSize;
      width = w;
    }  

    virtual int16_t getX(){
      int16_t xAligned = *x;  // align left by default
      if (areaAlign == Align::Center) xAligned = *x - width/2;
      if (areaAlign == Align::Right) xAligned = *x - width;
      return xAligned;
    }
  
    virtual bool refresh(bool force=false){
      refreshed = force;
      Widget::refresh(force);

      if (text == nullptr) {
        oldText = "";
        oldTextPtr = nullptr;
        return refreshed;
      }

      bool textPointerChanged = (text != oldTextPtr);
      if (textPointerChanged) oldTextPtr = text;

      if (*text != oldText || force || textPointerChanged){

        LineBreakInfo lbi = computeLineBreakInfo(*text, width);
        //if (debug) DEBUG_PRINT("\nlbi.lines:%i, lbi.maxWidth:%i, lbi.maxHeight:%i\n",lbi.lines,lbi.maxWidth,lbi.maxHeight);

        std::vector<String> allWords = getWords(*text);
        //if (debug) for (auto &&word : allWords) DEBUG_PRINT("\n%s",word.c_str());
        std::vector<String> phrases;
        String phrase = "";

        int j=0;
        
        for (int i=0; i<text->length(); i++){
          char c = text->charAt(i);
          //if (debug) DEBUG_PRINT("%c",c);
          if(j < lbi.lineBreaks.size() && i == lbi.lineBreaks[j]){
              j++;        
              phrases.push_back(phrase);
              phrase = "";
              continue;
          }
          phrase += c;
        }
        phrases.push_back(phrase);  // do not forget last one

        int16_t offsetY = 0;
        //height = size * lbi.lines;
        //height = Label::ofr->getTextHeight(text->c_str())*lbi.lines;
        Label l(text, phraseAlign, &GUI::zero, &GUI::zero, size,
                LIMIT_TEXT_RENDERING_LENGTH, fColor, bColor, brightness, debug);
        l.refresh(true);    // the only way to know the maximum height of a line is to draw it (without rendering)
        height = l.height * lbi.lines;
        createSprite();

        for (auto &&nextPhrase : phrases){
          //if (debug) DEBUG_PRINT("\n%s",nextPhrase.c_str());
          //tft.drawString(nextPhrase, 0, offsetY, FONT4);

          Label* label = new Label(&nextPhrase,phraseAlign,&GUI::zero,&GUI::zero,size,LIMIT_TEXT_RENDERING_LENGTH,fColor,bColor,brightness,debug);
          label->refresh(true);
          label->sprite->pushToSprite(sprite,getAlignedX(label->width),offsetY);     
          offsetY += label->height;
          delete label;         

          //offsetY += Label::ofr->getTextHeight(nextPhrase.c_str());
          //offsetY += size;

        }
        
        oldText = *text;
        refreshed = true;

      }
      
      return refreshed;
    }

    virtual int16_t getAlignedX(uint16_t phraseWidth){
      int16_t xAligned = 0;  // align left by default
      if (phraseAlign == Align::Center) xAligned = (width-phraseWidth)/2;
      if (phraseAlign == Align::Right) xAligned = width-phraseWidth;
      return xAligned;
    }

  protected:

  private:
    Align areaAlign, phraseAlign;
    const String* text;
    String oldText;
    const String* oldTextPtr = nullptr;
    uint8_t size;

    std::vector<String> getWords(String text){

      std::vector<String> words;
      int index = 0; // Index dans la chaîne
      
      // Boucle pour parcourir chaque caractère de la chaîne
      while (index < text.length()) {
        // Trouver la position du prochain espace
        int separatorIndex = text.indexOf(" ", index);
        if (separatorIndex == -1) {
          separatorIndex = text.length(); // Si aucun espace n'est trouvé, la fin de la chaîne est utilisée
        }
        
        // Extraire le mot entre l'index actuel et l'index du prochain espace
        String word = text.substring(index, separatorIndex);
        words.push_back(word); // Ajouter le mot au tableau
        index = separatorIndex + 1; // Mettre à jour l'index pour passer au prochain mot
      }

      return words;
    }


    float getTextWidth(String text){
      Label::ofr->setFontSize(size);
      return Label::ofr->getTextWidth(text.c_str());
      //return text.length()*fontPixelSize;
    }

    std::vector<std::vector<int>> computeLineCosts(std::vector<String> words, int n, int linePixelWidth, float spacePixelWidth){
      std::vector<std::vector<int>> lineCosts(n+1, std::vector<int>(n+1, INT_MAX));
      for(int y = 1; y <= n; y++){
          float wordPixelWidth = getTextWidth(words[y-1]);

          int cost = linePixelWidth - wordPixelWidth;
          lineCosts[y][y] = cost * cost * cost;

          for(int x = y+1; x <= n; x++){
              wordPixelWidth = getTextWidth(words[x-1]);

              cost -= (wordPixelWidth + spacePixelWidth);
              if(cost < 0)
                  break;
              else
                  lineCosts[y][x] = cost * cost * cost;
          }
      }
      lineCosts[n][n] = 0;
      return lineCosts;
    }

    std::vector<int> computeLineBreakStartIndices(std::vector<std::vector<int>> lineCosts, int n){
      std::vector<int> startIndices(n+1, 0);
      std::vector<int> minCosts(n+1, 0);
      for(int y = 1; y <= n; y++){
          minCosts[y] = INT_MAX;
          for(int x = 1; x <= y; x++){
              /// if from a previous word to a current word
              /// the cost is INT_MAX, we skip
              if(lineCosts[x][y] == INT_MAX)
                  continue;

              if(minCosts[x-1] + lineCosts[x][y] < minCosts[y]){
                  minCosts[y] = minCosts[x-1] + lineCosts[x][y];
                  startIndices[y] = x;
              }
          }
      }
      return startIndices;
    }


    LineBreakInfo computeLineBreakInfo(String text,int linePixelWidth){
      std::vector<String> words = getWords(text);
      int n = words.size();

      float spacePixelWidth = getTextWidth(" ");

      std::vector<std::vector<int>> lineCosts = computeLineCosts(words, n, linePixelWidth, spacePixelWidth);
      std::vector<int> startIndices = computeLineBreakStartIndices(lineCosts, n);

      std::vector<int> endPositions(n+1, 0);
      for(int i = startIndices.size()-1; i > 0 ;i--){
          int cur = i;
          int start = startIndices[i];
          while( start != i)
              i--;
          endPositions[i] = cur;
      }

      /// compute LineBreakInfo
      std::vector<int> lineBreaks;
      int curLineWidth = 0, maxLineWidth = 0;
      int charIndex = 0;

      for(int i = 0; i<words.size(); i++){
          charIndex += (words[i].length()+1);
          curLineWidth = getTextWidth(words[i]);

          int end = endPositions[i+1];
          while( (i+1) < end)
          {
              i++;
              charIndex += (words[i].length()+1);
              curLineWidth += (spacePixelWidth + getTextWidth(words[i]));

          }
          lineBreaks.push_back(charIndex-1);
          maxLineWidth = max(maxLineWidth, curLineWidth);
      }

      if(!lineBreaks.empty())
          lineBreaks.pop_back();

      return LineBreakInfo(lineBreaks.size()+1, lineBreaks, maxLineWidth);
    }
};


class NumericLabel : public Label{
  public:
    NumericLabel(const float* n,uint8_t dec, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness, String _unit="", int _alter = 1, bool d=false):number(n), decimal(dec),alter(_alter),unit(_unit), Label(&convertedNumber,  a,  posX, posY, s,12,fC, bC, _brightness, d){      
      id = concatStrings("NumericLabel", *posX, "x", *posY);
    }

    virtual bool refresh(bool force=false){
      refreshed = force;
      if (*number != oldNumber || force){
        if ((*number * alter) <10 && (*number * alter) >=0 && decimal == 0){
          char buffer[3]; // 1 digit + leading zero + null terminator
          sprintf(buffer, "0%d", (int)*number * alter);
          convertedNumber = buffer;          
        }else convertedNumber = String((float)*number * alter,(unsigned int)decimal); // set precision
        //if (debug){DEBUG_PRINT("\n%s changed number from %.2f to %.2f", this->id.c_str(), oldNumber, *number);}
        convertedNumber += unit;

        oldNumber = *number;
        refreshed = true;
      }
      Label::refresh(refreshed);
      return refreshed;
    }

  private:
    const float* number;
    float oldNumber = -1;
    String convertedNumber ="NaN";
    uint8_t decimal;
    String unit="";
    int alter = 1;
};

class Icon : public Font{
  public:
    static OpenFontRender* ofr; 
    
    Icon(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor,  const float* _brightness = &GUI::fullBrightness,bool d=false):Font(v, a, posX, posY, s,12, fC , bC , _brightness, d){
      id = concatStrings("Icon", *posX, "x", *posY, "~", *v);
      //if (debug) DEBUG_PRINT("\n new %s[%s]",id.c_str(),v->c_str()); // => cause problem as value may not be initialized when call from AnimatedIcon
    };
    
  protected:
    virtual OpenFontRender* getOfr(){return ofr;}
};

enum{
  ANIMATE_ONCE,
  ANIMATE_REPEAT,
  ANIMATE_PING_PONG,
};

class AnimatedIcon : public Icon {
public:
  AnimatedIcon(const String* frames,
               uint8_t nbFrames,
               Align a,
               const int16_t* posX,
               const int16_t* posY,
               uint8_t size,
               uint8_t animType = ANIMATE_REPEAT,
               uint16_t animDelay = 500,
               const uint16_t* fC = &GUI::defaultFcolor,
               const uint16_t* bC = &GUI::defaultBcolor,
               const float* _brightness = &GUI::fullBrightness,
               bool d = false)
  : Icon(&currentIcon, a, posX, posY, size, fC, bC, _brightness, d)
  {
    id = concatStrings("AnimatedIcon", *posX, "x", *posY);

    iconArray      = frames;
    numberOfFrames = nbFrames;
    animateType    = animType;
    animateDelay   = animDelay;

    oldIconArray = nullptr;
    lastTick     = 0;
    frame        = 0;
    inc          = 1;
    running      = true;

    // Sécurisation
    if (iconArray == nullptr || numberOfFrames == 0) {
      numberOfFrames = 0;
      currentIcon = "";
    } else {
      currentIcon = iconArray[0];
    }
  }

  virtual bool refresh(bool force = false) override {
    refreshed = force;

    // Rien à afficher
    if (iconArray == nullptr || numberOfFrames == 0) {
      currentIcon = "";
      return Icon::refresh(true);
    }

    // Si le tableau a changé, on repart proprement
    if (oldIconArray != iconArray) {
      oldIconArray = iconArray;
      reset();
      refreshed = true;
    }

    // Mettre à jour l’icône courante avant affichage
    currentIcon = iconArray[frame];

    // On affiche si forcé ou si le timer est arrivé
    bool timeToAdvance = shouldAdvance();
    bool didDraw = Icon::refresh(refreshed || timeToAdvance);

    if (timeToAdvance && running) {
      advanceFrame();
      return true;
    }

    return didDraw;
  }

  void reset() {
    frame = 0;
    inc = 1;
    lastTick = millis();
    if (iconArray != nullptr && numberOfFrames > 0) {
      currentIcon = iconArray[0];
    } else {
      currentIcon = "";
    }
  }

  void start() {
    running = true;
    lastTick = millis();
  }

  void stop() {
    running = false;
  }

  void setDelay(uint16_t newDelay) {
    animateDelay = newDelay;
  }

  void setFrames(const String* newFrames, uint8_t nbFrames) {
    iconArray = newFrames;
    numberOfFrames = nbFrames;

    if (iconArray == nullptr || numberOfFrames == 0) {
      numberOfFrames = 0;
      currentIcon = "";
      frame = 0;
    } else {
      if (frame >= numberOfFrames) frame = 0;
      currentIcon = iconArray[frame];
    }

    oldIconArray = nullptr; // force reset logique au prochain refresh
  }

private:
  int8_t frame = 0;
  int8_t inc = 1;

  uint8_t numberOfFrames = 0;
  uint8_t animateType = ANIMATE_REPEAT;
  uint16_t animateDelay = 500;

  const String* iconArray = nullptr;
  const String* oldIconArray = nullptr;

  unsigned long lastTick = 0;
  bool running = true;

  String currentIcon = "";

  bool shouldAdvance() {
    if (!running) return false;
    if (animateDelay == 0) return true;

    unsigned long now = millis();
    if ((unsigned long)(now - lastTick) >= animateDelay) {
      lastTick = now;
      return true;
    }
    return false;
  }

  void advanceFrame() {
    if (numberOfFrames <= 1) {
      frame = 0;
      return;
    }

    switch (animateType) {
      case ANIMATE_ONCE:
        if (frame < (int8_t)(numberOfFrames - 1)) {
          frame++;
        }
        break;

      case ANIMATE_REPEAT:
        frame++;
        if (frame >= (int8_t)numberOfFrames) {
          frame = 0;
        }
        break;

      case ANIMATE_PING_PONG:
        frame += inc;

        if (frame >= (int8_t)numberOfFrames) {
          frame = numberOfFrames - 2;
          inc = -1;
        } else if (frame < 0) {
          frame = 1;
          inc = 1;
        }
        break;

      default:
        frame = 0;
        break;
    }
  }
};

class FromToWidget : public Widget {
  public:
    FromToWidget(const float* v, const float mini, const float maxi,
                 const int16_t* posX, const int16_t* posY,
                 const uint16_t* fC, const uint16_t* bC,
                 const float* _brightness = &GUI::fullBrightness,
                 bool d=false)
      : Widget(posX, posY, fC, bC, _brightness, d)
    {
      value    = v; 
      oldValuePtr = v;
      oldValue = (v != nullptr) ? (*v - 1.0f) : 0.0f; // force un premier refresh si possible
      minValue = mini;
      maxValue = maxi;
    }
  
    virtual bool refresh(bool force=false) { 
      // Par défaut, pas de sprite (le widget peut décider dans refreshFrame/refreshContent)
      if (sprite != nullptr) delete sprite;
      sprite = nullptr;

      bool forceFull = force;

      // Changement de luminosité => redraw complet
      if (oldBrightness != *brightness) {
        oldBrightness = *brightness;
        forceFull = true;
      }
      
      bool frameChanged = false;
      bool logicChanged = false;
      bool colorChanged = false;

      const bool valuePointerChanged = (value != oldValuePtr);
      if (valuePointerChanged) {
        oldValuePtr = value;
        forceFull = true;
        if (value != nullptr) oldValue = *value - 1.0f;
      }

      // Changement de position ou de couleur de fond => redraw frame
      if (forceFull || oldX != *x || oldY != *y || oldbColor != *bColor) {
        refreshFrame();
        oldX      = *x;
        oldY      = *y;
        oldbColor = *bColor;
        frameChanged = true;
      }

      // Logique spécifique au widget (angles, valeurs, etc.)
      // IMPORTANT : chaque dérivé doit appeler lui-même refreshContent()
      // quand la valeur change (partiel ou complet).
      if (value != nullptr) {
        logicChanged = refreshLogic(forceFull);
      }

      // Changement de couleur de premier plan => redraw complet du contenu
      if (forceFull || oldfColor != *fColor) {
        oldfColor   = *fColor;
        refreshContent(true);   // redraw complet
        colorChanged = true;
      }

      // Le widget est considéré comme rafraîchi si au moins un de ces éléments a changé
      refreshed = frameChanged || logicChanged || colorChanged || forceFull || valuePointerChanged;
      return refreshed;
    }

  protected:
    virtual bool refreshLogic(bool force) = 0;     // doit retourner true si la valeur / affichage a changé
    virtual void refreshContent(bool full) = 0;    // full = redraw complet
    virtual void refreshFrame() = 0;

    uint16_t to = 0, oldTo = 254;
    int16_t  from = 0, oldFrom = 254;

    const float* value;
    const float* oldValuePtr = nullptr;
    float oldValue;
    float minValue, maxValue;
};


class Arc : public FromToWidget {
  public:
    Arc(const float *v, const float mini, const float maxi,
        const int16_t* posX, const int16_t* posY, uint16_t r,
        const uint16_t *fC = &GUI::defaultFcolor,
        const uint16_t *bC = &GUI::defaultBcolor,
        uint16_t sAngle = 0,
        const float* _brightness = &GUI::fullBrightness,
        bool d=false)
      : FromToWidget(v, mini, maxi, posX, posY, fC, bC, _brightness, d)
    {            
      radius = r;
      width = height = (radius * 2) + 2;        // +2 because of outer circle                
      id = concatStrings("Arc", *posX, "x", *posY);
      startAngle = sAngle;
      from = startAngle;
      to   = startAngle + 1;
    }

    virtual int16_t getX() { return *x - width  / 2; }
    virtual int16_t getY() { return *y - height / 2; }    

  protected:
    uint16_t startAngle, radius;

    // Logic: calcule les angles, choisit full/partial, et appelle refreshContent()
    virtual bool refreshLogic(bool force) { 
      bool changed = false;

      float v = *value;

      // On peut utiliser un epsilon si souhaité, sinon == suffit
      // if (force || fabsf(v - oldValue) > 0.0001f)
      if (force || v != oldValue) {

        uint16_t newTo = mapFloatToInt(v, minValue, maxValue,
                                      startAngle, 360 - startAngle);

        // Clamp sûr :
        if (newTo >= 360) newTo = 359;
        if (newTo < startAngle) newTo = startAngle;


        // Seulement si l'angle a réellement changé, on redessine
        if (force || newTo != to) {
          uint16_t previousTo = to;
          to = newTo;

          if (force) {
            // redraw complet
            from = startAngle;
            refreshContent(true);
            from = to;   // on se recale
          } else {
            // redraw partiel : on laisse refreshContent(false) gérer
            from = previousTo;
            refreshContent(false);
            from = to;   // on se recale pour la prochaine fois
          }

          changed = true;
        }

        oldValue = v;
      }

      return changed || force;
    }
 
    virtual void refreshFrame() {
      // Si tu veux un contour, fond, etc. c’est ici.
      // Laisser vide si l'Arc ne nécessite pas de frame spécifique.
    }

    void refreshContent(bool full) {
      createSprite();       
          
      // Épaisseur de l’arc : sécurisation pour petits rayons
      uint8_t thickness = (radius / 5) + 1;
      if (radius < 25) thickness = (radius / 3) - 1;
      if (thickness < 1) thickness = 1;

      uint16_t color = *fColor;
      uint16_t fg    = GUI::luminance(color,
                          *brightness * GUI::systemBrightness * 255);
      uint16_t bg    = getClearColor();

      if (full) {
        // Redessine tout l’arc (de startAngle à to)
        sprite->drawArc(width/2, height/2,
                        radius-3, radius - thickness,
                        startAngle, to,
                        fg, bg);
      } else {
        // redraw partiel seulement entre from et to
        if (to > from) {
          // la valeur augmente : on dessine le nouveau segment
          sprite->drawArc(width/2, height/2,
                          radius-3, radius - thickness,
                          from, to,
                          fg, bg);
        } else if (to < from) {
          // la valeur diminue : on "efface" le surplus
          uint16_t eraseColor = GUI::luminance(widgetInnerColor,
                                   *brightness * GUI::systemBrightness * 255);

          sprite->drawArc(width/2, height/2,
                          radius-2, radius - thickness -1,
                          to, from,
                          bg, eraseColor,false);
        }
        // si to == from, rien à faire
      }
    }
};

class RingMeter : public Arc{
  #define START_ANGLE 30
  public:
    
    RingMeter(const float *v, float mini, float maxi, uint8_t deci, const int16_t* posX, const int16_t* posY, uint16_t r, const String* u,  const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool d=false):Arc(v,mini, maxi, posX, posY, r, fC, bC, START_ANGLE, _brightness,d){
      unit = u; 
      id = concatStrings("RingMeter", *posX, "x", *posY);
      if (deci>1) deci = 1;
      decimal = deci;        
    }

  protected:
    virtual bool refreshLogic(bool force){ 

      if (Arc::refreshLogic(force)) refreshValueLabel();
      
      if ((unit != nullptr) && (*unit != oldUnit || force || unit != oldUnitPtr)){
        refreshUnitLabel();
        oldUnit = *unit;
        oldUnitPtr = unit;
        refreshed = true;
        //if (debug){DEBUG_PRINT("\n%s.refreshed unit, new: %s",this->id.c_str(),unit->c_str());}
      } 
      
      return refreshed;      
    }  

  private:        
    Label*  label;
    uint16_t valueLabelHeight;
    const String *unit;
    String oldUnit ;
    const String* oldUnitPtr = nullptr;
    uint8_t decimal, clearArea=0;

    void refreshFrame(){
      //DEBUG_PRINT("\n%s.refreshFrame()",this->id.c_str());
      createSprite();
      sprite->fillSmoothCircle(width/2, height/2, radius,GUI::luminance(widgetInnerColor,*brightness * GUI::systemBrightness * 255) );
      sprite->drawSmoothCircle(width/2, height/2, radius, GUI::luminance(TFT_SILVER,*brightness * GUI::systemBrightness * 255), GUI::luminance(widgetInnerColor,*brightness * GUI::systemBrightness * 255));
      uint16_t tmp = radius - 3;
      sprite->drawArc(width/2, height/2, tmp, tmp - tmp / 5, startAngle-1, 360-startAngle+1, TFT_NEARLYBLACK, GUI::luminance(widgetInnerColor,*brightness * GUI::systemBrightness * 255));           
    }

    void refreshUnitLabel(){
      createSprite();
      //DEBUG_PRINT("\n%s.refreshUnitLabel()",this->id.c_str());
      // THE UNIT LABEL
      if (unit == nullptr || unit->length() == 0) return;
      uint8_t unitFontSize = (2 * radius*0.7) / (unit->length());
      uint16_t color = widgetInnerColor;
      color = GUI::luminance(widgetInnerColor,*brightness * GUI::systemBrightness * 255);
      label = new Label(unit,Align::Center,&GUI::zero,&GUI::zero,unitFontSize,12, &widgetSubLabelColor,&color,brightness,debug);
      label->refresh(true);
      label->sprite->pushToSprite(sprite,width/2-label->width/2,height/2+(radius-unitFontSize/1.1));     //height/2+valueLabelHeight/3
      delete label;         
    }

    void refreshValueLabel(){      
      createSprite();

      // THE VALUE LABEL        
      //DEBUG_PRINT("\n%s.refreshValueLabel()",this->id.c_str());
      String fToS = String((float)*value,(unsigned int)decimal);   // convert the floating value to String with precision so we can get the number of character/digit to determine the fontSize        
      uint8_t digitFontSize;
      if (decimal == 0){
        digitFontSize = (3.5 * radius) / (fToS.length()); // 4
        //if (*value<100 || *value>0) digitFontSize *= 0.70; // for 2 digit int only and positive number, we reduced a bit the size otherwise we have a square around 0.65
      }else{
        digitFontSize = (4.8 * radius) / (fToS.length());
      }
      
      sprite->fillSmoothCircle(width/2, height/2.2, radius*0.65,GUI::luminance(widgetInnerColor,*brightness * GUI::systemBrightness * 255)); //
      // we create the numeric label but with an empty unit as this one is handled separately as it is below the value for the RingerMeter 
      NumericLabel* digits = new NumericLabel(value,decimal,Align::Center,&GUI::zero,&GUI::zero,digitFontSize,&GUI::defaultFcolor,&GUI::transparentColor,brightness, "", 1, debug); 
      digits->refresh(true);
      valueLabelHeight = digits->height; // is reused by the unit label to be disdplayed to properly display it
      digits->sprite->pushToSprite(sprite,width/2-digits->width/2,height/2-valueLabelHeight/1.5,GUI::transparentColor);    
      delete digits;      
    }  

};

#define TOGGLE_CHAR_MAX_LEN 20

class Toggle : public Widget{
  public:
    const bool* revert;
    const String* text;

    Toggle(const String* v, const int16_t* posX, const int16_t* posY, uint8_t h, const bool* p, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor,  const float* _brightness = &GUI::fullBrightness, bool d=true):Widget(posX, posY, fC, bC, _brightness,d){
      width = TFT_WIDTH;
      height = h;
      revert = p;
      oldRevert = (revert != nullptr) ? !*revert : false;
      text = v;
      id = "Toggle";
    }

    virtual bool refresh(bool force=false){
      Widget::refresh(force);

      if (revert == nullptr || text == nullptr) {
        oldRevertPtr = nullptr;
        oldTextPtr = nullptr;
        return refreshed;
      }

      bool revertPointerChanged = (revert != oldRevertPtr);
      bool textPointerChanged = (text != oldTextPtr);
      if (revertPointerChanged) oldRevertPtr = revert;
      if (textPointerChanged) oldTextPtr = text;

      if (refreshed || oldRevert != *revert || oldText != *text || revertPointerChanged || textPointerChanged){

        uint16_t fillColor, textColor;

        if(!*revert) {
          //DEBUG_PRINT("\nFlip");
          fillColor    = TFT_RED;
          textColor    = TFT_WHITE;
        } else {
          //DEBUG_PRINT("\nFlop");
          fillColor    = GUI::defaultBcolor;
          textColor    = *fColor;
        }      

        createSprite(GUI::defaultBcolor);        

        sprite->fillSmoothRoundRect(0,0 , width, height, height/5, GUI::luminance(fillColor,*brightness * GUI::systemBrightness * 255));
        
        uint16_t borderColor = *revert?*bColor:TFT_WHITE;        
        sprite->drawRoundRect(0,0, width, height, height/5, GUI::luminance(borderColor,*brightness * GUI::systemBrightness * 255));

        uint16_t transparent = TFT_TRANSPARENT;
        Label* label = new Label(text,Align::Center,&GUI::zero,&GUI::zero,height*1.4,TOGGLE_CHAR_MAX_LEN, &textColor,&transparent,  brightness, debug);
        label->refresh(true);
        label->sprite->pushToSprite(sprite,width/2-label->width/2,0,TFT_TRANSPARENT);    //center
        delete label;       

        refreshed = true;
        oldRevert = *revert;
        oldText = *text;
        
      }
      return refreshed;

    }

  private:    
    bool oldRevert;    
    String oldText;
    const bool* oldRevertPtr = nullptr;
    const String* oldTextPtr = nullptr;

};

class VuMeter : public Widget{
// vertical https://github.com/HX2003/color_bar_tft/blob/master/color_bar_tft.ino
// horizontal https://forum.arduino.cc/t/tft_espi-new-example-for-animated-dials/643382/3
};


class Bar : public FromToWidget{
  
  public:
    Bar(const float *v, const float min, const float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset=0, uint8_t r=0, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool d=true):FromToWidget(v, min, max, posX, posY, fC, bC,_brightness, d){
      radius = r;
      width = w; height = h;
      frameOffset = fOffset;
      from = frameOffset;
      value = v;      
      from = 0;      
    }

  protected:
    
  uint8_t frameOffset = 0, radius = 5;

    virtual void refreshFrame(){
      createSprite(GUI::transparentColor);
      if (frameOffset != 0){
        sprite->fillRoundRect(0,0,width,height,radius,GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));
        sprite->drawRoundRect(0,0,width,height,radius,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));
      }      
    }  

    virtual bool refreshLogic(bool force){
      if (from !=oldFrom || to != oldTo){
        refreshContent(false); // partial refresh
        oldFrom = from;                    
        oldTo = to;
        refreshed = true;
      }
      return refreshed;
    }

};


class VerticalBar : public Bar{
  public:

    VerticalBar(const float *v, const float min, const float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h,   uint8_t fOffset=0, uint8_t r=0,const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool d=true):Bar(v, min, max, posX, posY, w, h, fOffset, r, fC, bC, _brightness,d){
      from = height-frameOffset;
      to = 0+frameOffset;
    }      
  
    virtual void refreshContent(bool full){
      refreshed = full;

      createSprite(GUI::transparentColor);  //  

      if (full){
        // full content refresh, means FROM -> TO                
        sprite->fillRoundRect(0+frameOffset,to+frameOffset,width-frameOffset*2,from-to-frameOffset*2,radius,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        //DEBUG_PRINT("\nFull refreshcontent");
      }else{
        // only refresh tail/and or head
        refreshed = true;

        uint16_t blockHeight;

        if (oldFrom > from){
          // remove tail
          sprite->fillRect(0+frameOffset,from+frameOffset,width-frameOffset*2,oldFrom-from,GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));   
          //DEBUG_PRINT("\nRemove tail");
        }else if (from > oldFrom){
          // add tail
          //DEBUG_PRINT("\nAdd tail");
          sprite->fillRect(0+frameOffset,oldFrom+frameOffset,width-frameOffset*2,from-oldFrom,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        }          

        if (oldTo > to){
          // add head
          blockHeight = oldTo-to;
          //DEBUG_PRINT("\nAdd head: %i",blockHeight);
          sprite->fillRect(0+frameOffset,to+frameOffset,width-frameOffset*2,blockHeight,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        }else if (to > oldTo){
          // remove head
          blockHeight = to-oldTo;
          //DEBUG_PRINT("\nRemove head: %i",blockHeight);
          sprite->fillRect(0+frameOffset,oldTo,width-frameOffset*2,blockHeight,GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));                    
        }       
      }
    }

};

class HorizontalBar : public Bar{
  public:

    HorizontalBar(const float *v, const float min, const float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset=0, uint8_t r=0, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness, bool d=true):Bar(v, min, max, posX, posY,w,h, fOffset, r,fC, bC, _brightness,d){
      to = width-frameOffset;
    }
   
  protected:

    virtual void refreshContent(bool full){

      refreshed = full;
      createSprite(GUI::transparentColor); 
      if (full){
        // full content refresh, means FROM -> TO
        //sprite->fillRoundRect(from+frameOffset,frameOffset,to-frameOffset,height-frameOffset*2,radius,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        sprite->fillRect(from+frameOffset,frameOffset,to-frameOffset,height-frameOffset*2,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        //DEBUG_PRINT("\nFull refreshcontent");
      }else{
        // only refresh tail/and or head
        refreshed = true;

        int16_t blockWidth;

        if (from > oldFrom){
          // remove tail
          if (oldFrom<0) oldFrom = 0;
          blockWidth = from-oldFrom-frameOffset;
          sprite->fillRect(oldFrom+frameOffset,frameOffset,blockWidth,height-frameOffset*2,GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));   
          //DEBUG_PRINT("\tRemove tail: %d",blockWidth);
        }else if (from < oldFrom){
          // add tail
          blockWidth = oldFrom-from-frameOffset;
          //DEBUG_PRINT("\tAdd tail :%d",blockWidth);
          sprite->fillRect(from+frameOffset,frameOffset,blockWidth,height-frameOffset*2,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        }          

        if (to > oldTo){
          // add head
          blockWidth = to-oldTo;
          //DEBUG_PRINT("\tAdd head: %i",blockWidth);
          sprite->fillRect(oldTo+frameOffset,frameOffset,blockWidth,height-frameOffset*2,GUI::luminance(*fColor,*brightness * GUI::systemBrightness * 255));        
        }else if (to < oldTo){
          // remove head
          blockWidth = oldTo-to;
          //DEBUG_PRINT("\tRemove head: %i",blockWidth);
          sprite->fillRect(to+frameOffset,frameOffset,blockWidth,height-frameOffset*2,GUI::luminance(*bColor,*brightness * GUI::systemBrightness * 255));                    
        }       
      }
    }

};

class ProgressBarH : public HorizontalBar{

  public:

    ProgressBarH(float* v, float min, float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset, uint8_t r,const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool d=false):HorizontalBar(v, min, max, posX, posY,w, h,fOffset, r,fC,bC,_brightness,d){
      id = concatStrings("ProgressBarH", *posX, "x", *posY);
    }

    virtual bool refreshLogic(bool force=false){ 
      refreshed = force;

      if (oldValue != *value || force){

        // Zone utile: on laisse 1px de marge par rapport au frame
        int16_t innerMin = (int16_t)frameOffset + 1;
        int16_t innerMax = (int16_t)width - (int16_t)frameOffset - 1;

        // Sécurité si frameOffset trop grand (ou widget trop petit)
        if (innerMax < innerMin) innerMax = innerMin;

        to = mapFloatToInt(*value, minValue, maxValue, innerMin, innerMax);

        // Clamp sécurité
        if (to < innerMin) to = innerMin;
        if (to > innerMax) to = innerMax;

        HorizontalBar::refreshLogic(refreshed);
        oldValue = *value;
      }

      return refreshed;
    }




};


class ProgressBarV : public VerticalBar{

  public:

    ProgressBarV(float* v, float min, float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset, uint8_t r,const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor,  const float* _brightness = &GUI::fullBrightness,bool d=false):VerticalBar(v, min, max, posX, posY,w, h,fOffset, r,fC,bC,_brightness,d){
      id = concatStrings("ProgressBarV", *posX, "x", *posY);
    }

    virtual bool refreshLogic(bool force=false){ 
      refreshed = force;
      if (oldValue != *value){
        to = height - mapFloatToInt(*value,minValue,maxValue,0,height);    
        //DEBUG_PRINT("\tFrom: %d To: %i",from, to) ;        
        VerticalBar::refreshLogic(refreshed);        
        oldValue = *value;
      }
      return refreshed;
    }


};



class PendingBar : public HorizontalBar{

  public:
    PendingBar(float* size, float min, float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, bool pp = false, uint8_t fOffset = 0, uint8_t r = 0, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, uint16_t _speed = 40, uint8_t step = 2, const float* _brightness = &GUI::fullBrightness,bool d=false) : pingPong(pp), stepDirection(step), speed(_speed), HorizontalBar(size, min, max, posX, posY,w, h,fOffset, r,fC,bC,_brightness,d){
      id = concatStrings("PendingBar", *posX, "x", *posY);
      if (stepDirection > *size) stepDirection = *size;
    }   

    virtual bool refresh(bool force) override{
      // delay 
      if (millis() - lastRefreshTime < speed && !force) return false;
      lastRefreshTime = millis();
      refreshed = HorizontalBar::refresh(force);
      return refreshed;
    }

    virtual bool refreshLogic(bool force = false) override{     

      // Ensure 'value' is a valid pointer before dereferencing
      if (!value) return false; // Exit early to avoid crash 
      // Mark as refreshed if forced
      refreshed = force;  
      // Update the 'from' position, ensuring it remains within bounds
      from += stepDirection;  

      //from -= *value/2;     // center, otherwise: left align

      // Handle negative wrap-around and keep 'from' in valid range
      //if (from < - *value) from = - *value;
  
      // Check if we reached the boundaries
      if (from >= width - abs(stepDirection) || from < - *value) { 
        
        if (from < - *value) from = - *value;

        if (pingPong) { 
          stepDirection = -stepDirection;  // Reverse direction
          //EBUG_PRINT("\n%s", stepDirection > 0 ? "ping" : "pong");
          if (from >= width) from = width - 1; 
        }else{
          if (from >= width) from = - *value; 
        }
      }
  
      // Compute the 'to' position while ensuring safe bounds
      to = from + *value;  
  
      // Ensure 'to' does not exceed the width
      if (to > width) to = width;
  
      // If 'value' is zero, ensure at least a 1-pixel width (optional)
      if (*value < stepDirection ) {
          //DEBUG_PRINT("\nWarning: 'value' is zero, ensuring visibility.");
          to = from + stepDirection; 
      }
  
      // Debug output to track bar movement
      //DEBUG_PRINT("\npendingBar from: %d to:%d", from, to);
  
      // Call the base class method (assuming HorizontalBar is a parent class)
      HorizontalBar::refreshLogic(force);
  
      return refreshed;
    }
  


  private:
    bool pingPong = false;
    int8_t stepDirection = 15;
    uint16_t speed = 50;
    unsigned long lastRefreshTime = 0;

};

class CompositeWidget: public Widget{
  public:
    CompositeWidget(const int16_t *x, const int16_t* y, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor,  const float* _brightness = &GUI::fullBrightness,bool debug=false):Widget(x,y,fC,bC,_brightness,debug){}

    ~CompositeWidget(){
      while (guiList.size()>0){
        //DEBUG_PRINT("\nremaining to unstack: %i",gui->screenGUIs.size());
        Widget* widget = guiList.shift();
        //DEBUG_PRINT("\nobject: %s ",widget->id.c_str());
        delete(widget);
        widget = nullptr;
      }      
    }

    virtual bool refresh(bool force=false){
      //push all composite sprite to the main sprite
      //refreshed = force;    
      for (uint8_t i=0;i<guiList.size();i++){
        Widget* widget =  guiList.get(i);                
        if (widget->refresh(force)){
          createSprite(GUI::transparentColor); 
          refreshed = true;
          widget->sprite->pushToSprite(sprite,widget->getX(),widget->getY(),GUI::transparentColor);
          delete widget->sprite;
          widget->sprite = nullptr;
        }
      }      
      return refreshed; 
    }  
              
  protected:
    LinkedList<Widget*> guiList;
};

class Top2Toggles : public CompositeWidget {
  public:
    Top2Toggles(const String* topLabel, const String* bottomLabel, const bool* topReverse = &GUI::off, const bool* bottomReverse = &GUI::off, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const uint16_t *fC2 = &GUI::defaultFcolor, const uint16_t *bC2 = &GUI::silver, const float* _brightness = &GUI::fullBrightness,bool debug=false) : CompositeWidget(&GUI::zero,&GUI::zero,fC, bC, _brightness,debug){
      id = "Top2Toggles";
      
      topToggle = new Toggle(topLabel,&GUI::zero,&GUI::un,TOP2WIDGET_SIZE,topReverse,fC,bC, brightness, debug);        
      bottomToggle = new Toggle(bottomLabel,&GUI::zero,&GUI::y1,TOP2WIDGET_SIZE,bottomReverse,fC2, bC2, brightness, debug);        
      
      guiList.add(topToggle);
      guiList.add(bottomToggle);         

      width = max(topToggle->width,bottomToggle->width);    // WIDTH = LARGEST WIDGET
      height = topToggle->height + bottomToggle->height + 1;    // HEIGHT = SUM OF ALL WIDGET HEIGHT + 1 pixel in between
 
    }    
    
    void setPointers(
      const String* topString,
      const String* bottomString,
      bool* topBool,
      bool* bottomBool,
      uint16_t* fColor,
      uint16_t* bColor
    )
    {
      topToggle->text = topString;
      topToggle->revert = topBool;
      topToggle->fColor = fColor;
      topToggle->bColor = bColor;
      
      bottomToggle->text = bottomString;
      bottomToggle->revert = bottomBool;
    }    

  protected:
    Toggle *topToggle, *bottomToggle;

};


class Top1Toggle1Label : public CompositeWidget {  
  public:
    Top1Toggle1Label(const String* topS, const String* bottomS, const bool* topReverse = &GUI::off, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const uint16_t *fC2 = &GUI::defaultFcolor, const uint16_t *bC2 = &GUI::defaultBcolor, const float* _brightness = &GUI::fullBrightness,bool debug=false) : CompositeWidget(&GUI::zero,&GUI::zero,fC, bC, _brightness,debug){
      id = "Top1Toggle1Label";

      topString = topS; bottomString = bottomS; fColor2 = fC2;
      topBool = topReverse; bColor2 =bC2;
       
      topToggle = new Toggle(topString,&GUI::zero,&GUI::un,TOP2WIDGET_SIZE,topBool,fColor,bColor,  brightness, debug);        
      bottomLabel = new Label(bottomString, Align::Center,&GUI::center, &GUI::y1,(TOP2WIDGET_SIZE)*1.2,TOGGLE_CHAR_MAX_LEN, fC2,bC2,brightness, debug);        

      width = max(topToggle->width,bottomLabel->width);
      height = topToggle->height+(TOP2WIDGET_SIZE)*1.2; // BE CAREFUL !! /!\ /!\ /!\ The lqbel height is determined zhen rendering; hence we DO AN APPROXIMANTE BASED ON THE REQUESTED FONT SIZE BUT THIS MAY NOT BE THE FINAL RIGHT HEIGHT

      guiList.add(topToggle);
      guiList.add(bottomLabel);         
    }    

  private:
    const String *topString, *bottomString; 
    const bool *topBool;
    Toggle *topToggle;
    Label *bottomLabel;
    int16_t y1 = TOP2WIDGET_SIZE;
    const uint16_t *fColor2, *bColor2;
};


/*class CompositeWidget : public Widget {

  public:
    
    LinkedList<Widget*> widgets = LinkedList<Widget*>();
    
    CompositeWidget(bool debug):Widget(&GUI::zero, &GUI::zero, &GUI::defaultFcolor, &GUI::defaultBcolor, debug){}

    void add(Widget* widget) {
        widgets.add(widget);
    }

    virtual bool refresh(bool force=false) override {          
      for (uint8_t i=0;i<widgets.size();i++) {
        Widget* widget = widgets.get(i);
        refreshed |= widget->refresh(force);
      }
      return refreshed;
    }

    virtual void setText(const String* text){

    }      

};*/

class SliderWidget{
//https://forum.arduino.cc/t/tft-espi-example-slider-demo-code-need-help-please/1144640/5
};


class Led : public Widget {

  public:
    Led(const int16_t* posX,
        const int16_t* posY,
        uint16_t rad,
        const String* props,
        const float* _brightness = &GUI::fullBrightness,
        const bool* _enabled = &GUI::on,
        bool d = false
        )
      : Widget(posX, posY, &ledBaseColor, &GUI::transparentColor, _brightness, d),
        properties(props),
        radius(rad),
        enabled(_enabled) {

      id = concatStrings("Led", *posX, "x", *posY);
      width = rad * 2;
      height = rad * 2;

      parseProperties();
      oldProperties = *properties;
    }

    virtual bool refresh(bool force = false) {
      refreshed = force;

      bool baseNeedsRefresh = Widget::refresh(force);
      bool propertyChanged = false;

      if (*properties != oldProperties) {
        parseProperties();
        oldProperties = *properties;
        propertyChanged = true;
      }

      bool stateChanged = updateState(force);

      if (baseNeedsRefresh || propertyChanged || stateChanged || force) {
        drawLed();
        refreshed = true;
      }

      return refreshed;
    }

  private:
    static const uint16_t FLASH_MEDIUM_INTERVAL = 350;
    static const uint16_t FLASH_FAST_INTERVAL   = 100;
    static const uint16_t FLASH_SLOW_INTERVAL   = 1500;

    const String* properties;
    String oldProperties;

    const bool* enabled = nullptr;   // pointeur externe on/off

    uint16_t ledBaseColor = TFT_RED;
    uint16_t radius;

    bool flashEnabled = false;
    uint16_t flashInterval = FLASH_MEDIUM_INTERVAL;

    bool ledOn = true;               // état logique "normal" interne
    bool effectiveLedOn = true;      // état réellement affiché après override externe

    unsigned long lastToggle = 0;

    void parseProperties() {
      String lower = *properties;
      lower.trim();
      lower.toLowerCase();

      if (lower.length() == 0) lower = "red";

      ledBaseColor = TFT_RED;
      bool flashRequested = false;
      flashInterval = FLASH_MEDIUM_INTERVAL;

      while (lower.length() > 0) {
        int spaceIndex = lower.indexOf(' ');
        String token = (spaceIndex == -1) ? lower : lower.substring(0, spaceIndex);

        if (spaceIndex == -1) {
          lower = "";
        } else {
          lower = lower.substring(spaceIndex + 1);
          lower.trim();
        }

        if (token.length() == 0) continue;

        if (token == "red") ledBaseColor = TFT_RED;
        else if (token == "black") ledBaseColor = TFT_BLACK;
        else if (token == "yellow") ledBaseColor = TFT_YELLOW;
        else if (token == "blue") ledBaseColor = TFT_BLUE;
        else if (token == "green") ledBaseColor = TFT_GREEN;
        else if (token == "orange") ledBaseColor = TFT_ORANGE;
        else if (token == "grey" || token == "gray") ledBaseColor = TFT_SILVER;
        else if (token == "fast") {
          flashRequested = true;
          flashInterval = FLASH_FAST_INTERVAL;
        }
        else if (token == "medium") {
          flashRequested = true;
          flashInterval = FLASH_MEDIUM_INTERVAL;
        }
        else if (token == "low" || token == "slow") {
          flashRequested = true;
          flashInterval = FLASH_SLOW_INTERVAL;
        }
      }

      flashEnabled = flashRequested;

      if (!flashEnabled) {
        ledOn = true;
      }

      lastToggle = millis();
    }

    bool updateState(bool force) {
      bool changed = false;
      unsigned long now = millis();

      // 1) Calcul de l'état interne normal (couleur fixe ou clignotement)
      bool previousLedOn = ledOn;

      if (!flashEnabled) {
        ledOn = true;
      } else {
        if (force) {
          lastToggle = now;
        } else if (now - lastToggle >= flashInterval) {
          ledOn = !ledOn;
          lastToggle = now;
        }
      }

      if (ledOn != previousLedOn) {
        changed = true;
      }

      // 2) Application de l'override externe
      bool previousEffectiveLedOn = effectiveLedOn;

      if (enabled != nullptr) {
        effectiveLedOn = (*enabled) ? ledOn : false;
      } else {
        effectiveLedOn = ledOn;
      }

      if (effectiveLedOn != previousEffectiveLedOn) {
        changed = true;
      }

      return changed;
    }

    void drawLed() {
      createSprite(*bColor);

      int16_t centerX = width / 2;
      int16_t centerY = height / 2;
      int16_t mainRadius = (min(width, height) / 2) - 1;
      if (mainRadius < 2) mainRadius = 2;

      uint16_t innerColor = effectiveLedOn ? ledBaseColor : TFT_DARKGREY;
      uint16_t rimColor = GUI::luminance(GUI::defaultFcolor, *brightness * GUI::systemBrightness * 255);

      sprite->drawCircle(centerX, centerY, mainRadius - 1, TFT_BLACK);
      sprite->drawSmoothCircle(centerX, centerY, mainRadius, rimColor, *bColor);

      int16_t innerRadius = mainRadius - 2;
      if (innerRadius > 1) {
        sprite->fillSmoothCircle(centerX, centerY, innerRadius,
          GUI::luminance(innerColor, *brightness * GUI::systemBrightness * 255));
      }
    }

    virtual int16_t getX() { return *x - width / 2; }
    virtual int16_t getY() { return *y - height / 2; }
};


bool GUI::off = false;
bool GUI::on = true;

// COLORS DEFINITIONS
uint16_t GUI::defaultFcolor = TFT_FCOLOR;
uint16_t GUI::defaultBcolor = TFT_BGCOLOR;
uint16_t GUI::unUsedColor = TFT_BLACK;

const uint16_t GUI::transparentColor = TFT_TRANSPARENT;
const uint16_t GUI::green = TFT_GREEN;
const uint16_t GUI::silver = TFT_SILVER;
const uint16_t GUI::red = TFT_RED;
const uint16_t GUI::orange = TFT_ORANGE;
const uint16_t GUI::white = TFT_WHITE;
const uint16_t GUI::purple = TFT_PURPLE;
const uint16_t GUI::yellow = TFT_YELLOW;
const uint16_t GUI::blue = TFT_BLUE;
const uint16_t GUI::nearlyBlack = TFT_NEARLYBLACK;
const uint16_t GUI::gold = TFT_GOLD;

// BRIGHTNESS DEFINITIONS
const float GUI::fullBrightness = 1.0;
const float GUI::greydOutBrightness = 0.5;
#define DEFAULT_BRIGHTNESS 0.7
#define BRIGHTNESS_MAX 1.0
#define BRIGHTNESS_MIN 0.2
#define BRIGHTNESS_NOFOCUS 0.5
float GUI::systemBrightness = DEFAULT_BRIGHTNESS;

// WIDGETS AND MENU COLORS DEFINITIONS
uint16_t GUI::widgetInnerColor = GUI::luminance(DARKER_GREY,120);
uint16_t GUI::widgetSubLabelColor = TFT_GOLD;
AlteredColor GUI::glowingGoldColor = AlteredColor{.baseColor = GUI::gold, .value = &GUI::unUsedColor};
const uint16_t GUI::menuActiveIconColor = GUI::luminance(TFT_BLUE,150); 
const uint16_t GUI::menuInactiveIconColor = GUI::luminance(TFT_BLUE,255); 
const uint16_t GUI::menuGreydoutColor= GUI::luminance(TFT_LIGHTGREY,255); 
const uint16_t GUI::menuBackGroundColor = TFT_WHITE;

// WSIZE AND POSITIONS DEFINITIONS
const int16_t GUI::zero = 0;
const int16_t GUI::un = 1;
const int16_t GUI::three = 3;
const int16_t GUI::center = TFT_WIDTH/2;
const int16_t GUI::y1 = TOP2WIDGET_SIZE+1;
const int16_t GUI::tenPixels = 10;
const int16_t GUI::midScreenHoriz = TFT_WIDTH/2;
const int16_t GUI::midScreenVertical = TFT_HEIGHT/2;
const int16_t GUI::bigWidgetMidY = GUI::midScreenVertical-WIDGET_BIG_WIDTH/2;
const int16_t GUI::iconMidY = GUI::midScreenVertical-ICON_MAX_SIZE/2 ;  
const int16_t GUI::topTextY = GUI::y1+TOP2WIDGET_SIZE;
const int16_t GUI::iconY = GUI::topTextY + CONFIG_TEXTAREA_MESSAGE_SIZE;
const int16_t GUI::bottomTextY = GUI::iconY + MEDIUM_ICON_SIZE;
const int16_t GUI::belowBottomTexty = GUI::bottomTextY+SIZE_14*1.1;
const int16_t GUI::leftMenuIconXpos = GUI::midScreenHoriz-MENU_ICON_SIZE*1.4;
const int16_t GUI::rightMenuIconXpos = GUI::midScreenHoriz+MENU_ICON_SIZE*1.4;
const int16_t GUI::hquarter1xPos = TFT_HQUARTER-TFT_HQUARTER/2;
const int16_t GUI::hquarter2xPos = TFT_HQUARTER*2-TFT_HQUARTER/2;
const int16_t GUI::hquarter3xPos = TFT_HQUARTER*3-TFT_HQUARTER/2;
const int16_t GUI::hquarter4xPos = TFT_HQUARTER*4-TFT_HQUARTER/2;
const int16_t GUI::vquarter1xPos = TFT_VQUARTER-TFT_VQUARTER/2;
const int16_t GUI::vquarter2xPos = TFT_VQUARTER*2-TFT_VQUARTER/2;
const int16_t GUI::vquarter3xPos = TFT_VQUARTER*3-TFT_VQUARTER/2;
const int16_t GUI::vquarter4xPos = TFT_VQUARTER*4-TFT_VQUARTER/2;
const int16_t GUI::menuIconYpos = TFT_HEIGHT/2-MENU_ICON_SIZE/2.2;
const int16_t GUI::menuTitleY = TFT_HEIGHT-MENU_TITLE_SIZE*1.1; 

#endif
