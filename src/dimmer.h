#include "types.h"
#include "worker.h"

class Dimmer 
{
    private:

    public:
        void init();
        void loop();
        void dimChannel(int channel, int brightness);
};

extern Dimmer dimmer;