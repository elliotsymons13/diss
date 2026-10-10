
#ifndef ONECOMMS_H
#define ONECOMMS_H  

#include <Arduino.h>
#include <EverywhereCommon.h>
class OneComms {
    public:
        virtual bool pre_config() = 0;
        virtual bool begin_duplex_mode() = 0;
        virtual bool begin_rx_only_mode() = 0; // TODO aim for consistency in behaviour in implementatons: all libs should check/call the appropriate mode toggle as first steps in send, receive funcs
        virtual bool begin_tx_only_mode() = 0;
        virtual bool post_config() = 0;
        virtual bool send(const uint8_t* data, size_t size) = 0;
        virtual err receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) = 0;
        virtual err receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) = 0;

    protected:
        comm mode = comm::mode_invalid; // TODO use in ALL derived classes
};

#endif // ONECOMMS_H