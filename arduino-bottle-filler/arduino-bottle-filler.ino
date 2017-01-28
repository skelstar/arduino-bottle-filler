
#include <myWifiHelper.h>
#include <Adafruit_NeoPixel.h>
#include <myPushButton.h>
#include <SevenSegmentTM1637.h>
#include <SevenSegmentExtended.h>
#include <TaskScheduler.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif


/* ----------------------------------------------------------- */

void t1Callback();

Scheduler runner;

Task t1(200, 2, &t1Callback, &runner, false);

void t1Callback() {
    Serial.print("t1: ");
    Serial.println(millis());

    if (t1.isFirstIteration()) {
        Serial.println("First Iteration");
    }
    if (t1.isLastIteration()) {
      Serial.println("Last Iteration");
    }

}

/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            2
#define NUMPIXELS      1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4*4, PIN, NEO_GRB + NEO_KHZ800);

uint32_t COLOR_BUTTON_PRESSED = pixels.Color(0, 100, 0);
uint32_t COLOR_NORMAL = pixels.Color(100, 0, 0);
uint32_t COLOR_DELETED = pixels.Color(255, 0, 0);

/* ----------------------------------------------------------- */

// BUTTON
#define BUTTON_PIN  13

void listener_Button(int eventCode, int eventParams);

myPushButton button(BUTTON_PIN, true, 1000, 1, listener_Button);

// WIFI
/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper("arduino-bottle-filler-dev");

#define FEEDBOTTLE_FEED  "dev/bottleFeed"

/* ----------------------------------------------------------- */
// CLOCK

#define MED_BRIGHT      30
#define LOW_BRIGHT      15

int8_t TimeDisp[] = {0x01,0x02,0x03,0x04};

#define CLK 5   // SCL
#define DIO 4   // SDA
SevenSegmentExtended sevenSeg(CLK,DIO);

long lastTimeRead = millis();

/* ----------------------------------------------------------- */

// void devtime_callback(byte* payload, unsigned int length) {

//     if (payload[4] <= '9') {
//         int hour = (payload[0]-'0') * 10;
//         hour += payload[1]-'0';
//         int minute = (payload[3]-'0') * 10;
//         minute += payload[4]-'0';
//         sevenSeg.setBacklight(MED_BRIGHT);
//         sevenSeg.printTime(hour, minute, false);
//     } else {
//         char msg[5];
//         msg[0] = payload[0];
//         msg[1] = payload[1];
//         msg[2] = payload[2];
//         msg[3] = payload[3];
//         msg[4] = NULL;
//         sevenSeg.print(msg);
//     }

//     pinMode(0, OUTPUT);
//     digitalWrite(0, LOW);
//     delay(100);
//     digitalWrite(0, HIGH);

//     lastTimeRead = millis();
// }

// bool mqttTimeOnline() {
//     bool online =  lastTimeRead + 65000 > millis();
//     if (online == false) {
//         Serial.print("lastTimeRead: "); Serial.print(lastTimeRead); Serial.print(" millis(): "); Serial.println(millis());
//     }
//     return online;
// } 

void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");

    pixels.begin();
    pixels.show(); // This sends the updated pixel color to the hardware.

    pixels.setPixelColor(0, COLOR_NORMAL);
    pixels.show();

    // sevenSeg.begin();          
    // sevenSeg.setBacklight(LOW_BRIGHT);
    // sevenSeg.print("----");

    wifiHelper.setupWifi();

    wifiHelper.setupOTA("arduino-bottle-filler-dev");

    runner.startNow();
    Serial.println("Initialized scheduler");

    //wifiHelper.setupMqtt();

    // wifiHelper.mqttAddSubscription("dev/bottle-feed-time", devtime_callback);
}

/* ----------------------------------------------------------- */

void loop() {

    //wifiHelper.loopMqtt();

    ArduinoOTA.handle();

    button.serviceEvents();

    runner.execute();
    
    //delay(100);
}

void listener_Button(int eventCode, int eventParams) {

    switch (eventParams) {
        
        case button.EV_BUTTON_PRESSED:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_BUTTON_PRESSED");
            pixels.setPixelColor(0, COLOR_BUTTON_PRESSED);
            pixels.show();
            Serial.println("EV_BUTTON_PRESSED");
            break;          
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_HELD_FOR_LONG_ENOUGH");
            Serial.println("EV_HELD_FOR_LONG_ENOUGH");
            pixels.setPixelColor(0, COLOR_DELETED);
            pixels.show();

            //runner.addTask(t1);
            t1.restart();
            Serial.println("Restarted t1");

            break;
        
        case button.EV_RELEASED:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_RELEASED");
            Serial.println("EV_RELEASED");
            pixels.setPixelColor(0, COLOR_NORMAL);
            pixels.show();

            // Serial.println("Disabled and deleted t1");
            
            break;
        
        case button.EV_DOUBLETAP:
            //wifiHelper.mqttPublish(FEEDBOTTLE_FEED, "EV_DOUBLETAP");
            Serial.println("EV_DOUBLETAP");
            break;
    }
}

