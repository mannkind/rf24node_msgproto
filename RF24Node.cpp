/*
 *
 *   NRF24L01+ to Message Protocol gateway
 *   Copyright (C) 2013 Dustin Brewer
 *   License: MIT
 */
#include <algorithm>
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>

#include "csiphash.c"
#include "StringSplit.h"
#include "IMessageProtocol.h"
#include "IRadioNetwork.h"
#include "RF24Node.h"

RF24Node::RF24Node(IRadioNetwork& _network, IMessageProtocol& _msg_proto, std::vector<char> _key) : 
  msg_proto(_msg_proto), network(_network), key(_key) { }

void RF24Node::begin(void) {
    this->msg_proto.set_on_message_callback([this](std::string subject, std::string body) { this->handle_receive_message(subject, body); });
    this->msg_proto.begin();

    this->network.begin();
}

void RF24Node::end(void) {
    this->msg_proto.end();
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
                this->handle_receive_rgb(header);
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
    this->msg_proto.loop();
}

bool RF24Node::write(RF24NetworkHeader& header, const void* message, size_t len) {
    const auto max_retries = 1;
    auto ok = false;

    auto retries = max_retries;
    while (!ok && retries-- > 0) {
        ok = this->network.write(header, message, len);
    }
    return ok;
}

/**
 * Upon receiving a command via the C++/Python IPC topic, queue the message
 */
void RF24Node::handle_receive_message(std::string subject, std::string body) {
    if (this->debug) printf("Received '%s' via topic '%s' from MQTT\n", subject.c_str(), body.c_str());

    auto elements = split(subject, '/');
    uint16_t to_node = std::stoul("0" + elements[3], nullptr, 0);
    uint8_t type_command = std::stoi(elements[4], nullptr);
    uint8_t type = type_command % 64;

    // Asking for sensor data is unsupported 
    if (type_command < 64) {
        return;
    }
        
    this->queued_payloads[to_node][type] = body;

    if (this->debug) printf("Queuing: '%s' for node 0%o, payload type %d\n", body.c_str(), to_node, type);

    auto payload = pkt_challenge_t { 0, type };
    RF24NetworkHeader header(to_node, PKT_CHALLENGE);
    this->write(header, &payload, sizeof(payload));
}

/*
 * Upon receiving a header specifying a challenge response, store the challenge
 */
void RF24Node::handle_receive_challenge(RF24NetworkHeader& header) {
    if (this->debug) printf("Handling challenge request for node 0%o.\n", header.from_node);

    // Read the challenge request response
    auto payload = pkt_challenge_t();
    this->network.read(header, &payload, sizeof(payload));

    if (this->queued_payloads.find(header.from_node) == this->queued_payloads.end() ||
        this->queued_payloads[header.from_node].find(payload.type) == this->queued_payloads[header.from_node].end()) {
        if (this->debug) printf("No queued payload for for node 0%o of payload type %d.\n", header.from_node, payload.type);
        return;
    }
    
    switch (payload.type) {
        case PKT_SWITCH:
            this->handle_send_switch(header.from_node, this->queued_payloads[header.from_node][payload.type], payload.challenge);
            break;
        case PKT_RGB:
            this->handle_send_rgb(header.from_node, this->queued_payloads[header.from_node][payload.type], payload.challenge);
            break;
    }

    this->queued_payloads[header.from_node].erase(payload.type);
}

/*
 * Upon receiving a header specifying a timesync request, send the time to the node
 */
void RF24Node::handle_receive_timesync(RF24NetworkHeader& header) {
    if (this->debug) printf("Handling timesync request for node 0%o.\n", header.from_node);

    // Read in the packet; not used
    this->network.read(header, nullptr, 0);

    // Set the current timestamp 
    auto payload = pkt_time_t { time(0) };

    // Send the packet (timestamp) to the desired node
    RF24NetworkHeader new_header(header.from_node, PKT_TIME);
    this->write(new_header, &payload, sizeof(payload));
}

/*
 * Publish temps on MQTT 
 */
