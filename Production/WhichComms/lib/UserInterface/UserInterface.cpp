#include "UserInterface.h"
#include <DebugLog.h>

UserInterface::UserInterface(M5GFX& display, M5Canvas& canvas) : display(display), canvas(canvas) {
    LOG_TRACE("UserInterface constructor called. ");
}

void UserInterface::configure(uint8_t text_size, uint8_t num_buttons, uint8_t num_rows_buttons, uint8_t num_status_rows)
{
    // validate args
    this->text_size = text_size;
    this->num_buttons = num_buttons;
    this->num_rows_status = num_status_rows;
    if (num_status_rows > MAX_NUM_STATUS_ROWS) {
        this->num_rows_status = MAX_NUM_STATUS_ROWS;
        LOG_WARN("UserInterface number of status rows clipped to max of ", MAX_NUM_STATUS_ROWS, " (given ", num_status_rows, "). ");
    }
    
    // calculate UI dimensions
    this->num_rows_buttons = num_rows_buttons;
    this->button_space_h = ROW_HEIGHT_PX * this->num_rows_buttons;
    this->button_space_w = (DISPLAY_WIDTH_PX / this->num_buttons);
    this->status_row_h = this->text_size; 
    this->num_rows = DISPLAY_HEIGHT_PX / ROW_HEIGHT_PX;
    LOG_TRACE("Number of rows calculated as ", this->num_rows, ". ");
    this->num_rows_logs = this->num_rows - this->num_rows_status - this->num_rows_buttons;
    if (this->num_rows_logs < 0) {
        this->num_rows_logs = 0;
        LOG_ERROR("UserInterface number of log rows calculated as negative; your configuration is invalid and invalid combination of button size and/or number of status rows for available screen space. ");
    }

    // store empty initial content for status and log lines, buttons
    for (uint8_t i = 0; i < this->num_rows_status; i++) {
        this->statuses[i].content = "Status row " + String(i);
        this->statuses[i].y = i * ROW_HEIGHT_PX;
    }
    for (uint8_t i = 0; i < this->num_rows_logs; i++) {
        this->logs[i].content = "Log row " + String(i);
        this->logs[i].y = (this->num_rows_status + i) * ROW_HEIGHT_PX;
    }
    for (uint8_t i = 0; i < this->num_buttons; i++) {
        this->buttons[i].label = "Btn " + String(i);
    }

    // configure canvas
    canvas.setTextSize((float) this->text_size);
    canvas.setTextWrap(false);
    canvas.setColorDepth(lgfx::v1::palette_8bit);
    canvas.createSprite(DISPLAY_WIDTH_PX, DISPLAY_HEIGHT_PX);
    canvas.createPalette(COLOURS, 256); // COLOURS
    canvas.setTextColor(canvas.getPaletteIndex(CHRISTMAS_GREEN));

    LOG_TRACE("UserInterface configured with", this->num_rows_status, "status rows", this->num_rows_logs, "log rows, and", this->num_rows_buttons, "button rows. ");
}

// (re-)draw all status rows
void UserInterface::drawStatuses() {
    canvas.setTextColor(canvas.getPaletteIndex(WHITE));
    for (uint8_t i = 0; i < this->num_rows_status; i++) {
        canvas.setCursor(0, this->statuses[i].y);
        canvas.fillRect(0, this->statuses[i].y, DISPLAY_WIDTH_PX, ROW_HEIGHT_PX, canvas.getPaletteIndex(BLACK)); // clear previous status line
        canvas.println(this->statuses[i].content);
    }

    display.waitDisplay();
    canvas.pushSprite(&display, 0, 0);
}

// (re-)draw all log rows
void UserInterface::drawLogs() {
    canvas.setTextColor(canvas.getPaletteIndex(SOMETHING_GREEN));
    for (uint8_t i = 0; i < this->num_rows_logs; i++) {
        canvas.setCursor(0, this->logs[i].y);
        canvas.fillRect(0, this->logs[i].y, DISPLAY_WIDTH_PX, ROW_HEIGHT_PX, canvas.getPaletteIndex(BLACK)); // clear previous log line
        canvas.println(this->logs[i].content);
    }

    display.waitDisplay();
    canvas.pushSprite(&display, 0, 0);
}

// (re-)draw all buttons
void UserInterface::drawButtons() {
    canvas.setTextColor(canvas.getPaletteIndex(WHITE));
    uint8_t button_space_start_y_px = (num_rows_status + num_rows_logs) * ROW_HEIGHT_PX;

    for (int i = 0; i < this->num_buttons; i++) {
        this->buttons[i].x = (i * this->button_space_w) + OUTER_PADDING_PX;
        this->buttons[i].y = button_space_start_y_px + OUTER_PADDING_PX;
        this->buttons[i].w = this->button_space_w - OUTER_PADDING_PX*2;
        this->buttons[i].h = this->button_space_h - OUTER_PADDING_PX*2;

        int max_label_length = (this->buttons[i].w - INNER_PADDING_PX*2) / (TEXT_W_BASE * this->text_size);
        if (this->buttons[i].label.length() > max_label_length) {
            LOG_WARN("Button label \"", this->buttons[i].label, "\" will be truncated to fit available button width. ");
            this->buttons[i].label = this->buttons[i].label.substring(0, max_label_length - 3) + "...";
        }

        canvas.fillRoundRect(this->buttons[i].x, this->buttons[i].y, this->buttons[i].w, this->buttons[i].h, CORNER_RADIUS, canvas.getPaletteIndex(CHRISTMAS_RED));
        uint16_t text_draw_x = this->buttons[i].x + INNER_PADDING_PX;
        uint16_t text_draw_y = button_space_start_y_px + (this->button_space_h - TEXT_H_BASE * this->text_size) / 2;
        canvas.setCursor(text_draw_x, text_draw_y);
        canvas.println(this->buttons[i].label);
    }

    display.waitDisplay();
    canvas.pushSprite(&display, 0, 0);
}

