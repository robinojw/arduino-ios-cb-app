#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "ArduinoJson.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

//Colour sensor imports
#include <Wire.h>
#include "Adafruit_TCS34725.h"


#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define BLUEFRUIT_HWSERIAL_NAME           Serial1 

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);


/*=========================================================================*/
/* ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line */
 Adafruit_BluefruitLE_UART ble(Serial1, BLUEFRUIT_UART_MODE_PIN);

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}


// set to false if using a common cathode LED
#define commonAnode true

// our RGB -> eye-recognized gamma color
byte gammatable[256];


Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

int scanNum = 0;
int redCb = 0;
int greenCb = 0;
int blueCb = 0;
int yellowCb = 0;
int index = 0;

String scannedColours[100];
String colourBlindness;

void setup() {
  while (!Serial);  // required for Flora & Micro
  delay(500);
  
  Serial.begin(9600);
  Serial.println("Colur Blindness Detector");
  Serial.println(F("---------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }


  if (tcs.begin()) {
    //Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }



  //Convert to RGB 255
  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;

    if (commonAnode) {
      gammatable[i] = 255 - x;
    } else {
      gammatable[i] = x;
    }
    //Serial.println(gammatable[i]);
  }
}


void loop() {
  float red, green, blue;
  
  tcs.setInterrupt(false);  // turn on LED
  delay(60);  // takes 50ms to read
  tcs.getRGB(&red, &green, &blue);
  tcs.setInterrupt(true);  // turn off LED


  getColour(red, green, blue);
//  delay(1000);
}

void getColour(float red, float green, float blue) {
   String colour = "";

    //Limit possible values:
   red = constrain(red, 0, 255);
   green = constrain(green, 0, 255);
   blue = constrain(blue, 0, 255);
  
   //find brightest color:
   int maxVal = max(red, blue);
   maxVal = max(maxVal, green);
   //map new values
   red = map(red, 0, maxVal, 0, 255);
   green = map(green, 0, maxVal, 0, 255);
   blue = map(blue, 0, maxVal, 0, 255);
   red = constrain(red, 0, 255);
   green = constrain(green, 0, 255);
   blue = constrain(blue, 0, 255);

     //decide which color is being scanned in
   if (red > 250 && green > 250 && blue > 250) {
     colour = "White";//white
   }
   else if (red < 25 && green < 25 && blue < 25) {
     colour = "Black";//black
   }
   else if (red > 200 &&  green > 200 && blue < 100) {
     colour = "Yellow";//yellow
   }
   else if (red > 200 &&  green > 165 /*&& blueColor < 100*/) {
     colour = "Orange";//orange
   }
   else if (red > 200 &&  green < 100 && blue > 200) {
     colour = "Purple";//purple
   }
   else if (red > 250 && green < 200 && blue < 200) {
     colour = "Red";//red
   }
   else if (red < 200 && green > 250 && blue < 200) {
     colour = "Green";//green
   }
   else if (red < 200 /*&& greenColor < 200*/ && blue > 250) {
     colour = "Blue";//blue
   }
   else {
     colour = "Unknown";//unknown
   }

  //Convert RBG to HEX
  long rgb = 0;
  rgb = ((long)red << 16) | ((long)green << 8 ) | (long)blue;

  //Add scanned colours
  if(colour == "Red") {redCb++;}
  if(colour == "Blue") {blueCb++;}
  if(colour == "Green") {greenCb++;}
  if(colour == "Yellow") {yellowCb++;}
  int total = redCb + blueCb + greenCb + yellowCb;

  //Red and Green Colour Blindness Types
  if( total * redCb / 100 + total * greenCb / 100 > total * blueCb / 100 + total * yellowCb / 100){
    if(total * redCb / 100 > total * greenCb / 100){
      colourBlindness = "Deuteranomaly";
      } else { colourBlindness = "Protanomaly";}
  }

  //Blue Yellow Colour Blindness Types
  if(total * blueCb / 100 + total * yellowCb / 100 > total * redCb / 100 + total * greenCb / 100){
    if(total * redCb / 100 > total * greenCb / 100){
      colourBlindness = "Tritanomaly";
    } else { colourBlindness = "Tritanopia";}
  }

  //Send data to the app depending on the index
  if(index == 0){
     ble.print("AT+BLEUARTTX=");
  ble.println(colour);
  Serial.println(colour);
  index++;
  }
  if(index == 1){
    ble.print("AT+BLEUARTTX=");
  ble.println("#" + String(rgb, HEX));
  Serial.println(String(rgb, HEX));
  index++;
  }
  if(index == 2){
    ble.print("AT+BLEUARTTX=");
    if(colourBlindness != ""){
      ble.println(colourBlindness);
    } else {
      ble.println("CB Type");
    }
   Serial.println(colourBlindness);
   index = 0;
  }
  
}
