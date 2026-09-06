/* Adapted from example code, licensed as follows:
 *
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
#ifndef RF433_H
#define RF433_H

#include <Arduino.h>
#include <SPI.h>
#include <OneComms.h>
#include <EverywhereCommon.h>
#include <driver/rmt.h>

#define RX_WAIT_INTERVAL_MS 100

#define RMT_TX_CHANNEL  (RMT_CHANNEL_0)
#define RMT_RX_CHANNEL  (RMT_CHANNEL_4)
#define RTM_RX_GPIO_NUM (8)// (18) for port C, or (8) for port B // modified for CoreS3 // FIXME RMT typos
#define RTM_TX_GPIO_NUM (9)// (17) for port C, or (9) for port B // modified for CoreS3 // FIXME clashes with LoRa433 pin
#define RTM_BLOCK_NUM   (1)
#define RMT_CLK_DIV     (80) /*!< RMT counter clock divider */
#define RMT_1US_TICKS   (80000000 / RMT_CLK_DIV / 1000000)
#define RMT_1MS_TICKS   (RMT_1US_TICKS * 1000)

#define T0H 670
#define T1H 320
#define T0L 348
#define T1L 642

#define RMT_CODE_H      {670, 1, 320, 0}
#define RMT_CODE_L      {348, 1, 642, 0}
#define RMT_START_CODE0 {4868, 1, 2469, 0}
#define RMT_START_CODE1 {1647, 1, 315, 0}

class RF433 : public OneComms {
    public:
        bool pre_config();
        bool begin();
        bool post_config();
        bool send(const uint8_t* data, size_t size);
        err receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size);
        err receiveOne(uint8_t* buffer, size_t buffer_size, size_t* rxd_size, uint32_t timeout_ms);

    private:
        RingbufHandle_t rx_ring_buff = nullptr;
        bool _tryRx(uint8_t* buffer, size_t buffer_size, size_t* rxd_size);
};

#endif // RF433_H