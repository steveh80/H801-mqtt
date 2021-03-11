# H801-mqtt
MQTT enabled ESP8266 firmware for the famous H801

This firmware enables the H801 to subscribe to dim commands via MQTT.
The main advantage is, that MQTT supports retaining of messages. This makes sure that the H801 gets commands even in situations where the wifi is bad. 
The H801 tries to reconnect to the wifi/mqtt broker and collects the retained dim command. This means that the dim command will be delivered (maybe a bit later) and won't get lost.

## Installation
* Compile the firmware using VSCode.
* Upload it to your H801
* The H801 will open a wifi network named H801
* Connect to that wifi and enter your wifi credentials, your mqtt broker, mqtt credentials and the device name the H801 should use from now on.

## MQTT 
The H801 will ...
* give its connection state to the topic H801/device_name/connected (1 or 0)
* subscribe to the topic H801/device_name/channel/1 to channel/5

### Message protocol
The H801 takes a json package for each channel. Some commands require multiple channels (eg. RGB), in that case you have to send the message to the first channel. 

Example: If you are using channel 1 to 3 for and RGB LED, send your message only to channel 1.

#### Parameters:
* bri - brightness in percent
* mode - Type of message. Has to be one of single, rgb, cct (tunable white), lumitech-cct (special mode for tunable white in loxone lumitech format)
* colortemp - Color temperature in Kelvin. Only used in mode "cct". 
* rgb - RGB value in loxone format (BBBGGGRRR, each section would be 0-100). Example "100000000" would be 100% blue, "50" would be 50% red. (loxone format: R + G * 1000 + B * 1000000)
* speed - Optional. A factor for fading speed. Integer 0-255 (255 no fading, 1 fast, 99 slow, +100 4-times faster, +200 8-times faster)
* curve - Optional. Dimming curve. Integer (0 - linear, 1-3 logarithmic curve)

Example messages:
```
{
    "bri": 0, // 0 means off
    "mode": "single",
    "speed": 4,
    "curve": 2
}
```

```
{
    "bri": 20,
    "mode": "cct", // tunable white (ww/cw)
    "colortemp": "3000"
}
```

```
{
    "mode": "rgb",
    "rgb": "100000000" // blue
}
```

```
{
    "mode": "lumitech-cct",
    "lumitech": "200252700" // 25% at 2700K
}
```

## Links
* Find more about how to use this project with Loxone here: https://www.loxwiki.eu/pages/viewpage.action?pageId=13307747#RGBWWWLANLEDDimmermitH801(ESP8266)-EinenochganzandereFirmware:


## Thanks to

This project was highly influenced by the awesome work of:
* Robert Lechners UDPtoDMX https://github.com/LechnerRobert/UDPtoDMX
* Stefan Brüns https://github.com/StefanBruens/ESP8266_new_pwm