void RF24Node::handle_receive_temp(RF24NetworkHeader& header) {
    auto payload = pkt_temp_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << (double)(payload.temp / 10.0);

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Temp: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish humidity on MQTT 
 */
void RF24Node::handle_receive_humidity(RF24NetworkHeader& header) {
    auto payload = pkt_humid_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << ((double)(payload.humidity / 10.0));

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Humidity: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish power on MQTT 
 */
void RF24Node::handle_receive_power(RF24NetworkHeader& header) {
    auto payload = pkt_power_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.battery << "|" << payload.solar << "|" << payload.vcc << "|" << payload.vs;

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Power: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish moisture on MQTT 
 */
void RF24Node::handle_receive_moisture(RF24NetworkHeader& header) {
    auto payload = pkt_moisture_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.moisture;

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Moisture: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish energy on MQTT 
 */
void RF24Node::handle_receive_energy(RF24NetworkHeader& header) {
    auto payload = pkt_energy_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.energy;

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Energy: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish rgb on MQTT 
 */
void RF24Node::handle_receive_rgb(RF24NetworkHeader& header) {
    auto payload = pkt_rgb_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.rgb[0] << "|" << payload.rgb[1] 
        << "|" << payload.rgb[2] << "|" << payload.timer;

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing RGB: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

/*
 * Publish switch on MQTT 
 */
void RF24Node::handle_receive_switch(RF24NetworkHeader& header) {
    auto payload = pkt_switch_t();
    this->network.read(header, &payload, sizeof(payload));

    std::stringstream s_value;
    s_value << payload.id << "|" << payload.state << "|" << payload.timer;

    auto topic = this->generate_msg_proto_subject(header);
    auto value = s_value.str();

    if (this->debug) printf("Republishing Switch: %s:%s\n", topic.c_str(), value.c_str());
    this->msg_proto.send_message(topic, value);
}

void RF24Node::handle_send_rgb(uint16_t node, std::string queued_payload, time_t challenge) {
    auto siphash = this->generate_siphash(challenge);
    auto elements = split(queued_payload.c_str(), '|');

    auto payload = pkt_rgb_t();
    payload.id = std::stoi(elements[0], nullptr, 0);
    payload.rgb[0] = std::stoi(elements[1], nullptr, 0);
    payload.rgb[1] = std::stoi(elements[2], nullptr, 0);
    payload.rgb[2] = std::stoi(elements[3], nullptr, 0);
    payload.timer = std::stoi(elements[4], nullptr, 0);
    std::copy(siphash.begin(), siphash.end(), payload.hash);

    if (this->debug) {
        printf("Republishing RGB Command: 0%o:%s\n", node, queued_payload.c_str());
        printf("-- payload: %d, (%d, %d, %d), %d\n", payload.id, payload.rgb[0], payload.rgb[1], payload.rgb[2], payload.timer);
        printf("-- using siphashed (%d, %d, %d, %d, %d, %d, %d, %d) challenge %lu\n",
        payload.hash[0], payload.hash[1], payload.hash[2], payload.hash[3], payload.hash[4],
        payload.hash[5], payload.hash[6], payload.hash[7], challenge);
    }

    // Send a challenge request to the appropriate node
    RF24NetworkHeader header(node, PKT_SWITCH);
    this->write(header, &payload, sizeof(payload));
}

void RF24Node::handle_send_switch(uint16_t node, std::string queued_payload, time_t challenge) {
    auto siphash = this->generate_siphash(challenge);
    auto elements = split(queued_payload.c_str(), '|');

    auto payload = pkt_switch_t();
    payload.id = std::stoi(elements[0], nullptr, 0);
    payload.state = std::stoi(elements[1], nullptr, 0);
    payload.timer = std::stoi(elements[2], nullptr, 0);
    std::copy(siphash.begin(), siphash.end(), payload.hash);


    if (this->debug) {
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

std::string RF24Node::generate_msg_proto_subject(RF24NetworkHeader& header) {
    char from_node_oct[] = { 0, 0, 0, 0, 0 };
    sprintf(from_node_oct, "%o", header.from_node);

    std::stringstream s_topic;
    s_topic << "/sensornet/out/" << from_node_oct << "/" << std::to_string(header.type);

    return s_topic.str();
}

std::vector<uint8_t> RF24Node::generate_siphash(time_t challenge) {
    // Convert the challenge into a byte array
    auto data = std::vector<char>({ 0, 1, 2, 3 });
    for (auto &d : data) {
        d = (challenge >> (8 * d) ) & 0xFF;
    }

    // Generate the hash
    auto siphash = siphash24(&data[0], data.size(), &this->key[0]);

    // Convert the siphash into a byte array
    auto hash = std::vector<uint8_t>({ 0, 1, 2, 3, 4, 5, 6, 7 });
    for (auto &h : hash) {
        h = (siphash >> (8 * h)) & 0xFF;
    }

    return hash;
}
