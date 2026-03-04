#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

#define ADDRESS_DISCOVERY "/connect/%s"
#define INTERFACE_BEHRINGERXM "BehringerXM"

class DiscoveryDataSet : public DataSet{
  
  public:
    uint16_t count = 0;
    String name[APPLEMIDI_SESSIONS_NUMBER+2]; //+2 because of the saved session than comes in addition and the "passive" entry

    DiscoveryDataSet(Address* _address): DataSet(_address, INTERFACE_BEHRINGERXM, &IFdataRepo){}
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &name[index];    
    }  

    virtual uint16_t getSize(){
      return count;
    }
     
};

class BehringerXM: public NetworkOSC, public MixerInterface{
  public:

    DiscoveryDataSet discoveryDs = DiscoveryDataSet(new DropDownAddress(ADDRESS_DISCOVERY));
    
    BehringerXM():NetworkOSC(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023, "/xinfo",true,GROUP_INTERFACE_MIXER){
      //destination->host = "192.168.0.1";
    }     
};



#endif