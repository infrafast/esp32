#ifndef DATAMGT_h
#define DATAMGT_h



/**
 * @file DataManagement.h
 * @brief Parameter descriptions and named value access shared by configuration UI.
 *
 * Address describes a field's pattern and editor. DataSet exposes values under a
 * provider name and pattern. DataRepository resolves that pair without knowing
 * the storage or device backend. SerializableParameters publishes field templates;
 * JsonTemplateRepository collects them for the web UI. Templates describe what can
 * be configured, while datasets supply choices. Device I/O and applying selected
 * values remain responsibilities of the consuming interface/strip.
 *
 * DynamicDataSet forwards reads to a DataProvider using its address pattern.
 * This exposes existing application strings without duplicating their storage.
 * A DataSet subclass may instead manage storage directly; providers are optional.
 *
 * Concrete project examples:
 * - Mixer DataSetSynced instances publish channel/bus names in dataRepo. Strip
 *   skeletons describe dropdowns; OneSelectStrip looks up the selected label using
 *   the interface's provider name, address pattern and zero-based dataset index.
 *   The caller converts configured one-based channel numbers to dataset indices.
 * - Network interfaces register addressIP/addressPort. FreeTextAddress describes
 *   their label and validation identifier; ConfigDataSet exposes an address-only
 *   entry in IFdataRepo, without a list of choices or the actual configured value.
 * - WebLibrary loads a compressed JSON library under WEBAPP_PATH and exposes its
 *   entry names through the same dataset lookup and serialization contract.
 *
 * Illustrative provider-backed usage inside an application scope:
 *   // MyProvider implements getSize(pattern) and getValuePtr(pattern, index).
 *   MyProvider provider("MyDevice");
 *   DropDownAddress choices("/choice/%d");
 *   DynamicDataSet view(&choices, &provider);
 *   DataSet* ds = dataRepo.getDataSet("MyDevice", "/choice/%d");
 *   const String* label = ds ? ds->getValuePtr(0) : nullptr;
 *   // Consume label before provider storage is invalidated.
 *   // Declaration order keeps provider/descriptor alive until view is destroyed.
 *
 * Repositories borrow their entries; datasets register on construction and
 * unregister on destruction. Descriptor/provider pointers are borrowed too.
 * Returned String pointers are read-only views whose validity depends on concrete
 * storage and mutation. There is no locking or snapshot mechanism here.
 */
///////////////////////////// DATA MANAGEMENT ///////////////////////////// 
/**
 * Supplies address-specific values from application-owned storage on demand.
 * Implementations define bounds/fallback behavior and String-pointer validity.
 * dataProviderName is the provider key used by dataset consumers.
 */
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

/**
 * Describes a field's lookup/format pattern and web editor type.
 * rules is a borrowed JSON view rebound during serialization; do not use it after
 * its destination document expires. Descriptors do not own datasets.
 */
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

/**
 * Dropdown descriptor; choices come from a matching provider/pattern dataset.
 */
class DropDownAddress: public Address{
  public:
    DropDownAddress(String p):Address(p,INPUTSELECT){}
    virtual void serializeProperties(){}; // No extra dropdown metadata.
};

/**
 * Free-text metadata: display label and validation identifier (for example "ip").
 * Validation is not executed here. hide is stored but currently not serialized.
 */
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

/**
 * Abstract named value view, registered in targetRepo or dataRepo by default.
 * Borrows its Address and repository; destruction unregisters the view only.
 * Subclasses define value storage and pointer validity. The destructor is not
 * virtual, so deletion of derived objects through DataSet* is not generally safe.
 */
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

/**
 * Address-only entry for configuration fields without selectable choices.
 * Overrides content serialization to omit values, despite reporting size zero.
 * Does not store the interface's actual configured value.
 */
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

/**
 * Non-owning dataset registry keyed by provider name and address pattern.
 * Serializes entries grouped by provider; does not delete entries or enforce
 * unique keys. Lookup returns the first match. Entries must remain valid in use.
 */
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
        ds->serializeContent(o);   // Serialize content without repeating the provider key.
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

  // Legacy format: positive size emits an array; zero reads index 0 as a scalar.
  // An empty collection is therefore not represented by [] here.
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


/**
 * Non-owning adapter exposing one provider address as a DataSet.
 * Reads are forwarded without caching. Construction requires a non-null provider;
 * reads also require a valid descriptor. Later null guards do not make a null
 * provider safe at construction. Returned strings still belong to the provider.
 */
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

/**
 * Collects borrowed SerializableParameters pointers for web editor templates.
 * Serialization appends each template as a JSON string, not as a nested object.
 * No removal API exists: entries must remain alive while this registry is read.
 */
class JsonTemplateRepository{
  public:
    std::vector<SerializableParameters*> jsonTemplateRepo;

    void serialize(JsonArray* jsonTPLArray);
    bool add(SerializableParameters* sd);

}jsonInputRepo;


#define DATASET_PROPERTIES "properties"

/**
 * Publishes an object's ordered field descriptors and registers its web template.
 * Borrows descriptors and repo. Optional auto-registration creates address-only
 * ConfigDataSets; these entries are tracked and deleted on unregister/destruction,
 * but descriptors are never deleted. Current deletion through DataSet* retains
 * the non-virtual-destructor limitation described above.
 * Destruction does not unregister the template from jsonInputRepo. Consumers must
 * not access that entry afterwards. serialize() exports metadata, not live values.
 */
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

      // Remove the borrowed descriptor from the ordered template.
      for (auto it = parameterAddresses.begin(); it != parameterAddresses.end(); ++it) {
        if (*it == address) {
          parameterAddresses.erase(it);
          break;
        }
      }

      // Remove the matching automatically created configuration entry.
      if (registerConfigDataSets) {
        for (auto it = ownedDataSets.begin(); it != ownedDataSets.end(); ++it) {

          DataSet* ds = *it;

          // Match by descriptor identity.
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

    // Derived classes may extend this editor template.
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

/**
 * File-backed choice dataset using JsonListFile storage.
 * reload() clears entries and loads WEBAPP_PATH/filename.gz, adding a fallback
 * entry on failure. Exposes entry names rather than their associated payloads.
 * Treat borrowed name pointers as invalid after reload or storage mutation.
 */
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
      if (index>getSize()-1) return &errorDatasetOOR;
      return getName(index);    
    }  

    virtual uint16_t getSize(){
      return size();                //number of pair k/v loaded from the file
    }

  private:
    String filename;
    
};


#endif
