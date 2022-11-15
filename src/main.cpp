#include <WiFi.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <WEMOS_SHT3X.h>
#include "secrets.h"
//#include <AirGradient.h>

#define LED 2
// performs String Concat in Compiler
#define mqttTopic(name) ("home_automation/" MQTT_LOCATION "/" name)

SHT3X sht30(0x45);
BH1750 lightMeter;
AirGradient ag = AirGradient();

WiFiClient wlanclient;
PubSubClient mqttClient(wlanclient, SECRET_MQTT_SERVER);

void mqttCallback(const MQTT::Publish &pub) {
    if (pub.has_stream()) {
        Serial.println("Error! MQTT Payload is a stream.");
        return;
    }

    String topic = pub.topic();
    String payload = pub.payload_string();

    Serial.print("Message arrived on Topic:");
    Serial.print(topic);
    Serial.print(" => ");
    Serial.println(payload);

#if ENABLED_LED_CONTROL
    if (topic.equals(mqttTopic("led"))) {
        if (payload.equals("ON")) {
            digitalWrite(LED, LOW);
        } else if (payload.equals("OFF")) {
            digitalWrite(LED, HIGH); // LED off
        }
    }
#endif
}

void setup() {
    // Initialize Serial
    Serial.begin(9600);
    Serial.println("");

    // Initialize CO2
#if ENABLED_CO2
    ag.CO2_Init();
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
}

void loop() {

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Connecting...");

        //Set new hostname
        WiFi.hostname('lolli' + MQTT_LOCATION);

        WiFi.mode(WIFI_STA); //Indicate to act as wifi_client only
        WiFi.begin(SECRET_SSID, SECRET_PASS);

        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            Serial.println("Error! Wifi connection failure");
            delay(1000);
            return;
        }

        Serial.print("Connected, IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Connected, Hostname: ");
        Serial.println(WiFi.hostname());
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            if (mqttClient.connect("ESP_Client")) {
                Serial.println("Connected to MQTT Broker");
                mqttClient.set_callback(mqttCallback);

                // Subscribe to LED Control
#if ENABLED_LED_CONTROL
                mqttClient.subscribe(mqttTopic("led"));
#endif
            }

            if (!mqttClient.connected()) {
                Serial.println("MQTT Broker connection failed");
                delay(1000);
                return;
            }
        }

        // Connected to Wi-Fi and MQTT

        mqttClient.loop();

        /*
         * setup done; read and publish values
         */

        // Read and publish temperature and humidity to MQTT
#if ENABLED_TEMP
        if (sht30.get() == 0) {
            float temperature = sht30.cTemp;
            float humidity = sht30.humidity;

            Serial.print("Temperature in Celsius: ");
            Serial.print(temperature);
            Serial.print(" Relative Humidity: ");
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

                MQTT::Publish temp_pub(mqttTopic("temperature"), temp_string);
                mqttClient.publish(temp_pub);

                MQTT::Publish humid_pub(mqttTopic("humidity"), humid_string);
                mqttClient.publish(humid_pub);
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

                MQTT::Publish light_pub(mqttTopic ("light"), light_string);
                mqttClient.publish(light_pub);
            }
        }
#endif

        // Read and publish CO2 Value
#if ENABLED_CO2
        int CO2 = ag.getCO2_Raw();
        if (CO2 != -1 && CO2 != -2) {
            if (CO2 >= 0 && CO2 <= 4000) {
                // read success
                char Char_CO2[10];

                sprintf(Char_CO2, "%d", CO2);

                Serial.print("CO2: ");
                Serial.print(Char_CO2);
                Serial.println(" ppm");

                MQTT::Publish co2_pub(mqttTopic ("co2"), Char_CO2);
                mqttClient.publish(co2_pub);
            } else {
                Serial.println("Error! 'CO2' reading out of range");
            }
        } else {
            Serial.println("Error! Reading sensor 'CO2' data failed.");
        }
#endif

        // Do measurements every second
        delay(2000);
    }
}
