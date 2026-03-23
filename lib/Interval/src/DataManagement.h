#ifndef DATAMGT_h
#define DATAMGT_h



///////////////////////////// DATA MANAGEMENT ///////////////////////////// 
class DataProvider{
  public :
    String dataProviderName;
    DataProvider(String pName):dataProviderName(pName){};
    virtual uint16_t getSize(String addrr) = 0;
    virtual const String* getValuePtr(String addrr, uint16_t index) = 0;
  protected:
    String voidStatic="voidStatic";
};

/*typedef  struct  {
    String pattern;
    String object;
} Address;*/


enum {
  ADDRESS_TYPE,
  ADDRESS_FRIENDLYNAME,
  ADDRESS_REGEX
};

#define INPUTSELECT "dropdown"
#define INPUTTEXT   "freetext"

#define REGEX_0to126                "0-126"
#define REGEX_0to65535              "0-65535"
#define REGEX_SYSEX256              "sysex256"
#define REGEX_ALPHANUM40            "alphanum40"
#define REGEX_IPADDRESS             "ip"
#define REGEX_STRING                "string"

#define ADDRESS_IP "/ip/%s"  
#define ADDRESS_PING "/ping/%s"
#define ADDRESS_PORT "/port/%d"

#define KEY_PROPERTIES    "properties"

class Address {
  public:
    String pattern;
    String object;
    JsonArray rules;    

    Address(String p, String o):pattern(p),object(o){}
  
    virtual void serializeProperties() = 0;      

    String serialize(){      
      JsonDocument doc;
      serializeTo(doc, doc.as<JsonObject>());
      String data;
      serializeJson(doc, data);
      //DEBUG_PRINT("\t%s",data.c_str());
      return data;
    }

    void serializeTo(JsonDocument& doc, JsonObject parent){
      //DEBUG_PRINT("\n%s.serialize() => ",pattern.c_str());
      rules = parent[pattern].to<JsonArray>();
      rules[(int)ADDRESS_TYPE] = object;             // base      
      serializeProperties();
      doc.shrinkToFit();
    }

    void serializeTo(JsonArray properties){
      JsonObject entry = properties.createNestedObject();
      rules = entry[pattern].to<JsonArray>();
      rules[(int)ADDRESS_TYPE] = object;
      serializeProperties();
    }

};

class DropDownAddress: public Address{
  public:
    DropDownAddress(String p):Address(p,INPUTSELECT){}
    virtual void serializeProperties(){}; // nothin more than what the base does
};

class FreeTextAddress: public Address{
  public:
    String friendlyName = "";
    String regex = "";
    bool hide = false;

    FreeTextAddress(String p, String _fName, String _regex):friendlyName(_fName), regex(_regex), Address(p,INPUTTEXT){}

    virtual void serializeProperties(){
      rules[(int)ADDRESS_FRIENDLYNAME] = friendlyName;      
      rules[(int)ADDRESS_REGEX] = regex;      
    }    
};

FreeTextAddress addressIP = FreeTextAddress(ADDRESS_IP, "ip", REGEX_IPADDRESS);    
FreeTextAddress addressPing = FreeTextAddress(ADDRESS_PING, "ping", REGEX_STRING);  
FreeTextAddress addressPort = FreeTextAddress(ADDRESS_PORT,"port", REGEX_0to65535);  

String errorDatasetOOR = "outOfRange";

class DataRepository;

class DataSet{
  public:
    String name ="";
    Address* address = nullptr;
    DataRepository* repo = nullptr;

    DataSet(Address* adress, const String& name, DataRepository* targetRepo = nullptr);
    ~DataSet(); 

    virtual uint16_t getSize() = 0;
    virtual const String* getValuePtr(uint16_t i) = 0;
    virtual JsonObject serialize(JsonObject o);
    virtual void serializeContent(JsonObject o);
};

class ConfigDataSet : public DataSet{
  public:
    ConfigDataSet(Address* addr, const String& name, DataRepository* targetRepo = nullptr)
      : DataSet(addr, name, targetRepo) {}

