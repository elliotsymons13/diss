#include <Arduino.h>
#include <M5GFX.h>
M5GFX display;

//#include <M5UnitOLED.h>
//M5UnitOLED display; // default setting
//M5UnitOLED display ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5UnitLCD.h>
//M5UnitLCD display;  // default setting
//M5UnitLCD display  ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5UnitGLASS2.h>
//M5UnitGLASS2 display;  // default setting
//M5UnitGLASS2 display ( 21, 22, 400000 ); // SDA, SCL, FREQ

//#include <M5AtomDisplay.h>
//M5AtomDisplay display;

M5Canvas canvas(&display);


// colour palette info
#define CHRISTMAS_RED 0xd1a6
#define CHRISTMAS_GREEN 0x0565
// there seems to be an undocumented thing where the first item in the array is the default background colour for hte text/canvas?
static uint16_t pale[] = {BLACK, WHITE, CHRISTMAS_RED, CHRISTMAS_GREEN};


static constexpr char text[] = "Merry Christmas fellow science Z***ots. Time to drink and be merry. Yipee ki yay mother******. Die hard IS a christmas movie, I will die on this hill. ";
static constexpr size_t textlen = sizeof(text) / sizeof(text[0]);
int textpos = 0;
int scrollstep = 2;


void setup(void) 
{
  Serial.begin(115200);
  display.begin();
  display.setColorDepth(8);

  if (display.isEPD())
  {
    scrollstep = 16;
    display.setEpdMode(epd_mode_t::epd_fastest);
    display.invertDisplay(true);
    display.clear(TFT_BLACK);
  }
  if (display.width() < display.height())
  {
    display.setRotation(display.getRotation() ^ 1);
  }


  canvas.setFont(&fonts::lgfxJapanMinchoP_32);
  canvas.setTextWrap(false);
  canvas.setTextSize(2);

  canvas.setColorDepth(lgfx::v1::palette_8bit);
  canvas.createSprite(display.width() + 64, 72);
  canvas.createPalette(pale, 256);
  canvas.setTextColor(canvas.getPaletteIndex(CHRISTMAS_RED));

}

bool nextGreen = false;

// one loop per letter
void loop(void)
{
  // move cursor left by scrollstep each loop(), resetting to right edge of screen when it reaches the left
  int32_t cursor_x = canvas.getCursorX() - scrollstep; 
  if (cursor_x <= 0)
  {
    textpos = 0;
    cursor_x = display.width();
  }

  canvas.setCursor(cursor_x, 0);
  canvas.scroll(-scrollstep, 0);
  while (textpos < textlen && cursor_x <= display.width())
  {
    if (nextGreen == true) {
      canvas.setTextColor(canvas.getPaletteIndex(CHRISTMAS_GREEN));
      // Serial.println("This print: green");
    } else {
      canvas.setTextColor(canvas.getPaletteIndex(CHRISTMAS_RED));
      // Serial.println("This print: red");
    }
    nextGreen = !nextGreen;

    canvas.print(text[textpos++]);
    cursor_x = canvas.getCursorX();
  }
  display.waitDisplay();
  int y = (display.height() - canvas.height()) >> 1; // venter vertically 

  canvas.pushSprite(&display, 0, y);
//*/
}
