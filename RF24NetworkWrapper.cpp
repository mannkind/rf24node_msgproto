#include "RF24NetworkWrapper.h"

RF24NetworkWrapper::RF24NetworkWrapper(uint8_t _channel, uint16_t _node_address, rf24_pa_dbm_e _palevel, rf24_datarate_e _datarate) :
  channel(_channel), node_address(_node_address), palevel(_palevel), datarate(_datarate),
  radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ), network(this->radio) { }

void RF24NetworkWrapper::begin(void) {
    this->radio.begin();
    this->network.begin(this->channel, this->node_address);
    this->radio.setDataRate(this->datarate);
    this->radio.setPALevel(this->palevel);
    this->radio.printDetails();
}

void RF24NetworkWrapper::update(void) {
    this->network.update();
}

bool RF24NetworkWrapper::available(void) {
    return this->network.available();
}

void RF24NetworkWrapper::peek(RF24NetworkHeader& header) {
    this->network.peek(header);
}

size_t RF24NetworkWrapper::read(RF24NetworkHeader& header, void* message, size_t maxlen) {
    return this->network.read(header, message, maxlen);
}

bool RF24NetworkWrapper::write(RF24NetworkHeader& header,const void* message, size_t len) {
    return this->network.write(header, message, len);
}
