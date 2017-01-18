
#include "paj7620.h"
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>

/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            15
#define NUMPIXELS      12

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ400);


// BUTTON
#define BUTTON_PIN  14

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, 1000, 1, listener_Button);

// WIFI
/* ----------------------------------------------------------- */
void wifiMsgCallback(char* message);

MyWifiHelper wifiHelper(wifiMsgCallback);

void wifiMsgCallback(char* message) {
    Serial.println(message);
}

#define FEEDBOTTLE_FEED  "dev/bottleFeed"

// CLOCK

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

int8_t TimeDisp[] = {0x01,0x02,0x03,0x04};

#define CLK 5   // SCL
#define DIO 4   // SDA
SevenSegmentExtended tm1637(CLK,DIO);

long lastTimeRead = millis();

/* ----------------------------------------------------------- */

void devtime_callback(byte* payload, unsigned int length) {

    if (payload[4] <= '9') {
        int hour = (payload[0]-'0') * 10;
        hour += payload[1]-'0';
        int minute = (payload[3]-'0') * 10;
        minute += payload[4]-'0';
        tm1637.setBacklight(MED_BRIGHT);
        tm1637.printTime(hour, minute, false);
    } else {
        char msg[5];
        msg[0] = payload[0];
        msg[1] = payload[1];
        msg[2] = payload[2];
        msg[3] = payload[3];
        msg[4] = NULL;
        tm1637.print(msg);
    }

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    delay(100);
    digitalWrite(0, HIGH);

    lastTimeRead = millis();
}

bool mqttTimeOnline() {
    bool online =  lastTimeRead + 65000 > millis();
    if (online == false) {
        Serial.print("lastTimeRead: "); Serial.print(lastTimeRead); Serial.print(" millis(): "); Serial.println(millis());
    }
    return online;
} 

void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");

    pixels.begin();
    for (int i=0; i<NUMPIXELS; i++)
        pixels.setPixelColor(i, pixels.Color(30,0,0));
    pixels.show(); // This sends the updated pixel color to the hardware.

    tm1637.begin();          
    tm1637.setBacklight(LOW_BRIGHT);
    tm1637.print("----");

    wifiHelper.setupWifi();

    wifiHelper.setupOTA("arduino-bottle-filler2");

    wifiHelper.setupMqtt();
    wifiHelper.mqttAddSubscription("dev/bottle-feed-time", devtime_callback);
}

/* ----------------------------------------------------------- */

void loop() {

    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    button.serviceEvents();
    delay(100);
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_BUTTON_PRESSED");
            Serial.println("EV_BUTTON_PRESSED");
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_HELD_FOR_LONG_ENOUGH");
            Serial.println("EV_HELD_FOR_LONG_ENOUGH");
            break;
        
        case button.EV_RELEASED:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_RELEASED");
            Serial.println("EV_RELEASED");
            break;
        
        case button.EV_DOUBLETAP:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_DOUBLETAP");
            Serial.println("EV_DOUBLETAP");
            break;
    }
}

