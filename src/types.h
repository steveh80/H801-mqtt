#define ADRUINO 10805
#include <WString.h>

#define DMX_SIZE 5

typedef uint16_t  uint_dmxChannel;
#define QUEUESIZE DMX_SIZE
typedef uint16_t  uint_dmxValue;
typedef uint32_t uint_times;
#define maxValue 5000
#define simulatorUpdateMillis 10
#define DIMMER_START_LEVEL 1

#define RefreshTime 10000
#define RefreshBaseTime 11090    /*(1109 * RefreshTime / 10000)  * 10  */  
