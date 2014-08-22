# Ripcord

Ripcord is yet another system for home automation; a system to monitor and control remote sensors and devices. The "brains" of the operation is a Raspberry Pi, and the *nodes* are Arduino-like devices that communicate with the RPi via RF24L01+ radios.

The components of Ripcord communicate via MQTT.

Example plugins for Ripcord can be found at my [Ripcord Plugins](https://github.com/mannkind/Ripcord-Plugins) repository, and example Arduino sketches can be found on my [Repository](https://github.com/search?q=%40mannkind+RF24Node-) page.

## Technology

The web-application utilizies Python, Flask, Bootstrap, and AngularJS.

## Thanks

Originally I dreamed up a terribly-hacky plugin-like architecture for message passing using *yapsy* and it worked, kinda. Shortly after that I discovered MQTT and [*xbee2mqtt*](https://raw.github.com/xoseperez/xbee2mqtt).

rf24mqtt, bt2mqtt, and mqtt2sqlite code is loosely based *xbee2mqtt*

# Notice - Unmaintained

Unmaintained; I discovered OpenHAB and was able to replace 90% of Ripcord and Ripcord-Plugins with it. The only thing I still use is the RF24MQTT gateway (rf24mqtt and RF24Node.cpp)
