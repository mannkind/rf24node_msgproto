/*
 *
 *   NRF24L01+ to MQTT gateway
 *   Copyright (C) 2013 Dustin Brewer
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <getopt.h>

#include "csiphash.c"
#include "RF24/RF24.h"
#include "RF24Network/RF24Network.h"
#include "RF24Node.h"

bool debug = false;
const size_t max_packet_size = 32 - sizeof(RF24NetworkHeader);

RF24Node::RF24Node(RF24Node_Config config) : 
    mqtt(config.id.c_str(), config.host.c_str(), config.port), 
    radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ), network(this->radio), 
    palevel(config.palevel), datarate(config.datarate), 
    channel(config.channel), node_address(config.node_address) {
      std::copy(std::begin(config.key), std::end(config.key), std::begin(this->key));
};

void RF24Node::begin(void) {
    this->mqtt.begin([this](const struct mosquitto_message *message) { this->on_message(message); });

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
            case 'C':
                this->handle_challenge(header);
                break;
            case 'T':
                this->handle_timesync(header);
                break;
            default:
                this->handle_republish(header);
                break;
        }
    }
    this->mqtt.loop();
}

bool RF24Node::write(RF24NetworkHeader& header,const void* message, size_t len) {
    bool ok = false; 
    uint8_t retries = 5;
    while (!ok && --retries > 0) {
        delay(this->network.routeTimeout);
        ok = this->network.write(header, message, len);
    }
    return ok;
}

/**
 * Upon receiving a command via the C++/Python IPC topic, queue the message
 */
void RF24Node::on_message(const struct mosquitto_message *message) {
    if (debug) printf("Received '%s' via topic '%s' from MQTT\n", message->topic, reinterpret_cast<char *>(message->payload));
    if(strcmp(message->topic, "/ipc/rf24_node")){ 
        if (debug) printf("Ignoring because the topic wasn't '/ipc/rf24_node'\n");
        return; 
    }

    std::vector<std::string> elements = split(reinterpret_cast<char *>(message->payload), '~');
    if (elements.size() != 2) { 
        if (debug) printf("Ignoring because splitting the payload by '~' didn't result in two strings.\n");
        return; 
    }

    uint16_t to_node = std::stoul(elements[0], nullptr, 0);
    uint8_t command = std::stoul(elements[1], nullptr, 0);

    // Queue the message
    if (debug) printf("Queuing: '%s' for node 0%o\n", elements[1].c_str(), to_node);
    this->queued_commands[to_node] = command;

    // Send a challenge request to the appropriate node
    RF24NetworkHeader header(to_node, 'C');
    this->write(header, "", 0);
}

/*
 * Upon receiving a header specifying a challenge response, store the challenge
 */
void RF24Node::handle_challenge(RF24NetworkHeader& header) {
    if (debug) printf("Handling challenge request for node 0%o.\n", header.from_node);

    // Read the challenge request response
    C_request_message_t c_req;;
    this->network.read(header, &c_req, sizeof(c_req));

    // Read in the stored command
    uint8_t command = this->queued_commands[header.from_node];

    // Determine the desired node; setup the packet
    if (c_req.challenge == 0) { 
      if (debug) printf("Failed to rebroadcast; challenge was zero or command was empty\n");
      return; 
    }

    // Send the command and siphash, and send it on its way
    C_command_message_t c_msg;
    c_msg.command = command;
    this->generateSipHash(c_req.challenge, c_msg.hash);

    if (debug) printf("Rebroadcasting: '%d' using siphashed (%i%i%i%i%i%i%i%i) challenge %lu to node 0%o\n",
        c_msg.command, c_msg.hash[0], c_msg.hash[1], c_msg.hash[2], c_msg.hash[3], c_msg.hash[4],
        c_msg.hash[5], c_msg.hash[6], c_msg.hash[7], c_req.challenge, header.from_node);

    // Send the packet (challenge and message) to the desired node
    RF24NetworkHeader new_header(header.from_node);
    this->write(new_header, &c_msg, sizeof(c_msg));

    // Clear queued message
    this->queued_commands.erase(header.from_node);
}

/*
 * Upon receiving a header specifying a timesync request, send the time to the node
 */
void RF24Node::handle_timesync(RF24NetworkHeader& header) {
    if (debug) printf("Handling timesync request for node 0%o.\n", header.from_node);

    // Read in the packet; not used
    this->network.read(header, nullptr, 0);

    // Set the current timestamp 
    T_message_t t = { time(0) };

    // Send the packet (timestamp) to the desired node
    RF24NetworkHeader new_header(header.from_node, 'T');
    this->write(new_header, &t, sizeof(t));
}

/*
 * Upon receiving a header with a unhandled type, just republish on MQTT
 */
void RF24Node::handle_republish(RF24NetworkHeader& header) {
    // Read in the packet
    // cppcheck-suppress negativeIndex
    char message[max_packet_size]; 
    this->network.read(header, &message, max_packet_size);

    // Republish to MQTT
    if (debug) printf("Republishing: %s\n", message);
    this->mqtt.publish(nullptr, "/ipc/rf24mqtt", sizeof(message), message, 0);
}


void RF24Node::generateSipHash(time_t challenge, unsigned char (&hash)[8]) {
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

int main(int argc, char *argv[]) {
    RF24Node_Config config = {
        "RF24Node",
        "localhost",
        1883,
        RF24_PA_MAX,
        RF24_250KBPS,
        0x4c,
        00,
        { 
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        }
    };

    static struct option long_options[] = {
      {"datarate", required_argument, nullptr, 'd'},
      {"palevel", required_argument, nullptr, 'p'},
      {"channel", required_argument, nullptr, 'c'},
      {"node", required_argument, nullptr, 'n'},
      {"key", required_argument, nullptr, 'k'},
      {"verbose", no_argument, nullptr, 'v'},
      {nullptr, 0, nullptr, 0}
    };

    std::vector<std::string> key_elements;
    int opt = 0, long_index = 0;
    while ((opt = getopt_long(argc, argv,"n:c:d:p:k:v", long_options, &long_index )) != -1) {
        switch (opt) {
            case 'n' : 
                if (debug) printf ("option -n with value '%s'\n", optarg);
                config.node_address = std::stoul(optarg, nullptr, 0);
                break;
            case 'c' : 
                if (debug) printf ("option -c with value '%s'\n", optarg);
                config.channel = std::stoul(optarg, nullptr, 0);
                break;
            case 'd':
                if (debug) printf ("option -d with value '%s'\n", optarg);
                config.datarate = static_cast<rf24_datarate_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'p':
                if (debug) printf ("option -p with value '%s'\n", optarg);
                config.palevel = static_cast<rf24_pa_dbm_e>(std::stoi(optarg, nullptr, 0));
                break;
            case 'k':
                if (debug) printf ("option -k with value '%s'\n", optarg);
                key_elements = split(optarg, ' ');

                for (unsigned int i = 0; i < key_elements.size(); i++) {
                    if (debug) printf ("'%s'\n", key_elements[i].c_str());
                    config.key[i] = std::stoul(key_elements[i].c_str(), nullptr, 0);
                }
                break;
            case 'v':
                if (debug) printf ("option -v\n");
                debug = true;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    RF24Node node(config);

    node.begin();
    while(true) {
        node.loop();
    }
    node.end();

    return 0;
}
