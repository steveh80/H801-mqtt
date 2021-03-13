#include "wifi.h"

class OTAUpdate {
    private:
        void addRoutes();

    public:
        void init();
        void loop();
};

extern OTAUpdate otaUpdate;
