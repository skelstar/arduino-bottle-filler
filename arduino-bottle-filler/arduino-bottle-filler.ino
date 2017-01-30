
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>
#include <TaskScheduler.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


#define NEOPIXEL_PIN            D5
#define NUMPIXELS      1
#define DELETE_WINDOW   10000    // ms

char versionText[] = "MQTT Bottle Feeder v0.9";


/* ----------------------------------------------------------- */

// NEOPIXEL

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_BUTTON_PRESSED = pixels.Color(0, 100, 0);
uint32_t COLOR_NORMAL = pixels.Color(100, 0, 0);
uint32_t COLOR_DELETED = pixels.Color(255, 0, 0);
uint32_t COLOR_OFF = pixels.Color(0, 0, 0);

/* ----------------------------------------------------------- */

void ledOnCallback();
void ledOffCallback();
void deleteWindowCallback();


Scheduler runner;

#define FLASH_TWICE 4
#define FLASH_TEN_TIMES 20

Task tConfirmDeleteBlinker(100, FLASH_TWICE, &ledOffCallback, &runner, false);
Task tDeleteWindowFlasher(500, TASK_FOREVER, &ledOffCallback, &runner, false);
Task tDeleteWindowTimer(DELETE_WINDOW, 1, &deleteWindowCallback, &runner, false);

void ledOnCallback() {
    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();
    tConfirmDeleteBlinker.setCallback(&ledOffCallback);
    tDeleteWindowFlasher.setCallback(&ledOffCallback);
}

void ledOffCallback() {
    pixels.setPixelColor(0, COLOR_OFF);
    pixels.show();
    tConfirmDeleteBlinker.setCallback(&ledOnCallback);
    tDeleteWindowFlasher.setCallback(&ledOnCallback);
}

void deleteWindowCallback() {
    tDeleteWindowFlasher.disable();
    pixels.setPixelColor(0, COLOR_NORMAL);
}

/* ----------------------------------------------------------- */

// BUTTON
#define BUTTON_PIN  D4

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, 1000, 1, listener_Button);

// WIFI
/* ----------------------------------------------------------- */

#define FEEDBOTTLE_FEED  "/dev/bottleFeed"
#define HHmm_FEED        "/dev/HHmm"
#define WIFI_OTA_NAME   "arduino-bottle-filler"
#define WIFI_HOSTNAME   "arduino-bottle-filler"

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/* ----------------------------------------------------------- */
// CLOCK

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

int8_t TimeDisp[] = {0x01,0x02,0x03,0x04};

#define CLK D1   // SCL
#define DIO D2   // SDA
SevenSegmentExtended sevenSeg(CLK,DIO);

long lastReadTime = millis();
long lastLongPressTime = 0;

char msg[5];
int hour = 0;
int minute = 0;

/* ----------------------------------------------------------- */

void devtime_callback(byte* payload, unsigned int length) {

    hour = (payload[0]-'0') * 10;
    hour += payload[1]-'0';
    minute = (payload[3]-'0') * 10;
    minute += payload[4]-'0';

    pinMode(0, OUTPUT);
    digitalWrite(0, LOW);
    delay(100);
    digitalWrite(0, HIGH);

    lastReadTime = millis();
}

void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");
    Serial.println(versionText);

    pixels.begin();
    pixels.show(); // This sends the updated pixel color to the hardware.

    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();

    sevenSeg.begin(); 
    sevenSegClear();

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_OTA_NAME);

    runner.startNow();
    Serial.println("Initialized scheduler");

    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(HHmm_FEED, devtime_callback);
}

/* ----------------------------------------------------------- */

void loop() {

    // need this for both subscribing and publishing
    wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    button.serviceEvents();

    runner.execute();

    delay(100);
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_BUTTON_PRESSED");
            pixels.setPixelColor(0, COLOR_BUTTON_PRESSED);
            pixels.show();
            Serial.println("EV_BUTTON_PRESSED");
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH: {
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_HELD_FOR_LONG_ENOUGH");
            Serial.println("EV_HELD_FOR_LONG_ENOUGH");

            long sinceLastHeld = millis()-lastReadTime;
            Serial.print("Since last held (ms): "); Serial.println(sinceLastHeld);
            if (sinceLastHeld > DELETE_WINDOW) {
                // update display with new time
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_HELD_FOR_LONG_ENOUGH");
                sevenSegDisplayTime();
                tDeleteWindowFlasher.restart();
                tDeleteWindowTimer.restart();
                lastReadTime = millis();
            } else {
                // delete/clear last time
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_FEED_TIME_CLEARED");
                pixels.setPixelColor(0, COLOR_DELETED);
                pixels.show();
                sevenSegClear();
                tConfirmDeleteBlinker.restart();
                lastReadTime = 0;   // not millis() as we want to re-trigger quickly
            }
            }
            break;
        
        case button.EV_RELEASED:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_RELEASED");
            Serial.println("EV_RELEASED");
            pixels.setPixelColor(0, COLOR_NORMAL);
            pixels.show();
            break;
        
        case button.EV_DOUBLETAP:
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_DOUBLETAP");
            Serial.println("EV_DOUBLETAP");
            break;
    }
}

void sevenSegClear() {
    sevenSeg.setBacklight(LOW_BRIGHT);
    sevenSeg.print("----");
}

void sevenSegDisplayTime() {
    sevenSeg.setBacklight(MED_BRIGHT);
    sevenSeg.printTime(hour, minute, false);
}

