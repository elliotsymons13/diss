#include "RF433.h"


const uint16_t RX_BUFF_SIZE = 2048*8;
rmt_item32_t rmtbuff[RX_BUFF_SIZE];

bool RF433::pre_config() {
    pinMode(RMT_TX_GPIO_NUM, OUTPUT);
    pinMode(RMT_RX_GPIO_NUM, INPUT);

    this->txconfig = {};
    this->txconfig.rmt_mode                 = RMT_MODE_TX;
    this->txconfig.channel                  = RMT_TX_CHANNEL;
    this->txconfig.gpio_num                 = gpio_num_t(RMT_TX_GPIO_NUM);
    this->txconfig.mem_block_num            = RMT_BLOCK_NUM;
    this->txconfig.tx_config.loop_en        = false;
    this->txconfig.tx_config.carrier_en     = false;
    this->txconfig.tx_config.idle_output_en = true;
    this->txconfig.tx_config.idle_level     = rmt_idle_level_t(0);
    this->txconfig.clk_div                  = RMT_CLK_DIV;
    this->txconfig.tx_config.carrier_duty_percent = 10; // as per Ofcom duty cycle limit for 10mW transmission at device's fixed frequency 

    this->rxconfig = {}; 
    this->rxconfig.rmt_mode                      = RMT_MODE_RX;
    this->rxconfig.channel                       = RMT_RX_CHANNEL;
    this->rxconfig.gpio_num                      = gpio_num_t(RMT_RX_GPIO_NUM);
    this->rxconfig.mem_block_num                 = RMT_BLOCK_NUM; // was hardcoded 6;
    this->rxconfig.clk_div                       = RMT_CLK_DIV;
    this->rxconfig.rx_config.filter_en           = true;
    this->rxconfig.rx_config.filter_ticks_thresh = 200 * RMT_1US_TICKS;
    this->rxconfig.rx_config.idle_threshold      = 3 * RMT_1MS_TICKS;

    return true;
}

bool RF433::begin_duplex_mode() {
    this->mode = comm::mode_invalid;
    LOG_WARN("RF433 bearer functions in simplex mode, and does not support full duplex mode. Invoke tx OR rx mode instead. Half-duplex may be achieved via alterating configurations.");
    return false;
}

bool RF433::begin_tx_only_mode() {

    if (this->mode == comm::mode_tx) {
        return true; // already in TX mode
    }
    
    if (!this->txInstalled) {
        rmt_channel_t tx_channel = this->txconfig.channel;
        ESP_ERROR_CHECK(rmt_config(&this->txconfig));
        ESP_ERROR_CHECK(rmt_driver_install(tx_channel, 0, 0));
        this->txInstalled = true;
    }
    this->mode = comm::mode_tx;
    return true;
}

bool RF433::begin_rx_only_mode() {

    if (this->mode == comm::mode_rx) {
        return true; // already in RX mode
    }
    this->mode = comm::mode_rx;

    if (!this->rxInstalled) {
        rmt_channel_t rx_channel = this->rxconfig.channel;
        ESP_ERROR_CHECK(rmt_config(&this->rxconfig));
        ESP_ERROR_CHECK(rmt_driver_install(rx_channel, RX_BUFF_SIZE, 0));
        esp_err_t err = rmt_get_ringbuf_handle(rx_channel, &this->rx_ring_buff);
        if (err != ESP_OK || this->rx_ring_buff == nullptr) {
            LOG_ERROR("Failed to get RMT ring buffer handle, error code:", err);
            return false;
        }
        this->rxInstalled = true;
    }
    return true;
}

bool RF433::post_config() {
    return true;
}


bool RF433::send(const uint8_t* data, size_t buffer_size) {
    uint8_t size[LEN_FIELD_SIZE];
    size[0] = buffer_size; // TODO could support larger length field with simple change here and in function param type

    this->begin_tx_only_mode();
    size_t packet_len_with_header = SYNC_HEADER_SIZE + LEN_FIELD_SIZE + buffer_size;
    
    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    
    LOG_TRACE("About to send:");

    // sync header
    int i = 0;
    for (; i < SYNC_HEADER_SIZE; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((header[i] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        if (GLOBAL_LOG_LEVEL == DebugLogLevel::LVL_TRACE) Serial.printf("%02X ", header[i]);
    }
    // length field
    for (; i < SYNC_HEADER_SIZE + LEN_FIELD_SIZE; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((size[i-SYNC_HEADER_SIZE] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        if (GLOBAL_LOG_LEVEL == DebugLogLevel::LVL_TRACE) Serial.printf("%02X ", size[i-SYNC_HEADER_SIZE]);
    }
    // variable length body
    for (; i < packet_len_with_header; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((data[i-SYNC_HEADER_SIZE-LEN_FIELD_SIZE] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        if (GLOBAL_LOG_LEVEL == DebugLogLevel::LVL_TRACE) Serial.printf("%02X ", data[i-SYNC_HEADER_SIZE-LEN_FIELD_SIZE]);
    }
    if (GLOBAL_LOG_LEVEL == DebugLogLevel::LVL_TRACE) Serial.println();
    const uint32_t rmt_items = 2 + packet_len_with_header * 8; // RMT protocol has 2 leading items (start codes) and then 8 items per byte of data
    LOG_TRACE("Sending packet -", buffer_size, "bytes encoded in", rmt_items, "RMT items");
    for (int i = 0; i < 5; i++) { // sending 5 times - rx only seems to work if I do this. may be a poor lib, poor hardware, but this works
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, rmt_items, false));
        ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
    }

    return true;
}

/*
 * Parse received RMT items into a byte array
 * 
 * Takes an array of encoded bits ('RMT items'), and samples each to deduce 0 or 1. 
 * 
 */
int parsedData(rmt_item32_t* item, size_t rx_size, uint8_t* dataptr, size_t maxsize)
{
    if (item == nullptr) return -2;
    if (rx_size < 4) return -1;
    unsigned int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, byte_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) && (item[0].duration0 < 2600)) {
        LOG_TRACE("Phy header detected");
        rmt_item32_t candidateBit;
        candidateBit.level0    = 1;
        candidateBit.level1    = 0;
        candidateBit.duration0 = item[0].duration1;
        do {
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
                        // LOG_TRACE("byte:", byte_cnt, "=", data); // print the byte in hex (2 digits)
                        if (byte_cnt >= maxsize) {
                            LOG_WARN("Buffer overflow, maxsize=", maxsize, ". Partial rx only.");
                            return byte_cnt;
                        }
                        dataptr[byte_cnt] = data;
                        data             = 0;
                        byte_cnt++;
                        bitcnt = 0;
                    }
                } else {
                    return byte_cnt;
                }
            }
            candidateBit.duration0 = item[cnt].duration1;
        } while (cnt + 1 < rx_size);

    }
    return byte_cnt;
}

