#include "RF433.h"

const uint16_t RX_BUFF_SIZE = 2048*8;
rmt_item32_t rmtbuff[RX_BUFF_SIZE];

bool RF433::pre_config() {
    pinMode(RTM_TX_GPIO_NUM, OUTPUT);
    pinMode(RTM_RX_GPIO_NUM, INPUT);

    this->txconfig = {};
    this->txconfig.rmt_mode                 = RMT_MODE_TX;
    this->txconfig.channel                  = RMT_TX_CHANNEL;
    this->txconfig.gpio_num                 = gpio_num_t(RTM_TX_GPIO_NUM);
    this->txconfig.mem_block_num            = RTM_BLOCK_NUM;
    this->txconfig.tx_config.loop_en        = false;
    this->txconfig.tx_config.carrier_en     = false;
    this->txconfig.tx_config.idle_output_en = true;
    this->txconfig.tx_config.idle_level     = rmt_idle_level_t(0);
    this->txconfig.clk_div                  = RMT_CLK_DIV;

    this->rxconfig = {}; 
    this->rxconfig.rmt_mode                      = RMT_MODE_RX;
    this->rxconfig.channel                       = RMT_RX_CHANNEL;
    this->rxconfig.gpio_num                      = gpio_num_t(RTM_RX_GPIO_NUM);
    this->rxconfig.mem_block_num                 = RTM_BLOCK_NUM; // was hardcoded 6;
    this->rxconfig.clk_div                       = RMT_CLK_DIV;
    this->rxconfig.rx_config.filter_en           = true;
    this->rxconfig.rx_config.filter_ticks_thresh = 200 * RMT_1US_TICKS;
    this->rxconfig.rx_config.idle_threshold      = 3 * RMT_1MS_TICKS;

    return true;
}

bool RF433::begin_duplex_mode() {
    this->mode = comm::mode_invalid;
    Serial.println("RF433 bearer functions in simplex mode, and does not support full duplex mode. Invoke tx OR rx mode instead. Half-duplex may be achieved via alterating configurations. ");
    return false;
}

bool RF433::begin_tx_only_mode() {

    if (this->mode == comm::mode_tx) {
        return true; // already in TX mode
    }
    
    // if (!this->txInstalled) {
        rmt_channel_t tx_channel = this->txconfig.channel;
        ESP_ERROR_CHECK(rmt_driver_uninstall(tx_channel));
        ESP_ERROR_CHECK(rmt_config(&this->txconfig));
        ESP_ERROR_CHECK(rmt_driver_install(tx_channel, 0, 0));
        // this->txInstalled = true;
        // }
    this->mode = comm::mode_tx;
    return true;
}

bool RF433::begin_rx_only_mode() {

    if (this->mode == comm::mode_rx) {
        return true; // already in RX mode
    }
    this->mode = comm::mode_rx;

    // if (!this->rxInstalled) {
        rmt_channel_t rx_channel = this->rxconfig.channel;
        ESP_ERROR_CHECK(rmt_driver_uninstall(rx_channel));
        ESP_ERROR_CHECK(rmt_config(&this->rxconfig));
        ESP_ERROR_CHECK(rmt_driver_install(rx_channel, RX_BUFF_SIZE, 0));
        esp_err_t err = rmt_get_ringbuf_handle(rx_channel, &this->rx_ring_buff);
        if (err != ESP_OK || this->rx_ring_buff == nullptr) {
            Serial.printf("Failed to get RMT ring buffer handle, error code: %d\n", err);
            return false;
        }
        this->rxInstalled = true;
    // }
    return true;
}

bool RF433::post_config() {
    // TODO
    return true;
}

bool RF433::send(const uint8_t* data, size_t buffer_size) {

    // TODO return codes
    // TODO bounds checks for rmt buffer sizes, etc. 

    this->begin_tx_only_mode();
    
    // memset(rmtbuff, 0, sizeof(rmtbuff)); // TODO zero the rmtbuff
    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    for (int i = 0; i < buffer_size; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((data[i] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
    }
    const uint32_t rmt_items = 2 + buffer_size * 8; // RMT protocol has 2 leading items (start codes) and then 8 items per byte of data
    Serial.printf("Sending packet -  %d bytes encoded in %u RMT items\n", buffer_size, rmt_items);
    // for (int i = 0; i < tx_repeats; i++) {
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, rmt_items, false));
        ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
    // }
    return true;
}

/**
 * Attempt to decode a single packet from the recieved RMT items. 
 * 
 * Where a packet is not yet completely received, the function will 
 * return parsed items to the buffer, for later re-parsing once more data is 
 * received in subsequent call. 
 */
