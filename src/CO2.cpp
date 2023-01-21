/*
 * Part of this code was copied/modified from: https://github.com/airgradienthq/arduino under MIT License
 */

#include "CO2.h"

CO2::CO2(bool displayMsg, int baudRate)
{
    _debugMsg = displayMsg;

    Serial.begin(baudRate);

    if (_debugMsg) {
        Serial.println("CO2 Library instantiated successfully.");
    }
}

void CO2::CO2_Init(int rx_pin,int tx_pin,int baudRate){
    if (_debugMsg) {
        Serial.println("Initializing CO2...");
    }
    _SoftSerial_CO2 = new SoftwareSerial(rx_pin,tx_pin);
    _SoftSerial_CO2->begin(baudRate);

    if(getCO2_Raw() == -1){
        if (_debugMsg) {
            Serial.println("CO2 Sensor Failed to Initialize ");
        }
    }
    else{
        Serial.println("CO2 Successfully Initialized. Heating up for 10s");
        delay(10000);
    }
}

int CO2::getCO2_Raw() {

    while (_SoftSerial_CO2->available())  // flush whatever we might have
        _SoftSerial_CO2->read();

    const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
    byte CO2Response[] = {0, 0, 0, 0, 0, 0, 0};

    // tt
    int datapos = -1;
    //

    const int commandSize = 7;

    int numberOfBytesWritten = _SoftSerial_CO2->write(CO2Command, commandSize);

    if (numberOfBytesWritten != commandSize) {
        // failed to write request
        return -2;
    }

    // attempt to read response
    int timeoutCounter = 0;
    while (_SoftSerial_CO2->available() < commandSize) {
        timeoutCounter++;
        if (timeoutCounter > 10) {
            // timeout when reading response
            return -3;
        }
        delay(50);
    }

    // we have 7 bytes ready to be read
    for (int i = 0; i < commandSize; i++) {
        CO2Response[i] = _SoftSerial_CO2->read();

        // tt
        if ((CO2Response[i] == 0xFE) && (datapos == -1)) {
            datapos = i;
        }

        if (_debugMsg) {
            Serial.print(CO2Response[i], HEX);
            Serial.print(":");
        }
        //
    }
    // return CO2Response[3]*256 + CO2Response[4];
// tt
    return CO2Response[datapos + 3] * 256 + CO2Response[datapos + 4];
    //
}
