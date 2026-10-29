#ifndef DATAMGT_h
#define DATAMGT_h



/**
 * @file DataManagement.h
 * @brief Makes settings and lists of choices available to the configuration UI.
 *
 * This design lets different parts of the application share data in the same way.
 * Each setting describes how it should be edited, and each list supplies the
 * available choices. The UI can use them without knowing where the data comes from.
 *
 * For example:
 * - Show mixer channel names in a dropdown and in a strip's label.
 * - Describe IP address and port fields for network configuration.
 * - Offer names loaded from a web library as selectable choices.
 *
 * DataProvider supplies the data, DataSet makes it available under a name, and
 * DataRepository helps consumers find it. This keeps UI code reusable when the
 * device or data source changes.
 */
//////////////////////////// DATA MANAGEMENT /////////////////////////////
/**
 * @brief Supplies address-specific values from application-owned storage on demand.
 *
 * Implementations define bounds/fallback behavior and String-pointer validity.
 * dataProviderName is the provider key used by dataset consumers.
 */
class DataProvider{
  public :
    String dataProviderName;
    /**
     * @brief Initialize DataProvider and its configured member bindings.
     */
    DataProvider(const String& pName):dataProviderName(pName){};
    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(String addrr) = 0;
    /** @brief Return a borrowed choice label for the requested index. */
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
 * @brief Describes a field's lookup/format pattern and web editor type.
 *
 * rules is a borrowed JSON view rebound during serialization; do not use it after
 * its destination document expires. Descriptors do not own datasets.
 */
class Address {
  public:
    String pattern;
    String object;
    JsonArray rules;    

    /**
     * @brief Initialize Address and its configured member bindings.
     */
    Address(const String& p, const String& o):pattern(p),object(o){}
  
    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties() = 0;      

    /** @brief Export this object using its JSON representation. */
    String serialize(){      
      JsonDocument doc;
      serializeTo(doc, doc.as<JsonObject>());
      String data;
      serializeJson(doc, data);
      //DEBUG_PRINT("\t%s",data.c_str());
      return data;
    }

    /** @brief Append the field description to the supplied JSON destination. */
    void serializeTo(JsonDocument& doc, JsonObject parent){
      //DEBUG_PRINT("\n%s.serialize() => ",pattern.c_str());
      rules = parent[pattern].to<JsonArray>();
      rules[(int)ADDRESS_TYPE] = object;             // base      
      serializeProperties();
      doc.shrinkToFit();
    }

    /** @brief Append the field description to the supplied JSON destination. */
    void serializeTo(JsonArray properties){
      JsonObject entry = properties.createNestedObject();
      rules = entry[pattern].to<JsonArray>();
      rules[(int)ADDRESS_TYPE] = object;
      serializeProperties();
    }

};

/**
 * @brief Dropdown descriptor; choices come from a matching provider/pattern dataset.
 */
class DropDownAddress: public Address{
  public:
    /**
     * @brief Initialize DropDownAddress and its configured member bindings.
     */
    DropDownAddress(const String& p):Address(p,INPUTSELECT){}
    /** @brief Append this type's property strings to the active JSON properties array. */
    virtual void serializeProperties(){}; // No extra dropdown metadata.
};

/**
 * @brief Free-text metadata: display label and validation identifier (for example "ip").
 *
 * Validation is not executed here. hide is stored but currently not serialized.
 */
class FreeTextAddress: public Address{
  public:
    String friendlyName = "";
    String regex = "";
    bool hide = false;

    /**
     * @brief Initialize FreeTextAddress and its configured member bindings.
     */
    FreeTextAddress(const String& p, const String& _fName, const String& _regex):friendlyName(_fName), regex(_regex), Address(p,INPUTTEXT){}

