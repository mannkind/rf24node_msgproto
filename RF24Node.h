#pragma once

#include "RF24/RF24.h"
#include "RF24Network/RF24Network.h"
#include "MQTTWrapper.h"
#include "RF24Node_types.h"
#include <string>

class RF24Node {
  protected:
    payload_map queued_payloads;

    MQTTWrapper mqtt;
    RF24 radio;
    RF24Network network;

    rf24_pa_dbm_e palevel;
    rf24_datarate_e datarate;
    uint8_t channel;
    uint16_t node_address;
    char key[16];
    bool debug;

    bool write(RF24NetworkHeader& header,const void* message, size_t len);

    void handle_receive_mqtt(const struct mosquitto_message *message);
    void handle_receive_temp(RF24NetworkHeader& header);
    void handle_receive_humidity(RF24NetworkHeader& header);
    void handle_receive_power(RF24NetworkHeader& header);
    void handle_receive_switch(RF24NetworkHeader& header);
    void handle_receive_energy(RF24NetworkHeader& header);
    void handle_receive_moisture(RF24NetworkHeader& header);
    void handle_receive_challenge(RF24NetworkHeader& header);
    void handle_receive_timesync(RF24NetworkHeader& header);
    void handle_send_switch(uint16_t node, std::string payload, time_t challenge);

    void generate_siphash(time_t challenge, unsigned char (&hash)[8]);
    std::string generate_mqtt_topic(RF24NetworkHeader& header);


  public:
    RF24Node(RF24Node_Config config);

    void begin(void);
    void end(void);
    void loop(void);
};
