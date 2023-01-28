#include <LOLIN_EPD.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <sntp.h>
#include "ESP8266HTTPClient.h"

#include "Bitmaps.h"
#include "secrets.h"

#ifndef DISPLAY_CLASS_H
#define DISPLAY_CLASS_H

// D1 mini
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

class Display {
public:
    Display();

    void initDisplay(WiFiClient clientParam);

    void refreshDisplay();

    void setCO2(int co2Param);

    void refreshWeatherInformation();

    void refreshSunset();

private:
    unsigned int last_refresh_timestamp = 0;
    bool shouldRefreshDisplay();

    int co2 = -1;
    int displayedCO2 = 0;

    WiFiClient client;

    String weatherStr;
    String displayedWeatherStr;
    int last_weather_refresh_counter = -1;
    bool shouldRefreshWeather();

    String sunsetString;
    String displayedSunsetString;
    int last_sunset_refresh_counter = -1;
    bool shouldRefreshSunset();
};

#endif