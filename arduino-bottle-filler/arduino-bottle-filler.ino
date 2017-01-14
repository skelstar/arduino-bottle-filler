
#include "paj7620.h"
#include <myWifiHelper.h>

#include <Adafruit_NeoPixel.h>

/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            15
#define NUMPIXELS      12

#define SUCCESSFUL_CONNECT  1

int head = 4;
#define TRAIL_LENGTH    6
int brightnesses[] = {140,125,75,30,10,5,0,0,0,0,0,0};
bool lightsOn = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 400;
unsigned long timer;


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
    for (int i=0; i<NUMPIXELS; i+=2)
        pixels.setPixelColor(i, pixels.Color(150,0,0));
    pixels.show(); // This sends the updated pixel color to the hardware.

    wifiHelper.setupWifi();

    wifiHelper.setupOTA("arduino-bottle-filler2");
}

/* ----------------------------------------------------------- */

void loop() {

    for (int i=0; i< 12; i++) {
        pixels.setPixelColor(i, pixels.Color(0,0,0));
    }
    pixels.show();
    
    ArduinoOTA.handle();
}


