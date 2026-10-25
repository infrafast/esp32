/**
 * @file s_mixer.h
 * @brief Build mixer controls by combining reusable strip UI with mixer access.
 *
 * There are two related but separate families: mixers talk to remote devices
 * (see mixer.h), while strips show and edit their values. A strip refers to a
 * configured mixer; it does not inherit from XMSeries or MixingStation.
 *
 * For the send control, read the inheritance from the finished control back:
 * Input2MixbusStrip adds send commands; SendStrip adds shared send/bus values;
 * StripMixerMixin<TwoSelectStrip> joins the two-selection UI with StripMixer.
 * TwoSelectStrip itself extends OneSelectStrip, which extends Strip. The
 * StripMixer branch only adds access to MixerBase, so there is one Strip base.
 * inputGainStrip instead uses StripMixerMixin<OneSelectStrip> for one channel.
 * Its device-write hook is still empty: it is not a complete backend example.
 */

#ifndef _S_MIXER_h
#define _S_MIXER_h

#include "mixers/mixer.h"
#include "mixers/values.h"

// STRIPS TO IMPLEMENT: SEE https://github.com/onyx-and-iris/xair-api-python

/**
 * @brief Finds the mixer selected in a strip's configuration.
 *
 * This small helper handles mixer access, without being another visual Strip.
 * baseStrip is a borrowed pointer to the UI part of the same control; mixer is
 * a borrowed pointer to the configured backend's MixerBase part. Neither is
 * owned here. getMixer() uses the binding registry instead of assuming that a
 * DeviceInterface* points to a particular concrete mixer.
 *
 * Resolve the binding after interface assignment, before using mixer hooks.
 * Re-resolve after replacing an interface. The focus/visibility helpers assume
 * boundInterface is valid; they do not themselves call getMixer().
 */
class StripMixer{
  public:
    
    /**
     * @brief Initialize StripMixer and its configured member bindings.
     */
    StripMixer(Strip* strip):baseStrip(strip){}

  protected:
    Strip* baseStrip = nullptr;   ///< need a reference to the base class to access mixer methods & properties
    MixerBase* mixer = nullptr;

    DeviceInterface* boundInterface = nullptr;

    /**
     * @brief Resolve the current strip interface through the shared mixer binding registry.
     *
     * Cache the association only while the interface pointer is unchanged. This
     * supports assignment after construction without casting to a concrete backend.
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

    /** @brief Request mixer configuration when the bound interface is ready. */
    void gotFocus(){
      if (boundInterface->state.code == IF_READY) getConfig();  // we get config when getting the focus to be sure to have the latest data from the interface, in case some change were made while not having the focus, this is important for instance for the color of the strip which may change based on the state of the interface (like for instance for a channel that is red when it is clipping) and we want to be sure to reflect this in the GUI as soon as possible when getting the focus, even if it is not visible yet (for instance in details layout where a strip can get the focus without being visible)
    }
    
    /** @brief Request mixer values when the bound interface is ready. */
    void becameVisible(){
      if (boundInterface->state.code == IF_READY) getValues();  // we get values when getting visible to be sure to have the latest data from the interface, in case some change were made while not visible, this is important for instance for the color of the strip which may change based on the state of the interface (like for instance for a channel that is red when it is clipping) and we want to be sure to reflect this in the GUI as soon as possible when the strip is visible, even if it does not have the focus yet (for instance in details layout where a strip can be visible without having the focus) 
    }    

  protected:
    
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void getConfig(){}
    /** @brief Placeholder hook; no active operation in this implementation. */
    virtual void getValues(){}

};


/**
 * @brief Adds mixer focus, visibility and refresh handling to a strip base.
 *
 * @tparam BaseStripT A Strip-derived UI class providing the overridden hooks,
 * such as OneSelectStrip or TwoSelectStrip.
 *
 * A "mixin" adds a reusable piece of behavior to another class. Here the template
 * chooses which strip UI to extend: one dropdown for a channel, or two dropdowns
 * for a channel and a bus. Both get the same mixer lookup and refresh behavior,
 * without copying it into two separate base classes. This is ordinary multiple
 * inheritance; the template argument is a UI base, not the final derived class.
 *
 * For a send control, inherit StripMixerMixin<TwoSelectStrip> (as SendStrip does),
 * pass the skeleton and UI arguments to its constructor, then implement
 * getConfig(), getValues() and the relevant button callbacks in the derived
 * control. Call mixer methods through MixerBase, not a concrete backend cast.
 * Register the strip skeleton and selection addresses as in MixerApplication.h.
 *
 * For example, select channel 5 and bus 2: focus requests their names and mute
 * states via getConfig(); becoming visible requests the send level via
 * getValues(). A button callback writes through mixer->setCHBUSlevel(5, 2, level).
 * Virtual hooks choose Input2MixbusStrip's implementation even though this
 * shared class handles focus and visibility.
 *
 * The template only wires these hooks together. It does not choose a mixer,
 * bind shared values, or supply missing device commands. init() attempts mixer
 * lookup but currently returns true even if no binding exists; derived controls
 * must respect the existing initialization and pointer-lifetime constraints.
 */