    /** @brief Append this type's property strings to the active JSON properties array. */
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
 * @brief Abstract named value view, registered in targetRepo or dataRepo by default.
 *
 * Borrows its Address and repository; destruction unregisters the view only.
 * Subclasses define value storage and pointer validity. The virtual destructor
 * makes deletion of derived objects through DataSet* safe.
 */
class DataSet{
  public:
    String name ="";
    Address* address = nullptr;
    DataRepository* repo = nullptr;

    /**
     * @brief Register this dataset in the supplied repository, or dataRepo by default.
     *
     * The address/repository pointers are borrowed. Destruction unregisters this
     * dataset; the registry does not own or delete it.
     */
    DataSet(Address* adress, const String& name, DataRepository* targetRepo = nullptr);
    /** @brief Release resources held by this object. */
    virtual ~DataSet();

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize() = 0;
    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t i) = 0;
    /** @brief Export this object using its JSON representation. */
    virtual JsonObject serialize(JsonObject o);
    /**
     * @brief Export an address and its current values using the legacy wire format.
     *
     * A positive size creates a values array. Zero reads getValuePtr(0) as a scalar,
     * so an empty list is not represented as [] here. A null descriptor emits nothing.
     */
    virtual void serializeContent(JsonObject o);
};

/**
 * @brief Address-only entry for configuration fields without selectable choices.
 *
 * Overrides content serialization to omit values, despite reporting size zero.
 * Does not store the interface's actual configured value.
 */
class ConfigDataSet : public DataSet{
  public:
    /**
     * @brief Initialize ConfigDataSet and its configured member bindings.
     */
    ConfigDataSet(Address* addr, const String& name, DataRepository* targetRepo = nullptr)
      : DataSet(addr, name, targetRepo) {}

    /**
     * @brief Return zero because this entry describes an address without a choice list.
     */
    virtual uint16_t getSize() { return 0; }
    /**
     * @brief Return an empty placeholder; no configuration value is stored here.
     */
    virtual const String* getValuePtr(uint16_t) { return &emptyString; }
    /**
     * @brief Export only the address, deliberately omitting a values field.
     */
    virtual void serializeContent(JsonObject o) {
      if (address == nullptr) return;
      o["address"] = address->pattern;
    }
};

/**
 * @brief Non-owning dataset registry keyed by provider name and address pattern.
 *
 * Serializes entries grouped by provider; does not delete entries or enforce
 * unique keys. Lookup returns the first match. Entries must remain valid in use.
 */
class DataRepository{
  public:
    /**
     * @brief Initialize DataRepository and its configured member bindings.
     */
    DataRepository(){}

    /**
     * @brief Locate a borrowed dataset by position or by provider/address pattern.
     *
     * Name-based lookup returns the first match. This registry does not enforce
     * unique keys; callers must supply valid indices and live descriptors.
     */
    DataSet* getDataSet(int i){
      if (i<size()) return dataSets[i];
      else return nullptr;
    }

    /**
     * @brief Locate a borrowed dataset by position or by provider/address pattern.
     *
     * Name-based lookup returns the first match. This registry does not enforce
     * unique keys; callers must supply valid indices and live descriptors.
     */
    DataSet* getDataSet(String provider, String address){
      for (uint8_t i=0;i<dataSets.size();i++){
        DataSet* ds = dataSets[i];        
        if (address == ds->address->pattern && provider == ds->name){          
          return ds;
        } 
      }
      return nullptr;
    }

    /** @brief Register a borrowed dataset pointer. */
    bool addDataSet(DataSet*);
    /** @brief Remove a dataset pointer without deleting the dataset. */
    void removeDataSet(DataSet*);
  

    /**
     * @brief Group dataset contents under their provider names in the supplied object.
     *
     * Each provider contains an array of address/value entries; this avoids repeating
     * the provider name inside every entry. Dataset storage is not copied into a cache.
     */
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

    /** @brief Return the current number of entries. */
    int size(){
      return dataSets.size();
    }

