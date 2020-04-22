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
* introduce it self using the topic H801/announce
* subscribe to the topic H801/device_name/commands

For now the commands are using the same protocol as UDPtoDMX:
https://github.com/steveh80/UDPtoDMX/blob/master/docs/protocol.pdf

## Thanks to

This project is highly influenced by the awesome work of:
* Robert Lechners UDPtoDMX https://github.com/LechnerRobert/UDPtoDMX
