#ifndef BANKSDEF_h
#define BANKSDEF_h

/**
 * @file Banks.h
 * @brief Groups strips into named banks and draws their navigation overview.
 *
 * A bank is a page of controls, for example the strips used during one song.
 * It has a name, an icon, a display layout and a selected strip.
 * Saving turns its settings and strips into JSON; loading rebuilds the strips
 * through their registered types. ApplicationManager handles the profile file
 * and stores each bank under its name.
 */
#include "Interval.h"

#define C_CURRENTLAYOUT     "layout"
#define C_POSSIBLELAYOUT    "possibleLayout"
#define C_BANKS             "banks"
  #define C_STRIPS          "strips" 


namespace Interval {
  /**
   * Holds the strips belonging to one bank and owns their lifetime.
   * LAYOUT_DETAILS uses each strip's position and size; LAYOUT_ZOOM shows the
   * focused strip in the main view. BankNavigator provides a compact overview.
   * The bank does not read/write files or manage switching between banks.
   */
  class Bank{  
    // Navigation is designed for three windows and four strips per window.
    // BANK_MAX_STRIPS is 12 and limits entries read by deserialize(), not the
    // vector capacity: direct addStrip() calls do not enforce this limit.
    // Strip geometry uses four grid divisions per axis, not four total cells.
    #define BANK_MAXWINDOW 3
    #define STRIP_PER_WINDOW 4
    #define BANK_MAX_STRIPS BANK_MAXWINDOW*STRIP_PER_WINDOW
  
    public:
      // One-based strip id (Strip::index), not a zero-based vector position.
      // Navigation expects consecutive ids; init() selects id 1.
      uint8_t selector=1; 
      //uint8_t oldSelector=0;
   
      //uint8_t numberOfStrips = 0;
      //uint8_t maxWindow = 0;
  
      String name;                 // Supplied by the outer profile's bank key.
      unsigned char icon=' ';      // Single-byte icon code, saved as a number.
  
      // Saved on export, but loading currently always restores LAYOUT_DETAILS.
      uint8_t layout = LAYOUT_DETAILS;
      
      //LinkedList<Strip*> strips;
      std::vector<Strip*> strips; // Owns successfully added strips; lookups borrow them.
  
      // Build from a bank name and its JSON body. Construction does not expose
      // deserialize()'s result; callers must not assume every strip was restored.
      Bank(String n, String value/*, StripFactory* sf*/){ 
        name = n;
        //Strip* s = nullptr;
        //for (size_t i = 0; i < BANK_MAX_STRIPS; ++i) strips[i] = nullptr; //strips.add(s); // strips.push_back(nullptr);
        deserialize(value);
      }
  
      // Destroy contained strips. Any external pointers to them then become invalid.
      ~Bank() {
        for (auto& s : strips) {
          delete s;
          s = nullptr;
        }
        strips.clear();
      }
  
      // Let every strip process its data, including hidden strips. Each strip
      // decides what work it needs. Request a redraw if a visible strip's
      // interface changed; registered strips are expected to have an interface.
      bool update(){
        bool redraw = false;
        for (const auto& strip : strips) {         
          // we ask each strip of the bank to process itself
          // it's up to him to determine if he refreshes it's internal data
          // some may only do that if they are visible        
          if (strip != nullptr){
            strip->process();
            if (strip->visible && strip->interface->state.changed) redraw = true;
          }
        }
        return redraw;
      }

      // Reset focus to id 1; this does not check that a strip with that id exists.
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
  

      /**
       * Read one bank body: {"layout":...,"icon":...,"strips":[...]}.
       * The name comes from the constructor, not from this JSON body.
       *
       * Parse the JSON, restore the icon, then try the first 12 strip entries.
       * Each strip's type chooses its factory; its own deserialize() restores
       * the interface, position, size, color, icon and type-specific properties.
       * Required strip types/interfaces must already be registered.
       *
       * Current limits: saved layout is ignored, focus resets to 1, and this
       * appends to existing strips rather than replacing them. A parse error
       * returns false; otherwise true does not guarantee all strips loaded.
       * Invalid/unknown entries do not roll back strips already added. No bank
       * validation here checks overlapping geometry, unique ids or valid windows.
       */
      bool deserialize(String value){
        bool success = false;
        //DynamicJsonDocument bank(JSON_BANK_SIZE);
        JsonDocument bank;
        DeserializationError error = deserializeJson(bank, value);
        if (error){
          DEBUG_INFO("deserialize()::%s",error.c_str());
          return false;
        }else{
  
          layout = LAYOUT_DETAILS; //bank[C_CURRENTLAYOUT];
          icon = bank[C_ICON].as<unsigned char>();          
  
          JsonArray stripArray = bank[C_STRIPS];
          for (int i=0; i<stripArray.size() && i<BANK_MAX_STRIPS; i++){        
            JsonObject jsonStrip = stripArray[i];        
            if (!addStrip(jsonStrip)) continue; // process next strip
          }
          init();
          success = true;
        }
        return success;
      }
  
      /*bool insertStrip(JsonObject jsonStrip){
  
      }*/
  
