#ifndef WHICHCOMMSMETRICS_H
#define WHICHCOMMSMETRICS_H

#include <EverywhereCommon.h>

class WhichCommsMetrics {

    private:
        uint8_t counter = 0;
        int counter_start_time[256] = {0}; // indexed by uint8_t counter value (0-255)

    public:
        uint8_t get_counter();
        void increment_counter();
        void start_rtt();
        bool complete_rtt(uint8_t* tx_packet, size_t tx_data_len, uint8_t* rxd_packet, size_t rxd_size, int* rtt);
};


#endif // WHICHCOMMSMETRICS_H