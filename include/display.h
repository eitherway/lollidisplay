#include <LOLIN_EPD.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>

/*D1 mini*/
#define EPD_CS D0
#define EPD_DC D8
#define EPD_RST -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_MOSI D7
#define EPD_CLK D5

void initDisplay();
void refreshDisplay(int co2);