  private:
    std::vector<DataSet*> dataSets;

}dataRepo,    ///< used by skeletons, Shortcuts
IFdataRepo; ///< used by Interfaces

/**
 * @brief Register this dataset in the supplied repository, or dataRepo by default.
 *
 * The address/repository pointers are borrowed. Destruction unregisters this
 * dataset; the registry does not own or delete it.
 */
DataSet::DataSet(Address* addr, const String& _name, DataRepository* targetRepo): address(addr), name(_name) {
    repo = targetRepo ? targetRepo : &dataRepo;
    repo->addDataSet(this);
};

/** @brief Release resources held by this object. */
DataSet::~DataSet() {
  if (repo){
    repo->removeDataSet(this);
  }
};


/**
 * @brief Export an address and its current values using the legacy wire format.
 *
 * A positive size creates a values array. Zero reads getValuePtr(0) as a scalar,
 * so an empty list is not represented as [] here. A null descriptor emits nothing.
 */
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


/** @brief Export this object using its JSON representation. */
JsonObject DataSet:: serialize(JsonObject o) {
  o["provider"] = name;
  serializeContent(o);
  return o;
}


/**
 * @brief Non-owning adapter exposing one provider address as a DataSet.
 *
 * Reads are forwarded without caching. Construction requires a non-null provider;
 * reads also require a valid descriptor. Later null guards do not make a null
 * provider safe at construction. Returned strings still belong to the provider.
 */
class DynamicDataSet : public DataSet{
  public:
    DataProvider* dataProvider;    

    /**
     * @brief Initialize DynamicDataSet and its configured member bindings.
     */
    DynamicDataSet(Address* addr, DataProvider* dp, DataRepository* targetRepo = nullptr)
      : dataProvider(dp),
        DataSet(addr, dp->dataProviderName, targetRepo) {}

    /**
     * @brief Ensure dataProvider is valid before accessing it
     */
    virtual uint16_t getSize() {
        return (dataProvider) ? dataProvider->getSize(address->pattern) : 0;
    }

    /** @brief Return a borrowed choice label for the requested index. */
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


/** @brief Register a borrowed dataset pointer. */
bool DataRepository::addDataSet(DataSet* ds){
  dataSets.push_back(ds);
  /*String object = ds->object;
  String address = ds->address;      */
  return true;
}

/** @brief Remove a dataset pointer without deleting the dataset. */
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
 * @brief Collects borrowed SerializableParameters pointers for web editor templates.
 *
 * Serialization appends each template as a JSON string, not as a nested object.
 * No removal API exists: entries must remain alive while this registry is read.
 */
class JsonTemplateRepository{
  public:
    std::vector<SerializableParameters*> jsonTemplateRepo;

    /**
     * @brief Append each registered editor template as a JSON string.
     *
     * The input array belongs to the caller; registered template pointers are borrowed.
     */
    void serialize(JsonArray* jsonTPLArray);
    /** @brief Append an entry to this collection. */
    bool add(SerializableParameters* sd);

}jsonInputRepo;


#define DATASET_PROPERTIES "properties"

/**
 * @brief Publishes an object's ordered field descriptors and registers its web template.
 *
 * Borrows descriptors and repo. Optional auto-registration creates address-only
 * ConfigDataSets; these entries are tracked and deleted on unregister/destruction,
 * but descriptors are never deleted. Owned datasets are deleted through their
 * virtual DataSet destructor, so derived cleanup remains valid.
 * Destruction does not unregister the template from jsonInputRepo. Consumers must
 * not access that entry afterwards. serialize() exports metadata, not live values.
 */
class SerializableParameters {
  public:
    String name;
    String group;

    /** @brief Initialize SerializableParameters with the supplied configuration and bindings. */
    SerializableParameters(const String& n, const String& g, DataRepository* r, bool autoRegisterConfigDataSets = false)
      : name(n),
        group(g),
        repo(r),
        registerConfigDataSets(autoRegisterConfigDataSets) {
          jsonInputRepo.add(this);          
        }

