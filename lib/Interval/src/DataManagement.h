#ifndef DATAMGT_h
#define DATAMGT_h



///////////////////////////// DATA MANAGEMENT ///////////////////////////// 
class DataProvider{
  public :
    virtual const String& getDataSetName() = 0;
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
#define REGEX_NON_EMPTY_STRING      "Non empty string"
#define REGEX_SYSEX256              "sysex256"
#define REGEX_ALPHANUM40            "alphanum40"
#define REGEX_IPADDRESS             "ip"

#define ADDRESS_IP "/ip/%s"  

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
      //DEBUG_PRINT("\n%s.serialize() => ",pattern.c_str());
      rules = doc[pattern].to<JsonArray>();     
      rules[(int)ADDRESS_TYPE] = object;             // base      
      serializeProperties();
      String data;
      serializeJson(doc, data);
      //DEBUG_PRINT("\t%s",data.c_str());
      return data;
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

    FreeTextAddress(String p, String _fName, String _regex):friendlyName(_fName), regex(_regex), Address(p,INPUTTEXT){}

    virtual void serializeProperties(){
      rules[(int)ADDRESS_FRIENDLYNAME] = friendlyName;      
      rules[(int)ADDRESS_REGEX] = regex;      
    }    
};

FreeTextAddress addressIP = FreeTextAddress(ADDRESS_IP, "ip", REGEX_IPADDRESS);    

class DataSet{
  public:
    String name ="";
    Address* address = nullptr;

    String errorDatasetOOR = "outOfRange";

    DataSet(Address* adress, const String& name);
    ~DataSet(); 

    virtual uint16_t getSize() = 0;
    virtual const String* getValuePtr(uint16_t i) = 0;
    virtual String serialize();

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


    void serialize(JsonArray* jsonDataSets){
      for (uint8_t i=0;i<dataSets.size();i++){        
        //DEBUG_PRINT("\nDataRepository::serialize(*): %d ",i);
        DataSet* ds = dataSets[i];
        //DEBUG_PRINT("%s %s",ds == nullptr?"null":ds->name.c_str(),ds->address == nullptr?"null":ds->address->pattern.c_str());
        jsonDataSets->add(ds->serialize());
      }
    }

    int size(){
      return dataSets.size();
    }

  private:
    //LinkedList<DataSet*> dataSets = LinkedList<DataSet*>();
    std::vector<DataSet*> dataSets;

};

inline DataRepository& getDataRepository(){
  static DataRepository instance;
  return instance;
}


DataSet::DataSet(Address* addr, const String& _name): address(addr), name(_name) {
    getDataRepository().addDataSet(this);
};

DataSet::~DataSet() {
  getDataRepository().removeDataSet(this);
};

String DataSet::serialize(){
  JsonDocument doc;
  doc["provider"] = name;

  uint16_t n = getSize();

  if (address == nullptr) {
    doc["address"] = "undef";
    doc["values"] = "undef";
  }else{
    doc["address"] = address->pattern;
    
    // a dataset either return an array of value from a data provider (size>0)
    // or a no value (size=0) which is the name of a javascript function callback    
    if (n>0){
      // {"provider":"system","address":"/provider/Mixer/%s","values":["X+Mseries-Network(OSC)","MixingStation"]}
      JsonArray values = doc["values"].to<JsonArray>();
      for (uint8_t i=0; i< n; i++) values.add(*getValuePtr(i));
    }else{
      /*
      {"provider":"system","address":"/module/name/%s","values":"getModuleNames"}    => getModuleNames
          or
      {"provider":"system","address":"/midi/channel/%d","values":{"getNvalues":{"size":16,"label":"channel"}}} => {"getNvalues":{"size":16,"label":"channel"}}
                                              with: getNvalues is the javacript function to call and {"size":16,"label":"channel"} are the parameters
      */
      doc["values"] = *getValuePtr(0);
    }
  }
  String data;
  serializeJson(doc,data);
  removeJsonQuote(&data);      

  //DEBUG_PRINT("\nDataSet::serialize() <%s,%s> size:%d ==> %s",name.c_str(),doc["address"].as<String>().c_str(),n,data.c_str());

  return data;
} 

