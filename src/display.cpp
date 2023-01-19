#include <Arduino.h>
#include <sntp.h>
#include "display.h"
#include "Bitmaps.h"
#include "secrets.h"

uint32 last_refresh_timestamp = 0;
int last_co2 = 0;

LOLIN_SSD1680 EPD(250, 122, EPD_MOSI, EPD_CLK, EPD_DC, EPD_RST, EPD_CS, EPD_BUSY); //IO

unsigned int absVal(int x) {
    if (x < 0) {
        return -1 * x;
    } else {
        return x;
    }
}

void initDisplay() {
    configTime(1, 3600, SECRET_NTP_SERVER);
    EPD.begin();
    EPD.setRotation(0);
}

void refreshDisplay(int co2) {
    uint32 current_time = sntp_get_current_timestamp();

    bool refreshDisplay = false;
    if ((current_time - 15 * 60) > last_refresh_timestamp) {
        // longer than 15min since last refresh
        //Serial.println("Time: ");
        //Serial.println(current_time);
        refreshDisplay = true;
    }

    unsigned int co2Diff = absVal(co2 - last_co2);
    if ((co2 > 1000 && last_co2 < 1000) || co2Diff > 200) {
        refreshDisplay = true;
    }

    if (!refreshDisplay) {
        return;
    }

    // refresh display
    EPD.clearBuffer();
    // somehow bg-color & color need to be switched
    EPD.drawBitmap(0, 0, epd_bitmap_epaper_dashboard, 250, 122, EPD_WHITE, EPD_BLACK);

    // print Weather Info
    EPD.setTextColor(EPD_BLACK);
    String weatherStr = "Winds subsiding with flurries.";
    EPD.setTextSize(2);

    // 19 chars fit in a row in my bitmap
    // each char is 12x16 with this default font
    const unsigned int numOfCharsThatFitInRow = 19;

    if (weatherStr.length() > numOfCharsThatFitInRow) {
        String hardcutRow1 = weatherStr.substring(0, numOfCharsThatFitInRow - 1);
        size_t posSpace = hardcutRow1.lastIndexOf(" ");

        // this lets the row end on the end of a word
        String row1 = weatherStr.substring(0, posSpace);

        String row2 = weatherStr.substring(posSpace + 1);

        if (row2.length() > numOfCharsThatFitInRow) {
            // row2 exceeds limit, hardcut at char limit
            row2 = weatherStr.substring(posSpace + 1, posSpace + numOfCharsThatFitInRow + 1);
        }

        EPD.setCursor(10, 27);
        EPD.println(row1);
        EPD.setCursor(10, 43);
        EPD.println(row2);
    } else {
        EPD.setCursor(10, 27);
        EPD.println(weatherStr);
    }

    // print CO2 value
    if (co2 > 1000) {
        EPD.setTextColor(EPD_RED);
    } else {
        EPD.setTextColor(EPD_BLACK);
    }

    EPD.setTextSize(2);
    EPD.setCursor(10, 96);
    String co2Str = String(co2);
    EPD.println(co2Str);

    EPD.display();

    // set refresh time
    last_refresh_timestamp = current_time;
    last_co2 = co2;
}

