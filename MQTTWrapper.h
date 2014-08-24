#pragma once

#include <functional>
#include <mosquittopp.h>

typedef std::function<void(const struct mosquitto_message *message)> on_message_callback;

class MQTTWrapper: public  mosqpp::mosquittopp {
    public:
        MQTTWrapper(const char *id, const char *host, int port) : 
            mosqpp::mosquittopp(id, false), host(host), port(port) {}

        void begin(on_message_callback cb) {
            mosqpp::lib_init();
            this->cb = cb;
            this->connect(this->host, this->port, 60 /* keepalive */);
            this->subscribe(nullptr, "/ipc/rf24_node");
            this->subscribe(nullptr, "/sensornet/in/#");
        }

        void end(void) {
            mosqpp::lib_cleanup();
        }

        void set_callback(on_message_callback cb) {
            this->cb = cb;
        }

    protected:
        const char *host;
        int port;
        on_message_callback cb;
        void on_message(const struct mosquitto_message *message) {
            this->cb(message);
        }
};
