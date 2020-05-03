#include "dimmer.h"
#include "types.h"

#define onOffthreshold 10

void Dimmer::init() {
    queue.init();
    worker.init();
}

void Dimmer::loop() {
    worker.loop();
}


void Dimmer::dimChannel(uint8_t channel, uint8_t brightness, uint8_t speed, uint8_t curve, uint8_t onOffSpeed) {
    bool isOnOff = queue.add(channel, speed, brightness, curve, true) > onOffthreshold;
    queue.update(channel, (onOffSpeed == 0 ? speed : onOffSpeed), isOnOff);
}

void Dimmer::dimCCT(uint8_t startChannel, uint8_t brightness, uint16_t colorTemp, uint8_t speed, uint8_t curve, uint8_t onOffSpeed) {
    uint8_t d1, d2, dmax = 0;
    onOffSpeed = (onOffSpeed == 0 ? speed : onOffSpeed);

    // consider gamma value in percentage to make sure the temperature is the same at each dim level and according to gamma
    uint16_t percent = _prozToDim(brightness, curve);

    // calculate percentage for each channel
    uint16_t percentWW = (percent*(6500 - colorTemp))/3800;
    uint16_t percentCW = (percent*(colorTemp - 2700))/3800;

    d1 = queue.add(startChannel, speed, percentWW, 0, false); //WW
    d2 = queue.add(startChannel + 1, speed, percentCW, 0, false); //CW
    dmax = max(d1, d2);

    bool isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold);
    queue.update(startChannel, scaleSpeed(onOffSpeed, d1, dmax), isOnOff);
    queue.update(startChannel + 1, scaleSpeed(onOffSpeed, d1, dmax), isOnOff); 
}

void Dimmer::dimRGB(uint8_t startChannel, uint32_t rgb, uint8_t speed, uint8_t curve, uint8_t onOffSpeed) {
    uint8_t d1, d2, d3, dmax = 0;

    d1 = queue.add(startChannel, speed, rgb % 1000, curve, true); //RED
    rgb = rgb / 1000;
    d2 = queue.add(startChannel + 1, speed, rgb % 1000, curve, true); //GREEN
    rgb = rgb / 1000;
    d3 = queue.add(startChannel + 2, speed, rgb % 1000, curve, true); //BLUE
    
    bool isOnOff = (d1 > onOffthreshold) || (d2 > onOffthreshold) || (d3 > onOffthreshold);
    if (!isOnOff) {
        onOffSpeed = speed;
        isOnOff = true; 
    }
    dmax = max(d1, d2);
    dmax = max(d3, dmax);

    queue.update(startChannel, scaleSpeed(onOffSpeed, d1, dmax), isOnOff);
    queue.update(startChannel + 1, scaleSpeed(onOffSpeed, d2, dmax), isOnOff);
    queue.update(startChannel + 2, scaleSpeed(onOffSpeed, d3, dmax), isOnOff);
}


uint8_t Dimmer::scaleSpeed(uint8_t speed, uint8_t dimDelta, uint8_t maxDimDelta) {
    if ((dimDelta == 0) || (speed == 255)) {
        return speed; // already at max speed or no delta at all?
    }  
    
    uint16_t ret = speed % 100; // remove speed multipier, ret should be between 1 and 99 from here on
    ret = ret * maxDimDelta / dimDelta;
    if (speed < 200) { 
        ret = min(ret, (uint16_t) 99);
    } else {
        ret = min(ret, (uint16_t) 54);
    }
    return ret + (speed - (speed % 100));
}

Dimmer dimmer;