#include "types.h"
#include "worker.h"

class Dimmer 
{
    private:
        // helper function to scale dim-speed over multiple channels
        uint8_t scaleSpeed(uint8_t speed, uint8_t dimDelta, uint8_t maxDimDelta);

    public:
        void init();
        void loop();
        
        // dim a single channel
        void dimChannel(uint8_t channel, uint8_t brightness, uint8_t speed = 2, uint8_t curve = 0, uint8_t onOffSpeed = 2);

        // dim a tunable white led, starting with warm white on channel, followed by cold white on channel + 1
        void dimCCT(uint8_t startChannel, uint8_t brightness, uint16_t colorTemp, uint8_t speed = 2, uint8_t curve = 0, uint8_t onOffSpeed = 2);
};

extern Dimmer dimmer;