size_t _parseData(rmt_item32_t* item, size_t* conveyed_size, size_t rx_size, uint8_t* dataptr, size_t maxsize)
{
    if((item == nullptr)||(rx_size < 4))return 0;
    unsigned int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, byte_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) && (item[0].duration0 < 2600)) {
        // dataptr = (uint8_t*)malloc(rx_size * sizeof(uint8_t));
        rmt_item32_t candidateBit;
        candidateBit.level0    = 1;
        candidateBit.level1    = 0;
        candidateBit.duration0 = item[0].duration1;
        while (cnt+1 < rx_size) {
            cnt++;
            candidateBit.duration1 = item[cnt].duration0;
            if (cnt > 1) {
                if (((candidateBit.duration0 + candidateBit.duration1) < 1100) &&
                    ((candidateBit.duration0 + candidateBit.duration1) > 800)) {
                    data <<= 1;
                    if (candidateBit.duration0 > candidateBit.duration1) {
                        data += 1;
                    }

                    bitcnt++;
                    if (bitcnt >= 8) { // i.e. if byte complete
                        // Serial.printf("[%d:]%02x ", byte_cnt, data); // print the byte in hex (2 digits)
                        if (byte_cnt >= maxsize) {
                            // Serial.printf("\r\nBuffer overflow, maxsize=%d\r\n. Printing partial rx only. ", maxsize);
                            *conveyed_size = dataptr[0];
                            // Serial.printf("conveyed size: %d\r\n", *conveyed_size);
                            return byte_cnt;
                        }
                        dataptr[byte_cnt] = data;
                        data             = 0;
                        byte_cnt++;
                        bitcnt = 0;
                    }
                } else {
                    // Serial.printf("bytes rxd in pkt: %d", byte_cnt);
                    *conveyed_size = dataptr[0];
                    // Serial.printf("conveyed size: %d\r\n", *conveyed_size);
                    return byte_cnt;
                }
            }
            candidateBit.duration0 = item[cnt].duration1;
            // Serial.printf("symbols processed count %d, of %u\r\n", cnt, rx_size);
        }

        // Serial.println("END");
    }
    // record conveyed size (first byte)
    *conveyed_size = dataptr[0];
    // Serial.printf("conveyed size: %d\r\n", *conveyed_size);
    return byte_cnt;
}

/**
 * Check if conveyed lenth (as reported in the first byte of the packet), matches 
 * the actual parsed length. If the lengths match, the packet is complete (return true)
 * Otherwise return false. 
 */
bool packetComplete(size_t conveyed_data_len, size_t parsed_size) {
    return (conveyed_data_len == parsed_size);
}

void _rxCleanup() {
    rmt_rx_stop(RMT_RX_CHANNEL);
    Serial.println("RMT RX stopped.");
}

