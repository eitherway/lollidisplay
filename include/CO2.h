/*
 * Part of this code was copied/modified from: https://github.com/airgradienthq/arduino under MIT License
 */

#ifndef CO2_LIBRARY_H
#define CO2_LIBRARY_H

#include <SoftwareSerial.h>

struct CO2_READ_RESULT {
    int co2 = -1;
    bool success = false;
};

// library interface description
class CO2 {
    // user-accessible "public" interface
public:
    CO2(bool displayMsg = false, int baudRate = 9600);
    //void begin(int baudRate=9600);

    static void setOutput(Print &debugOut, bool verbose = true);

    bool _debugMsg;


    static const uint16_t BAUD_RATE = 9600;

    //CO2 VARIABLES PUBLIC START
    void CO2_Init(int, int, int);

    int getCO2_Raw();

    SoftwareSerial *_SoftSerial_CO2;


    // library-accessible "private" interface
private:
    int value;

    //CO2 VARABLES PUBLIC START
    char Char_CO2[10];

    //CO2 VARABLES PUBLIC END

};

#endif