      // Create the concrete strip from its saved "type". Unknown types return
      // nullptr; the factory only constructs it, addStrip() restores its settings.
      Strip* buildStrip(JsonObject jsonStrip){      
        String stripID = jsonStrip[C_STRIPTYPE];
        //DEBUG_PRINT("\nbuildStripID: %s",stripID.c_str());
        Strip* strip = stripFactory.build(stripID);  // build the strip by passing values to deserialize    
        return strip;
      }
  
      /**
       * Create and restore one strip, then append it only if restoration succeeds.
       * No capacity or position checks are made here. Returned pointers are
       * borrowed after insertion. Current failure handling has a limitation:
       * a constructed strip that fails restoration is returned but neither
       * inserted nor deleted, so a non-null result does not prove insertion.
       */
      Strip* addStrip(JsonObject jsonStrip){
        String jsonStripString;                           
        serializeJson(jsonStrip, jsonStripString);
        Strip* strip = buildStrip(jsonStrip);
        if (strip != nullptr){    
          if (strip->deserialize(jsonStripString)){
            strips.push_back(strip);
          }  
        }else{
          DEBUG_ERROR("\nbuildStrip(%s) failed",jsonStripString.c_str());
        }
        return strip;
      }
  
      /*void add(Strip* strip, uint8_t index){
        if (index<BANK_MAX_STRIPS){
  
          strips[index] = strip;
        }
      }*/
  
      // Find a strip by its exact origin and window, not by a point inside its
      // rectangle. Return the first match, or nullptr; ownership stays here.
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
  
      // Find the saved one-based strip id, not the vector offset. The early
      // size check assumes consecutive ids; gaps can make higher ids unreachable.
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
  
      // Delete a matching strip and remove its entry. Does not renumber remaining
      // ids or repair selector; callers must update selection/order when needed.
      bool trash(Strip* s) {
        for (auto it = strips.begin(); it != strips.end(); ++it) {
          if (*it == s) {
            delete *it;           // Destroy the owned strip.
            *it = nullptr;        // Clear the entry before removal.
            strips.erase(it);     // Remove the vector entry.
            return true;
          }
        }
        return false;
      }
  
      /**
       * Export the bank body with layout, numeric icon and the strips array.
       * Example with no strips: {"layout":2,"icon":3,"strips":[]}.
       * The name is the outer key added by ApplicationManager; selector and
       * runtime state are not saved. Each strip exports its own configuration,
       * including position {x,y,id,win}, type, interface and properties.
       *
       * Strip::serialize() returns JSON text. Entries are first added as strings;
       * removeJsonQuote() then removes wrappers/escapes to embed their objects.
       * This is legacy text replacement, not structural JSON composition:
       * arbitrary quotes/backslashes in values may not survive a round trip.
       * Reloading also resets layout/focus as described in deserialize().
       */
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

/**
 * Draws a small map of a bank beside its name; borrows the bank pointer.
 * Details mode shows strip geometry across windows; zoom mode shows an ordered
 * row of strips. This overview displays the arrangement, not live strip content.
 */
class BankNavigator: public Widget{
  //#define BANK_NAME_SIZE TFT_WIDTH/15
  public:
    IntervalBank *bank = nullptr;

    BankNavigator(IntervalBank* b/*, LinkedList<Widget*>* w*/, bool debug = false):bank(b), oldBank(b), Widget(&xPos, &yPos, &fC, &bC, &GUI::fullBrightness, debug){      
      //widgetList = w;      
      if (bank != nullptr) oldSelector = bank->selector - 1; //this is to force first refresh
      id = concatStrings("BankNavigator", xPos, "x", yPos);
    }

    // Rebuild the overview when selection, strip count or name changes.
    // Current change detection does not compare layout, geometry or colors,
    // and overwrites the force flag: those changes alone do not force a rebuild.
    // Keep the previous width when the name shrinks to cover its old screen area.
    virtual bool refresh(bool force = false) {
      refreshed = force;
      Widget::refresh(force);
          
      if (bank == nullptr){
        return refreshed;
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
      Canva* canva = new Canva(&GUI::zero, &GUI::zero, NAV_WIDTH, NAV_HEIGHT,0, &GUI::defaultBcolor, &GUI::defaultBcolor);
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

    // Details: scale x/y and the "width x height" size onto a four-by-four
    // miniature grid, then offset horizontally by the window number.
    // Zoom: divide the overview into equal strip slots using one-based ids.
    // Draw colors with strip/system brightness. Geometry is trusted here;
    // this drawing step does not validate the saved bank arrangement.
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
        canva->sprite->fillRect(cellX + XPADDING, cellY + YPADDING, cellWidth, cellHeight, GUI::luminance(colorMap[strip->colorID].fColor, strip->brightness * GUI::systemBrightness * 255));
        canva->sprite->drawRect(cellX, cellY, cellWidth + XPADDING, cellHeight + YPADDING,  GUI::luminance(colorMap[strip->colorID].bColor, strip->brightness * GUI::systemBrightness * 255));

      }      

    }

    // Optional selected-id overlay; the current refresh path does not call it.
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