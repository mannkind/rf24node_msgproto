#pragma once

#include "IMessageProtocol.h" 
#include <functional>
#include <mosquittopp.h>

class MQTTWrapper: public IMessageProtocol, public mosqpp::mosquittopp {
    public:
        MQTTWrapper(std::string id, std::string host, int port, std::string tls_ca_file, std::string tls_cert_file, std::string tls_key_file, bool tls_insecure_mode);
        void begin(void);
        void end(void);
        void loop(void);
        void send_message(std::string subject, std::string body);
        void set_on_message_callback(on_msg_cb cb);

    protected:
        std::string host;
        int port;

        std::string tls_ca_file;
        std::string tls_cert_file;
        std::string tls_key_file;
        bool tls_insecure_mode;

        on_msg_cb cb;
        void on_message(const struct mosquitto_message *message);
        void on_subscribe(int mid, int qos_count, const int *granted_qos);
        void on_disconnect(int mid);
        void on_log(int level, const char *str);
        void on_connect(int rc);
};
