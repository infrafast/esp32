#ifndef _S_MIXER_h
#define _S_MIXER_h

#include "mixers/mixer.h"


class StripMixer{
  public:
    
    StripMixer(Strip* strip):baseStrip(strip){
      mixer = (MixerBase*)strip->interface;
    }



  protected:
    Strip* baseStrip = nullptr;   // need a reference to the base class to access mixer methods & properties

    MixerBase* mixer = nullptr;
};

/*
    //int id1=0, id2=0; // these are the values extracted from the address
    // example
    // /fxrtn/03/mix/06/on
    // v1 = 03
    // v2 = 06

   /* bool match2AddressIDtoDropDownID(const char* address,const char* pattern){
      // this method is called when we have to decompose the ids embedded into an address according to a provided pattern
      // upon reception of an OSC frame
      if (sscanf(address, pattern,&id1,&id2) != 2) return false;
      if (mixerServer->mixerInterface->idStart == 0){ id1 += 1; id2 += 1;}    // we have to make ID consistent, they should all start at 1
      return true;
    }

    bool match1AddressIDtoDropDownID(const char* address,const char* pattern){
      if (sscanf(address, pattern,&id1) != 1) return false;
      if (mixerServer->mixerInterface->idStart == 0){ id1 += 1;}    // we have to make ID consistent, they should all start at 1
      return true;
    }

    uint8_t dropDownIDtoInterfaceID(uint8_t id){
      // this return the value starting either at 0 or one depending on the mixerInterface (mixing station starts at 0 so drop downvalue would need to be decreased by 1)
      return id-(1-mixerServer->mixerInterface->idStart);
    }


*/



class SendStrip : public TwoSelectStrip, public StripMixer{
  public:

    SendStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug),StripMixer(this){}

    ~SendStrip(){};
  
    virtual void composeWidgets(uint8_t layout) override{      
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        push4x4hBar();
      }
      else if (size == SHAPE1X4){
        push1x4vBar();
      }
      else if (size == SHAPE2X2){
        push2x2hBar();
      }
      else if (size == SHAPE4X1){
        push4x1hBar();
      }
    }
    
  protected:
    String unitString="U";

};

class Ch2BusStrip : public SendStrip{
  #define STRIP_CH2BUS "ch2bus"

  public:

    Ch2BusStrip(StripSkeleton* s,bool debug=false):SendStrip(s,debug){      
      unitString = "dB";

      /*select1PropertyAddress = ADDRESS_CH_NAME; 
      select2PropertyAddress = ADDRESS_BUS_NAME; */

      //valueMin = 0; valueMax = 1;
    }

    /*
    virtual const String* getTopLabel() override{
      return &select1Content;
      //return mixer->chNamesDS->getValuePtr(select1ID-1);
      //return &type;
    }

    virtual const String* getBottomLabel() override {
      return &select2Content;
      //return mixer->busNamesDS->getValuePtr(select2ID-1);
      //return select1ContentPtr;
    } */ 

    virtual bool actionCallback(uint8_t trigger) override{
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      DEBUG_PRINT("\n%s actionCallback() ",type.c_str());
      return true;        
    }

    virtual void onEvent(const Event& evt) override{}

    virtual void queryInterface() override{
      DEBUG_PRINT("\nqueryState %s",type.c_str());      
    }

    virtual float increaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::increaseCallback(time,trigger);
      if (delta != 0.0) setChBusLevel();
      return delta;
    }    

    virtual float decreaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setChBusLevel();
      return delta;
    }      

    void setChBusLevel(){
      /*char formattedCommandString[50];
      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHBUSVALUE];
      uint8_t chID = dropDownIDtoInterfaceID(select1ID);
      uint8_t busID = dropDownIDtoInterfaceID(select2ID);  
      sprintf(formattedCommandString,addressTemplate.c_str(),chID,busID);

      JsonDocument command;
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      osc[EXEC_ADDRESS] = formattedCommandString ;
      osc[EXEC_VALUE] = value;

      mixerServer->mixerInterface->execute(command.as<JsonObject>());    */
    }    
};

StripSkeleton* stripCh2BusSkel;
Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tch2busStripConstructorCallback()");
  return new Ch2BusStrip(stripCh2BusSkel);
}

#define CHGAINDB_MIN  -12
#define CHGAINDB_MAX  60
class ChGainStrip : public OneSelectStrip, public StripMixer{
  #define STRIP_CHGAIN "chGain"

  public:
    ChGainStrip(StripSkeleton* s,bool debug=false):OneSelectStrip(s,debug),StripMixer(this){
      //select1PropertyAddress = ADDRESS_CH_NAME; 
      
      valueMin = CHGAINDB_MIN;
      valueMax = CHGAINDB_MAX;
      value = (valueMin + valueMax)/2;
      unitString = "dB";
      //icon = ICON_CHGAIN_STRIP;
    }

    ~ChGainStrip(){};

    virtual void composeWidgets(uint8_t layout) override{      
      if (size == SHAPE4X4 || layout == LAYOUT_ZOOM){
        push4x4hBar();
      }
      else if (size == SHAPE1X4){
        push1x4vBar();
      }
      else if (size == SHAPE2X2){
        push2x2RingMeter();
      }
      else if (size == SHAPE4X1){
        push4x1hBar();
      }     

    }

    virtual void onEvent(const Event& evt) override{} 

    virtual void queryInterface() override{
      DEBUG_PRINT("\nqueryInterface %s",type.c_str());
    }

    virtual float increaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::increaseCallback(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setChGain();
      return delta;
    }

    void setChGain(){
      /*char formattedCommandString[50];
      String addressTemplate = mixerServer->mixerInterface->addresses[ADDR_CHGAIN];
      uint8_t chID = dropDownIDtoInterfaceID(select1ID);
      sprintf(formattedCommandString,addressTemplate.c_str(),chID);

      JsonDocument command;
      JsonObject osc = command[EXEC_OSC].to<JsonObject>();

      osc[EXEC_ADDRESS] = formattedCommandString ;
      osc[EXEC_VALUE] = value;

      mixerServer->mixerInterface->execute(command.as<JsonObject>());  */
    }

    virtual bool actionCallback(uint8_t trigger) override{
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      /*char addr[100];

      //select1ID = chID

      //sprintf(addr,mixerServer->chNames.address.c_str(),chID);
      String val = concatStrings("name", random(1000));
      DEBUG_PRINT("\n %s %s",addr,val.c_str());
    
      //chNames.values.set(chID-1, val);
      //mixerServer->chNames.setValue(chID-1,val);
      //app.broadCast(concatStrings("{\"updateDataSets\":[{\"", String(addr), "\":\"", val, "\"}]}"));

      refresh();*/
      return true;
    }       

};

StripSkeleton* stripChGainSkel;
Strip* gain2chStripConstructorCallback(){
  //DEBUG_PRINT("\n\tgain2chStripConstructorCallback()");
  return new ChGainStrip(stripChGainSkel);
}



#endif