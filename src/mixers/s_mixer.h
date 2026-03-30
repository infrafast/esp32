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

    void gotFocus(){
      if (boundInterface->state.code == IF_READY) getConfig();  // we get config when getting the focus to be sure to have the latest data from the interface, in case some change were made while not having the focus, this is important for instance for the color of the strip which may change based on the state of the interface (like for instance for a channel that is red when it is clipping) and we want to be sure to reflect this in the GUI as soon as possible when getting the focus, even if it is not visible yet (for instance in details layout where a strip can get the focus without being visible)
    }
    
    void becameVisible(){
      if (boundInterface->state.code == IF_READY) getValues();  // we get values when getting visible to be sure to have the latest data from the interface, in case some change were made while not visible, this is important for instance for the color of the strip which may change based on the state of the interface (like for instance for a channel that is red when it is clipping) and we want to be sure to reflect this in the GUI as soon as possible when the strip is visible, even if it does not have the focus yet (for instance in details layout where a strip can be visible without having the focus) 
    }    

  protected:
    virtual void getConfig() = 0;
    virtual void getValues() = 0;

};


template <typename BaseStripT>
class StripMixerMixin : public BaseStripT, public StripMixer {
public:
  template <typename... Args>
  StripMixerMixin(StripSkeleton* s, Args&&... args)
    : BaseStripT(s, std::forward<Args>(args)...),
      StripMixer(static_cast<Strip*>(this)) {}

  virtual void hasFocus() override {
    StripMixer::gotFocus();
  }

  virtual void hasVisibility() override {
    StripMixer::becameVisible();
  }

  // called when deserializing a Strip
  virtual bool init() override {
 
    MixerBase* activeMixer = getMixer();   
    return true;
  }  
  
};



class SendStrip : public StripMixerMixin<TwoSelectStrip> {
  public:

    SendStrip(StripSkeleton* s, bool debug = false) : StripMixerMixin<TwoSelectStrip>(s, debug){}

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

    virtual void deserializeProperty(String property) override{    
      
      TwoSelectStrip::deserializeProperty(property);
      
      toggle1 = &mixer->inputNamesDS->mutes[select1ID-1];  // select1ID = chID
      toggle2 = &mixer->mixbusNamesDS->mutes[select2ID-1];  // select2ID = busID

      value = &mixer->inputNamesDS->values[select1ID-1];  // select1ID = chID
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

    virtual void onEvent(const Event& evt) override{

      // if strip is not visible, we can skip event processing as the state will be refreshed when getting the focus
      if (!visible) return;

      // handle events from the mixer (e.g. to update strip state on external changes not made through the interface)
      JsonDocument evtData = evt.data != nullptr ? *((JsonDocument*)evt.data) : JsonDocument();
      int action = evtData[0];
    
      switch (action){
        /*case STRIP_COMMAND:
          // chek if the event is for us based on select1ID and select2ID which are representing the chID and busID respectively
          if (evtData[1] == select1ID && evtData[2] == select2ID){  // evtData[1] = chID, evtData[2] = busID
            toggle1 = evtData[3];  // mute state
            DEBUG_INFO("%s onEvent() STRIP_CH_BUS_MUTE",type.c_str());
          }
          break;
          */
      }

    }

    void getConfig(){
      DEBUG_INFO("getConfig %s",type.c_str());      

      // 1 . request name refresh
      mixer->getDSname(mixer->inputNamesDS, select1ID-1);  // select1ID = chID
      mixer->getDSname(mixer->mixbusNamesDS, select2ID-1);  // select2ID = busID 

      // 2. request mute state refresh
      mixer->getCHBUSmute(select1ID, select2ID);  // select1ID = chID, select2ID = busID
      mixer->getBUSmute(select2ID);  // select2ID = busID      
    }

    void getValues(){
      mixer->getCHBUSlevel(select1ID, select2ID);  // select1ID = chID, select2ID = busID
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

    virtual bool actionCallback(uint8_t trigger) override{
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      toggleCHBUSmute();
      return true;        
    }

    void setChBusLevel(){
      mixer->setCHBUSlevel(select1ID, select2ID, *value);  // select1ID = chID, select2ID = busID
    }    

    void toggleCHBUSmute(){
      *toggle1 = !(*toggle1);
      mixer->setCHBUSmute(select1ID, select2ID, *toggle1 ? 1 : 0);  // select1ID = chID, select2ID = busID, if muting we set level to 0, if unmuting we set it to the current value (which may be different from 0 if the level was changed while muted)
    }
};

StripSkeleton* stripInput2mixbusSkel;
Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tinput2mixbusStripConstructorCallback()");
  return new Input2MixbusStrip(stripInput2mixbusSkel);
}

#define INPUGAINDB_MIN  -12
#define INPUGAINDB_MAX  60
class inputGainStrip : public StripMixerMixin<OneSelectStrip> {
  #define STRIP_INPUTGAIN "inputGain"

  public:
    inputGainStrip(StripSkeleton* s, bool debug = false)
        : StripMixerMixin<OneSelectStrip>(s, debug) {
        valueMin = INPUGAINDB_MIN;
        valueMax = INPUGAINDB_MAX;
        *value = (valueMin + valueMax) / 2;
        unitString = "dB";
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

    virtual void getConfig() override{
      DEBUG_INFO("%s:getConfig %s",type.c_str());
    }

    virtual void getValues() override{
      DEBUG_INFO("%s:getValues %s",type.c_str());
      // request value refresh (in case of a change on the mixer that is not reflected in the interface yet)
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
