#include <SoftwareSerial.h>


/*
   11/17/2023

   This Program is used to demonstrate that the
   Bluetooth module HT-06 named BT_REC_5 with
   address 22,9,01BF5C is communicating with
   the Bluetooth module BF_SND_5.

   The Teensy 4.0 has 3.3 volt I/O and can communicate directly
   with the HT-06.

   The HT-06 will not work with serial commands that include either
   a carriage return or a line feed character. The HT-06 responds
   when a delay in character timing is seen.

   This program will remove the CR & LF characters and will
   also work with all of the other combinations of CR & LF.
   This program has a character display flag to show the ASCII
   characters sent and received, if desired.

   Both upper and lower case AT commands will work.
   The default baud rate on HT-06 modules from
   HiLetGo is 9600.

   The program uses SoftwareSerial.h which defines a
   second serial port and is programmed to use pins
   D7 (Rx) and D8 (Tx)of the Teensy.

   NOTE:
   SoftwareSerial.h does not support simultaneous
   sending and receiving of serial data.
   Therefore, a loop-back test will not work.

   Revision V1:
      1. When the test LED button is pressed, enter the test mode
      2. Rmove the 3 test buttons that were used with the original BT demo.
      3. Add the two-output LED test routine. Turn on LED 13 for
          the duration of the test.

    Revision V2:
      1. When the board receives a program number, flash the internal
         LED that number of times.  This serves as an  indicaation that
         the Blue Tooth communication is working.


*/
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

//===== LED String Set up
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      256
#define BACKPIN             11    //LED Data pin
#define FRONTPIN            12    //LED Data pin

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel backPixels = Adafruit_NeoPixel(NUMPIXELS, BACKPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel frontPixels = Adafruit_NeoPixel(NUMPIXELS, FRONTPIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial HC_06(7, 8); // RX, TX

String response = ""; // Stores response of bluetooth device
String command = "";  // Stores the command from the serial monitor

const int testModeSW = 4;
int testSWstate;    //LED Test mode switch wired to D4
int prevSWstate;    //Used to determine when the switch changes state

const int intLED = 13; //Onboard LED, used for troubleshooting
//int ledState;  //State of all 3 LEDs  Red = 1, Green = 2, Blue = 4

const int charDetailFlag = 0; //Set to 1 for troubleshooting serial traffic

int programNumber;    //The current program number being run on this stick

const byte five[] PROGMEM = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,
  0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,
  0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
  0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0
};

uint32_t red = frontPixels.Color(0,128, 0);
uint32_t green = frontPixels.Color(128, 0,0);
uint32_t blue = frontPixels.Color(0,0,128);

//==============================================
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("Sketch:   ");   Serial.println(__FILE__);
  Serial.print("Uploaded: ");   Serial.println(__DATE__);
  // SoftwareSerial "com port" data rate. JY-MCU v1.03 defaults to 9600.
  HC_06.begin(9600);   //9600

  backPixels.begin(); // This initializes the NeoPixel library.
  frontPixels.begin(); // This initializes the NeoPixel library.


  pinMode(testModeSW, INPUT_PULLUP);
  delay(10);   //Allow time to initialize the hardware
  testSWstate = !digitalRead(testModeSW);
  prevSWstate = testSWstate;
  //  Serial.print("testSWstate = ");
  //  Serial.println(testSWstate);

  pinMode(intLED, OUTPUT);
  digitalWrite(intLED, LOW);


}
//=================================================