template <typename BaseStripT>
class StripMixerMixin : public BaseStripT, public StripMixer {
public:
  /**
   * @brief Construct the chosen UI base and let the helper refer to its Strip part.
   *
   * Args... forwards optional UI arguments, such as debug, to BaseStripT.
   * static_cast<Strip*>(this) follows the known UI inheritance path; it does
   * not cast a configured interface to a guessed mixer backend.
   */
  template <typename... Args>
  StripMixerMixin(StripSkeleton* s, Args&&... args)
    : BaseStripT(s, std::forward<Args>(args)...),
      StripMixer(static_cast<Strip*>(this)) {}

  /** @brief Request configuration on selection. */
  virtual void hasFocus() override {
    StripMixer::gotFocus();
  }

  /** @brief Request current values when the strip becomes visible. */
  virtual void hasVisibility() override {
    StripMixer::becameVisible();
  }

  /**
   * @brief called when deserializing a Strip
   */
  virtual bool init() override {
 
    MixerBase* activeMixer = getMixer();   
    return true;
  }  

  /* event processing coming from the strip, like user interaction, visibility change, focus change, etc... 
   we can also receive events from the mixer here if we want to update the strip state based on some mixer event 
   (for instance if we want to update the color of the strip based on the channel level or something like this), 
   this is a powerful method that allow us to decouple the mixer implementation from the strip management and have a clean separation 
   of concerns between the two, we can for instance have a generic implementation of a fader strip that can work with any mixer implementation
    as long as it follows the same interface, and we can also have specific implementations for specific mixers 
   if we want to take advantage of some specific features of these mixers
  */
  /**
   * @brief Refresh visible mixer strips when the mixer requests periodic polling.
   *
   * Ignore hidden/unbound strips. When ready, retrieve live values for visible
   * strips and configuration as well for the focused strip.
   */
  virtual void processAction(int action,JsonDocument& data) override{  

    if (baseStrip == nullptr || !baseStrip->visible || boundInterface == nullptr) return;

    // if strip is not visible, we can skip event processing as the state will be refreshed when getting the focus
    if (!baseStrip->visible) return;

    switch (action){
      // called to force periodic refresh by the mixer
      case STRIP_REFRESH_VISIBLE:                 
        if (boundInterface->state.code != IF_READY) return;      
        getValues(); // strip is visible so we get its values
        if (baseStrip->focused) getConfig(); // plus get the toplabel name if it has focus 
      break;
    }
  }
  
};



/**
 * @brief Add shared send and bus values to the two-selection mixer UI.
 *
 * select1ID is the one-based channel and select2ID the one-based bus. With
 * channel 5 and bus 2, reserve(5, 2) gives the send level/mute and reserve(2)
 * gives the bus mute. Another strip selecting the same send gets the same
 * object, so both displays read the backend's latest value without copying it.
 * Input2MixbusStrip supplies the requests and button commands on top of this.
 * @note This class is not a complete ownership example: reservation pointers
 * are not initialized here and its destructor does not release reservations.
 * See the existing lifecycle issues in docs/roadmap.md before reusing it.
 */
class SendStrip : public StripMixerMixin<TwoSelectStrip> {
  public:

    TwoIndexValue* sendReturnValues;
    OneIndexValue* busValues;