bool RF433::_tryRx(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {
    // this->begin_rx_only_mode();

    // rmt_rx_start(RMT_RX_CHANNEL, true);
    // unsigned long start_time = millis();
    // unsigned long rx_timeout = 3000;

    // while (true) {
    //     if ((millis() - start_time >= rx_timeout)) {
    //         Serial.println("RX TIMEOUT 2 - FAILSAFE, should not occur in normal operation");
    //         break;
    //     }

    //     size_t rx_size     = 0;
    //     rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(this->rx_ring_buff, &rx_size, 500);
    //     // Serial.printf("---------\r\n");
    //     if (item == nullptr) {
    //         Serial.println("RX TIMEOUT 1");
    //         break; // timeout occurred, exit the loop
    //     }
    //     if (rx_size != 0) {
    //         uint8_t MAXSIZE = 255;
    //         uint8_t databuff[MAXSIZE+1] = {0};
    //         rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items

    //         size_t conveyed_data_len = 0;
    //         size_t parsed_size = _parseData(item, &conveyed_data_len, rx_size, databuff, MAXSIZE);
    //         if (packetComplete(conveyed_data_len, parsed_size)) {
    //             Serial.printf("PACKET COMPLETE - Data has length matching that self-described in packet header.\r\n");
    //             // TODO copy to buff
    //         } else {
    //             Serial.printf("INVALID DATA RECEIVED - too short.\r\n");
    //         }
            
    //     }
    //     vRingbufferReturnItem(this->rx_ring_buff, (void*)item);

    // }
    // rmt_rx_stop(RMT_RX_CHANNEL);
    // Serial.println("RMT RX stopped.");


    // TODO cleanup func locally ?
    // TODO refactor to go style approach, non-working version commented at bottom, once this version tested
    uint8_t MAXSIZE = 255;
    uint8_t databuff[MAXSIZE+1] = {0};
    bool success = true;

    this->begin_rx_only_mode();
    rmt_rx_start(RMT_RX_CHANNEL, true);

    size_t conveyed_data_len = UINT_MAX; // HACK to ensure nonequal on first iteration 
    size_t parsed_size = 0;
    unsigned long start_time = millis();
    uint32_t rx_timeout = timeout_ms;
    while (conveyed_data_len != parsed_size) {
        if ((millis() - start_time >= rx_timeout)) {
            Serial.println("RX TIMEOUT ");
            success = false; // TODO encode failure due to timeout
            break; // FIXME timeout never hits atm due to 500ms timeout below
        }

        size_t rx_size     = 0;
        rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(this->rx_ring_buff, &rx_size, 500);
        // Serial.printf("---------\r\n");
        if (item == nullptr) {
            Serial.println("Nothing in ring buffer. ");
            break; 
        }
        if (rx_size != 0) {

            rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items

            parsed_size = _parseData(item, &conveyed_data_len, rx_size, databuff, MAXSIZE);
            if (packetComplete(conveyed_data_len, parsed_size)) {
                Serial.printf("PACKET COMPLETE - Data has length matching that self-described in packet header.\r\n");
                if (parsed_size > buffer_size) {
                    Serial.printf("RX BUFFER OVERFLOW - parsed size %d exceeds buffer size %d\r\n", parsed_size, buffer_size);
                    // vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
                    // return false; // TODO
                    success = false; // TODO encode failure due to buffer overflow
                    break;
                }
            } else {
                Serial.printf("INVALID DATA RECEIVED - too short.\r\n");
            }
            
        } else {
            Serial.println("RX SIZE IS ZERO");
        }
        vRingbufferReturnItem(this->rx_ring_buff, (void*)item);

    }
    
    rmt_rx_stop(RMT_RX_CHANNEL);// TODO cleanup func
    Serial.println("RMT RX stopped.");
    if (success) {
        Serial.println("RX SUCCESS");
        memcpy(buffer, databuff, parsed_size);
        *rxd_size = parsed_size;
        return true;
    } else {
        Serial.println("RX FAILURE");
        return false;
    }


    // size_t rx_size     = 0;
    // rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(this->rx_ring_buff, &rx_size, 500);
    // if (item == nullptr) {
    //     Serial.println("RX TIMEOUT - nullptr from rx buffer");
    //     _rxCleanup();
    //     return false;
    // }
    // if (rx_size == 0) {
    //     Serial.println("RX TIMEOUT - rx_size is 0");
    //     _rxCleanup();
    //     vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
    //     return false;
    // }
    // uint8_t MAXSIZE = 255;
    // uint8_t databuff[MAXSIZE+1] = {0};
    // rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items

    // size_t conveyed_data_len = 0;
    // size_t parsed_size = _parseData(item, &conveyed_data_len, rx_size, databuff, MAXSIZE);
    // if (!packetComplete(conveyed_data_len, parsed_size)) {
    //     _rxCleanup();
    //     vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
    //     return false;             
    // }
    // if (parsed_size > buffer_size) {
    //     Serial.printf("RX BUFFER OVERFLOW - parsed size %d exceeds buffer size %d\r\n", parsed_size, buffer_size);
    //     _rxCleanup();
    //     vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
    //     return false;
    // }
    // memcpy(buffer, databuff, parsed_size);
    // *rxd_size = parsed_size;
    // _rxCleanup();
    // // do not return to buffer, as we have consumed said complete packet
    // return true;
}


err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) {
    return receiveOne(buffer, buffer_size, rxd_size, 0);
}
err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {
    // HACK rmt_rx_start(RMT_RX_CHANNEL, true);
    
    // for RF433 library, the example code uses a blocking receive approach
    // therefore this function delegates timeout handling to that modified code

    bool success = _tryRx(buffer, buffer_size, rxd_size, timeout_ms);
    if (success) {
        return err::none;
    } else {
        // HACK is an assumption, could be another type of error
        return err::timeout_expired;
    }

    // blocking case (no timeout)
    // if (timeout_ms == 0) {
    //     while (!_tryRx(buffer, buffer_size, rxd_size)) {
    //         // wait for packet
    //         delay(RX_WAIT_INTERVAL_MS);
    //     }
    //     // _rxCleanup();
    //     return err::none;
    // }
    
    // // timout case 
    // unsigned long startMillis = millis();
    // while (millis() - startMillis < timeout_ms) {
    //     if (_tryRx(buffer, buffer_size, rxd_size)) {
    //         // _rxCleanup();
    //         return err::none;
    //     }
    //     delay(RX_WAIT_INTERVAL_MS);
    // }
    // Serial.println("Timeout expired in RF433 receiveOne");
    
    // // _rxCleanup();
    // return err::timeout_expired;
}