#include "MQTTWrapper.h"
#include <unistd.h>

MQTTWrapper::MQTTWrapper(std::string id, std::string host, int port, std::string tls_ca_file, std::string tls_cert_file, std::string tls_key_file, bool tls_insecure_mode) : 
    mosqpp::mosquittopp(id.c_str(), true), host(host), port(port), tls_ca_file(tls_ca_file), tls_cert_file(tls_cert_file), tls_key_file(tls_key_file), tls_insecure_mode(tls_insecure_mode) {}

void MQTTWrapper::begin(void) {
    mosqpp::lib_init();

    if (!this->tls_ca_file.empty() && !this->tls_cert_file.empty() && !this->tls_key_file.empty()) {
        printf("Enabling TLS mode.\n");
        this->tls_set(this->tls_ca_file.c_str(), NULL, this->tls_cert_file.c_str(), this->tls_key_file.c_str());
        this->tls_insecure_set(this->tls_insecure_mode);
    }

    printf("Connecting to MQTT\n");
    this->connect(this->host.c_str(), this->port, 60 /* keepalive */);
}

void MQTTWrapper::end(void) {
    mosqpp::lib_cleanup();
}

void MQTTWrapper::loop(void) {
    mosqpp::mosquittopp::loop();
}

void MQTTWrapper::send_message(std::string subject, std::string body) {
    this->publish(nullptr, subject.c_str(), body.length(), body.c_str(), 0); 
}

void MQTTWrapper::set_on_message_callback(on_msg_cb cb) {
    this->cb = cb;
}

void MQTTWrapper::on_connect(int rc) {
    if (rc == 0) {
        printf("Connected to MQTT\n");
        this->subscribe(nullptr, "/sensornet/in/#");
    } else {
        printf("Connection error; reason code %d\n", rc);
    }
}

void MQTTWrapper::on_log(int level, const char *str) {
    //printf("LOG [%d] %s\n", level, str);
}

void MQTTWrapper::on_disconnect(int mid) {
    printf("Disconnected from MQTT\n");
    this->end();
    sleep(15);
    this->begin();
}

void MQTTWrapper::on_message(const struct mosquitto_message *message) {
    auto subject = std::string(message->topic);
    auto body = message->payloadlen > 0 ? 
        std::string(reinterpret_cast<char *>(message->payload)) : std::string("");
    this->cb(subject, body);
}