class DynamicDataSet : public DataSet{
  public:
    DataProvider* dataProvider;    

    DynamicDataSet(Address* addr, DataProvider* dp): dataProvider(dp), DataSet(addr, dp->getDataSetName()) {}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() {
        return (dataProvider) ? dataProvider->getSize(address->pattern) : 0;
    }

    virtual const String* getValuePtr(uint16_t i) {
        //DEBUG_PRINT("\tgetValuePtr(%d) provider: ",i);
        if (dataProvider == nullptr){
            DEBUG_PRINT("DS NULL");
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
    if ((*it)->address->pattern == ds->address->pattern) {  
      dataSets.erase(it);
      break;
    }
  }
}

#define SKELETON_PROPERTIES "properties"

class SerializableDataset {
  public:
    String name;
    String group;

    virtual ~SerializableDataset() = default;

    SerializableDataset(const String& n, const String& g) : name(n), group(g) {}    

    bool registerDataSetAddress(Address* address){
      dataSetAddresses.push_back(address);
      return true;
    }

    Address* getDataSetAddress(size_t index) const {
      if (index >= dataSetAddresses.size()){
        return nullptr;
      }
      return dataSetAddresses[index];
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
        json["name"] = name;
        json["group"] = group;

        JsonArray properties = json[SKELETON_PROPERTIES].to<JsonArray>();
        for (Address* address : dataSetAddresses) {
            if (!address) continue;

            JsonDocument prop;
            if (deserializeJson(prop, address->serialize())) {
                DEBUG_PRINT("\naddress properties serialization failed: %s", address->pattern.c_str());
                continue;
            }
            properties.add(prop);
        }
    }
    
    const String* getValuePtr(String provider, String address, uint8_t index){
      const String *retVal = &emptyString;

      //DEBUG_PRINT("\nSerializableDataset<%s>.getValuePtr(%s,%s,%i): ",stripID.c_str(),provider.c_str(),address.c_str(),index);
      DataSet* ds = getDataRepository().getDataSet(provider, address);

      if (ds != nullptr){
        retVal = ds->getValuePtr(index);
      }

      //DEBUG_PRINT("<%s>",retVal->c_str());

      return retVal;
    }    

    std::vector<Address*> dataSetAddresses;

  protected:
    void serializeProperties(JsonDocument& jsonDocStripSkeleton) const {
      JsonArray properties = jsonDocStripSkeleton[SKELETON_PROPERTIES].to<JsonArray>();
      JsonDocument property;

      for (size_t i = 0; i < dataSetAddresses.size(); i++){

        Address* address = dataSetAddresses[i];
        if (address != nullptr){
          DeserializationError err = deserializeJson(property,address->serialize()); // {"/midi/value1/%s": ["freetext","note", "0-126"]} or {"/midi/channel/%d": ["dropdown"]}
          if (err) {
            DEBUG_PRINT("\naddress properties serialization failed: %s",dataSetAddresses[i]->pattern.c_str());
            continue;
          }
        }else{
           DEBUG_PRINT("\naddress[%d] is null",i);
        }

        // doc contient maintenant ton objet JSON
        properties.add(property);

      }
    }
};



class JsonTemplateRepository{
  public:
    std::vector<SerializableDataset*> jsonTemplateRepo;

    void serialize(JsonArray* jsonTPLArray){
      for (uint8_t i=0;i<jsonTemplateRepo.size();i++){
        SerializableDataset* jsonTPL = jsonTemplateRepo[i];
        jsonTPLArray->add(jsonTPL->serialize());
      }
    }

    bool add(SerializableDataset* sd){
      jsonTemplateRepo.push_back(sd);
      return true;
    }

}jsonInputRepo;


#endif