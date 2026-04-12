
#include <M5GFX.h>
#include <roo_logging.h> // https://github.com/dejwk/roo_logging
M5GFX display;



M5Canvas canvas(&display);


// colour palette info
#define CHRISTMAS_RED 0xd1a6
#define CHRISTMAS_GREEN 0x0565
// there seems to be an undocumented thing where the first item in the array is the default background colour for hte text/canvas?
static uint16_t pale[] = {BLACK, WHITE, CHRISTMAS_RED, CHRISTMAS_GREEN};



void setup(void) 
{
  Serial.begin(115200);
  display.begin();
  display.setColorDepth(8);

  canvas.setTextWrap(false);
  canvas.setTextSize(2);

  canvas.setColorDepth(lgfx::v1::palette_8bit);
  canvas.createSprite(display.width(), display.height());
  canvas.createPalette(pale, 256);
  canvas.setTextColor(canvas.getPaletteIndex(CHRISTMAS_RED));

  canvas.setCursor(0, 0);
  canvas.print("c");
  display.waitDisplay();
  canvas.pushSprite(&display, 0, 0);

  LOG(INFO) << "Display is " << display.width() << " wide by " << display.height() << " tall. ";

}


// one loop per letter
void loop(void)
{

//*/
}
