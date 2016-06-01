#include <Time.h>
#include <TimeLib.h>

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <ADXL345.h>
#include <myStateMachine.h>
#include <RTClib.h>         // https://github.com/adafruit/RTClib

#include <ESP8266WiFi.h>
#include <DweetIO.h>
#include <Streaming.h>

// ----- neopixels ------

#define PIN            14
#define NUMPIXELS      12
#define PIXEL_BRIGHTNESS  12
#define CLOCK_ORIENTATION_OFFSET  9

Adafruit_NeoPixel ring = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint32_t  PIXEL_RED = ring.Color(PIXEL_BRIGHTNESS, 0, 0);
uint32_t  PIXEL_OFF = ring.Color(0, 0, 0);
uint32_t  PIXEL_HOUR_COLOUR = ring.Color(0, 5, 0); // green?
uint32_t  PIXEL_MINUTE_COLOUR = ring.Color(0, 0, 5); // blue?
uint32_t  PIXEL_15MIN = ring.Color(1, 0, 0);    // light red

uint32_t pixels[12] = {
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF,
            PIXEL_OFF           
          };

int inactiveTime = 0;


// ----- adxl ---------------------------------

ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
#define PIN_SDA 4
#define PIN_SCL 5

#define   LIGHTS_ON_SECONDS       30
#define   ADXL_ACTIVITY_STRENGTH  200   // 0-255 default was 75
#define   ABOUT_TO_BE_INACTIVE_PERIOD   5000  // ms


// ----- State Machine -------------------------
/* STATES */

#define   BECOMING_ACTIVE       0
#define   ACTIVE                1
#define   ABOUT_TO_BE_INACTIVE  2
#define   BECOME_INACTIVE       3
#define   INACTIVE              4

myStateMachine state(INACTIVE, true);

// ----- RTC ------------------------------------

#define DS3231_I2C_ADDRESS 0x68
RTC_DS3231 rtc;
DateTime latestDt;
time_t secondsElapsed = 0;
time_t activeStarted = 0;

// --------- Dweetio ----------------------------

#define   DWEETIO_CHANNEL_NAME "DWEETIOTIMETEST1" // shouldn't exist.. we just want the datetime from response

DweetIO channel(DWEETIO_CHANNEL_NAME, true);

// --------- wifi ------------

const char* ssid = "LeilaNet2";
const char* password = "ec1122%f*&";

void setup() {

  Serial.begin(115200);
  delay(500);
  Serial.println("Testing adxl345");

  state.pushState(  "BECOMING_ACTIVE" );
  state.pushState(  "ACTIVE" );
  state.pushState(  "ABOUT_TO_BE_INACTIVE" );
  state.pushState(  "BECOME_INACTIVE" );
  state.pushState(  "INACTIVE" );

  ring.begin();

  Wire.begin(PIN_SDA, PIN_SCL);
  delay(500);

  adxl.powerOn();
  adxlSetup();

  setupRTC();
}

void loop() {
  int x,y,z;  
  adxl.readXYZ(&x, &y, &z); //read the accelerometer values and store them in variables  x,y,z

// acceleration
  double xyz[3];
  double ax,ay,az;
  adxl.getAcceleration(xyz);
  
  byte interrupts = adxl.getInterruptSource();

  if (state.current == INACTIVE) { 
    if (adxl.triggered(interrupts, ADXL345_ACTIVITY)){
      Serial.println("activity");
      state.changeStateTo(BECOMING_ACTIVE);
    }
    else {
      latestDt = rtc.now();
      ringClock(latestDt);
    }
  } 
  
  else if (state.current == BECOMING_ACTIVE) {
      if (ringAddPixelUntilFull()) {
        state.changeStateTo(ACTIVE);
      }
      activeStarted = secondsElapsed;
  } 
  
  else if (state.current == ACTIVE) {

    Serial << "Secs: " << secondsElapsed << endl;

    if (secondsElapsed > activeStarted + LIGHTS_ON_SECONDS) {
      inactiveTime = millis();
      
      state.changeStateTo(ABOUT_TO_BE_INACTIVE);
    }
  } 
  
  else if (state.current == ABOUT_TO_BE_INACTIVE) {
    ringSetAlternatePixels(PIXEL_RED);
    if (millis() > inactiveTime + ABOUT_TO_BE_INACTIVE_PERIOD)
      state.changeStateTo(BECOME_INACTIVE);
  } 
  
  else if (state.current == BECOME_INACTIVE) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels[i] = PIXEL_OFF;
    }
    state.changeStateTo(INACTIVE);
  }

  updatePixels();
  ring.show();
  secondsElapsed = now();

  delay(200);
}

