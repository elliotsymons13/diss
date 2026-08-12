/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Unit 433T
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 */

#include <Arduino.h>
#include <M5GFX.h>
#include <M5CoreS3.h>

#include <driver/rmt.h>
#include "M5Unified.h"

// #define RMT_TX_CHANNEL  (RMT_CHANNEL_0)
#define RMT_RX_CHANNEL  (RMT_CHANNEL_4)
#define RTM_RX_GPIO_NUM (8) // modified for CoreS3
#define RTM_BLOCK_NUM   (1)
#define RMT_CLK_DIV     (80) /*!< RMT counter clock divider */
#define RMT_1US_TICKS   (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS   (RMT_1US_TICKS * 1000)

const uint16_t rx_buff_size = 2048*8;

#define T0H 670
#define T1H 320
#define T0L 348
#define T1L 642

#define RMT_CODE_H      {670, 1, 320, 0}
#define RMT_CODE_L      {348, 1, 642, 0}
#define RMT_START_CODE0 {4868, 1, 2469, 0}
#define RMT_START_CODE1 {1647, 1, 315, 0}

RingbufHandle_t rb = nullptr;

// XXX hardcoded!
// examples for use, in reality calling library will define and pass in the data to be sent
const uint8_t expected_data_len = 6; // expect 6 bytes of data to follow the length byte
uint8_t expected_data[1+expected_data_len] = {expected_data_len, 0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};

void initRMT()
{
    rmt_config_t rxconfig = {};
    rxconfig.rmt_mode                      = RMT_MODE_RX;
    rxconfig.channel                       = RMT_RX_CHANNEL;
    rxconfig.gpio_num                      = gpio_num_t(RTM_RX_GPIO_NUM);
    rxconfig.mem_block_num                 = RTM_BLOCK_NUM; // was hardcoded 6;
    rxconfig.clk_div                       = RMT_CLK_DIV;          // 时钟分频
    rxconfig.rx_config.filter_en           = true;                 // 开启滤波器
    rxconfig.rx_config.filter_ticks_thresh = 200 * RMT_1US_TICKS;  // 滤波信号宽度100*1M = 100us
    rxconfig.rx_config.idle_threshold      = 3 * RMT_1MS_TICKS;

    ESP_ERROR_CHECK(rmt_config(&rxconfig));
    ESP_ERROR_CHECK(rmt_driver_install(rxconfig.channel, rx_buff_size, 0));
}


void setup()
{
    Serial.begin(115200);
        while (!Serial) {
		// wait for Serial to become active
		delay(100);
	}
	delay(500);
	Serial.println("Serial connected. ");

    M5.begin();
    M5.Lcd.setRotation(1);
    M5.Lcd.fillRect(0, 0, 320, 240, TFT_BLACK);
    M5.Lcd.fillRect(0, 0, 320, 20, M5.Lcd.color565(38, 38, 38));
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.drawString("RF433 Rx Spike", 20, 2, 2);
    pinMode(RTM_RX_GPIO_NUM, INPUT);
    Serial.println("Pins configured. Initializing RMT...");
    initRMT();
    Serial.println("initRMT done");
    delay(100);
    M5.Lcd.setCursor(5, 100);
    M5.Lcd.setTextSize(2);

    M5.Lcd.print("Use Serial to view data");

}


/*
 * Parse received RMT items into a byte array
 * 
 * Takes an array of encoded bits ('RMT items'), and samples each to deduce 0 or 1. 
 * 
 */
int parsedData(rmt_item32_t* item, uint8_t* conveyed_size, size_t rx_size, uint8_t* dataptr, size_t maxsize)
{
    if((item == nullptr)||(rx_size < 4))return -1;
    if (rx_size < 4) return -1;
    Serial.printf("rx_size:%d\r\n", rx_size);
    unsigned int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, byte_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) && (item[0].duration0 < 2600)) {
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
                        Serial.printf("[%d:]%02x ", byte_cnt, data); // print the byte in hex (2 digits)
                        if (byte_cnt >= maxsize) {
                            Serial.printf("\r\nBuffer overflow, maxsize=%d\r\n. Printing partial rx only. ", maxsize);
                            *conveyed_size = dataptr[0];
                            Serial.printf("conveyed size: %d\n", *conveyed_size);
                            return byte_cnt;
                        }
                        dataptr[byte_cnt] = data;
                        data             = 0;
                        byte_cnt++;
                        bitcnt = 0;
                    }
                } else {
                    Serial.printf("bytes rxd in pkt: %d", byte_cnt);
                    *conveyed_size = dataptr[0];
                    Serial.printf("conveyed size: %d\n", *conveyed_size);
                    return byte_cnt;
                }
            }
            candidateBit.duration0 = item[cnt].duration1;
            Serial.printf("symbols processed count %d, of %u\n", cnt, rx_size);
        } while (cnt < rx_size);

        Serial.println("END");
    }
    // record conveyed size (first byte)
    *conveyed_size = dataptr[0];
    Serial.printf("conveyed size: %d\n", *conveyed_size);
    return byte_cnt;
}

bool validatedData(uint8_t conveyed_data_len, size_t parsed_size, uint8_t* databuff, size_t expected_data_len, uint8_t* expected_data) {
    
    if (conveyed_data_len != parsed_size) {
        Serial.printf("Conveyed data length (%d) does not match parsed size (%d)\r\n", conveyed_data_len, parsed_size);
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

void receiveValidateOnce() {
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);
    rmt_rx_start(RMT_RX_CHANNEL, true);
    while (rb) {
        size_t rx_size     = 0;
        rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 500);
        Serial.printf("---------\n");
        if (item != nullptr) {
            if (rx_size != 0) {
                uint8_t MAXSIZE = 255;
                uint8_t databuff[MAXSIZE+1] = {0};
                rx_size = rx_size / sizeof(rmt_item32_t); // convert from bytes to number of RMT items

                uint8_t conveyed_data_len = 0;
                size_t parsed_size = parsedData(item, &conveyed_data_len, rx_size, databuff, MAXSIZE);
                if (validatedData(conveyed_data_len, parsed_size, databuff, expected_data_len, expected_data)) {
                    Serial.printf("Data has correct length, content, etc. as expected by caller.\r\n");
                } else {
                    Serial.printf("INVALID DATA RECEIVED.\r\n");
                }
                
            }
            vRingbufferReturnItem(rb, (void*)item);
        }
    }
    rmt_rx_stop(RMT_RX_CHANNEL);
}

void loop()
{
    receiveValidateOnce();

    delay(10);
    M5.update();
}