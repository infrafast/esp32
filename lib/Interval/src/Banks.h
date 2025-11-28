#ifndef BANKSDEF_h
#define BANKSDEF_h

#include "Interval.h"

#define C_CURRENTLAYOUT     "layout"
#define C_POSSIBLELAYOUT    "possibleLayout"
#define C_BANKS             "banks"
  #define C_STRIPS          "strips" 


namespace Interval {
  class Bank{  
    #define BANK_MAXWINDOW 3
    #define STRIP_PER_WINDOW 4
    #define BANK_MAX_STRIPS BANK_MAXWINDOW*STRIP_PER_WINDOW
  
    public:
      uint8_t selector=1; 
      //uint8_t oldSelector=0;
   
      //uint8_t numberOfStrips = 0;
      //uint8_t maxWindow = 0;
  
      String name;
      unsigned char icon=' ';
  
      uint8_t layout = LAYOUT_DETAILS;
      
      //LinkedList<Strip*> strips;
      std::vector<Strip*> strips; // Vecteur de strips
  
      Bank(String n, String value/*, StripFactory* sf*/){ 
        name = n;
        //Strip* s = nullptr;
        //for (size_t i = 0; i < BANK_MAX_STRIPS; ++i) strips[i] = nullptr; //strips.add(s); // strips.push_back(nullptr);
        deserialize(value);
      }
  
      ~Bank() {
        for (auto& s : strips) {
          delete s;
          s = nullptr;
        }
        strips.clear();
      }
  
      bool update(){
        bool redraw = false;
        for (const auto& strip : strips) {         
          // we ask each strip of the bank to process itself
          // it's up to him to determine if he refreshes it's internal data
          // some may only do that if they are visible        
          if (strip != nullptr){
            strip->process();
            if (strip->visible && strip->interface->state.trigger.redraw) redraw = true;
          }
        }
        return redraw;
      }

      void init(){
        //set stripIndex to the first available strip
        selector = 1;
      }
  
      /*uint8_t getMaxWindow(){
        //determine how many strips are in use ... means if there are free strip,in between (value nullptr) we skip but as soon as we found one not 0 we set it to the current index
        uint8_t numberOfBusyStrip = 0;
        for (uint8_t i = 0; i < BANK_MAX_STRIPS; i++) {
          if (strips[i] != nullptr) {
            numberOfBusyStrip = i;
          }
        }
        uint8_t newMaxWindow = numberOfBusyStrip / STRIP_PER_WINDOW + 1;
        return newMaxWindow;
      }*/
  

      bool deserialize(String value){
        bool success = false;
        //DynamicJsonDocument bank(JSON_BANK_SIZE);
        JsonDocument bank;
        DeserializationError error = deserializeJson(bank, value);
        if (error){
  
        }else{
  
          layout = LAYOUT_DETAILS; //bank[C_CURRENTLAYOUT];
          icon = bank[C_ICON].as<unsigned char>();          
  
          JsonArray stripArray = bank[C_STRIPS];
          for (int i=0; i<stripArray.size() && i<BANK_MAX_STRIPS; i++){        
            JsonObject jsonStrip = stripArray[i];        
            if (!addStrip(jsonStrip)) return false;
          }
          init();
          success = true;
        }
        return success;
      }
  
      /*bool insertStrip(JsonObject jsonStrip){
  
      }*/
  
      Strip* buildStrip(JsonObject jsonStrip){      
        String stripID = jsonStrip[C_STRIPTYPE];
        //DEBUG_PRINT("\nbuildStripID: %s",stripID.c_str());
        Strip* strip = stripFactory.build(stripID);  // build the strip by passing values to deserialize    
        return strip;
      }
  
      Strip* addStrip(JsonObject jsonStrip){
        String jsonStripString;                           
        serializeJson(jsonStrip, jsonStripString);
        Strip* strip = buildStrip(jsonStrip);
        if (strip != nullptr){    
          if (strip->deserialize(jsonStripString)){
            strips.push_back(strip);
          }  
        }else{
          DEBUG_PRINT("\nbuildStrip(%s) failed",jsonStripString.c_str());
        }
        return strip;
      }
  
      /*void add(Strip* strip, uint8_t index){
        if (index<BANK_MAX_STRIPS){
  
          strips[index] = strip;
        }
      }*/
  