    virtual uint16_t getSize() { return 0; }
    virtual const String* getValuePtr(uint16_t) { return &emptyString; }
    virtual void serializeContent(JsonObject o) {
      if (address == nullptr) return;
      o["address"] = address->pattern;
    }
};

class DataRepository{
  public:
    DataRepository(){}

    DataSet* getDataSet(int i){
      if (i<size()) return dataSets[i];
      else return nullptr;
    }

    DataSet* getDataSet(String provider, String address){
      for (uint8_t i=0;i<dataSets.size();i++){
        DataSet* ds = dataSets[i];        
        if (address == ds->address->pattern && provider == ds->name){          
          return ds;
        } 
      }
      return nullptr;
    }

    bool addDataSet(DataSet*);
    void removeDataSet(DataSet*);
  

    void serialize(JsonObject jsonByProvider) {
      for (uint8_t i = 0; i < dataSets.size(); i++) {
        DataSet* ds = dataSets[i];
        if (!ds || !ds->name) continue;

        JsonArray arr;
        if (jsonByProvider.containsKey(ds->name)) {
          arr = jsonByProvider[ds->name].as<JsonArray>();
        } else {
          arr = jsonByProvider.createNestedArray(ds->name);
        }

        JsonObject o = arr.createNestedObject();
        ds->serializeContent(o);   // ✅ factorisation propre
      }
    }

    int size(){
      return dataSets.size();
    }

  private:
    std::vector<DataSet*> dataSets;

}dataRepo,    // used by skeletons, Shortcuts
IFdataRepo; // used by Interfaces

DataSet::DataSet(Address* addr, const String& _name, DataRepository* targetRepo): address(addr), name(_name) {
    repo = targetRepo ? targetRepo : &dataRepo;
    repo->addDataSet(this);
};

DataSet::~DataSet() {
  if (repo){
    repo->removeDataSet(this);
  }
};


void DataSet:: serializeContent(JsonObject o) {
  if (address == nullptr) return;

  o["address"] = address->pattern;

  uint16_t n = getSize();
  if (n > 0) {
    JsonArray values = o.createNestedArray("values");
    for (uint16_t k = 0; k < n; k++) {
      const String* p = getValuePtr(k);
      values.add(p ? *p : "");
    }
  } else {
    const String* p0 = getValuePtr(0);
    o["values"] = p0 ? *p0 : "";
  }
}


JsonObject DataSet:: serialize(JsonObject o) {
  o["provider"] = name;
  serializeContent(o);
  return o;
}


class DynamicDataSet : public DataSet{
  public:
    DataProvider* dataProvider;    

    DynamicDataSet(Address* addr, DataProvider* dp, DataRepository* targetRepo = nullptr)
      : dataProvider(dp),
        DataSet(addr, dp->dataProviderName, targetRepo) {}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() {
        return (dataProvider) ? dataProvider->getSize(address->pattern) : 0;
    }

    virtual const String* getValuePtr(uint16_t i) {
        //DEBUG_PRINT("\tgetValuePtr(%d) provider: ",i);
        if (dataProvider == nullptr){
            DEBUG_ERROR("DS NULL");
            return &emptyString;
        } 
        const String* v = dataProvider->getValuePtr(address->pattern, i);
        //DEBUG_PRINT("%s => %s",dataProvider->getDataSetName().c_str(),v->c_str());               
        return v;
        
    }
};


bool DataRepository::addDataSet(DataSet* ds){
  dataSets.push_back(ds);
  /*String object = ds->object;
  String address = ds->address;      */
  return true;
}

void DataRepository::removeDataSet(DataSet* ds) {
  for (auto it = dataSets.begin(); it != dataSets.end(); ++it) {
    if (*it == ds) {  
      dataSets.erase(it);
      break;
    }
  }
}

class SerializableParameters;

class JsonTemplateRepository{
  public:
    std::vector<SerializableParameters*> jsonTemplateRepo;

    void serialize(JsonArray* jsonTPLArray);
    bool add(SerializableParameters* sd);

}jsonInputRepo;


#define DATASET_PROPERTIES "properties"

class SerializableParameters {
  public:
    String name;
    String group;

