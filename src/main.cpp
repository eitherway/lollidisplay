#ifdef ESP8266

#include <ESP8266WiFi.h>

#endif
#ifdef ESP32
#include <WiFi.h>
#endif

#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <WEMOS_SHT3X.h>
#include <LOLIN_HP303B.h>
#include <Adafruit_SGP30.h>
#include <SSD1306Wire.h>

#include "secrets.h"
#include "secrets-desk.h"
//#include "secrets-floor.h"
//#include "secrets-papa.h"

// local code
#include "display.h"
#include "CO2.h"

#define LED 2
// performs String Concat in Compiler
#define mqttTopic(name) ("home_automation/" MQTT_LOCATION "/" name)
#define HOSTNAME "lolli" MQTT_LOCATION

// Sensors
#if ENABLED_PIR
const int PIR = D3;
#endif
#if ENABLED_DISPLAY
SSD1306Wire display(0x3D, SDA, SCL);
#endif
SHT3X sht30(0x45);
BH1750 lightMeter;
#if ENABLED_CO2
CO2 co2 = CO2();
#endif
LOLIN_HP303B HP303BPressureSensor;
Adafruit_SGP30 sgp30;

// General Stuff
//IPAddress server(192, 168, 8, 102);
WiFiClient wlanclient;
PubSubClient mqttClient(wlanclient);

#if ENABLED_EPD
Display display = Display();
#endif

String convertByteArrayToString(byte *a, unsigned int size) {
    int i;
    String s = "";
    for (i = 0; i < size; i++) {
        s = s + (char) a[i];
    }
    return s;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    if (length > 50) {
        Serial.println("Error! MQTT Payload is a stream.");
        return;
    }

    Serial.print("Message arrived on Topic: ");
    Serial.println(topic);

#if ENABLED_LED_CONTROL
    if (topic.equals(mqttTopic("led"))) {
      // Switch on the LED if an 1 was received as first character
      if ((char)payload[0] == '1') {
        digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
        // but actually the LED is on; this is because
        // it is active low on the ESP-01)
      } else {
        digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
      }
    }
#endif

#if ENABLED_DISPLAY_MQTT
    if (strcmp(topic, "home_automation/test/eco2") == 0) {

        Serial.println("Display should refresh");
        String eco2 = convertByteArrayToString(payload, length);

        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.drawString(0, 10, "CO2");
        display.setFont(ArialMT_Plain_24);
        display.drawString(0, 26, eco2 + " ppm");
        display.display();
    }
#endif
}

void setup() {
    // Initialize Serial
    Serial.begin(9600);
    Serial.println("");

    Serial.println(SECRET_SSID);
    // Set WiFi hostname
    WiFi.mode(WIFI_STA); //Indicate to act as wifi_client only
    WiFi.hostname(HOSTNAME); // needs to be set after Wifi.mode before Wifi.begin

    // Init MQTT
    mqttClient.setServer(SECRET_MQTT_SERVER, 1883);
    mqttClient.setCallback(mqttCallback);

    // Initialize PIR
#if ENABLED_PIR
    pinMode(PIR, INPUT);
#endif

    // Initialize CO2
#if ENABLED_CO2
    co2.CO2_Init(D4, D3, 9600);
#endif

    // Initialise LED
    // cannot be used, because PIN 2 (D4) is used for CO2
#if ENABLED_LED_CONTROL
    pinMode(LED, OUTPUT);
    digitalWrite(LED, HIGH); // LED off
#endif

    // Initialize I2C Bus
    Wire.begin();

    // Initialize Lightmeter
#if ENABLED_LIGHT
    lightMeter.begin();
#endif

    // Initialize Pressure Sensor
#if ENABLED_PRESSURE
    HP303BPressureSensor.begin();
#endif

    // Initialize TVOC & eCO2 Sensor
#if ENABLED_TVOC
    sgp30.begin();
#endif

#if ENABLED_DISPLAY
    display.init();

    display.flipScreenVertically();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
#endif

#if ENABLED_EPD
    display.initDisplay(wlanclient);
#endif
}

void loop() {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting...");

        WiFi.begin(SECRET_SSID, SECRET_PASS);
#ifdef ARDUINO_LOLIN_C3_MINI
        // You have to add this code after running WiFi.softAP or WiFi.begin for the Wifi to work.
        WiFi.setTxPower(WIFI_POWER_8_5dBm);             // https://www.wemos.cc/en/latest/tutorials/c3/get_started_with_arduino_c3.html#wifi
#endif
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("Error! Wifi connection failure");
            Serial.println(WiFi.status());
            delay(1000);
            return;
        }

        Serial.print("Connected, IP address: ");
        Serial.println(WiFi.localIP());
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            // attempt to connect
            if (mqttClient.connect(HOSTNAME)) {
                Serial.println("Connected to MQTT Broker");
#if ENABLED_DISPLAY_MQTT
                mqttClient.subscribe("home_automation/test/eco2");
#endif
                // Subscribe to LED Control
#if ENABLED_LED_CONTROL
                mqttClient.subscribe(mqttTopic("led"));
#endif
            } else {
                Serial.println("MQTT Broker connection failed");
                Serial.println(mqttClient.state());
                delay(1000);
                return;
            }
        }

        // Connected to Wi-Fi and MQTT
        mqttClient.loop();

        /*
         * setup done; read and publish values
         */
        Serial.println("---Start of Measurement Series---");

