#include <stdint.h>
#include "types.h"
#include "dmx.h"
#include "dmxqueue.h"
#include "helper.h"
#include "timetable.h"

class workerClass
{  
  private:
    uint32_t lastTime;
  public:
    void step(); 
    void init();  
    void loop();
};

extern workerClass worker;
