#ifndef _ISYSTEM_h
#define _ISYSTEM_h  

#include "Interfaces.h"

 
#define ADDRESS_PROVIDER      "/provider/"
#define ADDRESS_TEXT    "/text/%s"

#define GROUP_INTERFACE_UTILS "utils"

class SystemInterface: public DeviceInterface, public DataProvider{
    #define SYSTEM_INTERFACE "system"
    public:    

    SerializableParameters systemDS = SerializableParameters(SYSTEM_INTERFACE, GROUP_INTERFACE_UTILS, &dataRepo);   // one instance
        
    DynamicDataSet dsText = DynamicDataSet(new FreeTextAddress(ADDRESS_TEXT, "free text", REGEX_STRING),this);  // create as many parameters as required

    SystemInterface(bool _configurable = false):DeviceInterface(SYSTEM_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),DataProvider(getProviderNameForData()){
      state.code = CONNECTED;     
      state.enabled = true;       
    }

    virtual bool restore(const JsonObject& config){return false;}

    virtual void derivedConfig(JsonObject& config){};    
    virtual void monitor(){}
    
    virtual void* read(){return nullptr;}
    
    virtual bool begin(){

      systemDS.registerDataSetAddress(dsText.address);      // register all ds             
      jsonInputRepo.add(&systemDS);     // registered once

      //dataProviderName = getProviderNameForData();      

      return true;
    }

    virtual uint16_t getSize(String addrr){
      if (addrr == ADDRESS_TEXT) return 1;
      else{
        // IS IT AN INTERFACE ?
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX            
          uint16_t count = 0;
          for (const auto& interface : interfaceRepo.interfaces){
            // check if we request provider listing
            if (interface->group == group) count++;
          }
          //DEBUG_PRINT("\nDataSet getSize(%s) has %d items",addrr.c_str(),count);
          return count;
        }

      }

      
      DEBUG_PRINT("\n<%s>DataSet getSize(%s) is not implemented",dataProviderName.c_str(),addrr.c_str());
      return 0;

    }    
    
    virtual const String* getValuePtr(String addrr, uint16_t index){
      if (addrr == ADDRESS_TEXT){
        textValue = concatStrings("hello", index);
        return &textValue;
      }else{
        char group[64] = {0};
        //DEBUG_PRINT("\nscanning %s against /provider/xxxx/~s",addrr.c_str());
        if (sscanf(addrr.c_str(), "/provider/%63[^/]/%%s", group) == 1) {
          //DEBUG_PRINT("\n found: %s\n", group);  // → DMX    
          int16_t count = -1;        
          for (const auto& interface : interfaceRepo.interfaces){
            if (interface->group == group) count++;
            if (count == index){
              String* s = &interface->interfaceName;
              //DEBUG_PRINT("\nDataSet getValuePtr(%s,%d) = %s",addrr.c_str(),index,s->c_str());
              return s;
            } 
          }
        }      
      }     

      DEBUG_PRINT("\n<%s>DataSet getValuePtr(%s,%d) is not implemented",dataProviderName.c_str(),addrr.c_str(),index);
      return &voidStatic;      
    }

  private:

    String textValue = "";    
        
    
}systemInterface(false);


class DirectoryInterface: public DeviceInterface, public NetWorkService{
    #define DIRECTORY_INTERFACE "Directory"
    public:    
        DirectoryInterface(bool _configurable = false):DeviceInterface(DIRECTORY_INTERFACE,GROUP_INTERFACE_UTILS,_configurable),NetWorkService(){
            attachDeviceInterface(this);
        }

        virtual bool restore(const JsonObject& config){return false;}

        virtual void derivedConfig(JsonObject& config){};    

        virtual void monitor(){
            monitorNetwork();
        }

        virtual void* read(){return nullptr;}

        virtual void monitorService(){}
        virtual bool handleConnectService(){return false;}
        virtual bool handleDisconnectService(){return false;}        

    private:

};



#endif