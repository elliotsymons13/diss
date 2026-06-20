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

// #define RMT_TX_CHANNEL  (RMT_CHANNEL_0)
#define RMT_RX_CHANNEL  (RMT_CHANNEL_4)
#define RTM_RX_GPIO_NUM (8) // modified for CoreS3
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
    ESP_ERROR_CHECK(rmt_driver_install(rxconfig.channel, 2048, 0));
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

    // delay(1000); //HACK for some reason needed so screen persists
    M5.Lcd.print("Use Serial to view data");
    // delay(1000); //HACK for some reason needed so screen persists

}


int parsedData(rmt_item32_t* item, size_t size, uint8_t* dataptr, size_t maxsize)
{
    // if((item == nullptr)||(size < 4))return -1;
    if (size < 4) return -1;
    int cnt        = 0;
    uint8_t data   = 0;
    uint8_t bitcnt = 0, hex_cnt = 0;
    if (((item[0].level0 == 0)) && (item[0].duration0 > 2300) && (item[0].duration0 < 2600)) {
        // dataptr = (uint8_t*)malloc(size * sizeof(uint8_t));
        rmt_item32_t dataitem;
        dataitem.level0    = 1;
        dataitem.level1    = 0;
        dataitem.duration0 = item[0].duration1;
        do {
            cnt++;
            dataitem.duration1 = item[cnt].duration0;
            if (cnt > 1) {
                // Serial.printf("%d:%d,%d:%d
                // ",dataitem.level0,dataitem.duration0,dataitem.level1,dataitem.duration1);
                if (((dataitem.duration0 + dataitem.duration1) < 1100) &&
                    ((dataitem.duration0 + dataitem.duration1) > 800)) {
                    data <<= 1;
                    if (dataitem.duration0 > dataitem.duration1) {
                        data += 1;
                    }

                    bitcnt++;
                    if (bitcnt >= 8) {
                        Serial.printf("%02x ", data);
                        if (hex_cnt >= maxsize) {
                            return hex_cnt;
                        }
                        dataptr[hex_cnt] = data;
                        data             = 0;
                        hex_cnt++;
                        bitcnt = 0;
                    }
                } else {
                    Serial.printf("%d %d:%d,%d:%d ", hex_cnt, dataitem.level0, dataitem.duration0, dataitem.level1,
                                  dataitem.duration1);
                    return hex_cnt;
                }
            }
            dataitem.duration0 = item[cnt].duration1;
        } while (cnt < size);

        Serial.println("END");
    }
    return hex_cnt;
}


void loop()
{
    int revicecnt = 0;

    RingbufHandle_t rb = nullptr;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);
    rmt_rx_start(RMT_RX_CHANNEL, true);
    while (rb) {
        size_t rx_size     = 0;
        rmt_item32_t* item = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 500);
        if (item != nullptr) {
            if (rx_size != 0) {
                uint8_t databuff[256];
                Serial.printf("\r\nsize:%d\r\n", rx_size);
                int size = parsedData(item, rx_size, databuff, 255);
                if ((size >= 5) && (databuff[0] == 0xaa) && (databuff[1] == 0x55) && (databuff[2] == 0x01) &&
                    (databuff[3] == 0x02) && (databuff[4] == 0x03)) {
                    revicecnt++;
                    Serial.printf("Revice %d\r\n", revicecnt);
                    if (revicecnt == 4) {
                        M5.Lcd.fillRect(0, 20, 320, 220, TFT_GREEN);
                    }
                }
            }
            vRingbufferReturnItem(rb, (void*)item);
        } else {
            if (revicecnt != 0) {
                Serial.printf("Revice %d\r\n", revicecnt);
                M5.Lcd.fillRect(0, 20, 320, 220, TFT_RED);
            }
            revicecnt = 0;
        }
    }
    rmt_rx_stop(RMT_RX_CHANNEL);

    delay(10);
    M5.update();
}