    SerializableParameters(const String& n, const String& g, DataRepository* r, bool autoRegisterConfigDataSets = false)
      : name(n),
        group(g),
        repo(r),
        registerConfigDataSets(autoRegisterConfigDataSets) {
          jsonInputRepo.add(this);          
        }

    virtual ~SerializableParameters() {
      for (DataSet* ds : ownedDataSets) {
        delete ds;
      }
      ownedDataSets.clear();
    }

    bool registerDataSetAddress(Address* address){
      parameterAddresses.push_back(address);
      if (registerConfigDataSets) {
        ownedDataSets.push_back(new ConfigDataSet(address, name, repo));
      }
      return true;
    }

    bool unregisterDataSetAddress(Address* address) {

      if (!address) {
        return false;
      }

      // 1️⃣ suppression dans parameterAddresses
      for (auto it = parameterAddresses.begin(); it != parameterAddresses.end(); ++it) {
        if (*it == address) {
          parameterAddresses.erase(it);
          break;
        }
      }

      // 2️⃣ suppression du ConfigDataSet associé
      if (registerConfigDataSets) {
        for (auto it = ownedDataSets.begin(); it != ownedDataSets.end(); ++it) {

          DataSet* ds = *it;

          // on suppose que ConfigDataSet possède getAddress()
          if (ds && ds->address == address) {

            delete ds;
            ownedDataSets.erase(it);
            break;
          }
        }
      }

      return true;
    }
    


    Address* getDataSetAddress(size_t index) const {
      if (index >= parameterAddresses.size()){
        return nullptr;
      }
      return parameterAddresses[index];
    }

    String serialize(){
      JsonDocument jsonDoc;
      JsonObject root = jsonDoc.to<JsonObject>();
      serialize(root);

      String data;
      serializeJson(root, data);
      return data;
    }

    // 🚩 Méthode virtuelle : chaque classe dérivée enrichit ici le JSON
    virtual void serialize(JsonObject& json) {
        json[name] = group;

        JsonArray properties = json[DATASET_PROPERTIES].to<JsonArray>();
        for (Address* address : parameterAddresses) {
            if (!address) continue;

            address->serializeTo(properties);
        }
    }
    
    const String* getValuePtr(String provider, String address, uint8_t index){
      const String *retVal = &emptyString;

      //DEBUG_PRINT("\nSerializableDataset::getValuePtr(%s,%s,%i): ",provider.c_str(),address.c_str(),index);
      DataSet* ds = repo->getDataSet(provider, address);

      if (ds != nullptr){
        retVal = ds->getValuePtr(index);
      }

      //DEBUG_PRINT("\t ====> <%s>",retVal->c_str());

      return retVal;
    }    

    std::vector<Address*> parameterAddresses;
    std::vector<DataSet*> ownedDataSets;
    DataRepository* repo;
    bool registerConfigDataSets = false;

    protected:
      char* buf;    //used for serialization

};

void JsonTemplateRepository::serialize(JsonArray* jsonTPLArray){
      for (uint8_t i=0;i<jsonTemplateRepo.size();i++){
        SerializableParameters* jsonTPL = jsonTemplateRepo[i];
        jsonTPLArray->add(jsonTPL->serialize());
      }
    };

bool JsonTemplateRepository::add(SerializableParameters* sd){
      jsonTemplateRepo.push_back(sd);
      return true;
    };

class WebLibrary : public DataSet, public JsonListFile{
  public:
    WebLibrary(String _filename, Address* _address, String _provider): filename(_filename), DataSet(_address, _provider){ 
      reload();      
    }      

    bool reload(){      
      clean();      
      String fullFilePathName = concatStrings(WEBAPP_PATH,"/",filename,".gz");
      bool success = load(fullFilePathName);
      if (!success) addValue(filename,"error");      
      return success;
    }

    virtual const String* getValuePtr(uint16_t index){      
      if (index<size()) getName(index);
      //return getString(getName(index));       // return the valuenof the jkey   
      return &errorDatasetOOR;                      
    }  

    virtual uint16_t getSize(){
      return size();                //number of pair k/v loaded from the file
    }

  private:
    String filename;
    
};


#endif
