#include <Arduino.h>
#include <M5GFX.h>
#include <M5CoreS3.h>
#include <UserInterface.h>
#include <DebugLog.h>

// user-configurable UI configuration constants
#define NUM_BUTTONS 3
#define NUM_STATUS_ROWS 1
#define NUM_BUTTON_ROWS 4

M5Canvas canvas;
M5GFX display;
UserInterface ui(display, canvas);

void setup(void) 
{
	LOG_SET_LEVEL(DebugLogLevel::LVL_TRACE);
	Serial.begin(115200);
	while (!Serial) {
		// wait for Serial to become active
		delay(100);
	}
	delay(500);
	LOG_INFO("Serial connected. ");

	// Initialise M5CoreS3
	auto cfg = M5.config();
	CoreS3.begin(cfg);

	// Setup UI
	ui.configure(TEXT_SIZE, NUM_BUTTONS, NUM_BUTTON_ROWS, NUM_STATUS_ROWS);
	if (!ui.drawInitial()) {
		LOG_ERROR("UserInterface initial draw failed. ");
	} else {
		LOG_TRACE("UserInterface initial draw succeeded. ");
	}

	ui.setStatusContent(0, "THIS IS THE STATUS BAR");
	ui.drawStatuses();
	ui.setButtonLabel(0, "Btn 0");
	ui.setButtonLabel(1, "Btn it");
	ui.setButtonLabel(2, "Btn up");
	ui.drawButtons();
}

// check if any button was newly pressed; if so, return true and set btn to buttonID
bool newButtonPress(UserInterface& ui, uint8_t& btn) {
    CoreS3.update();
    m5::Touch_Class::touch_detail_t touch = CoreS3.Touch.getDetail();
	
    if (touch.wasPressed()) {        
        // determine which, if any, button was pressed
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            if (ui.buttonWasPressed(i, touch.x, touch.y)) {
            LOG_TRACE("Button ", i, " was pressed. ");
            btn = i;
            return true;
            }
        }
        return false;
    }
	return false;
}


void loop(void)
{
	uint8_t logs_written = 0;
	uint8_t lastBtnPressed = -1;

	while (true) {
		CoreS3.update();
		bool newPress = newButtonPress(ui, lastBtnPressed);
		if (newPress) {
			LOG_TRACE("New button press detected: ", lastBtnPressed);
			ui.setLogContent(logs_written++, "Button " + String(lastBtnPressed) + " was pressed!\n");
			ui.drawLogs();
		}
	}

}
