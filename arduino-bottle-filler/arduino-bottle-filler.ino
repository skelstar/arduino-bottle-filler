
#include "paj7620.h"
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>

/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            15
#define NUMPIXELS      12

//#define SUCCESSFUL_CONNECT  1

//int head = 4;
//#define TRAIL_LENGTH    6
//int brightnesses[] = {140,125,75,30,10,5,0,0,0,0,0,0};
//bool lightsOn = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ400);

//int delayval = 400;
//unsigned long timer;

#define BUTTON_PIN  14

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, 2000, 1, listener_Button);

/* ----------------------------------------------------------- */
void wifiMsgCallback(char* message);

MyWifiHelper wifiHelper(wifiMsgCallback);

void wifiMsgCallback(char* message) {
    Serial.println(message);
}

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

    wifiHelper.setupWifi();

    wifiHelper.setupOTA("arduino-bottle-filler2");
}

/* ----------------------------------------------------------- */

void loop() {

    ArduinoOTA.handle();

    button.serviceEvents();
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

