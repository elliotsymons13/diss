#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include "Arduino.h"
#include <M5GFX.h>

// everything scales based on text size
// user must be careful to use sensible combinations that fit within display dimensions
#define TEXT_H_BASE 8 // TODO estimated based on Adafruit docs for 5x7 font +1 each to be sure / based on empirical evidence 
#define TEXT_W_BASE 6
#define TEXT_SIZE 2 // TODO height/width --> what number of rows / columns will fit?
#define ROW_HEIGHT_PX (TEXT_H_BASE * TEXT_SIZE)
#define DISPLAY_WIDTH_PX 320
#define DISPLAY_HEIGHT_PX 240
#define MAX_NUM_LOG_ROWS 10
#define MAX_NUM_STATUS_ROWS 3

#define MAX_NUM_BUTTONS 5
#define INNER_PADDING_PX 3
#define OUTER_PADDING_PX 6
#define CORNER_RADIUS 8


// button colour palette info
#define SOMETHING_RED 0xF81F
#define SOMETHING_GREEN 0x07E0
#define CHRISTMAS_RED 0xd1a6
#define CHRISTMAS_GREEN 0x0565
// there seems to be an undocumented thing where the first item in the array is the default background colour for the text/canvas?
static uint16_t COLOURS[] = {BLACK, WHITE, SOMETHING_RED, SOMETHING_GREEN, CHRISTMAS_RED, CHRISTMAS_GREEN};


/**
 * Struct storing button properties
 * Struct as UserInterface owns responsibility for calculating button relative positions and sizes
 * This struct simply stores the outputs of that calculation for ease of use later
 */
struct Button {
    String label;
    uint16_t x, y, w, h;
};

/**
 * Struct storing status or log line properties
 * Struct as UserInterface owns responsibility for calculating line relative positions (height of top left pixel, y)
 */
struct TextLine {
    String content;
    uint16_t y;
};

/**
 * UserInterface class specifically targeted at the M5StackCoreS3 built in screen, with h=240px, w=320px
 * 
 * - All groups of elements are zero indexes i.e. first button is buttonID=0
 */
class UserInterface {

public:
    UserInterface() = delete; // prevent default constructor
    UserInterface(M5GFX& display, M5Canvas& canvas); // only constructor, explicitly does nothing internal-facing to delegate configuration to configure() method
    void configure(uint8_t text_size, uint8_t num_buttons, uint8_t num_rows_buttons, uint8_t num_status_rows);

    void clearAllContents(); // TODO clear contents from all elements
    bool drawInitial();
    void drawStatuses();
    void drawLogs();
    void drawButtons();

    void clearStatusContent(); 
    bool clearStatusContent(uint8_t rowID);
    bool setStatusContent(uint8_t rowID, String content);
    bool addStatusContent(uint8_t rowID, String extraContent);

    void clearLogContent();
    bool clearLogContent(uint8_t logID, String content); 
    bool setLogContent(uint8_t logID, String content);
    bool addLogContent(uint8_t logID, String extraContent);

    void clearButtonLabel();
    bool clearButtonLabel(uint8_t buttonID);
    bool setButtonLabel(uint8_t buttonID, String label);
    bool buttonWasPressed(uint8_t buttonID, uint16_t touch_x, uint16_t touch_y);
    

private:

    M5GFX& display;
    M5Canvas& canvas; //= M5Canvas(&display);

    // user input dimensions
    uint8_t text_size; 
    uint8_t num_buttons;
    uint8_t num_rows;
    uint8_t num_rows_status, num_rows_logs, num_rows_buttons;

    // derived dimensions
    uint16_t button_space_h;
    uint16_t button_space_w;
    uint32_t status_row_h; // TODO check over all type widths, not sure they make sense
    uint32_t status_row_w = DISPLAY_WIDTH_PX;

    Button buttons[MAX_NUM_BUTTONS];
    TextLine statuses[MAX_NUM_STATUS_ROWS];
    TextLine logs[MAX_NUM_LOG_ROWS];    
    // TODO do I actually need to store these, or the log content, or just write direct to Gfx?
};

#endif // USER_INTERFACE_H