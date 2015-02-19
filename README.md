# RF42Node_MsgProto

## Quick Start

* Install a MQTT broker, `mosquitto` is simple and great

          sudo apt-get install mosquitto mosquitto-clients

* Install `libssl-dev` it's needed for the MQTT client

          sudo apt-get install libssl-dev

* Clone this repository and compile

          git clone https://github.com/mannkind/RF24Node_MsgProto.git
          cd RF24Node_MsgProto
          make

* Enable SPI via raspi-config

          sudo raspi-config

  * Select: 8 - Advanced Options
  * Select: A6 - SPI
    * Answer: Yes
    * Answer: Yes
  * Select: Finish

  * Load SPI module

          sudo modprobe spi-bcm2708
    
* Start RF24Node_MsgProto

        sudo ~/RF24Node_MsgProto/RF24Node_MsgProto

  Options include...
  
      --verbose, -v : defaults to false  
      --node, -n : defaults to 0  
      --datarate, -d : defaults to RF24_250KBPS   
      --palevel, -p : defaults to RF24_PA_MAX  
      --channel, -c : defaults to 0x4c  
      --key, -k : defaults to "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f"  
