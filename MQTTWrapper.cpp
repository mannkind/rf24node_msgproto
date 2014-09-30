#include "MQTTWrapper.h"

MQTTWrapper::MQTTWrapper(std::string id, std::string host, int port) : 
    mosqpp::mosquittopp(id.c_str(), false), host(host), port(port) {}

void MQTTWrapper::begin(void) {
    mosqpp::lib_init();
    this->connect(this->host.c_str(), this->port, 60 /* keepalive */);
    this->subscribe(nullptr, "/ipc/rf24_node");
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

void MQTTWrapper::on_message(const struct mosquitto_message *message) {
    std::string subject(message->topic);
    std::string body(reinterpret_cast<char *>(message->payload));
    this->cb(subject, body);
}