//=================================================
void loop()
{
  int c;  //ASCII value of character received from serial monitor
  //--------------------------------------------------------------
  // Read HC_06 output if available.
  if (HC_06.available())    //Characters at HC_06 Port
  {
    delay(50);
    while (HC_06.available())
    {
      // While there is more to be read, keep reading.
      response += (char)HC_06.read();
    }
    int y = response.length();
    Serial.print(y);
    Serial.print(" characters received = ");
    Serial.println(response);

    if (charDetailFlag)
    {
      for (int x = 0; x < y; x++)
      {
        Serial.print("  ");
        Serial.print((int)response[x]);
        Serial.print("  ");
        Serial.println(response[x]);
      }
    }

    if (y == 1) //A single character was received
    {
      programNumber = response.toInt();
      Serial.print("programNumber = ");
      Serial.println(programNumber);
      runProgram(programNumber);
    }
    response = ""; // No repeats
  }
  //--------------------------------------------------------------

  // Read input from Serial Monitor if available.
  if (Serial.available())    //Characters at Nano Serial Port
  {
    Serial.println("");
    delay(50);
    while (Serial.available())
    {
      // While there is more to be read, keep reading.
      c = Serial.read();
      if ((c != 13) && (c != 10)) command += (char)c;
    }
    int y = command.length();
    Serial.print(y);
    Serial.print(" characters sent = ");
    Serial.println(command);
    for (int x = 0; x < y; x++)
    {
      if (charDetailFlag)
      {
        Serial.print("  ");
        Serial.print((int)command[x]);
        Serial.print("  ");
        Serial.println(command[x]);
      }
      HC_06.write(command[x]);
    }
    command = ""; // No repeats
  }
  testSWstate = !digitalRead(testModeSW);
  if (testSWstate != prevSWstate && testSWstate == 0)
  {
    //    Serial.print("  ");
    Serial.println("Test Switch turned off");
    digitalWrite(intLED, HIGH); //Turn on LED 13 for the duration of the test
    testLEDs(NUMPIXELS);
    digitalWrite(intLED, LOW);  //Turn off LED 13
  }
  prevSWstate = testSWstate;

}
//==========================================
void runProgram(int progNum)
{
  for (int x = 0; x < progNum; x++)
  {
    digitalWrite(intLED, HIGH); //Turn on LED 13
    delay(150);
    digitalWrite(intLED, LOW);  //Turn off LED 13
    delay(250);
  }

  Serial.print("Run Program Number ");
  Serial.println(progNum);

  switch (progNum)
  {
    case 1:
      Serial.println("red steady 578 program");
      for (int i = 0; i < NUMPIXELS; i++) 
      {
        if (pgm_read_byte(&(five[i]))) 
        {
          frontPixels.setPixelColor(i, red);
          backPixels.setPixelColor(i, red);
        }
      }
      frontPixels.show();  // This sends the updated pixel color to the hardware.
      backPixels.show();   // This sends the updated pixel color to the hardware.
      break;

    case 2:
      Serial.println("blue steady 578 program");
            for (int i = 0; i < NUMPIXELS; i++) 
      {
        if (pgm_read_byte(&(five[i]))) 
        {
          frontPixels.setPixelColor(i, blue);
          backPixels.setPixelColor(i, blue);
        }
      }
      frontPixels.show(); 
      backPixels.show();  
      break;    
  }

}
//==========================================

void testLEDs(int pixelCount)
{
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  // pixels.Color takes GRB values, from 0,0,0 up to 255,255,255
  Serial.print("pixelCount = ");
  Serial.println(pixelCount);

  uint8_t G = 0;
  uint8_t R = 0;
  uint8_t B = 0;

  for (int x = 0; x < pixelCount; x++)
  {
    G = 150;
    R = 150;
    B = 150;
    backPixels.setPixelColor( x, frontPixels.Color(G, R, B));
    backPixels.show(); // This sends the updated pixel color to the hardware.
    frontPixels.setPixelColor( x, frontPixels.Color(G, R, B));
    frontPixels.show(); // This sends the updated pixel color to the hardware.

    delay(60); // Delay for a period of time (in milliseconds).
    backPixels.clear();
    frontPixels.clear();
  }
  backPixels.clear();
  backPixels.show();   //Turns off the last LED
  frontPixels.clear();
  frontPixels.show();   //Turns off the last LED
}
