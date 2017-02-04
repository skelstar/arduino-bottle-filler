
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>
#include <TaskScheduler.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


#define NEOPIXEL_PIN        D5
#define NUMPIXELS           1
#define LONG_PRESS_TIME     1000
#define DELETE_WINDOW_TIME  60 * 1000
#define BUTTON_PIN  D4

#define FEEDBOTTLE_FEED  "/dev/bottleFeed"
#define HHmm_FEED        "/dev/HHmm"
#define WIFI_OTA_NAME   "arduino-bottle-filler"
#define WIFI_HOSTNAME   "arduino-bottle-filler"

#define CLK D1   // SCL
#define DIO D2   // SDA

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

char versionText[] = "MQTT Bottle Feeder v1.0.0";

/* ------------------------------------------------------------ */

// NEOPIXEL

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_BUTTON_PRESSED = pixels.Color(10, 0, 0);
uint32_t COLOR_DELETE_WINDOW = pixels.Color(0, 20, 0);
uint32_t COLOR_NORMAL = pixels.Color(50, 0, 0);
uint32_t COLOR_DELETED = pixels.Color(100, 0, 0);
uint32_t COLOR_OFF = pixels.Color(0, 0, 0);

/* ----------------------------------------------------------- */

void ledOffCallback();
void ledOnCallback();
void deleteWindowCallback();

Scheduler runner;

#define FLASH_ONCE 2
#define FLASH_TWICE 4
#define FLASH_TEN_TIMES 20

uint32_t oldColor;

Task tOneFlash(100, FLASH_ONCE, &ledOffCallback, &runner, false);
Task tDeleteWindow(DELETE_WINDOW_TIME, FLASH_ONCE, &deleteWindowCallback, &runner, false);

void ledOffCallback() {
    oldColor = pixels.getPixelColor(0);
    pixels.setPixelColor(0, COLOR_OFF);
    pixels.show();
    tOneFlash.setCallback(&ledOnCallback);
}

void ledOnCallback() {
    pixels.setPixelColor(0, oldColor);
    pixels.show();
    tOneFlash.setCallback(&ledOffCallback);
}

void deleteWindowCallback() {
    if (tDeleteWindow.isLastIteration()) {
        pixels.setPixelColor(0, COLOR_NORMAL);
        pixels.show();
    }
}

/* ----------------------------------------------------------- */

// BUTTON

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, LONG_PRESS_TIME, 1, listener_Button);

// WIFI
/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/* ----------------------------------------------------------- */
// CLOCK

char TimeDisp[] = "--:--";
char OutBuffer[] = "--:--";

SevenSegmentExtended sevenSeg(CLK,DIO);

volatile long lastReadTime = millis();
long lastLongPressTime = 0;

char msg[5];
volatile int hour = 0;
volatile int minute = 0;

bool callback_occurred = false;

/* ----------------------------------------------------------- */

void devtime_callback(byte* payload, unsigned int length) {

    TimeDisp[0] = payload[0];
    TimeDisp[1] = payload[1];
    TimeDisp[3] = payload[3];
    TimeDisp[4] = payload[4];

    callback_occurred = true;
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

    //runner.init();
    runner.addTask(tOneFlash);
    runner.addTask(tDeleteWindow);

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

    button.serviceEvents();

    runner.execute();
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            pixels.setPixelColor(0, COLOR_BUTTON_PRESSED);
            pixels.show();
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:

            if (!tDeleteWindow.isEnabled()) {
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "1");
                sevenSegDisplayTime();
                pixels.setPixelColor(0, COLOR_DELETE_WINDOW);
                pixels.show();
                tDeleteWindow.restart();
                tOneFlash.restart();
            } else {
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "2");
                sevenSegClear();
                tDeleteWindow.disable();
            }

            break;
        
        case button.EV_RELEASED:
            pixels.setPixelColor(0, COLOR_NORMAL);
            pixels.show();
            break;

        case button.EV_RELEASED_FROM_HELD_TIME:
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