    /**
     * @brief Delete automatically created configuration entries, leaving descriptors borrowed.
     *
     * @note Owned entries are deleted through DataSet*, whose destructor is virtual.
     * The template pointer also remains registered in jsonInputRepo after destruction.
     */
    virtual ~SerializableParameters() {
      for (DataSet* ds : ownedDataSets) {
        delete ds;
      }
      ownedDataSets.clear();
    }

    /**
     * @brief Add a borrowed descriptor to the ordered editor template.
     *
     * If automatic configuration datasets are enabled, create and track an address-only
     * ConfigDataSet as well. Descriptor order matters for strip selection properties.
     */
    bool registerDataSetAddress(Address* address){
      parameterAddresses.push_back(address);
      if (registerConfigDataSets) {
        ownedDataSets.push_back(new ConfigDataSet(address, name, repo));
      }
      return true;
    }

    /** @brief Remove the descriptor and its automatically created configuration dataset. */
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
    


    /** @brief Return a borrowed parameter descriptor, or nullptr for an invalid index. */
    Address* getDataSetAddress(size_t index) const {
      if (index >= parameterAddresses.size()){
        return nullptr;
      }
      return parameterAddresses[index];
    }

    /** @brief Export this object using its JSON representation. */
    String serialize(){
      JsonDocument jsonDoc;
      JsonObject root = jsonDoc.to<JsonObject>();
      serialize(root);

      String data;
      serializeJson(root, data);
      return data;
    }

    /**
     * @brief Derived classes may extend this editor template.
     */
    virtual void serialize(JsonObject& json) {
        json[name] = group;

        JsonArray properties = json[DATASET_PROPERTIES].to<JsonArray>();
        for (Address* address : parameterAddresses) {
            if (!address) continue;

            address->serializeTo(properties);
        }
    }
    
    /** @brief Return a borrowed choice label for the requested index. */
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
      char* buf;    ///< used for serialization

};

/**
 * @brief Append each registered editor template as a JSON string.
 *
 * The input array belongs to the caller; registered template pointers are borrowed.
 */
void JsonTemplateRepository::serialize(JsonArray* jsonTPLArray){
      for (uint8_t i=0;i<jsonTemplateRepo.size();i++){
        SerializableParameters* jsonTPL = jsonTemplateRepo[i];
        jsonTPLArray->add(jsonTPL->serialize());
      }
    };

/** @brief Append an entry to this collection. */
bool JsonTemplateRepository::add(SerializableParameters* sd){
      jsonTemplateRepo.push_back(sd);
      return true;
    };

/**
 * @brief File-backed choice dataset using JsonListFile storage.
 *
 * reload() clears entries and loads WEBAPP_PATH/filename.gz, adding a fallback
 * entry on failure. Exposes entry names rather than their associated payloads.
 * Treat borrowed name pointers as invalid after reload or storage mutation.
 */
class WebLibrary : public DataSet, public JsonListFile{
  public:
    /** @brief Initialize WebLibrary with the supplied configuration and bindings. */
    WebLibrary(const String& _filename, Address* _address, const String& _provider): filename(_filename), DataSet(_address, _provider){
      reload();      
    }      

    /**
     * @brief Replace library choices by loading WEBAPP_PATH/filename.gz.
     *
     * Clear old entries first. On load failure, add a fallback name/error entry.
     * Borrowed entry-name pointers must not be retained across this replacement.
     */
    bool reload(){      
      clean();      
      String fullFilePathName = concatStrings(WEBAPP_PATH,"/",filename,".gz");
      bool success = load(fullFilePathName);
      if (!success) addValue(filename,"error");      
      return success;
    }

    /** @brief Return a borrowed choice label for the requested index. */
    virtual const String* getValuePtr(uint16_t index){           
      if (index>getSize()-1) return &errorDatasetOOR;
      return getName(index);    
    }  

    /** @brief Return the number of choices exposed by this dataset. */
    virtual uint16_t getSize(){
      return size();                //number of pair k/v loaded from the file
    }

  private:
    String filename;
    
};


#endif
