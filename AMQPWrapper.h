#pragma once

#include "IMessageProtocol.h" 
#include <functional>
#include "libs/amqpcpp/include/AMQPcpp.h"

class AMQPWrapper: public IMessageProtocol {
    public:
        AMQPWrapper(std::string _connstr);
        void begin(void);
        void end(void);
        void loop(void);
        void send_message(std::string subject, std::string body);
        void set_on_message_callback(on_msg_cb cb);
        static int amqp_on_message(AMQPMessage *message);

    protected:
        AMQP amqp;
        AMQPExchange* exchange;
        AMQPQueue* queue;
        on_msg_cb cb;
        void on_message(AMQPMessage *message);
        void on_disconnect(int mid);
};
