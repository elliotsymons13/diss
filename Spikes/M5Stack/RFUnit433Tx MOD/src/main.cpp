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

// TODO unused analysis

#define RMT_TX_CHANNEL  (RMT_CHANNEL_0)
// #define RMT_RX_CHANNEL  (RMT_CHANNEL_4)
#define RTM_TX_GPIO_NUM (9) // modified for CoreS3 // FIXME clashes with LoRa433 pin
#define RTM_BLOCK_NUM   (1)
#define RMT_CLK_DIV     (80) /*!< RMT counter clock divider */
#define RMT_1US_TICKS   (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS   (RMT_1US_TICKS * 1000)

rmt_item32_t rmtbuff[2048];

#define T0H 670
#define T1H 320
#define T0L 348
#define T1L 642

#define RMT_CODE_H      {670, 1, 320, 0}
#define RMT_CODE_L      {348, 1, 642, 0}
#define RMT_START_CODE0 {4868, 1, 2469, 0}
#define RMT_START_CODE1 {1647, 1, 315, 0}

void initRMT()
{
    rmt_config_t txconfig = {};
    txconfig.rmt_mode                 = RMT_MODE_TX;
    txconfig.channel                  = RMT_TX_CHANNEL;
    txconfig.gpio_num                 = gpio_num_t(RTM_TX_GPIO_NUM);
    txconfig.mem_block_num            = RTM_BLOCK_NUM;
    txconfig.tx_config.loop_en        = false;
    txconfig.tx_config.carrier_en     = false;
    txconfig.tx_config.idle_output_en = true;
    txconfig.tx_config.idle_level     = rmt_idle_level_t(0);
    txconfig.clk_div                  = RMT_CLK_DIV;  // 时钟分频

    ESP_ERROR_CHECK(rmt_config(&txconfig));
    ESP_ERROR_CHECK(rmt_driver_install(txconfig.channel, 0, 0));

}


// examples for use, in reality calling library will define and pass in the data to be sent
// test data will generate (2 + 6 * 8) = 50 RMT items to convey 6 bytes
// the first byte encodes the length of the data to follow
const uint8_t user_data_len = 6;
uint8_t data[1+user_data_len] = {user_data_len, 0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};

const uint8_t invalid_user_data_len = 6;
uint8_t invalid_data[1+invalid_user_data_len] = {invalid_user_data_len+1, 0xAA, 0x55, 0x01, 0x02, 0x03, 0x04};

void send(uint8_t* buff, size_t size, uint8_t tx_repeats = 3)
{
    rmtbuff[0] = (rmt_item32_t){RMT_START_CODE0};
    rmtbuff[1] = (rmt_item32_t){RMT_START_CODE1};
    for (int i = 0; i < size; i++) {
        uint8_t mark = 0x80;
        for (int n = 0; n < 8; n++) {
            rmtbuff[2 + i * 8 + n] = ((buff[i] & mark)) ? ((rmt_item32_t){RMT_CODE_H}) : ((rmt_item32_t){RMT_CODE_L});
            mark >>= 1;
        }
    }
    const uint32_t rmt_items = 2 + size * 8; // RMT protocol has 2 leading items (start codes) and then 8 items per byte of data
    Serial.printf("Sending packet -  %d bytes encoded in %u RMT items - sending this %d times\n", size, rmt_items, tx_repeats);
    for (int i = 0; i < tx_repeats; i++) {
        Serial.printf("Repeat %d", i);
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, rmtbuff, rmt_items, false));
        ESP_ERROR_CHECK(rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY));
    }
}


void setup()
{
    Serial.begin(115200);    while (!Serial) {
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
    M5.Lcd.drawString("RF433 Tx Spike", 20, 2, 2);
    pinMode(RTM_TX_GPIO_NUM, OUTPUT);
    Serial.println("Pins configured. Initializing RMT...");
    initRMT();
    Serial.println("initRMT done");
    delay(100);
    M5.Lcd.setCursor(5, 100);
    M5.Lcd.setTextSize(2);

    M5.Lcd.print("Tap screen to send data");

}

uint32_t send_count = 0;
uint8_t retries = 6;

void loop()
{
    m5::Touch_Class::touch_detail_t touch = CoreS3.Touch.getDetail();
	if (touch.wasReleased()) {
        
        uint32_t old_count = send_count;
        send_count = send_count + retries;
        Serial.printf("SENDING %d packets %d - %d \r\n", retries, old_count+1, send_count);
        send(data, user_data_len, retries);
        // send(invalid_data, invalid_user_data_len);
	}

    delay(10);
    M5.update();
}