#include <Arduino.h>
#include <sntp.h>
#include "display.h"
#include "Bitmaps.h"

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
    configTime(1, 3600, "192.168.3.1");
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
    EPD.drawBitmap(0,0,epd_bitmap_epaper_dashboard,250,122,EPD_WHITE, EPD_BLACK);

    EPD.setTextColor(EPD_BLACK);
    if (co2 > 1000) {
        EPD.setTextColor(EPD_RED);
    }

    EPD.setTextSize(2);
    EPD.setCursor(12, 87);
    String co2Str = String(co2);
    EPD.println(co2Str);

    EPD.display();

    // set refresh time
    last_refresh_timestamp = current_time;
    last_co2 = co2;
}

