
#include <myWifiHelper.h>
#include <TaskScheduler.h>
#include <TimeLib.h>
#include <myPushButton.h>
#include <Adafruit_NeoPixel.h>


#define 	WIFI_HOSTNAME   			"arduino-bottle-filler"

#define     TOPIC_TIMESTAMP             "/dev/timestamp"
#define     TOPIC_COMMAND               "/liamsroom/bottle-filler/command"
#define     TOPIC_ONLINE                "/liamsroom/bottle-filler/online"
#define 	TOPIC_EVENT					"/liamsroom/bottle-filler/event"


#define 	BUTTON_PIN          2
#define 	PIXEL_PIN			0
#define 	NUMPIXELS           1

/* ----------------------------------------------------------- */

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/* ----------------------------------------------------------- */

MyWifiHelper wifiHelper(WIFI_HOSTNAME);

void mqttcallback_Timestamp(byte *payload, unsigned int length) {
    wifiHelper.mqttPublish(TOPIC_ONLINE, "1");
}


void mqttcallback_Command(byte *payload, unsigned int length) {

	//Serial.println("command: "); Serial.println((char*) payload);
    JsonObject& root = wifiHelper.mqttGetJson(payload);
    const char* command = root["command"];
    const char* value = root["value"];
    const char* brightness = root["param1"]; 

    if (strcmp(command, "LED") == 0) {
		int bright = atoi(brightness);
		
		if (strcmp(value, "ON") == 0) {
			turnLight(true, bright);
		} else if (strcmp(value, "OFF") == 0) {
			turnLight(false, bright);
		}
	}
}

/* ----------------------------------------------------------- */

void listener_Button( int eventCode, int eventParam );

#define 	LONG_PRESS_TIME		1000
#define 	ACTIVE_HIGH			1
#define		PULLUP				true
myPushButton button(BUTTON_PIN, PULLUP, LONG_PRESS_TIME, ACTIVE_HIGH, listener_Button);

void listener_Button( int eventCode, int eventParam ) {

     switch (eventParam) {
        
        case button.EV_HELD_FOR_LONG_ENOUGH:
        	wifiHelper.mqttPublish(TOPIC_EVENT, "1");
            break;
        
        case button.EV_RELEASED_FROM_HELD_TIME:
            break;
    }
}

/* ----------------------------------------------------------- */

Scheduler runner;

/* ----------------------------------------------------------- */

void setup() 
{
    Serial.begin(9600);
    delay(50);
    Serial.println("Booting");

	pixels.begin(); 

	pixels.setPixelColor(0, pixels.Color(0, 1, 0)); // Moderately bright green color.
    pixels.show();

    wifiHelper.setupWifi();

    wifiHelper.setupOTA(WIFI_HOSTNAME);

    wifiHelper.setupMqtt();
    wifiHelper.mqttAddSubscription(TOPIC_TIMESTAMP, mqttcallback_Timestamp);
    wifiHelper.mqttAddSubscription(TOPIC_COMMAND, mqttcallback_Command);

    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
}

/* ----------------------------------------------------------- */

void loop() {

    if (!wifiHelper.loopMqttNonBlocking()) {
        Serial.print('.');
    }

    wifiHelper.loopMqtt();

    button.serviceEvents();

    ArduinoOTA.handle();

    runner.execute();

    delay(50);
}

void turnLight(bool on, int brightness) {

    if (on) {
        pixels.setPixelColor(0, pixels.Color(0, 0, brightness));
        pixels.show();
    } else {
        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
        pixels.show();
    }
}

