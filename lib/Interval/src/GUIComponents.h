// label before icon

#ifndef GUIComponents_h
#define GUIComponents_h

#include <OpenFontRender.h>
#include <vector>

#define ICON_LAPTOP           "`"
#define ICON_EARTH            "p"
#define ICON_LANG             "p"
#define ICON_ETH              "["
#define ICON_ROLLING1         "S"        
#define ICON_ROLLING2         "T"
#define ICON_LINK             "j"
#define ICON_FOOTSWITCH       "W"
#define ICON_AP               "a"
#define ICON_MIXETHWIFI       "X"
#define ICON_TFT_ROTATE       "}"

#define ICON_WIFI_STRENGTH_0  "I"
#define ICON_WIFI_STRENGTH_1  "J"
#define ICON_WIFI_STRENGTH_2  "K"
#define ICON_WIFI_STRENGTH_3  "L"

#define ICON_BAT_LEVEL_0      "N"
#define ICON_BAT_LEVEL_1      "O"
#define ICON_BAT_LEVEL_2      "P"
#define ICON_BAT_LEVEL_3      "Q"

#define ICON_BAT_CHARGING     "M"

#define ICON_PLUG             "R"

#define ICON_NOTOK            "r"
#define ICON_WAIT             "i"
#define ICON_FORBID           "s"

#define ICON_POWER            "E"
#define ICON_RESET            "m"

#define ICON_TEXT_STRIP       "!"
#define ICON_SENDRETURN_STRIP "="
#define ICON_CHGAIN_STRIP     "5"
#define ICON_CHECKED          "]"
#define ICON_EMPTY            "H"
#define ICON_QUIT             "t"
#define ICON_INFO             "q"
#define ICON_CONFIG           "~"
#define ICON_PROFILES         "h"
#define ICON_SAVE             "o"
#define ICON_NODOT            "b"
#define ICON_LEFTDOT          "c"
#define ICON_RIGHTDOT         "d"
#define ICON_FULLDOT          "e"

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

