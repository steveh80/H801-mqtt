class OTAUpdate
{  
  private:
    char* device_name;
    void addIndex();

  public:
    void init();  
    void loop();
    void setDeviceName(char* device_name);
};

extern OTAUpdate otaUpdate;
