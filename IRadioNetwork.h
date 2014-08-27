#pragma once

#include <cstddef>

struct RF24NetworkHeader; 

class IRadioNetwork {
    public:
        virtual void begin(void) { };
        virtual void update(void) { };
        virtual bool available(void) { return false; };
        virtual void peek(RF24NetworkHeader& header) { };
        virtual size_t read(RF24NetworkHeader& header, void* message, size_t maxlen) { return 0; };
        virtual bool write(RF24NetworkHeader& header, const void* message, size_t len) { return false; };
};