bool UserInterface::drawInitial() {

    if (!display.begin()) {
        LOG_ERROR("Failed to initialize display.");
        return false;
    }

    clearAllContents();

    drawStatuses();
    drawLogs();
    drawButtons();


    // TODO button event handling, see NEXT devblog
    // TODO register callback for touch within button bounds. 

    return true;
}

// clear all contents from all UI elements
void UserInterface::clearAllContents() {
    clearStatusContent();
    clearLogContent();
    clearButtonLabel();
}

// clear all status row contents
void UserInterface::clearStatusContent() {
    for (uint8_t i = 0; i < this->num_rows_status; i++) {
        this->statuses[i].content = "";
    }
}

// clear specific status row content
bool UserInterface::clearStatusContent(uint8_t rowID) {
    if (rowID >= this->num_rows_status) {
        LOG_ERROR("Attempted to clear status content for invalid row ID ", rowID, ". Max is ", this->num_rows_status - 1, ". ");
        return false;
    }
    this->statuses[rowID].content = "";
    return true;
}

// set specific status row content
bool UserInterface::setStatusContent(uint8_t rowID, String content) {
    if (rowID >= this->num_rows_status) {
        LOG_ERROR("Attempted to set status content for invalid row ID ", rowID, ". Max is ", this->num_rows_status - 1, ". "); // TODO remove redundant spaces
        return false;
    }
    statuses[rowID].content = content;
    LOG_TRACE("Set status row ", rowID, " content to: ", statuses[rowID].content, ". ");
    return true;
}

// append to specific status row content
bool UserInterface::addStatusContent(uint8_t rowID, String extraContent) {
    if (rowID >= this->num_rows_status) {
        LOG_ERROR("Attempted to set status content for invalid row ID ", rowID, ". Max is ", this->num_rows_status - 1, ". ");
        return false;
    }
    statuses[rowID].content += extraContent;
    return true;

}

// clear all log row contents
void UserInterface::clearLogContent() {
    for (uint8_t i = 0; i < this->num_rows_logs; i++) {
        this->logs[i].content = "";
    }
}

// clear specific log row content
bool UserInterface::clearLogContent(uint8_t logID, String content) {
    if (logID >= this->num_rows_logs) {
        LOG_ERROR("Attempted to clear log content for invalid log ID ", logID, ". Max is ", this->num_rows_logs - 1, ". ");
        return false;
    }
    this->logs[logID].content = "";
    return true;
}

// clear specific log row content
bool UserInterface::setLogContent(uint8_t logID, String content) {
    if (logID >= this->num_rows_logs) {
        LOG_ERROR("Attempted to set log content for invalid log ID ", logID, ". Max is ", this->num_rows_logs - 1, ". ");
        return false;
    }
    logs[logID].content = content;
    return true;
}

// append to specific log row content
bool UserInterface::addLogContent(uint8_t logID, String extraContent) {
    if (logID >= this->num_rows_logs) {
        LOG_ERROR("Attempted to set log content for invalid log ID ", logID, ". Max is ", this->num_rows_logs - 1, ". ");
        return false;
    }
    logs[logID].content += extraContent;
    return true;
}

// clear all button labels
void UserInterface::clearButtonLabel() {
    for (uint8_t i = 0; i < this->num_buttons; i++) {
        this->buttons[i].label = "";
    }
}

// clear specific button label
bool UserInterface::clearButtonLabel(uint8_t buttonID) {
    if (buttonID >= this->num_buttons) {
        LOG_ERROR("Attempted to clear button label for invalid button ID ", buttonID, ". Max is ", this->num_buttons - 1, ". ");
        return false;
    }
    this->buttons[buttonID].label = "";
    return true;
}

// set specific button label
bool UserInterface::setButtonLabel(uint8_t buttonID, String label) {
    if (buttonID >= this->num_buttons) {
        LOG_ERROR("Attempted to set button label for invalid button ID ", buttonID, ". Max is ", this->num_buttons - 1, ". ");
        return false;
    }
    buttons[buttonID].label = label;
    return true;
}

bool UserInterface::buttonWasPressed(uint8_t buttonID, uint16_t touch_x, uint16_t touch_y) {
    if (buttonID >= this->num_buttons) {
        LOG_ERROR("Attempted to check button press for invalid button ID ", buttonID, ". Max is ", this->num_buttons - 1, ". ");
        return false;
    }
    Button& btn = this->buttons[buttonID];
    if (touch_x >= btn.x && touch_x <= (btn.x + btn.w) &&
        touch_y >= btn.y && touch_y <= (btn.y + btn.h)) {
        LOG_TRACE("Button ", buttonID, " was pressed at (", touch_x, ", ", touch_y, "). ");
        return true;
    }
    return false;
}