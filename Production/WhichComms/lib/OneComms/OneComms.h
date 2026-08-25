
#ifndef ONECOMMS_H
#define ONECOMMS_H  

#include <Arduino.h>

class OneComms {
    public:
        virtual void begin() = 0;
        // virtual bool send(uint8_t* data, size_t size) = 0;
        // virtual bool receive(uint8_t* buffer, size_t size) = 0;
};

#endif // ONECOMMS_H