#include "WhichCommsMetrics.h"

uint8_t WhichCommsMetrics::get_counter() {
    return counter;
}

void WhichCommsMetrics::increment_counter() {
    counter++;
}

void WhichCommsMetrics::start_rtt() {
    counter_start_time[counter] = micros();
}

bool WhichCommsMetrics::complete_rtt(uint8_t* tx_packet, size_t tx_size, uint8_t* rxd_packet, size_t rxd_size, int* rtt) {
    // Implement the logic for completing RTT here

    if (rxd_size == tx_size) {
        size_t packet_len_without_counter = rxd_size - 1;
        if (equal(rxd_packet, (uint8_t*)tx_packet, packet_len_without_counter)) {
            LOG_TRACE("RTT response matches sent data, excluding counter.");

            // handle lost packets
            uint8_t rxd_counter = rxd_packet[rxd_size - 1];
            if (rxd_counter > counter) {
                LOG_TRACE("RTT received counter", rxd_counter, "is greater than sent counter", counter);
                uint8_t lost_packets = rxd_counter - counter;
                LOG_INFO("Some PING,ACK packet transactions have been lost (approx ", lost_packets, ").");
            }
            // TODO uint8 underflow lost packet cleanup case

            // calculate RTT time
            int rtt_time = micros() - counter_start_time[rxd_counter];
            LOG_TRACE("RTT for message (counter", rxd_counter, ") is", rtt_time, "micros.");
            *rtt = rtt_time;
            return true;
        } else {
            LOG_TRACE("RTT response does NOT match sent data.");
        }
    } else {
        LOG_TRACE("RTT response does NOT match sent data.");
    }
    return false;
}
