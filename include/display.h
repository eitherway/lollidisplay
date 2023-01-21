#include <LOLIN_EPD.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>


/*D1 mini*/
#ifdef ARDUINO_ESP8266_WEMOS_D1MINI
#define EPD_CS D0
#define EPD_DC D8
#define EPD_RST -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_MOSI D7
#define EPD_CLK D5
#endif

// C3 Mini
#ifdef ARDUINO_LOLIN_C3_MINI
#define EPD_CS 1
#define EPD_DC 5
#define EPD_RST -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1 // can set to -1 to not use a pin (will wait a fixed delay)
#define EPD_MOSI 4
#define EPD_CLK 2
#endif

void initDisplay();
void refreshDisplay(int co2);
