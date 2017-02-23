
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <TaskScheduler.h>
#include <Wire.h>
#include <ADXL345.h>
#include <TimeLib.h>


#ifdef __AVR__
  #include <avr/power.h>
#endif


#define NEOPIXEL_PIN        D5
#define NUMPIXELS           1
#define LONG_PRESS_TIME     1000
#define LIGHT_ON_WINDOW_TIME  60 * 1000
#define BUTTON_PIN  D4

#define FEEDBOTTLE_FEED  "/dev/bottleFeed"
#define HHmm_FEED        "/dev/HHmm"
#define WIFI_OTA_NAME   "arduino-bottle-filler"
#define WIFI_HOSTNAME   "arduino-bottle-filler"

#define BOTTLE_FEED_TRIGGER_EV  "1"
#define BOTTLE_FEED_IGNORE_EV   "2"

#define CLK D3   // SCL
#define DIO D4   // SDA

#define SCL D1   // SCL
#define SDA D2   // SDA

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

char versionText[] = "MQTT Bottle Feeder v1.2.0";

/* ------------------------------------------------------------ */

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_LIGHT_ON = pixels.Color(40, 0, 0);
uint32_t COLOR_BUTTON_PRESSED = pixels.Color(10, 0, 0);
uint32_t COLOR_NORMAL = pixels.Color(1, 0, 0);
uint32_t COLOR_OFF = pixels.Color(0, 0, 0);

/* ----------------------------------------------------------- */

void ledOffCallback();
void lightOnWindowCallback();

Scheduler runner;

#define RUN_ONCE 2

uint32_t oldColor;

Task tLightOnWindow(LIGHT_ON_WINDOW_TIME, RUN_ONCE, &lightOnWindowCallback, &runner, false);

void lightOnWindowCallback() {

    if (tLightOnWindow.isFirstIteration()) {
        pixels.setPixelColor(0, COLOR_LIGHT_ON);
        pixels.show();
    }
    else if (tLightOnWindow.isLastIteration()) {
        pixels.setPixelColor(0, COLOR_NORMAL);
        pixels.show();
    }
}

/* ----------------------------------------------------------- */

ADXL345 adxl; //variable adxl is an instance of the ADXL345 library

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/* ----------------------------------------------------------- */
// CLOCK

char TimeDisp[] = "--:--";

int time_hour = 0;
int time_minute = 0;

bool callback_occurred = false;

/* ----------------------------------------------------------- */

void devtime_mqttcallback(byte* payload, unsigned int length) {

    if (payload[0] == '-') {
        time_hour = -1;
    } else {
        time_hour = (payload[0]-'0') * 10;
        time_hour += payload[1]-'0';
        time_minute = (payload[3]-'0') * 10;
        time_minute += payload[4]-'0';
    }

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

    Wire.begin(SDA, SCL); // sda, scl
    delay(500);

    adxl.powerOn();
    setupADXL();

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_OTA_NAME);

    //runner.init();
    runner.addTask(tLightOnWindow);

    wifiHelper.setupMqtt();

    wifiHelper.mqttAddSubscription(HHmm_FEED, devtime_mqttcallback);
}

/* ----------------------------------------------------------- */

void loop() {

    if (!wifiHelper.loopMqttNonBlocking()) {
        Serial.print('.');
    }

    ArduinoOTA.handle();

    byte interrupts = adxl.getInterruptSource();

    if (adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
        Serial.println("single tap");
        turnFeedLightOn();
    }

    runner.execute();

    delay(50);
}

void turnFeedLightOn() {

    if (!tLightOnWindow.isEnabled()) {
        wifiHelper.mqttPublish(FEEDBOTTLE_FEED, BOTTLE_FEED_TRIGGER_EV);
        tLightOnWindow.restart();
    } else {
        wifiHelper.mqttPublish(FEEDBOTTLE_FEED, BOTTLE_FEED_IGNORE_EV);
        tLightOnWindow.disable();
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
    adxl.setTapThreshold(100); //62.5mg per increment    (default 50)
    adxl.setTapDuration(15); //625us per increment      (default 15)

    adxl.setDoubleTapLatency(80); //1.25ms per increment    (default 80)
    adxl.setDoubleTapWindow(50); //1.25ms per increment    (default 200)

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
