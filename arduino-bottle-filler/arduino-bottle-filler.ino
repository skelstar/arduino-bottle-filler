
#include <myWifiHelper.h>
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <myPushButton.h>


#ifdef __AVR__
  #include <avr/power.h>
#endif


#define NEOPIXEL_PIN        D5
#define NUMPIXELS           1
#define LONG_PRESS_TIME     1000
#define LIGHT_ON_WINDOW_TIME  30 * 1000

#define BUTTON_PIN          D3
#define BUTTON_PIN_GND      D2
#define RELAY_PIN           D1

#define MQTT_EVENT_BOTTLE       "/dev/bottleFeed"

#define MQTT_EVENT_HHMM        "/dev/HHmm"
#define MQTT_EVENT_TIMESTAMP    "/dev/timestamp"
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

char versionText[] = "MQTT Bottle Feeder v1.3.0";

/* ----------------------------------------------------------- */

void listener_Button( int eventCode, int eventParam );

myPushButton button(BUTTON_PIN, true, LONG_PRESS_TIME, 1, listener_Button);

void listener_Button( int eventCode, int eventParam ) {

     switch (eventParam) {
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
            turnFeedLightOn();
            break;
        
        case button.EV_RELEASED_FROM_HELD_TIME:
            break;
    }
}

/* ----------------------------------------------------------- */

void ledOffCallback();
void lightOnWindowCallback();

Scheduler runner;

#define RUN_ONCE 2

uint32_t oldColor;

Task tLightOnWindow(LIGHT_ON_WINDOW_TIME, RUN_ONCE, &lightOnWindowCallback, &runner, false);

void lightOnWindowCallback() {
    Serial.println("lightOnWindowCallback");
    if (tLightOnWindow.isFirstIteration()) {
        Serial.println("isFirstIteration");
        digitalWrite(RELAY_PIN, HIGH);
    }
    else if (tLightOnWindow.isLastIteration()) {
        Serial.println("isLastIteration");
        Serial.println("Turning light OFF");
        digitalWrite(RELAY_PIN, LOW);
    }
}

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

/* ----------------------------------------------------------- */
// CLOCK

// char TimeDisp[] = "--:--";

// int time_hour = 0;
// int time_minute = 0;

// bool callback_occurred = false;

/* ----------------------------------------------------------- */

// void devtime_mqttcallback(byte* payload, unsigned int length) {

//     if (payload[0] == '-') {
//         time_hour = -1;
//     } else {
//         time_hour = (payload[0]-'0') * 10;
//         time_hour += payload[1]-'0';
//         time_minute = (payload[3]-'0') * 10;
//         time_minute += payload[4]-'0';
//     }

//     callback_occurred = true;
// }

// void devtimestamp_mqttcallback(byte* payload, unsigned int length) {
//     unsigned long pctime = strtoul((char*)payload, NULL, 10);
//     setTime(pctime);
// }

/* ----------------------------------------------------------- */

void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");
    Serial.println(versionText);

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_OTA_NAME);

    runner.addTask(tLightOnWindow);

    wifiHelper.setupMqtt();

    //wifiHelper.mqttAddSubscription(MQTT_EVENT_HHMM, devtime_mqttcallback);
    //wifiHelper.mqttAddSubscription(MQTT_EVENT_TIMESTAMP, devtimestamp_mqttcallback);

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    pinMode(BUTTON_PIN_GND, OUTPUT);
    digitalWrite(BUTTON_PIN_GND, LOW);
}

/* ----------------------------------------------------------- */

void loop() {

    if (!wifiHelper.loopMqttNonBlocking()) {
        Serial.print('.');
    }

    button.serviceEvents();

    ArduinoOTA.handle();

    runner.execute();

    digitalWrite(BUTTON_PIN_GND, LOW);

    delay(50);
}

void turnFeedLightOn() {

    if (!tLightOnWindow.isEnabled()) {
        wifiHelper.mqttPublish(MQTT_EVENT_BOTTLE, BOTTLE_FEED_TRIGGER_EV);
        Serial.println("Turning light ON");
        tLightOnWindow.restart();
    } else {
        wifiHelper.mqttPublish(MQTT_EVENT_BOTTLE, BOTTLE_FEED_IGNORE_EV);
        tLightOnWindow.disable();
    }
}

