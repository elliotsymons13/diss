bool RF433::send(const uint8_t* data, size_t buffer_size) {

    this->begin_tx_only_mode();
    size_t packet_len_with_header = 6 + buffer_size; // 6 bytes of header plus the data buffer size
    
    // memset(rmtbuff, 0, sizeof(rmtbuff)); // zero the rmtbuff
    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    
    // sync header
    uint8_t header[6] = {0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};
    int i = 0;
    for (; i < 6; i++) {  
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((header[i] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        Serial.printf("%02X ", header[i]);
    }
    for (; i < packet_len_with_header; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((data[i-6] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
        Serial.printf("%02X ", data[i-6]);
    }
    Serial.println();
    const uint32_t rmt_items = 2 + packet_len_with_header * 8; // RMT protocol has 2 leading items (start codes), 6 bytes of header, and then 8 items per byte of data
    Serial.printf("Sending packet -  %d bytes encoded in %u RMT items\n", buffer_size, rmt_items);
    ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, rmt_items, false));
    ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));

    return true;
}