bool validatedData(size_t parsed_size, uint8_t* databuff, size_t *conveyed_size) {
    const size_t minimum_size = SYNC_HEADER_SIZE + LEN_FIELD_SIZE;
    if (databuff == nullptr || conveyed_size == nullptr || parsed_size < minimum_size) {
        return false;
    }
    
    // check header
    bool header_valid = true;
    size_t i = 0;
    for (; i < SYNC_HEADER_SIZE; i++) {
        if (databuff[i] != header[i]) {
            LOG_TRACE("Header data mismatch at index", i, ": expected", header[i], ", got", databuff[i]);
            header_valid = false;
        }
    }
    if (!header_valid) { // done like this to ensure full header is to trace in event of errors
        return false;
    }

    // check size conveyed against parsed size
    // TODO hardcoded assumption that LEN_FIELD_SIZE is 1
    size_t remaining_size = parsed_size - SYNC_HEADER_SIZE - LEN_FIELD_SIZE;
    if (remaining_size != databuff[i]) {
        LOG_TRACE("Remaining size", remaining_size, "does not match conveyed size", databuff[i], "in packet index", i);
        return false;
    }
    *conveyed_size = databuff[i];
    return true;
}

err RF433::_tryRx(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {
    this->begin_rx_only_mode();

    rmt_rx_start(RMT_RX_CHANNEL, true);
    unsigned long start_time = millis();
    bool gotComplete = false;
    int parsed_size = 0;
    uint8_t MAXSIZE = 255;
    uint8_t databuff[MAXSIZE] = {0};
    size_t conveyed_size = 0;


    while (true) {
        if (millis() - start_time >= (timeout_ms)) {
            // LOG_WARN("RX TIMEOUT 2");
            return err::timeout_expired;
        }

        size_t rx_size     = 0;
        rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(this->rx_ring_buff, &rx_size, timeout_ms/3);

        if (item == nullptr) {
            // LOG_TRACE("RX TIMEOUT 1");
            break; // timeout occurred, exit the loop
        }
        if (rx_size != 0) {
            memset(databuff, 0, sizeof(databuff)); // reset
            // LOG_TRACE("---------");
            // LOG_TRACE("RX size:", rx_size);
            rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items
            // LOG_TRACE("RX size (in RMT items):", rx_size);

            parsed_size = parsedData(item, rx_size, databuff, MAXSIZE);
            // LOG_TRACE("Parsed size:", parsed_size);
            if (parsed_size > 0 && validatedData(parsed_size, databuff, &conveyed_size)) {
                // LOG_TRACE("Data has correct length, header. Parsed Ddta (incl header)", parsed_size, "bytes:");
                log_bytes(databuff, parsed_size);
                gotComplete = true;
                // (*received_count)++;
            } else if (parsed_size > 0) {
                // LOG_TRACE("INVALID DATA RECEIVED (but non-zero parsed_size =", parsed_size, ").");
            } else {
                // LOG_TRACE("NO DATA PARSED (parsed_size =", parsed_size, ").");
            }
            
        }
        vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
        if (gotComplete) {
            // LOG_TRACE("RECEIVED VALID DATA. exiting rx attempt");
            if (buffer == nullptr || rxd_size == nullptr || conveyed_size > buffer_size) {
                rmt_rx_stop(RMT_RX_CHANNEL);
                return err::buffer_overflow;
            }

            memcpy(buffer, databuff + SYNC_HEADER_SIZE + LEN_FIELD_SIZE, conveyed_size);
            *rxd_size = conveyed_size;
            rmt_rx_stop(RMT_RX_CHANNEL);
            // LOG_TRACE("RMT RX stopped - SUCCESS");
            return err::none;
        }

    }
    rmt_rx_stop(RMT_RX_CHANNEL);
    // LOG_TRACE("RMT RX stopped - Timeout");
    return err::timeout_expired;
}


err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) {
    return receiveOne(buffer, buffer_size, rxd_size, 0);
}
err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {    
    // for RF433 library, the example code uses a blocking receive approach
    // therefore this function delegates timeout handling to that modified code

    return _tryRx(buffer, buffer_size, rxd_size, timeout_ms);
    
}