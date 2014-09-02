#pragma once 

#include "IRadioNetwork.h"

#include "RF24/RF24.h"
#include "RF24Network/RF24Network.h"

class RF24NetworkWrapper: public IRadioNetwork {
    public:
        RF24NetworkWrapper(uint8_t _channel, uint16_t _node_address, rf24_pa_dbm_e _palevel, rf24_datarate_e _datarate);

        void begin(void);
        void update(void);
        bool available(void);
        void peek(RF24NetworkHeader& header);
        size_t read(RF24NetworkHeader& header, void* message, size_t maxlen);
        bool write(RF24NetworkHeader& header,const void* message, size_t len);

    protected:
        uint8_t channel;
        uint16_t node_address;
        rf24_pa_dbm_e palevel;
        rf24_datarate_e datarate;

        RF24 radio;
        RF24Network network;
};