    /**
     * @brief Initialize SendStrip and its configured member bindings.
     */
    SendStrip(StripSkeleton* s, bool debug = false) : StripMixerMixin<TwoSelectStrip>(s, debug){      
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~SendStrip(){      
    };
 
    /**
     * @brief Restore channel/bus selections and rebind shared send and bus values.
     *
     * Release the previous reservations before reserving the selected key pair and
     * bus. Bind the strip value and toggles to these shared objects so other strips
     * using the same keys see the same state.
     * @note Current code assumes valid mixer/reservation pointers and does not make
     * all failure and destruction paths safe; callers must respect existing limits.
     */
    virtual void deserializeProperty(String property) override{
      TwoSelectStrip::deserializeProperty(property);
      
      mixer->allInput2MixbusValues.unreserve(sendReturnValues);    // release any previous value this prevents memory consumption when user change the dropdown
      mixer->allBusMixValues.unreserve(busValues);                 // release any previous value this prevents memory consumption when user change the dropdown
      
      sendReturnValues = mixer->allInput2MixbusValues.reserve(select1ID,select2ID);      // reserve the new value
      busValues = mixer->allBusMixValues.reserve(select2ID);

      value = &sendReturnValues->value;           // value of the fader for a ch on the bus
      toggle1 = &sendReturnValues->toggle;        // the top toggle is linked to the ch mute on the bus
      toggle2 = &busValues->toggle;               // the bottom toggle is linked to the bus mute

    }

    /** @brief Add this strip's widgets for the selected bank layout. */
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
/**
 * @brief Control how much of one input channel is sent to one mix bus.
 *
 * This is the concrete send control: the parents provide UI, mixer lookup and
 * shared storage; this class decides which names, levels and mutes to request
 * and which commands a button sends. The same control works with either mixer
 * backend because all calls go through MixerBase. Incoming backend messages
 * update the reserved shared values that the widgets read.
 */
class Input2MixbusStrip : public SendStrip{
  public:

    /** @brief Initialize Input2MixbusStrip with the supplied configuration and bindings. */
    Input2MixbusStrip(StripSkeleton* s,bool debug=false):SendStrip(s,debug){      
      unitString = "dB";
    }

    /** @brief Request the configuration needed by the selected mixer strip. */
    virtual void getConfig() override{
      DEBUG_INFO("getConfig %s",type.c_str());      

      // 1 . request name refresh
      mixer->requestCHname(select1ID);  // select1ID = chID
      mixer->requestBUSname(select2ID);  // select2ID = busID

      // 2. request mute state refresh
      mixer->requestCHBUSmute(select1ID, select2ID);  // select1ID = chID, select2ID = busID
      mixer->requestBUSmute(select2ID);  // select2ID = busID      
      
    }

    /** @brief Request the live values needed by the visible mixer strip. */
    virtual void getValues() override{
      mixer->requestCHBUSlevel(select1ID, select2ID);  // select1ID = chID, select2ID = busID
    }



    /**
     * @brief Compute a decreased send level and write it through the mixer contract.
     *
     * Use the same click/hold curve as increasing, with the opposite target limit.
     */
    virtual float decreaseCallback(unsigned long time,uint8_t trigger) override{
      float newVal = *value;
      float delta = applyProgressive(&newVal, -1, trigger);
      if (delta != 0.0) mixer->setCHBUSlevel(select1ID, select2ID, newVal);  // select1ID = chID, select2ID = busID
      return delta;
    }      

    /**
     * @brief Compute an increased send level and write it through the mixer contract.
     *
     * Work on a temporary value; issue a device write only when progressive button
     * handling produces a nonzero change. Shared device state is updated by the backend.
     */
    virtual float increaseCallback(unsigned long time, uint8_t trigger) override{
      float newVal = *value;
      float delta = applyProgressive(&newVal, +1, trigger);
      if (delta != 0.0) mixer->setCHBUSlevel(select1ID, select2ID, newVal);  // select1ID = chID, select2ID = busID
      return delta;            
    }

    /**
     * @brief Toggle the selected input-to-bus mute through the mixer contract.
     */
    virtual bool actionCallback(uint8_t trigger) override{
      // trigger = HOLD, SIMPLE_CLIC, DOUBLE_CLIC, LONG_CLIC
      //mute/unmute
      mixer->setCHBUSmute(select1ID, select2ID, !*toggle1);  // select1ID = chID, select2ID = busID, if muting we set level to 0, if unmuting we set it to the current value (which may be different from 0 if the level was changed while muted)
      return true;        
    }

};

StripSkeleton* stripInput2mixbusSkel;
/** @brief Create the concrete strip associated with this registered type. */
Strip* sendStripConstructorCallback(){
  //DEBUG_PRINT("\n\tinput2mixbusStripConstructorCallback()");
  return new Input2MixbusStrip(stripInput2mixbusSkel);
}

#define INPUGAINDB_MIN  -12
#define INPUGAINDB_MAX  60
/**
 * @brief Input-gain strip skeleton; device write behavior remains incomplete.
 */
class inputGainStrip : public StripMixerMixin<OneSelectStrip> {
  #define STRIP_INPUTGAIN "inputGain"

  public:
    /** @brief Initialize inputGainStrip with the supplied configuration and bindings. */
    inputGainStrip(StripSkeleton* s, bool debug = false)
        : StripMixerMixin<OneSelectStrip>(s, debug) {
        valueMin = INPUGAINDB_MIN;
        valueMax = INPUGAINDB_MAX;
        *value = (valueMin + valueMax) / 2;
        unitString = "dB";
      }

    /** @brief Placeholder hook; no active operation in this implementation. */
    ~inputGainStrip(){};

    /** @brief Add this strip's widgets for the selected bank layout. */
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

    /** @brief Increase the selected value in response to the supplied button gesture. */
    virtual float increaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::increaseCallback(time,trigger);
      if (delta != 0.0) setInputGain();
      return delta;
    }

    /** @brief Decrease the selected value in response to the supplied button gesture. */
    virtual float decreaseCallback(unsigned long time,uint8_t trigger) override{
      float delta = Strip::decreaseCallback(time,trigger);
      if (delta != 0.0) setInputGain();
      return delta;
    }

    /** @brief Placeholder hook; no active operation in this implementation. */
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

    /**
     * @brief Placeholder hook; no active operation in this implementation.
     *
     * @return Always true; no device action is performed here.
     */
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
/** @brief Create the concrete strip associated with this registered type. */
Strip* gain2chStripConstructorCallback(){
  //DEBUG_PRINT("\n\tgain2chStripConstructorCallback()");
  return new inputGainStrip(stripInputGainSkel);
}



#endif