      Strip* getStrip(uint8_t x, uint8_t y, uint8_t window){
  
        Strip* strip = nullptr;
        // retrieve strip at given position
        uint8_t i = 0;
        while (i<strips.size()){
  
          if (strips[i]->x == x && strips[i]->y == y && strips[i]->win == window){
  
            strip = strips[i];
            break;
          }
          i++;
        }
        return strip;
      }
  
      Strip* getStrip(uint8_t id){
        //DEBUG_PRINT("\n%s:getStrip(%d)=>",name.c_str(),id); 
        if (id>strips.size()){
          //DEBUG_PRINT("nullptr"); 
          return nullptr;
        }
  
        // retrieve strip at given position
      
        for (const auto& strip : strips) {
          if (strip->index == id){
            //DEBUG_PRINT("%p",strip); 
            return strip;
          }
        }
        
        //DEBUG_PRINT("nullptr"); 
        return nullptr;
      }
  
      bool trash(Strip* s) {
        for (auto it = strips.begin(); it != strips.end(); ++it) {
          if (*it == s) {
            delete *it;           // détruire l’objet pointé
            *it = nullptr;        // sécuriser le pointeur dans le vecteur
            strips.erase(it);     // retirer l’entrée du vecteur
            return true;
          }
        }
        return false;
      }
  
      String serialize(){
        //DynamicJsonDocument doc(JSON_STRIP_TEMPLATE_SIZE*BANK_MAX_STRIPS);   
        JsonDocument doc;
  
        doc[C_CURRENTLAYOUT] = layout;
        doc[C_ICON] = icon;
  
        //JsonArray jsonStrips = doc.createNestedArray(C_STRIPS);            
        JsonArray jsonStrips = doc[C_STRIPS].to<JsonArray>();            
        for (uint8_t i = 0; i<strips.size(); i++){
          if (strips[i] != nullptr){
            jsonStrips.add(strips[i]->serialize());
          }
        }
        String data;
        serializeJson(doc, data);
        removeJsonQuote(&data);
  
        return data;
      }
  
    private:
      //std::vector<Strip*> strips;
      //Strip* strips[BANK_MAX_STRIPS];
  
  
  };
}

using IntervalBank = Interval::Bank;  

// Define UI sizes
#define NAV_HEIGHT OVERLAY_ICON_SIZE
#define WIN_WIDTH (OVERLAY_ICON_SIZE * 1.6)
#define YPADDING 1
#define XPADDING 2
#define SPACE_BETWEEN_WIN  3
#define NAV_WIDTH ((WIN_WIDTH * BANK_MAXWINDOW) + (SPACE_BETWEEN_WIN * BANK_MAXWINDOW))
#define CELL_WIDTH (WIN_WIDTH / STRIP_PER_WINDOW)
#define CELL_HEIGHT (NAV_HEIGHT / STRIP_PER_WINDOW)
#define CANVA_VS_BANKNAME_SPACE 2

class BankNavigator: public Widget{
  //#define BANK_NAME_SIZE TFT_WIDTH/15
  public:
    IntervalBank *bank = nullptr;

    BankNavigator(IntervalBank* b/*, LinkedList<Widget*>* w*/, bool debug = false):bank(b), oldBank(b), Widget(&xPos, &yPos, &fC, &bC, &GUI::fullBrightness, debug){      
      //widgetList = w;      
      if (bank != nullptr) oldSelector = bank->selector - 1; //this is to force first refresh
      id = concatStrings("BankNavigator", xPos, "x", yPos);
    }

