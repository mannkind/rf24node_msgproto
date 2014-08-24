/*
 *
 *   NRF24L01+ to MQTT gateway
 *   Copyright (C) 2013 Dustin Brewer
 *   License: MIT
 */
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>

#include "csiphash.c"
#include "StringSplit.h"
#include "RF24Node.h"

const size_t max_packet_size = 32 - sizeof(RF24NetworkHeader);

RF24Node::RF24Node(RF24Node_Config config) : 
    mqtt(config.id.c_str(), config.host.c_str(), config.port), 
    radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ), network(this->radio), 
    palevel(config.palevel), datarate(config.datarate), 
    channel(config.channel), node_address(config.node_address),
    debug(config.debug) {
      std::copy(std::begin(config.key), std::end(config.key), std::begin(this->key));
};

void RF24Node::begin(void) {
    this->mqtt.begin([this](const struct mosquitto_message *message) { this->handle_receive_mqtt(message); });

    this->radio.begin();
    this->network.begin(this->channel, this->node_address);
    this->radio.setDataRate(this->datarate);
    this->radio.setPALevel(this->palevel);
    this->radio.printDetails();
}

void RF24Node::end(void) {
    this->mqtt.end();
}

void RF24Node::loop(void) {
    this->network.update();
    while (this->network.available()) {
        RF24NetworkHeader header;
        this->network.peek(header);

        switch (header.type) {
            case PKT_POWER:
                this->handle_receive_power(header);
                break;
            case PKT_TEMP:
                this->handle_receive_temp(header);
                break;
            case PKT_HUMID:
                this->handle_receive_humidity(header);
                break;
            case PKT_SWITCH:
                this->handle_receive_switch(header);
                break;
            case PKT_MOISTURE:
                this->handle_receive_moisture(header);
                break;
            case PKT_ENERGY:
                this->handle_receive_energy(header);
                break;
            case PKT_RGB:
                // Not Implemented
                break;
            case PKT_TIME:
                this->handle_receive_timesync(header);
                break;
            case PKT_CHALLENGE:
                this->handle_receive_challenge(header);
                break;
            default:
                break;
        }
    }
    this->mqtt.loop();
}

bool RF24Node::write(RF24NetworkHeader& header,const void* message, size_t len) {
    const uint8_t max_retries = 1;
    bool ok = false;

    uint8_t retries = max_retries;
    uint8_t node_delay = (((this->node_address % 6) + 1) * 2) + 3;
    while (!ok && retries-- > 0) {
        //delay(((max_retries - retries - 1) * node_delay));
        ok = this->network.write(header, message, len);
    }
    return ok;
}

/**
 * Upon receiving a command via the C++/Python IPC topic, queue the message
 */
void RF24Node::handle_receive_mqtt(const struct mosquitto_message *message) {
    if (this->debug) printf("Received '%s' via topic '%s' from MQTT\n", message->topic, reinterpret_cast<char *>(message->payload));
    std::string received_topic(message->topic);

    std::vector<std::string> elements = split(message->topic, '/');
    uint16_t to_node = std::stoul("0" + elements[3], nullptr, 0);
    uint8_t type_command = std::stoi(elements[4], nullptr);
    uint8_t type = type_command % 64;

    if (type_command < 64) {
        // Asking for sensor data is unsupported 
        return;
    }
        
    std::string queue_payload((reinterpret_cast<char *>(message->payload)));
    this->queued_payloads[to_node][type] = queue_payload;

    if (debug) printf("Queuing: '%s' for node 0%o, payload type %d\n", queue_payload.c_str(), to_node, type);

    pkt_challenge_t payload;
    payload.type = type;
    RF24NetworkHeader header(to_node, PKT_CHALLENGE);
    this->write(header, &payload, sizeof(payload));
}

/*
 * Upon receiving a header specifying a challenge response, store the challenge
 */
void RF24Node::handle_receive_challenge(RF24NetworkHeader& header) {
    if (debug) printf("Handling challenge request for node 0%o.\n", header.from_node);

    // Read the challenge request response
    pkt_challenge_t payload;
    this->network.read(header, &payload, sizeof(payload));

    switch (payload.type) {
        case PKT_SWITCH:
            this->handle_send_switch(header.from_node, this->queued_payloads[header.from_node][payload.type], payload.challenge);
            break;
        case PKT_RGB:
            // Not implemented
            break;
    }

}

