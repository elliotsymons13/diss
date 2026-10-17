

#include <WhichCommsCommon.h>
#include <WhichCommsMetrics.h>
// #include "M5Unified.h"
#include <M5GFX.h>
#include <M5CoreS3.h>
#include <UserInterface.h>

#define AWAIT_SERIAL

// user-configurable UI configuration constants
#define NUM_BUTTONS 3
#define NUM_STATUS_ROWS 1
#define NUM_BUTTON_ROWS 4

M5Canvas canvas;
M5GFX display;
UserInterface ui(display, canvas);

LoRa868 lora868; 
RF433 rf433;

#define RTT_INTERVAL 2000 // ms, 2 seconds
#define RX_WAIT_TIME 1200 // master wait time must be higher than the slave wait time, to ensure slave has time to receive and respond
#define RX_BUFF_SIZE 256

bool setup_ui() {
	ui.configure(TEXT_SIZE, NUM_BUTTONS, NUM_BUTTON_ROWS, NUM_STATUS_ROWS);
	if (!ui.drawInitial()) {
		LOG_ERROR("UserInterface initial draw failed. ");
        return false;
	} else {
		LOG_TRACE("UserInterface initial draw succeeded. ");
	}

	ui.setStatusContent(0, "MASTER");
	ui.drawStatuses();
	ui.setButtonLabel(0, "SEND");
    ui.setButtonLabel(1, "<none>");
    ui.setButtonLabel(2, "<none>");
	ui.drawButtons();   
    return true;
}

// check if any button was newly pressed; if so, return true and set btn to buttonID
bool newButtonPress(UserInterface& ui, uint8_t& btn) {

    m5::Touch_Class::touch_detail_t touch = CoreS3.Touch.getDetail();
	
    if (touch.wasPressed()) {
        LOG_INFO("Touchscreen was pressed. ");
        
        // determine which, if any, button was pressed
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            if (ui.buttonWasPressed(i, touch.x, touch.y)) {
            LOG_TRACE("Button ", i, " was pressed. ");
            btn = i;
            return true;
            }
        }
        LOG_TRACE("A touch outside of any button area was detected. ");

        return false;
    }
    // LOG_TRACE("No touch detected. ");
	return false;
}

void setup() {
	// Initialise M5CoreS3
	auto cfg = M5.config();
	CoreS3.begin(cfg);
    
    LOG_SET_LEVEL(GLOBAL_LOG_LEVEL);
    #ifdef AWAIT_SERIAL
    await_serial(); // must await serial before setup, so failures can be logged
    #endif

    if (!setup_common_lora868(&lora868)) {
        LOG_ERROR("Critical error. BLOCKING.");
        block_forever();
    }
    
    rf433.pre_config();
    if (!rf433.begin_tx_only_mode()) {
        LOG_ERROR("Failed to test initialize TX RF433.");
        block_forever();
    }
    if (!rf433.begin_rx_only_mode()) {
        LOG_ERROR("Failed to test initialize RX RF433.");
        block_forever();
    }
    rf433.post_config();

    if (!setup_ui()) {
        LOG_ERROR("Failed to setup UI. BLOCKING.");
        block_forever();
    }

}


/**
 * Perform a packet loss test by sending a series of pings and counting acknowledgements. 
 * Blocks until complete. 
 */
uint8_t packet_loss_test(WhichCommsMetrics& metrics, OneComms& bearer) {
    
    const uint8_t pings = 5;
    uint8_t acks = 0;
    for (uint8_t i = 0; i < pings; i++) {
        const size_t tx_data_len = 6; 
        const uint8_t tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, metrics.get_counter()};
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;
        int rtt = 0;

        // send ping and wait for response
        LOG_DEBUG("Sending ping number", i);
        bearer.send(tx_data, tx_data_len);

        // await and handle any response
        switch (bearer.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            case err::none:
                LOG_TRACE("Response received.");
                if(metrics.complete_rtt((uint8_t*)tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
                    LOG_INFO("Received response for ping number", i);
                    acks++;
                    if (rtt > 0) {
                        LOG_INFO("RTT was", rtt, "microseconds,", rtt / 1000.0f, "milliseconds, and", rtt / 1000000.0f, "seconds.");
                    }
                } else {
                    LOG_INFO("Failed to receive response for ping number", i);
                }
                break;
            case err::timeout_expired:
                LOG_TRACE("Receive timed out.");
                break;
            case err::buffer_overflow:
                LOG_WARN("Buffer overflow occurred.");
                break;
            default:
                LOG_ERROR("Unknown error occurred.");
                break;
        }
    }
    uint8_t packet_loss_rate = (pings - acks) * 100.0f / pings;
    LOG_INFO("Packets sent:", pings);
    LOG_INFO("Acknowledgements received:", acks);
    LOG_INFO("Packet loss rate:", packet_loss_rate, "%");
    return packet_loss_rate; 
}

