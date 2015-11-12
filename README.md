# RF42Node_MsgProto

## Quick Start

* Install a MQTT broker - `mosquitto` or `rabbitmq-server`
  * Mosquitto
 
          sudo apt-get install mosquitto mosquitto-clients

  * RabbitMQ + MQTT Plugin
 
          sudo echo "deb http://www.rabbitmq.com/debian/ testing main" >> /etc/apt/sources.list          
          wget https://www.rabbitmq.com/rabbitmq-signing-key-public.asc
          sudo apt-key add rabbitmq-signing-key-public.asc
          rm rabbitmq-signing-key-public.asc
          sudo apt-get update
          sudo apt-get install rabbitmq-server
          sudo rabbitmq-plugins enable rabbitmq_mqtt

* Install `libssl-dev` and `libc-ares-dev` as needed for the MQTT client

          sudo apt-get install libssl-dev libc-ares-dev

* Clone this repository and compile

          git clone https://github.com/mannkind/RF24Node_MsgProto.git
          cd RF24Node_MsgProto
          git submodule update --init --recursive
  
* Compile RF24Node\_MsgProto

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
    
* Start RF24Node\_MsgProto

        sudo ~/RF24Node_MsgProto/RF24Node_MsgProto

  Options include...
  
      --verbose, -v : defaults to false  
      --node, -n : defaults to 0  
      --datarate, -d : defaults to RF24_250KBPS   
      --palevel, -p : defaults to RF24_PA_MAX  
      --channel, -c : defaults to 0x4c  
      --key, -k : defaults to "0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c 0x0d 0x0e 0x0f"  
      --msgproto_type: "MQTT" or "AMQP"; defaults to "MQTT"
      --mqtt_host: defaults to "localhost"
      --mqtt_port: defaults to 1883
      --tls_ca_file: File containing certificates for CA verification (MQTT only)
      --tls_insecure_mode: Don't attempt to verify CA certificate
      --tls_cert_file: TLS certificate file (MQTT only)
      --tls_key_file: TLS private key (MQTT only)
      --amqp_connstr: defaults to "localhost"

# Notice - Unmaintained

Unmaintained; I discovered MySensors and was able to replace RF24Node_MsgProto by utilizing a MySensors ESP8266/MQTT Gateway.
