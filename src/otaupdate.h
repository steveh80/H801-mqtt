#include "settings.h"
#include "wifi.h"

class OTAUpdate {
    private:
        Settings *settings;
        Wifi *wifi;
        void addRoutes();

    public:
        void initWithSettings(Settings *settings);
        void addWifi(Wifi *wifi);
        void loop();
};

extern OTAUpdate otaUpdate;