DateTime getDweetTimeDate() {
  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  channel.value = "999";

  DateTime result = channel.GetTime();
  Serial << "channel.GetTime(): " << result.hour() << ":" << result.minute() << endl;

  WiFi.mode(WIFI_OFF);
  
  return result;
}

void setupRTC() {
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
    
  Serial.println("Setting the time from Dweetio!");
  latestDt = getDweetTimeDate();
  rtc.adjust(DateTime(2016, 1, 1, latestDt.hour(), latestDt.minute(), latestDt.second()));
}

void printDateTime(String dtName, DateTime dt) {

  Serial << dtName << ": "<< dt.hour() << 
                      ":" << dt.minute() << 
                      ":" << dt.second() << 
                      endl;
}

void ringClock(DateTime dt) {

  int hr = dt.hour();
  if (hr > 12)
    hr -= 12;
  hr = ApplyClockOrientationCompensation(hr);
  
  float fmin = (dt.minute() / 60.0) * 12.0;
  int mm = (int)fmin;
  mm = ApplyClockOrientationCompensation(mm);

  
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels[i] = PIXEL_OFF;
  }

  pixels[0] = PIXEL_15MIN;
  pixels[3] = PIXEL_15MIN;
  pixels[6] = PIXEL_15MIN;
  pixels[9] = PIXEL_15MIN;  
  
  // really dim pixels for 15min indicators
  pixels[0] == PIXEL_HOUR_COLOUR;
  
  
  
  pixels[hr] = PIXEL_HOUR_COLOUR;
  if (dt.second() % 2 == 0)
    pixels[mm] = PIXEL_MINUTE_COLOUR;
}

int ApplyClockOrientationCompensation(int tm) {

  if (tm + CLOCK_ORIENTATION_OFFSET > 11)
    return tm = tm + CLOCK_ORIENTATION_OFFSET - 12;
  return tm + CLOCK_ORIENTATION_OFFSET;
}

// returns true if all pixels are now RED (added)
bool ringAddPixelUntilFull() {
  int i = 0;
  while (i < NUMPIXELS) {
    if (pixels[i] == PIXEL_OFF) {
      pixels[i] = PIXEL_RED;
      return i == NUMPIXELS - 1;
    }
    i++;
  }
  return i >= NUMPIXELS;
}

void ringSetAlternatePixels(uint32_t color) {

  bool evens = pixels[0] == PIXEL_OFF;

  for (int i = 0; i < NUMPIXELS; i++) {
    if ((evens && i % 2 == 0) || 
        (!evens && i % 2 != 0)) {
      pixels[i] = color;
    } else {
      pixels[i] = PIXEL_OFF;
    }
  }
}

void updatePixels() {
  for (int i = 0; i < NUMPIXELS; i++) {
    ring.setPixelColor(i, pixels[i]);
  }
}

void adxlSetup() {
    //set activity/ inactivity thresholds (0-255)
  adxl.setActivityThreshold(ADXL_ACTIVITY_STRENGTH); //62.5mg per increment
  adxl.setInactivityThreshold(ADXL_ACTIVITY_STRENGTH); //62.5mg per increment
  adxl.setTimeInactivity(LIGHTS_ON_SECONDS); // how many seconds of no activity is inactive?
 
  //look of activity movement on this axes - 1 == on; 0 == off 
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);
 
  //look of inactivity movement on this axes - 1 == on; 0 == off
  adxl.setInactivityX(1);
  adxl.setInactivityY(1);
  adxl.setInactivityZ(1);
 
  //look of tap movement on this axes - 1 == on; 0 == off
  adxl.setTapDetectionOnX(0);
  adxl.setTapDetectionOnY(0);
  adxl.setTapDetectionOnZ(0);

  //set values for what is a tap, and what is a double tap (0-255)
  adxl.setTapThreshold(50); //62.5mg per increment
  adxl.setTapDuration(15); //625us per increment
  adxl.setDoubleTapLatency(80); //1.25ms per increment
  adxl.setDoubleTapWindow(200); //1.25ms per increment
 
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

void ringOn() {
  ringSetAll(PIXEL_RED);
}

void ringOff() {
  ringSetAll(PIXEL_OFF);
}

void ringSetAll(uint32_t color) {
  
  for(int i=0; i<NUMPIXELS; i++){
    ring.setPixelColor(i, color);
  }
  ring.show(); // This sends the updated pixel color to the hardware.
}

