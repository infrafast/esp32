#ifndef OSCSTRIPS_h
#define OSCSTRIPS_h

#include "Strips.h"
#include "Interval.h"
#include "interfaces/i_osc.h"


#define ADDRESS_OSCMSG "/osc/%s"
#define OSCLIB_FILENAME "osclib.json"

class OSCLib : public  WebLibrary{
  public:
    OSCLib(Address* _address): WebLibrary(OSCLIB_FILENAME, _address, GROUP_INTERFACE_OSC){}
};

OSCLib* dsOSCMessages;



class OSCStrip : public Strip{  // should inherit from MidiStrip before so methode like data read castng are common
  #define STRIP_OSC "SendOSC"
  #define OSC_MAX_KEY_LENGTH 64

  public:
    OSCStrip(StripSkeleton* s,bool debug=false):Strip(s){
    }

    ~OSCStrip(){};

    virtual void composeWidgets(uint8_t layout) override{      

      uint16_t h = 0, w = 0;

      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        h = WIDGET4X4H/3;   // height of the bar is 3 time less big than the full height of the cell
        w = WIDGET4X4W;
        topY += h/2; 
        gui.add(new ProgressBarH(&value /*v*/, valueMin /*min*/,valueMax /*max*/, &topX /*x*/, &topY /*y*/, w /*w*/, h /*h*/, 2 /*offset*/, 5 /*radius*/, &frontColor /*fc*/ , &backColor/*bc*/, &brightness));
      }else if (size == SHAPE1X4){
        pushSimpleText(layout, lang.getString("SHAPE1X4 OSC"));
      }else if (size ==SHAPE2X2 ){
         pushSimpleText(layout, lang.getString("SHAPE2X2 OSC"));
      }else if (size == SHAPE4X1){
         pushSimpleText(layout, lang.getString("SHAPE4X1 OSC"));
      }else{

      }         
 
    }

    virtual void update(){
        ////update is called if data are availabe and must therefore cast the data to the right data structure
        OSCMessage* osc = static_cast<OSCMessage*>(interface->data); // WARNING CAREFULLY TEST IF POLYMORPHISM WORKS

        /*
        int numberOfValues = msg.size();

        for (int i = 0; i < numberOfValues; i++) {
            char type = msg.getType(i);

            switch (type) {
            case 's': {
                char str[64];
                msg.getString(i, str, sizeof(str));
                break;
            }
            case 'i':
                data[EXEC_VALUE][i] = msg.getInt(i);
                break;
            case 'f':
                data[EXEC_VALUE][i] = msg.getFloat(i);
                break;
            case 'T':
                data[EXEC_VALUE][i] = true;
                break;
            case 'F':
                data[EXEC_VALUE][i] = false;
                break;
            case 'b': { // blob (tableau d’octets)
                int blobSize = msg.getBlobLength(i);
                const uint8_t* blobData = msg.getBlob(i);
                // Créer un tableau JSON pour le blob
                JsonArray blobArray = data[EXEC_VALUE][i].createNestedArray();
                for (int j = 0; j < blobSize; j++) {
                    blobArray.add(blobData[j]);
                }
                break;
            }                    
            default:
                data[EXEC_VALUE][i] = "unsupported";
                break;
            }
        }

        msg.getAddress(buffer);*/


    }  

    virtual void queryInterface(){ // this code is called by focus() to send a command that will be executed, 
    //then its outcome will be read at the next process()
    } 

    virtual bool actionCallback(uint8_t trigger) {
        if (!interface) return false;
        
        OSCMessage msg(oscMessageString.c_str());
        
        return  ((NetworkOSC*)interface)->sendOSCMessage(&msg);
    }

    virtual void serializeProperties(){
      char* buf = (char*)malloc((strlen(ADDRESS_OSCMSG)+oscKey.length())*sizeof(char*)+1);
      sprintf(buf,ADDRESS_OSCMSG,oscKey);
      properties.add(String(buf)); //force la copie
      //DEBUG_PRINT("\nserializeProperties: %s",buf);
      delete buf;
    }

    virtual void deserializeProperty(String property){
      char OSCKeyBuf[OSC_MAX_KEY_LENGTH];
      if (sscanf(property.c_str(), ADDRESS_OSCMSG, &OSCKeyBuf)){
        oscKey = OSCKeyBuf;
        //DEBUG_PRINT("\ndeserializeProperty: %s = %s",property.c_str(),oscKey.c_str());
        oscMessageString = *dsSysexMessages->getString(oscKey,"undef sysex");         // return the value corresponding to the key from the jsonfile library        
      }    
    }

    virtual const String* getTopLabel(){return &type;}
    virtual const String* getBottomLabel(){return &oscKey;}      
       
  protected:
    String oscMessageString="";
    String oscKey ="";

};

StripSkeleton *oscSkel;

Strip* oscStripConstructorCallback(){
  return new OSCStrip(oscSkel);
}



#endif