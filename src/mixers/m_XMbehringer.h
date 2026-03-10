#ifndef M_XMBEHRINGER_h
#define M_XMBEHRINGER_h

#include "mixer.h"

#define ADDRESS_CONNECT_METHOD "/connect/%d"
#define INTERFACE_BEHRINGERXM "BehringerXM"

enum{
    CONNECT_MANUAL=1,                   // select always start at value 1
    CONNECT_DISCOVER,        
};

class ConnectMethodDS : public DataSet{
  
  public:

    #define ADDRESS_DISCOVERY_SIZE 2

    String options[ADDRESS_DISCOVERY_SIZE]={
        "manual",
        "discovery"
    };     

    ConnectMethodDS(Address* _address): DataSet(_address, INTERFACE_BEHRINGERXM, &IFdataRepo){}
    
    virtual const String* getValuePtr(uint16_t index){
      if (index>getSize()-1) return &errorDatasetOOR;
      return &options[index];    
    }  

    virtual uint16_t getSize(){
      return ADDRESS_DISCOVERY_SIZE;
    }
     
};

class BehringerXM: public NetworkOSC, public MixerInterface{
  public:

    int connectMethod = CONNECT_DISCOVER;

    ConnectMethodDS connectMethodDS = ConnectMethodDS(new DropDownAddress(ADDRESS_CONNECT_METHOD));
    
    BehringerXM():NetworkOSC(INTERFACE_BEHRINGERXM,8888,"192.168.0.1",10023,"/xinfo",true,GROUP_INTERFACE_MIXER){
        registerDataSetAddress(connectMethodDS.address);
    }     

    virtual void serializeProperties(JsonArray& properties) override{
      NetworkOSC::serializeProperties(properties);

      char* buf = (char*)malloc(strlen(ADDRESS_CONNECT_METHOD)*sizeof(char*));
      sprintf(buf,ADDRESS_CONNECT_METHOD,connectMethod);
      properties.add(String(buf));
      free(buf); 

    }

    virtual bool restoreProperty(const char* property) override{     
      bool changed = false;

      if (sscanf(property, ADDRESS_CONNECT_METHOD, &connectMethod)){
        changed = true;
      }

      return changed | NetworkOSC::restoreProperty(property);
    }   

    

};



#endif