stripColor colorMap[] = {
  {TFT_SILVER, TFT_GREY},
  {TFT_WHITE, TFT_RED},
  {TFT_GREY, TFT_GREEN},
  {TFT_GREY, TFT_YELLOW},
  {TFT_WHITE, TFT_BLUE},
  {TFT_WHITE, TFT_MAGENTA},
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


class GUI : public GenericComponent{
public:
    static TFT_eSPI* tft;      
    static bool off;

    static uint16_t unUsedColor;
    static uint16_t defaultFcolor;
    static uint16_t defaultBcolor;
    // const colors
    static const uint16_t transparentColor;
    static const uint16_t nearlyBlack;
    static const uint16_t green;   
    static const uint16_t silver;
    static const uint16_t red;
    static const uint16_t white;
    static const uint16_t purple;
    static const uint16_t yellow;
    static const uint16_t gold;
    static const uint16_t blue;
    static const uint16_t menuActiveIconColor;
    static const uint16_t menuInactiveIconColor;
    static const uint16_t menuBackGroundColor;
    // variable colors
    static uint16_t widgetInnerColor;
    static uint16_t widgetSubLabelColor;

    static AlteredColor glowingGoldColor;

    static const int16_t zero;
    static const int16_t center;

    /*static uint16_t networkIconColor;
    static uint16_t configMediaIconColor;*/
    
    static inline void cls(uint16_t color = GUI::defaultBcolor){
      tft->fillScreen(color);  
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

#ifdef TFT_WIDTH
  #undef TFT_WIDTH
#endif
#ifdef TFT_HEIGHT
  #undef TFT_HEIGHT
#endif


/*#define TFT_WIDTH GUI::width()
#define TFT_HEIGHT GUI::height()*/

#define TFT_WIDTH 320
#define TFT_HEIGHT 240
// #########################################################################
// Base class
// #########################################################################
class Widget : public GUI{
  public:
    uint16_t width=1;
    uint16_t height=1; 
    TFT_eSprite* sprite = nullptr;    

    Widget(const int16_t* xPos, const int16_t* yPos, const uint16_t* fC, const uint16_t* bC, bool d):GUI(d){
      x = xPos; y = yPos; fColor = fC; bColor = bC;
      id = "Widget"+String(*xPos)+"x"+String(*yPos); 
    }

    void setFcolor(const uint16_t* fC){
      fColor = fC;
    }

    virtual void setText(const String* text) = 0;

    virtual ~Widget(){
      if (sprite != nullptr) delete sprite;         
    }

    virtual bool refresh(bool force=false){
      refreshed = force;      
      if (oldX != *x || oldY != *y || oldfColor != *fColor || oldbColor != *bColor || refreshed){
        if (debug){
          DEBUG_PRINT("\n%s.refreshed()",id.c_str());
          if (oldfColor != *fColor) DEBUG_PRINT("\tfColor %i=>%i",this->id.c_str(),oldfColor,*fColor);
          if (oldbColor != *bColor) DEBUG_PRINT("\tbColor %i=>%i",this->id.c_str(),oldbColor,*bColor);
          if (oldX != *x) DEBUG_PRINT("\tx %i=>%i",this->id.c_str(),oldX,*x);
          if (oldY != *y) DEBUG_PRINT("\ty %i=>%i",this->id.c_str(),oldY,*y);
        }

        oldX=*x; oldY=*y;
        oldfColor = *fColor; oldbColor = *bColor;
        refreshed = true;
      }      
      return refreshed; // nothing to redraw
    }

    void createSprite(){
      //createSprite(*bColor);
      createSprite(*bColor);
    }
    
    void createSprite(uint16_t color){
      if (sprite == nullptr){
        sprite = new TFT_eSprite(tft);
        //sprite->setColorDepth(BIT_DEPTH);   // we keep a high definition for smooth and antialias rendering
        //sprite->setSwapBytes(true);
        sprite->createSprite(width,height);     
        sprite->fillSprite(color); 
        if (debug){DEBUG_PRINT("\tsprite(%i,%i)",width,height);}
      }     
    }


    virtual int16_t getX(){return *x;}
    virtual int16_t getY(){return *y;}  

  private:    

  protected:    
    const uint16_t *fColor, *bColor;
    const int16_t *x,*y;
    int16_t oldX=-1,oldY=-1;
    uint16_t oldfColor, oldbColor;
};

class Canva : public Widget{

  public:

    Canva(const int16_t* posX, const int16_t* posY, uint16_t width, uint16_t height, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=false):Widget(posX, posY, fC, bC, d){      
      id = "Canva"+String(*posX)+"x"+String(*posY); 
      this->width = width; this->height = height;
    } 

    virtual bool refresh(bool force=false){ 
      refreshed = force;
      if(Widget::refresh(force)){     
        createSprite();
        sprite->drawRect(*x,*y,width,height,*fColor);
        // background is done by filling the sprite with bColor when creting it
      }
      return refreshed;
    }

    virtual void setText(const String* text){    
    }

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
class Font : public Widget{  
  public:    
 
    Font(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Widget(posX, posY, fC, bC, d){
      value = v; size = s; align = a;
      id = "Font"+String(*posX)+"x"+String(*posY); 
    }

    virtual int16_t getX(){
      int16_t xAligned = *x;  // align left by default
      if (align == Align::Center) xAligned = *x - width/2;
      if (align == Align::Right) xAligned = *x - width;
      return xAligned;
    }

    virtual void setText(const String* text){
      value = text;      
    }

    virtual bool refresh(bool force=false){ 
      refreshed = force;
      if(Widget::refresh(force) || *value != oldValue){       
        if (debug){
          if (*value != oldValue) DEBUG_PRINT("\n%s changed value from '%s' to '%s'", this->id.c_str(), oldValue.c_str(), value->c_str());
        }
        // Load the font and check it can be read OK

        FT_BBox boundaries1 = getOfr()->calculateBoundingBox(0,0,size,Align::Left ,Layout::Horizontal,oldValue.c_str());
        FT_BBox boundaries2 = getOfr()->calculateBoundingBox(0,0,size,Align::Left ,Layout::Horizontal,value->c_str());
        width = max(boundaries1.xMax,boundaries2.xMax)+1; 
        // add 2 pixels for surrounding animation move to right/left
        //height = max(boundaries1.yMax,boundaries2.yMax)*0.8; 
        height = max(boundaries1.yMax,boundaries2.yMax)+1; 
        // the boundaries provided are a bit too big in height, therefore, we reduce it by 20%

        getOfr()->setFontSize(size);
        /*width = max(getOfr()->getTextWidth(value->c_str()),getOfr()->getTextWidth(oldValue.c_str()))+2;
        height = max(getOfr()->getTextHeight(value->c_str()),getOfr()->getTextHeight(oldValue.c_str()));*/

        if (width>tft->width()) width=tft->width();
        if (height>tft->height()) height=tft->height();

        createSprite();

        /*if (debug){
          DEBUG_PRINT("\t =>(%i,%i)",width,height);
          DEBUG_PRINT("\t sprite is : %ix%i",sprite->getViewportWidth(),sprite->getViewportHeight());  

        if (debug){
          DEBUG_PRINT("\t %ld,%ld  %ld,%ld =>(%i,%i)",boundaries1.xMax,boundaries1.yMax,boundaries2.xMax,boundaries2.yMax,width,height);
          DEBUG_PRINT("\t sprite is : %ix%i",sprite->getViewportWidth(),sprite->getViewportHeight());          
        }                  
        }*/

        //sprite->fillSprite(TFT_PURPLE);      

        //getOfr()->setDrawer(*sprite);
        getOfr()->setDrawer(*sprite);

        getOfr()->setCursor(width/2, 0);
        /*if (getOfr()->getTextHeight(value->c_str())>=getOfr()->getTextHeight(oldValue.c_str()))
        getOfr()->setCursor(width/2, -height*0.3);    // as we reduced the sprite windows (see above) we have to move the text a bit up (30% seems to be correct to cover all cases)
        else getOfr()->setCursor(width/2, -height/2);*/
        
                                                  // center + 1 pixel to let the surrounding pixel for animation scroll to right
        getOfr()->setFontColor(*fColor,*bColor);
        
        /*
        if (align == Align::Center)
          getOfr()->cprintf(value->c_str());           
        else if (align == Align::Left)
          getOfr()->printf(value->c_str());           
        else if (align == Align::Right)
          getOfr()->rprintf(value->c_str());           
          */
        getOfr()->cprintf(value->c_str());           

        //getOfr()->unloadFont();

        /*sprite->pushSprite(getX(),y,TFT_TRANSPARENT);

        delete sprite;*/
        
        oldValue = *value;            

        refreshed = true;          // and return the info that we did a refresh
      }
      return refreshed;
    }  
  private:    
    const String *value;
    String oldValue = "?";
    //OpenFontRender ofr;    
    Align align;
  protected:
      uint8_t size = 0;
      virtual OpenFontRender* getOfr()=0;
};

class Label : public Font{
  public:
      static OpenFontRender* ofr; 
      Label(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Font(v, a, posX, posY, s, fC , bC , d){
      };
  protected:
      virtual OpenFontRender* getOfr(){return ofr;}
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

    TextArea(const String* textvalue, uint16_t w, Align pAlign, Align aAlign, const int16_t* posX, const int16_t* posY, uint8_t fontSize, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Widget(posX, posY, fC, bC, d){
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
      if (*text != oldText || force){

        LineBreakInfo lbi = computeLineBreakInfo(*text, width);
        if (debug) DEBUG_PRINT("\nlbi.lines:%i, lbi.maxWidth:%i, lbi.maxHeight:%i\n",lbi.lines,lbi.maxWidth,lbi.maxHeight);

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
        Label l = Label(text,phraseAlign,&GUI::zero,&GUI::zero,size,fColor,bColor,debug);
        l.refresh(true);    // the only way to know the maximum height of a line is to draw it (without rendering)
        height = l.height * lbi.lines;
        createSprite();

        for (auto &&nextPhrase : phrases){
          if (debug) DEBUG_PRINT("\n%s",nextPhrase.c_str());
          //tft.drawString(nextPhrase, 0, offsetY, FONT4);

          Label* label = new Label(&nextPhrase,phraseAlign,&GUI::zero,&GUI::zero,size,fColor,bColor,debug);
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

    virtual void setText(const String* newText){text = newText;}

  protected:

  private:
    Align areaAlign, phraseAlign;
    const String* text;
    String oldText;
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
    NumericLabel(const float* n,uint8_t dec, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Label(&convertedNumber,  a,  posX, posY, s, fC, bC, d){
      number = n; decimal = dec;
      id = "NumericLabel"+String(*posX)+"x"+String(*posY); 
    }

    virtual bool refresh(bool force=false){
      refreshed = force;
      if (*number != oldNumber || force){
        if (*number<10 && *number >=0 && decimal == 0){
          char buffer[3]; // 1 digit + leading zero + null terminator
          sprintf(buffer, "0%d", (int)*number);
          convertedNumber = buffer;          
        }else convertedNumber = String((float)*number,(unsigned int)decimal); // set precision
        if (debug){DEBUG_PRINT("\n%s changed number from %.2f to %.2f", this->id.c_str(), oldNumber, *number);}
        oldNumber = *number;
        refreshed = true;
      }
      Label::refresh(refreshed);
      return refreshed;
    }

  private:
    const float* number;
    float oldNumber;
    String convertedNumber;
    uint8_t decimal;
};

class Icon : public Font{
  public:
    static OpenFontRender* ofr; 
    
    Icon(const String* v, Align a, const int16_t* posX, const int16_t* posY, uint8_t s, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Font(v, a, posX, posY, s, fC , bC , d){
      id = "Icon"+String(*posX)+"x"+String(*posY)+"~"+*v;   
      //if (debug) DEBUG_PRINT("\n new %s",id);
    };
    
  protected:
    virtual OpenFontRender* getOfr(){return ofr;}
};

enum{
  ANIMATE_ONCE,
  ANIMATE_REPEAT,
  ANIMATE_PING_PONG,
};

class AnimatedIcon : public Icon{

  public:
    AnimatedIcon(const String* stringArray, uint8_t nb, Align a, const int16_t* posX, const int16_t* posY, uint8_t size, uint8_t at=ANIMATE_REPEAT, uint16_t ad=500, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Icon(&iconArray[0], a, posX, posY, size, fC , bC , d){
      iconArray = stringArray;      
      animateType = at; animateDelay = ad;
      numberOfFrames = nb;
      id = "AnimatedIcon"+String(*posX)+"x"+String(*posY);
      //DEBUG_PRINT("\n%s contains %i frames",id.c_str(),numberOfFrames);
    };

    virtual void setText(const String* text){
      iconArray = text;      
      if (debug){DEBUG_PRINT("\n%s setText {%s}", id.c_str(), iconArray[0].c_str());}      
    }


    virtual bool refresh(bool force=false){
      refreshed = force;

      if (oldIconArray != iconArray){
        if (debug){DEBUG_PRINT("\n%s changed value to <%s>", id.c_str(), iconArray[0].c_str());}
        refreshed = true;
        oldIconArray = iconArray;
      }      

      if (wait() && !refreshed) return false;

      Icon::setText(&iconArray[frame]);
      Icon::refresh(refreshed);    
      if (refreshed){if (debug) DEBUG_PRINT("%s frame:%i",id.c_str(),frame);}    

      switch (animateType){
        case ANIMATE_ONCE:
          if (frame == numberOfFrames-1) break;
          else frame++;
        break;
        case ANIMATE_REPEAT:
          frame = (frame + 1) % numberOfFrames;
        break;
        case ANIMATE_PING_PONG:
          frame += inc;
          if (frame == -1){
            frame = 0;
            inc = 1;
          }else if (frame == numberOfFrames){
            frame = numberOfFrames-1;
            inc = -1;
          }                
        break;      
      }

      return refreshed;
    }    

    bool wait(){
      if(millis()-lastTick<animateDelay) return true;
      else{
        lastTick = millis();
        return false;
      }
    }

  private:
    int8_t frame = 0;
    uint8_t inc = -1;
    uint8_t numberOfFrames,animateType;
    uint16_t animateDelay;
    const String* iconArray;
    const String* oldIconArray;
    unsigned long lastTick=0;
};

class FromToWidget : public Widget{
  public:
    FromToWidget(const float* v, const float mini, const float maxi, const int16_t* posX, const int16_t* posY, const uint16_t* fC, const uint16_t* bC, bool d):Widget(posX, posY, fC, bC, d){
      value = v; 
      minValue = mini; maxValue = maxi;
    }
  
    virtual bool refresh(bool force=false){ 
      refreshed = force;
      sprite = nullptr;   // we set that as the GUIManager would need to know if we use a sprite or not, by default, no need.

      // location or bg color has changed, hence we redraw the frame, we don't call parent refresh as treatment for front color is different
      if(oldX != *x || oldY != *y || oldbColor != *bColor || force){
        refreshFrame();
        oldX=*x; oldY=*y;
        oldbColor = *bColor;        
        refreshed = true;
        if (debug){DEBUG_PRINT("\n%s.refreshed frame, new: %i,%i",this->id.c_str(),*x,*y);}
      }

      refreshLogic(force);

      if (oldfColor != *fColor){  // warning this may refresh twice the widget as it was done abve
        refreshContent(true); //when the color change we redraw the complete arc
        oldfColor = *fColor;  
        refreshed = true;
        if (debug){DEBUG_PRINT("\n%s.refreshed color, new: %i",this->id.c_str(),*fColor);}
      }

      return refreshed;
    }

  protected:
    virtual bool refreshLogic(bool force) = 0;
    virtual void refreshContent(bool full) = 0;
    virtual void refreshFrame() = 0;
    uint16_t from,to,oldFrom,oldTo;
    const float* value;
    float oldValue;
    float minValue, maxValue;

};

class Arc : public FromToWidget{
  public:
    Arc(const float *v, const float mini, const float maxi, const int16_t* posX, const int16_t* posY, uint16_t r, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, uint16_t sAngle = 0, bool d=false):FromToWidget(v, mini, maxi, posX, posY, fC, bC, d){            
      radius = r;
      width = height = (radius*2)+2;        // +2 because of outer circle                
      id = "Arc"+String(*posX)+"x"+String(*posY);
      startAngle = sAngle;
      from = startAngle;
    }

    virtual int16_t getX(){return *x-width/2;}
    virtual int16_t getY(){return *y-height/2;}  

    virtual void setText(const String* text){
      id = *text;      
    }    

  protected:
    uint16_t startAngle, radius;

    virtual bool refreshLogic(bool force){ 
      refreshed = force;
      if (*value != oldValue || force){
        //to = map(*value, minValue, maxValue, START_ANGLE, 360-START_ANGLE);
        to = mapFloatToInt(*value, minValue, maxValue, startAngle, 360-startAngle);
        if (to > 360-startAngle) to = 360-startAngle;
        else if (to<startAngle) to = startAngle;

        oldValue = *value;
        refreshed = true;
        if (debug){DEBUG_PRINT("\n%s.refreshed value, new: %.2f",id.c_str(),*value);}
      }

      if (to != from || force){
        refreshContent(force);  // or we may redrw the arc if its value change or it was forced to
        from = to;      
        refreshed = true;
        if (debug){DEBUG_PRINT("\n%s.refreshed angle, new: %i",this->id.c_str(),to);}
      }
      return refreshed;
    }

    virtual void refreshFrame(){
    };

    void refreshContent(bool full){
      createSprite();       
          
      // THE ARC VALUE
      // Allocate a value to the arc thickness dependant of radius
      uint8_t thickness = (radius / 5)+1;
      if ( radius < 25 ) thickness = radius / 3;        

      // if forced, it means we just redraw the complete indicator
      if (full) sprite->drawArc(width/2, height/2, radius-3, radius - thickness, startAngle, to, *fColor, TFT_BLACK); // TFT_SKYBLUE random(0x10000)
      else{ // otherwise we only redraw the required portion of it
        if (to > from) {
          sprite->drawArc(width/2, height/2, radius-3, radius - thickness, from, to, *fColor, TFT_NEARLYBLACK); // TFT_SKYBLUE random(0x10000)
        }else {
          sprite->drawArc(width/2, height/2, radius-3, radius - thickness, to, from, TFT_NEARLYBLACK, widgetInnerColor);
        }
      }
      refreshed = true;
    }
};


class RingMeter : public Arc{
  #define START_ANGLE 30
  public:
    
    RingMeter(const float *v, float mini, float maxi, uint8_t deci, const int16_t* posX, const int16_t* posY, uint16_t r, String* u, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Arc(v,mini, maxi, posX, posY, r, fC, bC, START_ANGLE, d){
      unit = u; 
      id = "RingMeter"+String(*posX)+"x"+String(*posY);
      if (deci>1) deci = 1;
      decimal = deci;        
    }

    virtual void setText(const String* text){
      unit = text;      
    }

  protected:
    virtual bool refreshLogic(bool force){ 

      if (Arc::refreshLogic(force)) refreshValueLabel();
      
      if (*unit != oldUnit || force){
        refreshUnitLabel();
        oldUnit = *unit;
        refreshed = true;
        if (debug){DEBUG_PRINT("\n%s.refreshed unit, new: %s",this->id.c_str(),unit->c_str());}
      }
      return refreshed;      
    }  

  private:        
    Label*  label;
    uint16_t valueLabelHeight;
    const String *unit;
    String oldUnit ;
    uint8_t decimal, clearArea=0;

    void refreshFrame(){
      createSprite();
      sprite->fillCircle(width/2, height/2, radius,widgetInnerColor );
      sprite->drawSmoothCircle(width/2, height/2, radius, TFT_SILVER, widgetInnerColor);
      uint16_t tmp = radius - 3;
      sprite->drawArc(width/2, height/2, tmp, tmp - tmp / 5, startAngle-1, 360-startAngle+1, TFT_NEARLYBLACK, widgetInnerColor);           
    }

    void refreshUnitLabel(){
      createSprite();
      // THE UNIT LABEL
      uint8_t unitFontSize = (2 * radius*0.7) / (unit->length());
      label = new Label(unit,Align::Center,&GUI::zero,&GUI::zero,unitFontSize,&widgetSubLabelColor,&widgetInnerColor,debug);
      label->refresh(true);
      label->sprite->pushToSprite(sprite,width/2-label->width/2,height/2+(radius-unitFontSize/1.1));     //height/2+valueLabelHeight/3
      delete label;         
    }

    void refreshValueLabel(){      
      createSprite();

      // THE VALUE LABEL        

      String fToS = String((float)*value,(unsigned int)decimal);   // convert the floating value to String with precision so we can get the number of character/digit to determine the fontSize        
      uint8_t digitFontSize;
      if (decimal == 0){
        digitFontSize = (3.5 * radius) / (fToS.length()); // 4
        //if (*value<100 || *value>0) digitFontSize *= 0.70; // for 2 digit int only and positive number, we reduced a bit the size otherwise we have a square around 0.65
      }else{
        digitFontSize = (4.8 * radius) / (fToS.length());
      }
      
      sprite->fillCircle(width/2, height/2.2, radius*0.65,widgetInnerColor); //

      NumericLabel* digits = new NumericLabel(value,decimal,Align::Center,&GUI::zero,&GUI::zero,digitFontSize,&GUI::defaultFcolor,&GUI::transparentColor,debug);
      digits->refresh(true);
      valueLabelHeight = digits->height; // is reused by the unit label to be disdplayed to properly display it
      digits->sprite->pushToSprite(sprite,width/2-digits->width/2,height/2-valueLabelHeight/1.5,GUI::transparentColor);    
      delete digits;      
    }  

};

class Toggle : public Widget{
  public:
    const bool* revert;

    Toggle(const String* v, const int16_t* posX, const int16_t* posY, uint8_t h, const bool* p, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Widget(posX, posY, fC, bC, d){
      height = h;
      revert = p;
      oldRevert = !*revert;
      text = v;
    }

    virtual bool refresh(bool force=false){
      Widget::refresh(force);
      if (refreshed || oldRevert != *revert || oldText != *text){

        uint16_t fillColor, textColor;

        if(*revert) {
          //DEBUG_PRINT("\nFlip");
          fillColor    = TFT_RED;
          textColor    = TFT_WHITE;
        } else {
          //DEBUG_PRINT("\nFlop");
          fillColor    = GUI::defaultBcolor;
          textColor    = *fColor;
        }

        width = TFT_WIDTH;

        createSprite(GUI::defaultBcolor);        

        sprite->fillSmoothRoundRect(0,0 , width, height, height/5, fillColor);
        if (!*revert) sprite->drawRoundRect(0,0, width, height, height/5, *bColor);

        uint16_t transparent = TFT_TRANSPARENT;
        Label* label = new Label(text,Align::Center,&GUI::zero,&GUI::zero,height*1.4,&textColor,&transparent, debug);
        label->refresh(true);
        label->sprite->pushToSprite(sprite,width/2-label->width/2,0,TFT_TRANSPARENT);    //center
        delete label;       

        refreshed = true;
        oldRevert = *revert;
        oldText = *text;
        
      }
      return refreshed;

    }

    virtual void setText(const String* value){
      text = value;      
    }


  private:    
    bool oldRevert;
    const String* text;
    String oldText;

};

class VuMeter : public Widget{
// vertical https://github.com/HX2003/color_bar_tft/blob/master/color_bar_tft.ino
// horizontal https://forum.arduino.cc/t/tft_espi-new-example-for-animated-dials/643382/3
};

class Bar : public FromToWidget{
  public:

    Bar(const float *v, const float min, const float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset=0, uint8_t r=0, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):FromToWidget(v, min, max, posX, posY, fC, bC, d){
      radius = r;
      width = w; height = h;
      frameOffset = fOffset;
      from = frameOffset;
      to = width-frameOffset;
    }

   
  protected:

    void refreshFrame(){
      createSprite();
      if (frameOffset != 0){
        sprite->fillRoundRect(*x,*y,width,height,radius,*bColor);
        sprite->drawRoundRect(*x,*y,width,height,radius,*fColor);
      }      
    }

    void refreshContent(bool full){
      refreshed = full;
      createSprite(); 
      if (full){
        // full content refresh, means FROM -> TO
        sprite->fillRoundRect(from,(*y)+frameOffset,to,height-frameOffset,radius,*fColor);        
      }else{
        // only refresh tail/and or head
        refreshed = true;
        
        if (from != oldFrom){              
          oldFrom = from;
          if (from > oldFrom){
            // remove tail
          }else{
            // add tail 
          }
        }

        if (to != oldTo){
          oldTo = to;        
          if (to > oldTo){
            // add head
          }else{
            // remove head
          }
        }
      }

    }

    bool refreshLogic(bool force){

      if (from !=oldFrom || to !=oldTo) refreshContent(false); // partial refresh
      return refreshed;
    }
  
  private:
    uint8_t frameOffset = 0, radius = 5;

};

class ProgressBar : public Bar{

  public:

    ProgressBar(float* v, float min, float max, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, uint8_t fOffset, uint8_t r,const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Bar(v, min, max, posX, posY,w, h,fOffset, r,fC,bC,d){
      id = "ProgressBar"+String(*posX)+"x"+String(*posY); 
      value = v;
      from = 0;
    }

    bool refreshLogic(bool force=false){ 
      refreshed = force;
      if(Bar::refresh(force) || force ){
        

      }
      return refreshed;
    }


};


class PendingBar : public Bar{

  public:
    PendingBar(float* v, float min, float max, const uint8_t* s, const int16_t* posX, const int16_t* posY, uint16_t w, uint16_t h, bool pp = false, uint8_t fOffset = 0, uint8_t r = 0, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool d=true):Bar(v, min, max, posX, posY,w, h,fOffset, r,fC,bC,d){
      id = "PendingBar"+String(*posX)+"x"+String(*posY); 
      pingPong = pp; size = s;
    }
    
    bool refreshLogic(bool force=false){ 
      refreshed = force;

      if(Bar::refresh(force) || force || *size != oldSize){
        
        oldSize = *size;
      }
      return refreshed;
    }

  private:
    bool pingPong = false;
    const uint8_t* size;
    uint8_t oldSize = 101;

    uint8_t begin = 0, finish = 0;

};

#define TOP2WIDGET_SIZE TFT_HEIGHT/3

class CompositeWidget: public Widget{
  public:
    CompositeWidget(LinkedList<Widget*>* g, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, bool debug=false):Widget(&GUI::zero,&GUI::zero,fC,bC,debug){
      guiList = g;
    }
  protected:
    LinkedList<Widget*>* guiList;
};

class Top2Toggles : public CompositeWidget {
  #define TOP2WIDGET_SIZE TFT_HEIGHT/3
  public:
    Top2Toggles(LinkedList<Widget*>* guiList, const String* topLabel, const String* bottomLabel, const bool* topReverse = &GUI::off, const bool* bottomReverse = &GUI::off, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const uint16_t *fC2 = &GUI::defaultFcolor, const uint16_t *bC2 = &GUI::silver, bool debug=false) : CompositeWidget(guiList, fC, bC, debug){
      id = "Top2Toggles";

      topString = topLabel; bottomString = bottomLabel; fColor2 = fC2;
      topBool = topReverse; bottomBool = bottomReverse; bColor2 =bC2;
       
      topToggle = new Toggle(topString,&GUI::zero,&GUI::zero,TOP2WIDGET_SIZE/2,topBool, fColor,bColor, debug);        
      bottomToggle = new Toggle(bottomString,&GUI::zero,&y2,TOP2WIDGET_SIZE/2,bottomBool, fColor2, bColor2,debug);        
      
      guiList->add(topToggle);
      guiList->add(bottomToggle);         
    }    

    virtual void setText(const String* text){};

  private:
    const String *topString, *bottomString; 
    const bool *topBool, *bottomBool;
    Toggle *topToggle, *bottomToggle;
    int16_t y2 = TOP2WIDGET_SIZE/2+1;
    const uint16_t *fColor2, *bColor2;
};


class Top1Toggle1Label : public CompositeWidget {  
  public:
    Top1Toggle1Label(LinkedList<Widget*>* guiList, const String* topS, const String* bottomS, const bool* topReverse = &GUI::off, const uint16_t *fC = &GUI::defaultFcolor, const uint16_t *bC = &GUI::defaultBcolor, const uint16_t *fC2 = &GUI::defaultFcolor, const uint16_t *bC2 = &GUI::defaultBcolor, bool debug=false) : CompositeWidget(guiList, fC, bC, debug){
      id = "Top1Toggle1Label";

      topString = topS; bottomString = bottomS; fColor2 = fC2;
      topBool = topReverse; bColor2 =bC2;
       
      topToggle = new Toggle(topString,&GUI::zero,&GUI::zero,TOP2WIDGET_SIZE/2,topBool, fColor,bColor, debug);        
      bottomLabel = new Label(bottomString, Align::Center,&GUI::center, &y2,(TOP2WIDGET_SIZE/2)*1.2,fC2,bC2,false);        

      guiList->add(topToggle);
      guiList->add(bottomLabel);         
    }    

    virtual void setText(const String* text){};

  private:
    const String *topString, *bottomString; 
    const bool *topBool;
    Toggle *topToggle;
    Label *bottomLabel;
    int16_t y2 = TOP2WIDGET_SIZE/2;
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

bool GUI::off = false;
uint16_t GUI::defaultFcolor = TFT_FCOLOR;
uint16_t GUI::defaultBcolor = TFT_BGCOLOR;
uint16_t GUI::unUsedColor = TFT_BLACK;

const uint16_t GUI::transparentColor = TFT_TRANSPARENT;
const uint16_t GUI::green = TFT_GREEN;
const uint16_t GUI::silver = TFT_SILVER;
const uint16_t GUI::red = TFT_RED;
const uint16_t GUI::white = TFT_WHITE;
const uint16_t GUI::purple = TFT_PURPLE;
const uint16_t GUI::yellow = TFT_YELLOW;
const uint16_t GUI::blue = TFT_BLUE;
const uint16_t GUI::nearlyBlack = TFT_NEARLYBLACK;
const uint16_t GUI::gold = TFT_GOLD;

uint16_t GUI::widgetInnerColor = GUI::luminance(DARKER_GREY,120);
uint16_t GUI::widgetSubLabelColor = TFT_GOLD;

const uint16_t GUI::menuActiveIconColor = GUI::luminance(TFT_BLUE,150); 
const uint16_t GUI::menuInactiveIconColor = GUI::luminance(TFT_BLUE,255); 
const uint16_t GUI::menuBackGroundColor = TFT_WHITE;
/*uint16_t GUI::networkIconColor = TFT_RED;
uint16_t GUI::configMediaIconColor = TFT_RED;*/

const int16_t GUI::zero = 0;
const int16_t GUI::center = TFT_WIDTH/2;

AlteredColor GUI::glowingGoldColor = AlteredColor{.baseColor = GUI::gold, .value = &GUI::unUsedColor};

#endif