
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "appconfig.h"
#include "paj7620.h"


#include <Adafruit_NeoPixel.h>

/* ----------------------------------------------------------- */

// NEOPIXEL
#define PIN            12
#define NUMPIXELS      12

int head = 4;
#define TRAIL_LENGTH    6
int brightnesses[] = {140,125,75,30,10,5,0,0,0,0,0,0};
bool lightsOn = false;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 400;
unsigned long timer;

// GESTURES
#define GES_REACTION_TIME        500             // You can adjust the reaction time according to the actual circumstance.
#define GES_ENTRY_TIME          800             // When you want to recognize the Forward/Backward gestures, your gestures' reaction time must less than GES_ENTRY_TIME(0.8s).
#define GES_QUIT_TIME           1000


/* ----------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

    pixels.begin();
    for (int i=0; i<NUMPIXELS; i+=2)
        pixels.setPixelColor(i, pixels.Color(150,0,0));
    pixels.show(); // This sends the updated pixel color to the hardware.

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    
    ArduinoOTA.setHostname(host);
    
    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // GESTUREs

    uint8_t error = 0;

    error = paj7620Init();          // initialize Paj7620 registers
    if (error)
    {
        Serial.print("INIT ERROR,CODE:");
        Serial.println(error);
    }
    else
    {
        Serial.println("INIT OK");
    }

    timer = millis();
}

/* ----------------------------------------------------------- */

void loop() {

    bool found = ReadGestures();

    if (found || timer + 3000 > millis() ) {
        if (found)
            timer = millis();
        int count = 0;
        int pointer = head;
        while (count < 12) {
            pixels.setPixelColor(pointer, pixels.Color(brightnesses[count++],0,0));
            pointer--;
            if (pointer < 0)
                pointer = NUMPIXELS - 1;
        }
        pixels.show();
        head++;
        if (head >= NUMPIXELS)
            head = 0;
            
        delay(delayval);
    }
    else {
        for (int i=0; i< 12; i++) {
            pixels.setPixelColor(i, pixels.Color(0,0,0));
        }
        pixels.show();
    }
    
    ArduinoOTA.handle();
}

bool ReadGestures() {
    return true;
    
    uint8_t data = 0, data1 = 0, error;
    bool found = false;

    error = paj7620ReadReg(0x43, 1, &data);             // Read Bank_0_Reg_0x43/0x44 for gesture result.
    if (!error)
    {
        switch (data)                                   // When different gestures be detected, the variable 'data' will be set to different values by paj7620ReadReg(0x43, 1, &data).
        {
            case GES_RIGHT_FLAG:
                delay(GES_ENTRY_TIME);
                paj7620ReadReg(0x43, 1, &data);
                if(data == GES_FORWARD_FLAG)
                {
                    Serial.println("Forward");
                    delay(GES_QUIT_TIME);
                }
                else if(data == GES_BACKWARD_FLAG)
                {
                    Serial.println("Backward");
                    delay(GES_QUIT_TIME);
                }
                else
                {
                    Serial.println("Right");
                }
                found = true;
                break;
            case GES_LEFT_FLAG:
                delay(GES_ENTRY_TIME);
                paj7620ReadReg(0x43, 1, &data);
                if(data == GES_FORWARD_FLAG)
                {
                    Serial.println("Forward");
                    delay(GES_QUIT_TIME);
                }
                else if(data == GES_BACKWARD_FLAG)
                {
                    Serial.println("Backward");
                    delay(GES_QUIT_TIME);
                }
                else
                {
                    Serial.println("Left");
                }
                found = true;
                break;
            case GES_UP_FLAG:
                delay(GES_ENTRY_TIME);
                paj7620ReadReg(0x43, 1, &data);
                if(data == GES_FORWARD_FLAG)
                {
                    Serial.println("Forward");
                    delay(GES_QUIT_TIME);
                }
                else if(data == GES_BACKWARD_FLAG)
                {
                    Serial.println("Backward");
                    delay(GES_QUIT_TIME);
                }
                else
                {
                    Serial.println("Up");
                }
                found = true;
                break;
            case GES_DOWN_FLAG:
                delay(GES_ENTRY_TIME);
                paj7620ReadReg(0x43, 1, &data);
                if(data == GES_FORWARD_FLAG)
                {
                    Serial.println("Forward");
                    delay(GES_QUIT_TIME);
                }
                else if(data == GES_BACKWARD_FLAG)
                {
                    Serial.println("Backward");
                    delay(GES_QUIT_TIME);
                }
                else
                {
                    Serial.println("Down");
                }
                found = true;
                break;
            case GES_FORWARD_FLAG:
                Serial.println("Forward");
                delay(GES_QUIT_TIME);
                found = true;
                break;
            case GES_BACKWARD_FLAG:
                Serial.println("Backward");
                delay(GES_QUIT_TIME);
                found = true;
                break;
            case GES_CLOCKWISE_FLAG:
                Serial.println("Clockwise");
                found = true;
                break;
            case GES_COUNT_CLOCKWISE_FLAG:
                Serial.println("anti-clockwise");
                found = true;
                break;
            default:
                paj7620ReadReg(0x44, 1, &data1);
                if (data1 == GES_WAVE_FLAG)
                {
                    Serial.println("wave");
                }
                break;
        }
        return found;
    }
}


