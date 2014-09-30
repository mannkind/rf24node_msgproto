#pragma once

#include "IMessageProtocol.h" 
#include <functional>
#include <mosquittopp.h>

class MQTTWrapper: public IMessageProtocol, public mosqpp::mosquittopp {
    public:
        MQTTWrapper(std::string id, std::string host, int port);
        void begin(void);
        void end(void);
        void loop(void);
        void send_message(std::string subject, std::string body);
        void set_on_message_callback(on_msg_cb cb);

    protected:
        std::string host;
        int port;
        on_msg_cb cb;
        void on_message(const struct mosquitto_message *message);
};
