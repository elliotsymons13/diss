
#ifndef ONECOMMS_H
#define ONECOMMS_H  

#include <Arduino.h>

class OneComms {
    public:
        virtual bool pre_config() = 0;
        virtual bool begin() = 0;
        virtual bool post_config() = 0;
        // virtual bool send(uint8_t* data, size_t size) = 0;
        // virtual bool receive(uint8_t* buffer, size_t size) = 0;
};

#endif // ONECOMMS_H