#if ENABLED_PIR
        int PIRState = digitalRead(PIR);
        if (PIRState == HIGH) {
            Serial.println("Movement: yes");
            mqttClient.publish(mqttTopic("movement"), "1");
        } else {
            Serial.println("Movement: no");
            mqttClient.publish(mqttTopic("movement"), "0");
        }
#endif

        // Read and publish temperature and humidity to MQTT
#if ENABLED_TEMP
        if (sht30.get() == 0) {
            float temperature = sht30.cTemp;
            float humidity = sht30.humidity;

            Serial.print("Temperature in Celsius: ");
            Serial.print(temperature);
            Serial.print("\tRelative Humidity: ");
            Serial.println(humidity);

            if (humidity > 90 || temperature > 60 || temperature < -15 || humidity < 10) {
                // Sensor failure
                // Don't publish data
                Serial.println("Error! Sensor 'SHT30'/'Temperature + Humidity' data seems invalid.");
            } else {
                // Data seems valid
                char temp_string[7]; // 7 = 4 value chars + 1 sign char + 1 dot char + 1 null
                char humid_string[7];
                // char* dtostrf(double __val, signed char __width, unsigned char __prec, char * __s )
                dtostrf(temperature, 7 - 1, 2, temp_string);
                dtostrf(humidity, 7 - 1, 2, humid_string);

                mqttClient.publish(mqttTopic("temperature"), temp_string);

                mqttClient.publish(mqttTopic("humidity"), humid_string);
            }
        } else {
            Serial.println("Error! Reading sensor 'SHT30'/'Temperature + Humidity' data failed.");
        }
#endif

        // Read and publish light value to MQTT
#if ENABLED_LIGHT
        if (lightMeter.measurementReady()) {
            float lux = lightMeter.readLightLevel();

            if (lux < 0) {
                // read failed
                if (lux == -1) {
                    Serial.println("Error! Sensor 'BH1750'/'Ambient Light' data seems invalid.");
                } else if (lux == -2) {
                    Serial.println("Error! Reading sensor 'BH1750'/'Ambient Light' failed.");
                } else {
                    Serial.println("Error! Unknown sensor error 'BH1750'/'Ambient Light'");
                }
            } else {
                // read success
                Serial.print("Light: ");
                Serial.print(lux);
                Serial.println(" lx");

                char light_string[9]; // 6 value chars + 1 dot char + 1 sign char + 1 null
                dtostrf(lux, 9 - 1, 1, light_string);

                mqttClient.publish(mqttTopic("light"), light_string);
            }
        }
#endif

        // Read and publish CO2 Value
#if ENABLED_CO2
        int CO2 = co2.getCO2_Raw();
        if (CO2 != -1 && CO2 != -2) {
            if (CO2 >= 0 && CO2 <= 4000) {
                // read success
                char Char_CO2[10];

                sprintf(Char_CO2, "%d", CO2);

                Serial.print("CO2: ");
                Serial.print(Char_CO2);
                Serial.println(" ppm");

                mqttClient.publish(mqttTopic("co2"), Char_CO2);

#if ENABLED_EPD
                display.setCO2(CO2);
#endif
            } else {
                Serial.println("Error! 'CO2' reading out of range");
            }
        } else {
            Serial.println("Error! Reading sensor 'CO2' data failed.");
        }
#endif

#if ENABLED_PRESSURE
        int32_t pressure;
        // second parameter is oversampling, higher values lead to higher precision, but take more time
        // sensor will take 2^oversampling and combine them
        int16_t ret = HP303BPressureSensor.measurePressureOnce(pressure, 1);
        if (ret != 0) {
            //Something went wrong.
            //Look at the library code for more information about return codes
            Serial.print("ERROR! 'Pressure' Sensor failed. ret = ");
            Serial.println(ret);
        } else {
            // Read Success
            Serial.print("Pressure: ");
            Serial.print(pressure);
            Serial.println(" Pascal");

            char Char_Pressure[20];

            sprintf(Char_Pressure, "%d", pressure);

            mqttClient.publish(mqttTopic("pressure"), Char_Pressure);
        }
#endif

#if ENABLED_TVOC
        if (sgp30.IAQmeasure()) {
            // Read Success
            Serial.print("eCO2: ");
            Serial.print(sgp30.eCO2);
            Serial.print(" ppm\tTVOC: ");
            Serial.print(sgp30.TVOC);
            Serial.println(" ppb");

            char Char_ECO2[20];
            char Char_TVOC[20];

            sprintf(Char_ECO2, "%d", sgp30.eCO2);
            sprintf(Char_TVOC, "%d", sgp30.TVOC);

            mqttClient.publish(mqttTopic("eco2"), Char_ECO2);
            mqttClient.publish(mqttTopic("tvoc"), Char_TVOC);
        } else {
            Serial.print("ERROR! 'TVOC' Sensor failed.");
        }
#endif

#if ENABLED_EPD
        display.refreshWeatherInformation();
        display.refreshDisplay();
#endif

        Serial.println("---End of Measurement Series-----");

        // Delay each Measurement a bit
        delay(3000);
    }
}
