#pragma once

#include <string>

#include "RF24Network/RF24Network.h"
#include "IMessageProtocol.h"
#include "IRadioNetwork.h"
#include "RF24Node_types.h"

class IMessageProtocol;
class IRadioNetwork;

class RF24Node {
    protected:
        payload_map queued_payloads;

        IMessageProtocol& msg_proto;
        IRadioNetwork& network;

        bool debug;
        std::vector<char> key;

        bool write(RF24NetworkHeader& header, const void* message, size_t len);

        void handle_receive_message(std::string subject, std::string body);
        void handle_receive_temp(RF24NetworkHeader& header);
        void handle_receive_humidity(RF24NetworkHeader& header);
        void handle_receive_power(RF24NetworkHeader& header);
        void handle_receive_switch(RF24NetworkHeader& header);
        void handle_receive_energy(RF24NetworkHeader& header);
        void handle_receive_rgb(RF24NetworkHeader& header);
        void handle_receive_moisture(RF24NetworkHeader& header);
        void handle_receive_challenge(RF24NetworkHeader& header);
        void handle_receive_timesync(RF24NetworkHeader& header);
        void handle_send_switch(uint16_t node, std::string payload, time_t challenge);
        void handle_send_rgb(uint16_t node, std::string payload, time_t challenge);

        std::vector<uint8_t> generate_siphash(uint16_t node, time_t challenge);
        std::string generate_msg_proto_subject(RF24NetworkHeader& header);


    public:
        RF24Node(IRadioNetwork& _network, IMessageProtocol& _msg_proto, std::vector<char>);

        void begin(void);
        void end(void);
        void loop(void);

        void set_debug(bool _debug) {
            this->debug = _debug;
        }
};
