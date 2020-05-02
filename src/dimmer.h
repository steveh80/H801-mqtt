#include "types.h"
#include "worker.h"

class Dimmer 
{
    private:

    public:
        void init();
        void loop();
        void dimChannel(uint8_t channel, int brightness, uint8_t speed = 2, uint8_t curve = 0, uint8_t onOffspeed = 2);
};

extern Dimmer dimmer;