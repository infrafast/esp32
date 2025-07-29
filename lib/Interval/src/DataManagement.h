#ifndef DATAMGT_h
#define DATAMGT_h

#define INPUTSELECT "dropdown"
#define INPUTTEXT   "freetext"
#define INPUTUINT   "uint"

///////////////////////////// DATA MANAGEMENT ///////////////////////////// 
class DataProvider{
  public :
    virtual String getDataSetName() = 0;
    virtual uint16_t getSize(String addrr) = 0;
    virtual const String* getValuePtr(String addrr, uint16_t index) = 0;
  protected:
    String voidStatic="voidStatic";
};

typedef  struct  {
    String pattern;
    String object;
} Address;

class DataSet{
  public:
    String name ="";
    Address address;

    String errorDatasetOverflow = "overflow";

    DataSet(const Address& adress, const String& name);
    ~DataSet(); 

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() = 0;
    virtual const String* getValuePtr(uint16_t i) = 0;


    virtual String serialize();

};

class StaticDataSet : public DataSet{
  public:

    StaticDataSet(const Address& adress, String name, uint16_t _size): DataSet(adress, name), size(_size){}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize(){
        return size;
    }

  protected:
    uint16_t size=0;
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
        if (address == ds->address.pattern && provider == ds->name){          
          return ds;
        } 
      }
      return nullptr;
    }

    bool addDataSet(DataSet*);
    void removeDataSet(DataSet*);


    void serialize(JsonArray* jsonDataSets){
      for (uint8_t i=0;i<dataSets.size();i++){        
        DataSet* ds = dataSets[i];
        //DEBUG_PRINT("\nserializing ds <%s>",ds->name.c_str());
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

DataRepository dataRepository;

DataSet::DataSet(const Address& addr, const String& _name): address(addr), name(_name) {
    dataRepository.addDataSet(this);
};

DataSet::~DataSet() {
  dataRepository.removeDataSet(this);
};

String DataSet::serialize(){
  JsonDocument doc;
  doc["provider"] = name;
  doc["address"] = address.pattern;
  // a dataset either return an array of value from a data provider (size>0)
  // or a no value (size=0) which is the name of a javascript function callback
  uint16_t n = getSize();
  //DEBUG_PRINT("\nDataSet::serialize() <%s,%s> size:%d ==> ",name.c_str(),address.pattern.c_str(),n);
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

  String data;
  serializeJson(doc,data);
  removeJsonQuote(&data);      

  return data;
} 

class DynamicDataSet : public DataSet{
  public:
    DataProvider* dataProvider;    

    DynamicDataSet(const Address addr, DataProvider* dp): dataProvider(dp), DataSet(addr, dp->getDataSetName()) {}

    // Ensure dataProvider is valid before accessing it
    virtual uint16_t getSize() {
        return (dataProvider) ? dataProvider->getSize(address.pattern) : 0;
    }

    virtual const String* getValuePtr(uint16_t i) {
        //DEBUG_PRINT("\tgetValuePtr(%d) provider: ",i);
        if (dataProvider == nullptr){
            DEBUG_PRINT("DS NULL");
            return &emptyString;
        } 
        const String* v = dataProvider->getValuePtr(address.pattern, i);
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
    if ((*it)->address.pattern == ds->address.pattern) {  
      dataSets.erase(it);
      break;
    }
  }
}


#endif