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

* Enable SPI by removing the broadcom chipset from the blacklist
  * Edit the raspi-blacklist.conf file

          sudo nano /etc/modprobe.d/raspi-blacklist.conf

    It should contain the following lines

         # blacklist spi and i2c by default (many users don't need them)

         blacklist spi-bcm2708
         blacklist i2c-bcm2708
    
    Add the hashmark in front of `spi-bcm2708` so that it looks like `#blacklist spi-bcm2708`
    
         # blacklist spi and i2c by default (many users don't need them)

         #blacklist spi-bcm2708
         blacklist i2c-bcm2708

  * Crtl+O to save the file
  * Ctrl+X to exit
  

* Reboot the RPi
* Start RF24Node_MsgProto

        ~/RF24Node_MsgProto/RF24Node_MsgProto

  Options include...
  
      --verbose, -v : defaults to false  
      --node, -n : defaults to 0  
      --datarate, -d : defaults to RF24_250KBPS   
      --palevel, -p : defaults to RF24_PA_MAX  
      --channel, -c : defaults to 0x4c  
      --key, -k : defaults to "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f"  
