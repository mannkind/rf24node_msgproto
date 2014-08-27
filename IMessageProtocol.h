#pragma once 

#include <string>
#include <functional>

typedef std::function<void(std::string, std::string)> on_msg_cb;

class IMessageProtocol {
    public:
        virtual void begin(void) { };
        virtual void end(void) { };
        virtual void loop(void) { };
        virtual void send_message(std::string subject, std::string body) { };
        virtual void set_on_message_callback(on_msg_cb cb) { };
};