    virtual bool refresh(bool force = false) {
      refreshed = force;
      Widget::refresh(force);
          
      if (bank == nullptr){
        return refreshed;
        /*if (oldBank != bank){
          // clear the bankNavigator
          oldBank = bank;
          Canva* canva = new Canva(&GUI::zero, &GUI::zero, NAV_WIDTH, NAV_HEIGHT, &GUI::defaultBcolor, &GUI::defaultBcolor);
          canva->refresh();
          width = NAV_WIDTH; height = NAV_HEIGHT;
          createSprite();
          canva->sprite->pushToSprite(sprite, 0, 2);
          delete canva;
          refreshed = true;
        }
        return refreshed;*/
      }
      
      refreshed = (bank->selector != oldSelector) || 
                  (bank->strips.size() != oldNumberOfStrips) ||
                  (bank->name != oldBankName);                    

      if (!refreshed) return false;
     
      oldNumberOfStrips = bank->strips.size();
      oldSelector = bank->selector;
      oldBankName = bank->name;

      uint16_t navFrameColor = GUI::luminance(GUI::gold, 100);
      
      // Create a canvas for the navigation UI
      Canva* canva = new Canva(&GUI::zero, &GUI::zero, NAV_WIDTH, NAV_HEIGHT, &GUI::defaultBcolor, &GUI::defaultBcolor);
      canva->refresh(true);

      // Draw each strip on the navigation UI
      drawStripMap(canva);

      // Adjust width
      width = canva->width + CANVA_VS_BANKNAME_SPACE;

      // Display focused strip ID
      //drawStripID(canva);

      // Display bank name
      Label* label = new Label(&bank->name, Align::Right, &GUI::zero, &GUI::zero, OVERLAY_ICON_SIZE * 2, 10, &fC, &bC, brightness, debug);
      label->refresh(true);
      width += label->width;
      height = max(label->height, canva->height);
      width = max(width, oldWidth);
      oldWidth = width;

      createSprite();
      
      // Push canvas and label to the main sprite
      canva->sprite->pushToSprite(sprite, 0, 2);
      label->sprite->pushToSprite(sprite, canva->width + CANVA_VS_BANKNAME_SPACE, -OVERLAY_ICON_SIZE / 5);

      // Clean up dynamically allocated objects
      delete canva;
      delete label;
      //DEBUG_PRINT("\nBankNavigator refreshed");
      return refreshed;
    }

    virtual int16_t getX(){
      return xPos;
    }

    void drawStripMap(Canva* canva){
      int index, cellX, cellY, cellWidth, cellHeight;          

      uint8_t numberOfStrips = bank->strips.size();
      
      for (uint8_t i = 0; i < numberOfStrips; i++) {            

        Strip* strip = bank->strips[i];
        if (strip == nullptr) continue;

        if (bank->layout == LAYOUT_DETAILS){
          // Calculate strip position
          cellX = (strip->x * CELL_WIDTH) + (strip->win * WIN_WIDTH) + (SPACE_BETWEEN_WIN * strip->win);
          cellY = strip->y * CELL_HEIGHT;

          index = strip->size.indexOf('x');         

          // Extract width & height from the strip using pattern "3x2" 
          int widgetWidth = strip->size.substring(0, index).toInt();
          int widgetHeight = strip->size.substring(index + 1).toInt();

          cellWidth = (widgetWidth * CELL_WIDTH) - XPADDING;
          cellHeight = (widgetHeight * CELL_HEIGHT) - YPADDING;

        }else{
          // fixed size when zooming, width & height take one cell
          cellWidth = (NAV_WIDTH / numberOfStrips) - XPADDING;          
          cellHeight =  NAV_HEIGHT / 2;          

          index = strip->index-1;

          cellX = (index * cellWidth) + XPADDING;          
          cellY = NAV_HEIGHT / 3;      

          //DEBUG_PRINT("\nCell width: %i, index:%i, cellX:%i",cellWidth,index,cellX);

        }
        // Draw strip on the canvas
        canva->sprite->fillRect(cellX + XPADDING, cellY + YPADDING, cellWidth, cellHeight, GUI::luminance(colorMap[strip->colorID].fColor, strip->brightness * GUI::screenBrightness * 255));
        canva->sprite->drawRect(cellX, cellY, cellWidth + XPADDING, cellHeight + YPADDING,  GUI::luminance(colorMap[strip->colorID].bColor, strip->brightness * GUI::screenBrightness * 255));

      }      

    }

    void drawStripID(Canva* canva){
      String idStr = String(bank->selector);
      Label* uniqueID = new Label(&idStr, Align::Center, &GUI::zero, &GUI::zero, OVERLAY_ICON_SIZE * 2, 2, &GUI::white,&GUI::defaultBcolor);
      uniqueID->refresh(true);
      uniqueID->sprite->pushToSprite(canva->sprite, canva->width / 2 - NAV_HEIGHT / 2, -5);
      delete uniqueID;
    }

  protected:
    IntervalBank *oldBank;
    //LinkedList<Widget*>* widgetList;
    uint8_t oldSelector=255;
    int8_t oldNumberOfStrips = 0;    
    uint16_t oldWidth=0;
    String oldBankName="";

    //int16_t xPos = TFT_WIDTH/2, yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE;
    int16_t xPos = OVERLAY_ICON_SIZE*2 , yPos = TFT_HEIGHT-OVERLAY_ICON_SIZE-3;
    uint16_t fC = GUI::defaultFcolor, bC = GUI::defaultBcolor;

};

#endif