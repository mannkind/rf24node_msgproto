#include "MQTTWrapper.h"
#include <unistd.h>

MQTTWrapper::MQTTWrapper(std::string id, std::string host, int port) : 
    mosqpp::mosquittopp(id.c_str(), false), host(host), port(port) {}

void MQTTWrapper::begin(void) {
    mosqpp::lib_init();
    printf("Connecting to MQTT\n");
    this->connect(this->host.c_str(), this->port, 60 /* keepalive */);
    this->subscribe(nullptr, "/sensornet/in/#");
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

void MQTTWrapper::on_disconnect(int mid) {
    printf("Disconnecting from MQTT\n");
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
