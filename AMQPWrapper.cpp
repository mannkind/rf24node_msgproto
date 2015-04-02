#include "AMQPWrapper.h"

AMQPWrapper *cb_obj_wrapper;

AMQPWrapper::AMQPWrapper(std::string _connstr) : amqp(_connstr), exchange(this->amqp.createExchange("RF24NodeEx")), queue(this->amqp.createQueue("RF24Node"))  {
    cb_obj_wrapper = this;
}

void AMQPWrapper::begin(void) {
    printf("Connecting to AMQP\n");
    this->exchange->Declare("RF24NodeEx", "topic");
    this->queue->Declare();
    this->queue->Bind("RF24NodeEx", ".sensornet.in.#");

    this->queue->addEvent(AMQP_MESSAGE, AMQPWrapper::amqp_on_message);
}

void AMQPWrapper::end(void) {
}

void AMQPWrapper::loop(void) {
}

void AMQPWrapper::send_message(std::string subject, std::string body) {
    this->exchange->Publish(body, subject);
}

void AMQPWrapper::set_on_message_callback(on_msg_cb cb) {
    this->cb = cb;
}

void AMQPWrapper::on_disconnect(int mid) {
    printf("Disconnecting from AMQP\n");
    this->end();
    sleep(15);
    this->begin();
}

void AMQPWrapper::on_message(AMQPMessage *message) {
    uint32_t j = 0;
    auto subject = message->getRoutingKey();
    auto body = std::string(message->getMessage(&j), j);
    this->cb(subject, body);
}

int AMQPWrapper::amqp_on_message(AMQPMessage *message) {
    cb_obj_wrapper->on_message(message);
    return 0;
}
