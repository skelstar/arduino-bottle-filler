
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>

#include <EventManager.h>

// #include <TaskScheduler.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


#define NEOPIXEL_PIN        D5
#define NUMPIXELS           1
#define LONG_PRESS_TIME     1000

char versionText[] = "MQTT Bottle Feeder v0.9.1";

/* ------------------------------------------------------------ */

EventManager thisEvm;
#define EVENT_CODE EventManager::kEventUser5
int _state = 0;

/* ----------------------------------------------------------- */

// NEOPIXEL

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_BUTTON_PRESSED = pixels.Color(0, 50, 0);
uint32_t COLOR_NORMAL = pixels.Color(50, 0, 0);
uint32_t COLOR_DELETED = pixels.Color(100, 0, 0);
uint32_t COLOR_OFF = pixels.Color(0, 0, 0);

/* ----------------------------------------------------------- */

void ledOnCallback();
void ledOffCallback();
void deleteWindowCallback();


//Scheduler runner;

#define FLASH_ONCE 2
#define FLASH_TWICE 4
#define FLASH_TEN_TIMES 20

uint32_t oldColor;

// Task tOneFlash(100, FLASH_ONCE, &ledOffCallback, &runner, false);
// Task tConfirmDeleteBlinker(100, FLASH_TWICE, &ledOffCallback, &runner, false);

/*
void ledOnCallback() {
    pixels.setPixelColor(0, oldColor);
    pixels.show();
    tOneFlash.setCallback(&ledOffCallback);
    tConfirmDeleteBlinker.setCallback(&ledOffCallback);
}

void ledOffCallback() {
    oldColor = pixels.getPixelColor(0);
    pixels.setPixelColor(0, COLOR_OFF);
    pixels.show();
    tOneFlash.setCallback(&ledOnCallback);
    tConfirmDeleteBlinker.setCallback(&ledOnCallback);
}

void deleteWindowCallback() {
    //tDeleteWindowFlasher.disable();
    pixels.setPixelColor(0, COLOR_NORMAL);
}
*/

/* ----------------------------------------------------------- */

// BUTTON
#define BUTTON_PIN  D4

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, LONG_PRESS_TIME, 1, listener_Button);

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

char TimeDisp[] = "--:--";
char OutBuffer[] = "--:--";

#define CLK D1   // SCL
#define DIO D2   // SDA
SevenSegmentExtended sevenSeg(CLK,DIO);

volatile long lastReadTime = millis();
long lastLongPressTime = 0;

char msg[5];
volatile int hour = 0;
volatile int minute = 0;

bool callback_occurred = false;

/* ----------------------------------------------------------- */

#define FLASH_NONE      0
#define FLASH_OFF       1
#define FLASH_OFF_TO_ON 2
#define FLASH_ON        3
#define FLASH_FINISHED  4

long flashOffTime = 0;

void listener_FlashingLed(int eventCode, int eventParams) {

    if (eventParams == FLASH_OFF) {
        flashOffTime = millis();
        pixels.setPixelColor(0, COLOR_OFF);
    }

    if (eventParams == FLASH_ON) {
        // todo : get pixel color before flashing
        pixels.setPixelColor(0, COLOR_NORMAL);
        _state = FLASH_FINISHED;
    }
}

/* ----------------------------------------------------------- */

void devtime_callback(byte* payload, unsigned int length) {

    TimeDisp[0] = payload[0];
    TimeDisp[1] = payload[1];
    TimeDisp[3] = payload[3];
    TimeDisp[4] = payload[4];

    callback_occurred = true;

    // hour = (payload[0]-'0') * 10;
    // hour += payload[1]-'0';
    // minute = (payload[3]-'0') * 10;
    // minute += payload[4]-'0';

    // pinMode(0, OUTPUT);
    // digitalWrite(0, LOW);
    // delay(100);
    // digitalWrite(0, HIGH);

    // lastReadTime = millis();
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

    thisEvm.addListener(EVENT_CODE, listener_FlashingLed);

    Serial.println("Initialized scheduler");

    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(HHmm_FEED, devtime_callback);
}

/* ----------------------------------------------------------- */

void loop() {

    // need this for both subscribing and publishing
    if (!wifiHelper.loopMqttNonBlocking()) {
        Serial.print('.');
    }

    ArduinoOTA.handle();

    serviceFlashingLedEvents();

    button.serviceEvents();

    if (callback_occurred) {
        callback_occurred = false;

        thisEvm.queueEvent(EVENT_CODE, FLASH_OFF);
        _state = FLASH_OFF;
    }
}

#define FLASH_OFF_PERIOD 100

void serviceFlashingLedEvents() {

    if (_state == FLASH_OFF) {
        _state = FLASH_OFF_TO_ON;
        flashOffTime = millis();
    }

    if (_state == FLASH_OFF_TO_ON && millis()-flashOffTime >= FLASH_OFF_PERIOD) {
        thisEvm.queueEvent(EVENT_CODE, FLASH_ON);
        _state = FLASH_ON;
    }

    thisEvm.processEvent(); // i.e. trigger listeners
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_BUTTON_PRESSED");
            pixels.setPixelColor(0, COLOR_BUTTON_PRESSED);
            pixels.show();
            //Serial.println("EV_BUTTON_PRESSED");
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_HELD_FOR_LONG_ENOUGH");
            //Serial.println("EV_HELD_FOR_LONG_ENOUGH");

            //updateTimeDisp();
            OutBuffer[0] = TimeDisp[0];
            OutBuffer[1] = TimeDisp[1];
            OutBuffer[2] = TimeDisp[2];
            OutBuffer[3] = TimeDisp[3];
            OutBuffer[4] = TimeDisp[4];
            Serial.print("TimeDisp: ");    Serial.println(TimeDisp);
            wifiHelper.mqttPublish(FEEDBOTTLE_FEED, OutBuffer);
            sevenSegDisplayTime();
            break;
        
        case button.EV_RELEASED:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_RELEASED");
            //Serial.println("EV_RELEASED");
            pixels.setPixelColor(0, COLOR_NORMAL);
            pixels.show();
            break;
        
        case button.EV_DOUBLETAP:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_DOUBLETAP");
            //Serial.println("EV_DOUBLETAP");
            break;
    }
}

void updateTimeDisp() {
    Serial.print("hour: "); Serial.println(hour);
    TimeDisp[0] = hour/10 + '0';
    TimeDisp[1] = hour%10 + '0';
    TimeDisp[2] = ':';
    Serial.print("minute: "); Serial.println(minute);
    TimeDisp[3] = minute/10 + '0';
    TimeDisp[4] = minute%10 + '0';
}

void clearTimeDisp() {
    TimeDisp[0] = '-';
    TimeDisp[1] = '-';
    TimeDisp[2] = ':';
    TimeDisp[3] = '-';
    TimeDisp[4] = '-';
}

void sevenSegClear() {
    sevenSeg.setBacklight(LOW_BRIGHT);
    sevenSeg.print("----");
    clearTimeDisp();
}

void sevenSegDisplayTime() {

    hour = (TimeDisp[0]-'0') * 10;
    hour += TimeDisp[1]-'0';
    minute = (TimeDisp[3]-'0') * 10;
    minute += TimeDisp[4]-'0';

    sevenSeg.setBacklight(MED_BRIGHT);
    sevenSeg.printTime(hour, minute, false);
}

