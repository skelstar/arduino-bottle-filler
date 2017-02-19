
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>
#include <TaskScheduler.h>
#include <Wire.h>
#include <ADXL345.h>


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

#define CLK D3   // SCL
#define DIO D4   // SDA

#define SCL D1   // SCL
#define SDA D2   // SDA

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

char versionText[] = "MQTT Bottle Feeder v1.0.0";

/* ------------------------------------------------------------ */

// NEOPIXEL

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_LIGHT_ON = pixels.Color(10, 0, 0);
uint32_t COLOR_BUTTON_PRESSED = pixels.Color(10, 0, 0);
uint32_t COLOR_NORMAL = pixels.Color(1, 0, 0);
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
Task tLightOnWindow(DELETE_WINDOW_TIME, FLASH_ONCE, &deleteWindowCallback, &runner, false);

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
    if (tLightOnWindow.isLastIteration()) {
        pixels.setPixelColor(0, COLOR_NORMAL);
        pixels.show();
    }
}

/* ----------------------------------------------------------- */

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, LONG_PRESS_TIME, 1, listener_Button);

/* ----------------------------------------------------------- */

ADXL345 adxl; //variable adxl is an instance of the ADXL345 library

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

    if (payload[0] == '-') {
        hour = -1;
    } else {
        hour = (payload[0]-'0') * 10;
        hour += payload[1]-'0';
        minute = (payload[3]-'0') * 10;
        minute += payload[4]-'0';
    }

    TimeDisp[0] = payload[0];
    TimeDisp[1] = payload[1];
    TimeDisp[3] = payload[3];
    TimeDisp[4] = payload[4];

    callback_occurred = true;

    if ((hour >= 6 && minute == 0) && (hour <= 18 && minute == 0)) {
        sevenSegClear();
    }
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

    Wire.begin(SDA, SCL); // sda, scl
    delay(500);

    adxl.powerOn();
    setupADXL();

    sevenSeg.begin(); 
    sevenSegClear();

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_OTA_NAME);

    //runner.init();
    runner.addTask(tOneFlash);
    runner.addTask(tLightOnWindow);

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

    byte interrupts = adxl.getInterruptSource();
    //double tap
    if (adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
        listener_Button(0, button.EV_HELD_FOR_LONG_ENOUGH);
        Serial.println("double tap");
    }

    runner.execute();

    delay(200);
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            pixels.setPixelColor(0, COLOR_BUTTON_PRESSED);
            pixels.show();
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:

            if (!tLightOnWindow.isEnabled()) {
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "1");
                sevenSegDisplayTime();
                pixels.setPixelColor(0, COLOR_LIGHT_ON);
                pixels.show();
                tLightOnWindow.restart();
                tOneFlash.restart();
            } else {
                wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "2");
                sevenSegClear();
                tLightOnWindow.disable();
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

void setupADXL() {
    //set activity/ inactivity thresholds (0-255)
    adxl.setActivityThreshold(75); //62.5mg per increment
    adxl.setInactivityThreshold(75); //62.5mg per increment
    adxl.setTimeInactivity(10); // how many seconds of no activity is inactive?

    //look of activity movement on this axes - 1 == on; 0 == off 
    adxl.setActivityX(1);   // default 1
    adxl.setActivityY(1);   // default 1
    adxl.setActivityZ(1);

    //look of inactivity movement on this axes - 1 == on; 0 == off
    adxl.setInactivityX(1);
    adxl.setInactivityY(1);
    adxl.setInactivityZ(1);

    //look of tap movement on this axes - 1 == on; 0 == off
    adxl.setTapDetectionOnX(0);
    adxl.setTapDetectionOnY(0);
    adxl.setTapDetectionOnZ(1);

    //set values for what is a tap, and what is a double tap (0-255)
    adxl.setTapThreshold(200); //62.5mg per increment    (default 50)
    adxl.setTapDuration(15); //625us per increment      (default 15)
    adxl.setDoubleTapLatency(80); //1.25ms per increment    (default 80)
    adxl.setDoubleTapWindow(200); //1.25ms per increment    (default 200)

    //set values for what is considered freefall (0-255)
    adxl.setFreeFallThreshold(7); //(5 - 9) recommended - 62.5mg per increment
    adxl.setFreeFallDuration(45); //(20 - 70) recommended - 5ms per increment
    
    //setting all interrupts to take place on int pin 1
    //I had issues with int pin 2, was unable to reset it
    adxl.setInterruptMapping( ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN );
    adxl.setInterruptMapping( ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN );
    adxl.setInterruptMapping( ADXL345_INT_FREE_FALL_BIT,    ADXL345_INT1_PIN );
    adxl.setInterruptMapping( ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN );
    adxl.setInterruptMapping( ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN );

    //register interrupt actions - 1 == on; 0 == off  
    adxl.setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 1);
    adxl.setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 1);
    adxl.setInterrupt( ADXL345_INT_FREE_FALL_BIT,  1);
    adxl.setInterrupt( ADXL345_INT_ACTIVITY_BIT,   1);
    adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 1);
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

