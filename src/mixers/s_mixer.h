#ifndef _S_MIXER_h
#define _S_MIXER_h

#include "mixers/mixer.h"


class StripMixer{
  public:
    
    StripMixer(Strip* strip):baseStrip(strip){}

  protected:
    Strip* baseStrip = nullptr;   // need a reference to the base class to access mixer methods & properties
    MixerBase* mixer = nullptr;

    DeviceInterface* boundInterface = nullptr;

    /*
      preserve the MixerBase abstraction and template-based MixerDevice<...> implementations, and safely resolve the concrete mixer instance used by a strip.
    */
    MixerBase* getMixer(){
      if (baseStrip == nullptr || baseStrip->interface == nullptr){
        mixer = nullptr;
        boundInterface = nullptr;
        return nullptr;
      }

      // interface can be assigned after strip construction or changed at runtime
      if (boundInterface == baseStrip->interface && mixer != nullptr) return mixer;

      mixer = MixerBase::fromDeviceInterface(baseStrip->interface);
      boundInterface = baseStrip->interface;
      if (mixer == nullptr){
        DEBUG_ERROR("StripMixer::getMixer() no mixer binding for interface: %s",baseStrip->interface->interfaceName.c_str());
        return nullptr;
      }
      return mixer;
    }
};


class SendStrip : public TwoSelectStrip, public StripMixer{
  public:

    SendStrip(StripSkeleton* s,bool debug=false):TwoSelectStrip(s,debug),StripMixer(this){}

    ~SendStrip(){};

    /* MANDATORY !!! */
    virtual bool init(){
      MixerBase* activeMixer = getMixer();
      return true;
    }
  
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

#define STRIP_INPUT2MIXBUS "input2mixbus"
class Input2MixbusStrip : public SendStrip{
  public:

    Input2MixbusStrip(StripSkeleton* s,bool debug=false):SendStrip(s,debug){      
      unitString = "dB";
    }

    virtual bool actionCallback(uint8_t trigger) override{
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      DEBUG_PRINT("\n%s actionCallback() ",type.c_str());
      return true;        
    }

    virtual void onEvent(const Event& evt) override{}

    virtual void queryInterface() override{
      DEBUG_INFO("queryState %s",type.c_str());      

      mixer->testCall();

      // 1 . request name refresh
      //mixer->dropDSnameRequest(mixer->chNamesDS, select1ID-1);  // select1ID = chID
      //mixer->dropDSnameRequest(mixer->busNamesDS, select2ID-1);  // select2ID = busID 

      // 2. request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
      // 2.a mute
      //mixer->dropCHMuteStateRequest(select1ID-1);  // select1ID = chID
      //mixer->dropBUSMuteStateRequest(select2ID-1);  // select2ID = busID

      // 2.b value

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

StripSkeleton* stripInput2mixbusSkel;
Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tinput2mixbusStripConstructorCallback()");
  return new Input2MixbusStrip(stripInput2mixbusSkel);
}

#define INPUGAINDB_MIN  -12
#define INPUGAINDB_MAX  60
class inputGainStrip : public OneSelectStrip, public StripMixer{
  #define STRIP_INPUTGAIN "inputGain"

  public:
    inputGainStrip(StripSkeleton* s,bool debug=false):OneSelectStrip(s,debug),StripMixer(this){
      //select1PropertyAddress = ADDRESS_CH_NAME; 
      
      valueMin = INPUGAINDB_MIN;
      valueMax = INPUGAINDB_MAX;
      value = (valueMin + valueMax)/2;
      unitString = "dB";
      //icon = ICON_CHGAIN_STRIP;
    }

    ~inputGainStrip(){};

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
      if (delta != 0.0) setInputGain();
      return delta;
    }

    virtual float decreaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setInputGain();
      return delta;
    }

    void setInputGain(){
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

StripSkeleton* stripInputGainSkel;
Strip* gain2chStripConstructorCallback(){
  //DEBUG_PRINT("\n\tgain2chStripConstructorCallback()");
  return new inputGainStrip(stripInputGainSkel);
}



#endif