/*
 * Upon receiving a header specifying a timesync request, send the time to the node
 */
void RF24Node::handle_receive_timesync(RF24NetworkHeader& header) {
    if (debug) printf("Handling timesync request for node 0%o.\n", header.from_node);

    // Read in the packet; not used
    this->network.read(header, nullptr, 0);

    // Set the current timestamp 
    pkt_time_t t = { time(0) };

    // Send the packet (timestamp) to the desired node
    RF24NetworkHeader new_header(header.from_node, PKT_TIME);
    this->write(new_header, &t, sizeof(t));
}

/*
 * Publish temps on MQTT 
 */
void RF24Node::handle_receive_temp(RF24NetworkHeader& header) {
    pkt_temp_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << (double)(payload.temp / 10.0);

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Temp: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

/*
 * Publish humidity on MQTT 
 */
void RF24Node::handle_receive_humidity(RF24NetworkHeader& header) {
    pkt_humid_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << ((double)(payload.humidity / 10.0));

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Humidity: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

/*
 * Publish power on MQTT 
 */
void RF24Node::handle_receive_power(RF24NetworkHeader& header) {
    pkt_power_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.battery << "|" << payload.solar << "|" << payload.vcc << "|" << payload.vs;

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Power: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

/*
 * Publish moisture on MQTT 
 */
void RF24Node::handle_receive_moisture(RF24NetworkHeader& header) {
    pkt_moisture_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.moisture;

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Moisture: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

/*
 * Publish energy on MQTT 
 */
void RF24Node::handle_receive_energy(RF24NetworkHeader& header) {
    pkt_energy_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.energy;

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Energy: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

/*
 * Publish switch on MQTT 
 */
void RF24Node::handle_receive_switch(RF24NetworkHeader& header) {
    pkt_switch_t payload;
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.state << "|" << payload.timer;

    std::string topic = this->generate_mqtt_topic(header);
    std::string value = s_value.str();

    if (debug) printf("Republishing Switch: %s:%s\n", topic.c_str(), value.c_str());
    this->mqtt.publish(nullptr, topic.c_str(), value.length(), value.c_str(), 0);
}

void RF24Node::handle_send_switch(uint16_t node, std::string queued_payload, time_t challenge) {
    std::vector<std::string> elements = split(queued_payload.c_str(), '|');

    pkt_switch_t payload;
    payload.id = std::stoi(elements[0], nullptr, 0);
    payload.state = std::stoi(elements[1], nullptr, 0);
    payload.timer = std::stoi(elements[2], nullptr, 0);
    this->generate_siphash(challenge, payload.hash);

    if (debug) {
        printf("Republishing Switch Command: 0%o:%s\n", node, queued_payload.c_str());
        printf("-- payload: %d, %d, %d\n", payload.id, payload.state, payload.timer);
        printf("-- using siphashed (%d, %d, %d, %d, %d, %d, %d, %d) challenge %lu\n",
        payload.hash[0], payload.hash[1], payload.hash[2], payload.hash[3], payload.hash[4],
        payload.hash[5], payload.hash[6], payload.hash[7], challenge);
    }

    // Send a challenge request to the appropriate node
    RF24NetworkHeader header(node, PKT_SWITCH);
    this->write(header, &payload, sizeof(payload));
}

std::string RF24Node::generate_mqtt_topic(RF24NetworkHeader& header) {
    char from_node_oct[5];
    sprintf(from_node_oct, "%o", header.from_node);

    std::stringstream s_topic;
    s_topic << "/sensornet/out/" << from_node_oct << "/" << std::to_string(header.type);

    return s_topic.str();
}

void RF24Node::generate_siphash(time_t challenge, unsigned char (&hash)[8]) {
    // Convert the challenge into a byte array
    char challenge_array[4];
    for(int i = 0; i <= 3; i++) {
        challenge_array[i] = (challenge >> (8 * i) ) & 0xFF;
    }

    // Generate the hash
    uint64_t siphash = siphash24(challenge_array, sizeof(challenge_array), this->key);

    // Convert the siphash into a byte array
    for(unsigned int i = 0; i < sizeof(hash); i++) {
        hash[i] = (siphash >> (8 * i) ) & 0xFF;
    }
}
