#include "settings.h"

class OTAUpdate
{  
  private:
    Settings* settings;
    void addIndex();

  public:
    void initWithSettings(Settings* settings);  
    void loop();
};

extern OTAUpdate otaUpdate;
