#include "helper.h"

uint_dmxValue c255(uint8_t proz, uint8_t gamma) {
  uint_dmxValue result = _prozToDim(proz, gamma);
  if ((proz > 0) && (result < DIMMER_START_LEVEL)) {
    result = DIMMER_START_LEVEL;
  }
  return result; 
}