/** Every time button 0 is pressed, trigger a single RTT measurement. 
 * Blocks forever. 
 * Can be used to assess if in range (ping, ack) or actually to measure RTT. */
void rtt_on_press(WhichCommsMetrics& metrics, OneComms& bearer) {
    while (true) {
        // Wait for 0 press to trigger next RTT test
        uint8_t lastBtnPressed = 255;
        while (!newButtonPress(ui, lastBtnPressed) && lastBtnPressed != 0) {
            // wait for press of 0
        }

        const uint8_t iter = metrics.get_counter();
        const size_t tx_data_len = 6; 
        const uint8_t tx_data[tx_data_len] = {0xFF, 0x00, 0xFF, 0x00, 0xFF, iter};
        uint8_t rx_buffer[RX_BUFF_SIZE] = {0};
        size_t rxd_size = 0;
        int rtt = 0;

        // send ping and wait for response
        if (err::none == bearer.receiveOne((uint8_t*)rx_buffer, sizeof(rx_buffer), &rxd_size, RX_WAIT_TIME)) {
            if(metrics.complete_rtt((uint8_t*)tx_data, tx_data_len, rx_buffer, rxd_size, &rtt)) {
                LOG_INFO("Received response for ping number", iter);
                if (rtt > 0) {
                    LOG_INFO("RTT was", rtt, "microseconds.");
                }
            } else {
                LOG_INFO("Failed to receive response for ping number", iter, "within timeout");
            }
        }
    }
}

uint8_t logs_written = 0;
uint8_t lastBtnPressed = -1;
uint8_t serial_count = 0;

void loop() {

    WhichCommsMetrics lora868metrics;

    LOG_DEBUG("MASTER entering main loop");
    while (true) {

        // check if any button was pressed
        bool newPress = newButtonPress(ui, lastBtnPressed);
        // if (newPress) {
        //     // notify the press registered using the UI log element, handling if log is full 
        //     if (!ui.setLogContent(logs_written++, "Button " + String(lastBtnPressed) + " was pressed!\n")) {
        //         LOG_ERROR("Failed to set log content for button press");
        //         if (!ui.addStatusContent(0, " - LOG OVERFLOW")) {
        //             LOG_ERROR("Failed to set status content for log overflow");
        //         }
        //     }
        // }
        // ui.drawLogs();
        // ui.drawStatuses();

        // if button 0 was pressed, perform packet loss test
        if (newPress && lastBtnPressed == 0) {
            uint8_t lastBtnPressed = -1;
            serial_count++;


            // notify the press registered using the UI log element, handling if log is full 
            if (!ui.setLogContent(logs_written, "SEND pressed, doing srl " + String(serial_count) + "...\n")) { // do not increment so rewrite same log line
                LOG_ERROR("Failed to set log content for button press");
                if (!ui.addStatusContent(0, " - LOG OVERFLOW")) {
                    LOG_ERROR("Failed to set status content for log overflow");
                }
            }
            ui.drawLogs();
            ui.drawStatuses();

            uint8_t packet_loss_rate = packet_loss_test(lora868metrics, lora868);
            
            // add result to UI log element, handling if log is full 
            if (!ui.setLogContent(logs_written++, "Srl " + String(serial_count) + " PktLs: " + String(packet_loss_rate) + "%\n")) {
                LOG_ERROR("Failed to set log content for button press");
                if (!ui.addStatusContent(0, " - LOG OVERFLOW")) {
                    LOG_ERROR("Failed to set status content for log overflow");
                }
            }
            ui.drawLogs();
            ui.drawStatuses();

        }

        CoreS3.update();
    }
}
