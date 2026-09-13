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
    Serial.println("RF433 bearer functions in simplex mode, and does not support full duplex mode. Invoke tx OR rx mode instead. Half-duplex may be achieved via alterating configurations. ");
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
            Serial.printf("Failed to get RMT ring buffer handle, error code: %d\n", err);
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

    const uint8_t user_data_len = 6;
    uint8_t dl[1+user_data_len] = {user_data_len, 0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};
    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    Serial.println("Sending");
    for (int i = 0; i < 1+user_data_len; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((dl[i] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        Serial.printf("%02X ", dl[i]);
    }
    Serial.println();
    const uint32_t rmt_items = 2 + (1+user_data_len) * 8; // RMT protocol has 2 leading items (start codes) and then 8 items per byte of data
    Serial.printf("Sending %d bytes and %u items\n", 1+user_data_len, rmt_items);
    for (int i = 0; i < 3; i++) {
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, rmt_items, false));
        ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
    }
    
    return true;
}
// XXX hardcoded!
// examples for use, in reality calling library will define and pass in the data to be sent
const uint8_t expected_data_len = 6; // expect 6 bytes of data to follow the length byte
uint8_t expected_data[1+expected_data_len] = {expected_data_len, 0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};

/*
 * Parse received RMT items into a byte array
 * 
 * Takes an array of encoded bits ('RMT items'), and samples each to deduce 0 or 1. 
 * 
 */
int parsedData(rmt_item32_t* item, uint8_t* conveyed_size, size_t rx_size, uint8_t* dataptr, size_t maxsize)
{
    // TODO max size checks
    if((item == nullptr)||(rx_size < 4))return -1;
    if (rx_size < 4) return -1;
    // Serial.printf("rx_size:%d\r\n", rx_size);
    unsigned int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, byte_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) && (item[0].duration0 < 2600)) {
        Serial.println("Phy header detected");
        // dataptr = (uint8_t*)malloc(rx_size * sizeof(uint8_t));
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
                        // Serial.printf("[%d:]%02x ", byte_cnt, data); // print the byte in hex (2 digits)
                        if (byte_cnt >= maxsize) {
                            Serial.printf("\r\nBuffer overflow, maxsize=%d\r\n. Printing partial rx only. ", maxsize);
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
        } while (cnt < rx_size);

        // Serial.println("END");
    }
    // record conveyed size (first byte)
    *conveyed_size = dataptr[0];
    // Serial.printf("conveyed size: %d\r\n", *conveyed_size);
    return byte_cnt;
}

bool validatedData(uint8_t conveyed_data_len, size_t parsed_size, uint8_t* databuff, size_t expected_data_len, uint8_t* expected_data) {
    
    if (conveyed_data_len != parsed_size) {
        // Serial.printf("Conveyed data length (%d) does not match parsed size (%d)\r\n", conveyed_data_len, parsed_size);
        return false;
    }

    for (int i = 0; i < expected_data_len; i++) {
        if (databuff[i] != expected_data[i]) {
            Serial.printf("Data mismatch at index %d: expected 0x%02X, got 0x%02X\r\n", i, expected_data[i], databuff[i]);
            return false;
        }
    }
    return true;
}

err RF433::_tryRx(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {
    this->begin_rx_only_mode();
    
    // RingbufHandle_t rb = nullptr;
    // rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &this->rx_ring_buff);

    rmt_rx_start(RMT_RX_CHANNEL, true);
    unsigned long start_time = millis();
    unsigned long rx_timeout = 3000;
    bool gotComplete = false;
    uint8_t parsed_size = 0;            
    uint8_t MAXSIZE = 255;
    uint8_t databuff[MAXSIZE+1] = {0};


    while (true) {
        if ((millis() - start_time >= rx_timeout)) {
            Serial.println("RX TIMEOUT 2 - FAILSAFE, should not occur in normal operation");
            return err::timeout_expired;
        }

        size_t rx_size     = 0;
        rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(this->rx_ring_buff, &rx_size, 500);

        // START HERE always null item
        // Serial.printf("---------\r\n");
        if (item == nullptr) {
            Serial.println("RX TIMEOUT 1");
            break; // timeout occurred, exit the loop
        }
        if (rx_size != 0) {
            databuff[MAXSIZE+1] = {0}; // reset
            rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items

            uint8_t conveyed_data_len = 0;
            parsed_size = parsedData(item, &conveyed_data_len, rx_size, databuff, MAXSIZE);
            Serial.println("---------");
            Serial.printf("\r\nConveyed data length: %d, Parsed size: %d\r\n", conveyed_data_len, parsed_size);
            Serial.printf("Parsed something: databuff: ");
            for (int i = 0; i < parsed_size; i++) {
                Serial.printf("%02X ", databuff[i]);
            }
            if (validatedData(conveyed_data_len, parsed_size, databuff, expected_data_len, expected_data)) {
                Serial.printf("Data has correct length, content, etc. as expected by caller.\r\n");
                Serial.printf("Parsed Data (%d bytes): \n", parsed_size);
                for (int i = 0; i < parsed_size; i++) {
                    Serial.printf("%02X ", databuff[i]);
                }
                Serial.println();
                gotComplete = true;
                // (*received_count)++;
            } else {
                Serial.printf("INVALID DATA RECEIVED.\r\n");
            }
            
        }
        vRingbufferReturnItem(this->rx_ring_buff, (void*)item);
        if (gotComplete) {
            Serial.println("RECEIVED VALID DATA. exiting rx attempt");
            memcpy(buffer, databuff, parsed_size);
            *rxd_size = parsed_size;
            return err::none;
        }

    }
    rmt_rx_stop(RMT_RX_CHANNEL);
    Serial.println("RMT RX stopped.");
}


err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size) {
    return receiveOne(buffer, buffer_size, rxd_size, 0);
}
err RF433::receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms) {    
    // for RF433 library, the example code uses a blocking receive approach
    // therefore this function delegates timeout handling to that modified code

    return _tryRx(buffer, buffer_size, rxd_size, timeout_ms);
    
}