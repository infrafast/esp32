#ifndef I_OSC_h
#define I_OSC_h

#include "Interfaces.h"
#include "DataManagement.h"

#define GROUP_INTERFACE_OSC "OSC"
class OSCInterface{
  public:
};

#define ADDRESS_OSCPORT "/oscport/%d"

class NetworkOSC: public OSCInterface, public NetworkDevice{

  public:

    //DynamicDataSet dsMacroNames = DynamicDataSet(new DropDownAddress(ADDRESS_MACRONAME),this); 
    FreeTextAddress* addressOSCport = new FreeTextAddress(ADDRESS_OSCPORT,"OSCport", REGEX_0to65535);

    String OSC_IPaddress;
    int OSC_port;

    String getFilename(){
      return interfaceName+"_if.cfg";
    }

    NetworkOSC(String name, String _OSC_IPaddress,int _OSC_port,bool _configurable=false):OSC_IPaddress(_OSC_IPaddress),OSC_port(_OSC_port), NetworkDevice(name, GROUP_INTERFACE_OSC,_configurable){
      interfaceName = name;
      icon = "g";     

      registerDataSetAddress(addressOSCport);
      registerDataSetAddress(&addressIP);           // global

      jsonInputRepo.add(this);

    }

    virtual bool restoreProperty(const char* property){

      uint16_t OSCport;
      if (sscanf(property, ADDRESS_OSCPORT, &OSCport)) {
        if (OSCport != OSC_port) configChanged = true;
        OSC_port = OSCport;       
      }

      String OSCIP;
      if (sscanf(property, ADDRESS_IP, &OSCIP)) {
        if (OSCIP != OSC_IPaddress) configChanged = true;
        OSC_IPaddress = OSCIP;       
      }

      return configChanged;
    }        

    virtual void derivedMonitor(){}

    virtual void* read(){return nullptr;}

    virtual bool derivedBegin(){
      return false;
    }

    virtual bool end(){
      return false;
    }

    virtual bool connect(){
      return false;
    }

    virtual bool disconnect(){
      return false;
    }


};



#endif