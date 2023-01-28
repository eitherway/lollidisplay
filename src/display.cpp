#include "display.h"

LOLIN_SSD1680 EPD(250, 122, EPD_MOSI, EPD_CLK, EPD_DC, EPD_RST, EPD_CS, EPD_BUSY); //IO

unsigned int absVal(int x) {
    if (x < 0) {
        return -1 * x;
    } else {
        return x;
    }
}

Display::Display() {

};

void Display::initDisplay(WiFiClient clientParam) {
    configTime(1, 3600, SECRET_NTP_SERVER);
    EPD.begin();
    EPD.setRotation(0);

    client = clientParam;
}

void Display::setCO2(int co2Param) {
    co2 = co2Param;
}

void Display::refreshWeatherInformation() {
    if (!shouldRefreshWeather()) {
        return;
    }

    HTTPClient http;

    String serverPath = SECRET_COLLECTOR_SERVER "/weather";

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        weatherStr = http.getString();
        Serial.println("Weather: " + weatherStr);
    } else if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
}

bool Display::shouldRefreshWeather() {
    bool shouldRefresh = false;

    if (last_weather_refresh_counter == -1) {
        shouldRefresh = true;
    } else if (last_weather_refresh_counter > 100) {
        shouldRefresh = true;
        last_weather_refresh_counter = -1;
    }

    last_weather_refresh_counter++;

    return shouldRefresh;
}

void Display::refreshSunset() {
    if (!shouldRefreshSunset()) {
        return;
    }

    HTTPClient http;

    String serverPath = SECRET_COLLECTOR_SERVER "/sunset";

    // Your Domain name with URL path or IP address with path
    http.begin(client, serverPath.c_str());

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode == 200) {
        sunsetString = http.getString();
        Serial.println("Sunset: " + sunsetString);
    } else if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
    } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
    }

    // Free resources
    http.end();
}

bool Display::shouldRefreshSunset() {
    bool shouldRefresh = false;

    if (last_sunset_refresh_counter == -1) {
        shouldRefresh = true;
    } else if (last_sunset_refresh_counter > 100) {
        shouldRefresh = true;
        last_sunset_refresh_counter = -1;
    }

    last_sunset_refresh_counter++;

    return shouldRefresh;
}

bool Display::shouldRefreshDisplay() {
#ifdef ESP8266
    unsigned int current_time = sntp_get_current_timestamp();
#endif
#ifdef ESP32
    unsigned int current_time = 0;
#endif

    bool refreshDisplay = false;
    if ((current_time - 15 * 60) > last_refresh_timestamp) {
        // longer than 15min since last refresh
        //Serial.println("Time: ");
        //Serial.println(current_time);
        refreshDisplay = true;
    }

    unsigned int co2Diff = absVal(co2 - displayedCO2);
    if ((co2 > 1000 && displayedCO2 < 1000) || co2Diff > 200) {
        refreshDisplay = true;
    }

    if (weatherStr != displayedWeatherStr) {
        refreshDisplay = true;
    }

    if (sunsetString != displayedSunsetString) {
        refreshDisplay = true;
    }

    if (co2 == -1) {
        refreshDisplay = false;
    }

    // set refresh time
    last_refresh_timestamp = current_time;

    return refreshDisplay;
}

void Display::refreshDisplay() {
    bool refreshDisplay = shouldRefreshDisplay();

    if (!refreshDisplay) {
        return;
    }

    // refresh display
    EPD.clearBuffer();
    // somehow bg-color & color need to be switched
    EPD.drawBitmap(0, 0, epd_bitmap_epaper_dashboard, 250, 122, EPD_WHITE, EPD_BLACK);

    // print Weather Info
    EPD.setTextColor(EPD_BLACK);
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

    // print Sunset Time
    EPD.setTextColor(EPD_BLACK);
    EPD.setTextSize(2);
    EPD.setCursor(100, 96);

    if (sunsetString.length() == 5) {
        EPD.println(sunsetString);
    } else {
        sunsetString = "error";
        EPD.println("error");
    }

    EPD.display();

    // update info for refresh algorithm
    displayedCO2 = co2;
    displayedWeatherStr = weatherStr;
    displayedSunsetString = sunsetString;
}

