#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <ADXL345.h>

// ----- neopixels ------

#define PIN            14
#define NUMPIXELS      12
#define PIXEL_BRIGHTNESS  10

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint32_t PIXELS_RED = pixels.Color(PIXEL_BRIGHTNESS, 0, 0);
uint32_t PIXELS_OFF = pixels.Color(0, 0, 0);

// ----- adxl -----

ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
#define PIN_SDA 4
#define PIN_SCL 5

#define   INACTIVITY_SECONDS  3
#define   ADXL_ACTIVITY_STRENGTH  150   // 0-255 default was 75

void setup() {

  Serial.begin(115200);
  delay(500);
  Serial.println("Testing adxl345");

  pixels.begin();

  Wire.begin(PIN_SDA, PIN_SCL);
  delay(500);

  adxl.powerOn();
  adxlSetup();
}

void loop() {
  int x,y,z;  
  adxl.readXYZ(&x, &y, &z); //read the accelerometer values and store them in variables  x,y,z

  // Output x,y,z values 
  Serial.print("x: ");  Serial.print(x);
  Serial.print(",    y: ");  Serial.print(y);
  Serial.print(",    z: ");  Serial.print(z);


// acceleration
  double xyz[3];
  double ax,ay,az;
  adxl.getAcceleration(xyz);
  ax = xyz[0];
  ay = xyz[1];
  az = xyz[2];
  Serial.print("    X=");  Serial.print(ax);    Serial.print(" g,");
  Serial.print("    Y=");  Serial.print(ay);    Serial.print(" g,");
  Serial.print("    Z=");  Serial.print(az);    Serial.println(" g");
  

  //read interrupts source and look for triggerd actions
  
  //getInterruptSource clears all triggered actions after returning value
  //so do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();
  
/* // freefall
  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
    Serial.println("freefall");
    //add code here to do when freefall is sensed
  } 
  */
  
  //inactivity
  if (adxl.triggered(interrupts, ADXL345_INACTIVITY)){
    Serial.println("inactivity");
    //add code here to do when inactivity is sensed

    pixelsOff();    
  }
  
  //activity
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
    Serial.println("activity"); 
     //add code here to do when activity is sensed

    pixelsOn();
  }
  
/* //double tap
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
    Serial.println("double tap");
     //add code here to do when a 2X tap is sensed
  }
*/
/* //tap
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    Serial.println("tap");
     //add code here to do when a tap is sensed
  } */

  Serial.println("**********************");
  delay(500);
}

void adxlSetup() {
    //set activity/ inactivity thresholds (0-255)
  adxl.setActivityThreshold(ADXL_ACTIVITY_STRENGTH); //62.5mg per increment
  adxl.setInactivityThreshold(ADXL_ACTIVITY_STRENGTH); //62.5mg per increment
  adxl.setTimeInactivity(INACTIVITY_SECONDS); // how many seconds of no activity is inactive?
 
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

/*  //set values for what is a tap, and what is a double tap (0-255)
  adxl.setTapThreshold(50); //62.5mg per increment
  adxl.setTapDuration(15); //625us per increment
  adxl.setDoubleTapLatency(80); //1.25ms per increment
  adxl.setDoubleTapWindow(200); //1.25ms per increment
*/ 
/*  //set values for what is considered freefall (0-255)
  adxl.setFreeFallThreshold(7); //(5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(45); //(20 - 70) recommended - 5ms per increment
  */
/*  //setting all interrupts to take place on int pin 1
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
*/
}

void pixelsOn() {
  pixelsSetAll(PIXELS_RED);
}

void pixelsOff() {
  pixelsSetAll(PIXELS_OFF);
}

void pixelsSetAll(uint32_t color) {
  
  for(int i=0; i<NUMPIXELS; i++){
    pixels.setPixelColor(i, color);
  }
  pixels.show(); // This sends the updated pixel color to the hardware.
}

