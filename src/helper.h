#include <Arduino.h>
#include <stdint.h>
#include "types.h"
#include "timetable.h"

inline uint32_t timeDiff(uint32_t from, uint32_t to) {
  return to - from;
}

uint_dmxValue c255(uint8_t proz, uint8_t gamma);