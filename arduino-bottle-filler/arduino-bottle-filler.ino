
#include "paj7620.h"
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <TM1637.h>     // 7 Segment display


/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            15
#define NUMPIXELS      12

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ400);


// BUTTON
#define BUTTON_PIN  14

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, 2000, 1, listener_Button);

// WIFI
/* ----------------------------------------------------------- */
void wifiMsgCallback(char* message);

MyWifiHelper wifiHelper(wifiMsgCallback);

void wifiMsgCallback(char* message) {
    Serial.println(message);
}

// CLOCK
int8_t TimeDisp[] = {0x01,0x02,0x03,0x04};
// unsigned char ClockPoint = 1;
// unsigned char Update;
// unsigned char halfsecond = 0;
// unsigned char second;
// unsigned char minute = 0;
// unsigned char hour = 12;

#define CLK 5   // SCL
#define DIO 4   // SDA
TM1637 tm1637(CLK,DIO);

/* ----------------------------------------------------------- */
void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");

    pixels.begin();
    for (int i=0; i<NUMPIXELS; i++)
        pixels.setPixelColor(i, pixels.Color(30,0,0));
    pixels.show(); // This sends the updated pixel color to the hardware.

    tm1637.set();
    tm1637.init();

    wifiHelper.setupWifi();

    wifiHelper.setupOTA("arduino-bottle-filler2");
}

/* ----------------------------------------------------------- */

void loop() {

    ArduinoOTA.handle();

    button.serviceEvents();

    tm1637.display(TimeDisp);
    delay(100);
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            Serial.println("EV_BUTTON_PRESSED");
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
            Serial.println("EV_HELD_FOR_LONG_ENOUGH");
            break;
        
        case button.EV_RELEASED:
            Serial.println("EV_RELEASED");
            break;
    }
}

