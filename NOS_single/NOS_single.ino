/*
   11/17/2023

   This Program is used to demonstrate that the circuit board
   with 2 HC-05 bluetooth modules, named BT_SND_5 and BT_SND_8 is
   communicating with the slaves.

   The hardware requires an Teensy 4.0 and 2 HC-05 bluetooth
   modules both from Amazon.

   The Teensy 4.0 has 3.3 volt I/O and can communicate directly
   with the HT-05.

   If you use the Serial Monitor to send commands to the HT-05,
   it requires that "Both NL & CR" be selected in the
   serial monitor.

   Both upper and lower case AT commands will work.

   The default baud rate on HT-05 modules from
   HiLetGo is 34800, even though there is a lot
   of literature saying that the default baud
   rate is 9600.

   The program uses SoftwareSerial.h which defines additional
   serial port pins.  The serial port pins for BT_SND_5 are
   D7 (Rx) and D8 (Tx). The serial port pins for BT_SND_8 are
   D15 (Rx) and D14 (Tx).

  Revision V1:
      1. When the test LED button is pressed, enter the test mode
      2. Rmove the 3 test buttons that were used with the original BT demo.
      3. Add the two-output LED test routine. Turn on LED 13 for
          the duration of the test.
      4. When the Change Program button is pressed, advance
         the program number.  After a delay, accept the number
         as the new program number and send it to Number 5 and 8

*/
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

//===== LED String Set up
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 256
#define BACKPIN 11   //LED Data pin
#define FRONTPIN 12  //LED Data pin

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel backPixels = Adafruit_NeoPixel(NUMPIXELS, BACKPIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel frontPixels = Adafruit_NeoPixel(NUMPIXELS, FRONTPIN, NEO_GRB + NEO_KHZ800);

SoftwareSerial bt_SND_5(7, 8);    // RX, TX
SoftwareSerial bt_SND_8(15, 14);  // RX, TX

String response = "";  // Stores Response from the HC-05
String command = "";   // Stores Command to be sent to HC-05

const int intLED = 13;  //Onboard LED, used for troubleshooting

//===== Input Switches...

//Debounce Inputs...
const int inputsInUse = 1;               //Define the Number of inputs in use -1 (zero based)
int inputs[inputsInUse + 1] = { 4, 5 };  //Specify the input pin assignments (Switch 1 - 5)

typedef struct
{
  int Pin;       // Pin Assignment from inputs[x]
  int Read;      // Input state as read from hardware
  int Prev;      // Previous Input state
  int Accepted;  // Final accepted input after debounce
  int Time;      // Time that each input has been in this state
  // Timer stops counting when debpunce time is met
} input_type;

input_type input[inputsInUse + 1];

//Variables for input debouncing
unsigned long timePrev;
unsigned long timeNew;
unsigned long timeDiff;
unsigned long timeLED;

unsigned long blinkOnTime;

const int debounceTime = 50;  //Time in ms

//Variables for Button Timing
unsigned long timeButton;

const int testModeSW = 0;  //LED Test mode switch wired to D4
int testSWstate;
int prevSWstate;  //Used to determine when the switch changed state

const int progChangeSW = 1;  //LED Program Change switch wired to D5
int progSWstate;
int prevProgSWstate;  //Used to determine when the switch changed state

int programNumber;  //The current program number being run on this stick
const int maxPrograms = 2;

const long go[] PROGMEM = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0,
  0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool blinkOn = false;

//==========================================================
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.print("Sketch:   ");
  Serial.println(__FILE__);
  Serial.print("Uploaded: ");
  Serial.println(__DATE__);

  /*
     The baud rate for bt_SND_X.begin()
     must be 38400 when communicating to the HC-05 in AT mode
     it is also 38400 when communicating with the bt_REC_X HC-06.
  */

  bt_SND_5.begin(38400);  //<<<<<< SEE NOTE ABOVE
  bt_SND_8.begin(38400);  //<<<<<< SEE NOTE ABOVE

  backPixels.begin();   // This initializes the NeoPixel library.
  frontPixels.begin();  // This initializes the NeoPixel library.

  //Set up inputs
  for (int x = 0; x <= inputsInUse; x++) {
    input[x].Pin = inputs[x];
    pinMode(input[x].Pin, INPUT_PULLUP);
    input[x].Prev = LOW;
    input[x].Accepted = LOW;
    input[x].Time = 0;
  }
  testSWstate = input[testModeSW].Accepted;
  prevSWstate = testSWstate;
  prevProgSWstate = 0;

  Serial.print("testSWstate = ");
  Serial.println(testSWstate);
  Serial.print("progSWstate = ");
  Serial.println(progSWstate);

  //Make sure the internal LED is off
  pinMode(intLED, OUTPUT);
  digitalWrite(intLED, LOW);

  programNumber = 0;  //Will eventually want this to be read from EEPROM
}
//==========================================================

//==========================================================
void loop() {
  //return;
  //Update master timer
  timeNew = millis();
  if (timeNew < timePrev) timeNew = timePrev;  //Check for clock rollover
  timeDiff = timeNew - timePrev;
  //  Serial.print("timeDiff = ");
  //  Serial.print(timeDiff);

  timePrev = timeNew;

  readInputs();  // Call to read and debounce the inputs

  char c;  //ASCII value of character received from serial monitor

  // Read input from Serial Monitor if available.
  if (Serial.available())  //Characters waiting at USB Serial Port from Serial Monitor
  {
    while (Serial.available()) {  // While there is more to be read, keep reading.
      c = Serial.read();
      command += (char)c;
    }
    if ((c == 10)) {
      //Send the command to BT Boards 5 & 8
      messgeToBt_SND_5();
      messgeToBt_SND_8();

      command = "";  // No repeats
      //Dump any extra characters that come in
      delay(10);
      while (Serial.available()) {
        c = Serial.read();
      }
    }
  }

  //-----------------------------------------------------------

  //See if the Test LEDs button was pressed and released
  testSWstate = input[testModeSW].Accepted;
  if (testSWstate != prevSWstate && testSWstate == 0) {
    Serial.println("Test Switch turned off");
    digitalWrite(intLED, HIGH);  //Turn on LED 13 for the duration of the test
    testLEDs(NUMPIXELS);
    digitalWrite(intLED, LOW);  //Turn off LED 13
  }
  prevSWstate = testSWstate;

  //-----------------------------------------------------------

  //See if the Program Change button was pressed and released
  progSWstate = input[progChangeSW].Accepted;
  if ((progSWstate != prevProgSWstate)) {
    Serial.print("progSWstate = ");
    Serial.println(progSWstate);
  }

  if ((progSWstate != prevProgSWstate) && (progSWstate == 0)) {
    Serial.println("Program Change Switch turned off");
    programNumber++;
    if (programNumber > maxPrograms) programNumber = 1;
    Serial.print("programNumber = ");
    Serial.println(programNumber);

    bt_SND_5.print(programNumber);  // Send text over bluetooth
    bt_SND_8.print(programNumber);  // Send text over bluetooth

    if (programNumber == 2) 
    {  // RED steady 578 program
      frontPixels.clear();
      backPixels.clear();
      for (int i = 0; i < NUMPIXELS; i++) 
      {
        if (pgm_read_dword(&(seven[i]))) 
        {
          frontPixels.setPixelColor(i,0,255, 0);
          backPixels.setPixelColor(i, 0, 255, 0);
        }
      }
      frontPixels.show();  // This sends the updated pixel color to the hardware.
      backPixels.show();   // This sends the updated pixel color to the hardware.
    }
    else if (programNumber == 1) 
    { // BLUE steady 578 program
      frontPixels.clear();
      backPixels.clear();
      for (int i = 0; i < NUMPIXELS; i++) 
      {
        if (pgm_read_dword(&(seven[i]))) 
        {
          frontPixels.setPixelColor(i, 0, 0, 255);
          backPixels.setPixelColor(i, 0, 0, 255);
        }
      }
      frontPixels.show(); 
      backPixels.show();  
    }
  }
  prevProgSWstate = progSWstate;
}

//==========================================================
void messgeToBt_SND_5() {
  char c;

  bt_SND_5.listen();
  bt_SND_5.print(command);
  Serial.print("Sent to Number 5: ");
  //Do not print the last character of the string which is \n
  Serial.println(command.substring(0, command.length() - 1));

  delay(50);  //Give the HT-05 a chance to respond
  if (bt_SND_5.available()) {
    delay(50);
    while (bt_SND_5.available()) {
      // While there is more to be read, keep reading.
      c = bt_SND_5.read();
      response += (char)c;
    }
    Serial.print("  Resp from bt_SND_5: ");
    Serial.println(response);
    response = "";  // No repeats
  }
}
//==========================================================
void messgeToBt_SND_8() {
  char c;

  bt_SND_8.listen();
  bt_SND_8.print(command);
  Serial.print("Sent to Number 8: ");
  //Do not print the last character of the string which is \n
  Serial.println(command.substring(0, command.length() - 1));

  delay(50);  //Give the HT-05 a chance to respond
  if (bt_SND_8.available()) {
    delay(50);
    while (bt_SND_8.available()) {
      // While there is more to be read, keep reading.
      c = bt_SND_8.read();
      response += (char)c;
    }
    Serial.print("  Resp from bt_SND_8: ");
    Serial.println(response);
    response = "";  // No repeats
  }
}
//==========================================================
void testLEDs(int pixelCount) {
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  // pixels.Color takes GRB values, from 0,0,0 up to 255,255,255
  //  Serial.print("pixelCount = ");
  //  Serial.println(pixelCount);

  uint8_t G = 0;
  uint8_t R = 0;
  uint8_t B = 0;

  for (int x = 0; x < pixelCount; x++) {
    G = 150;
    R = 150;
    B = 150;
    backPixels.setPixelColor(x, frontPixels.Color(G, R, B));
    backPixels.show();  // This sends the updated pixel color to the hardware.
    frontPixels.setPixelColor(x, frontPixels.Color(G, R, B));
    frontPixels.show();  // This sends the updated pixel color to the hardware.

    delay(60);  // Delay for a period of time (in milliseconds).
    backPixels.clear();
    frontPixels.clear();
  }
  backPixels.clear();
  backPixels.show();  //Turns off the last LED
  frontPixels.clear();
  frontPixels.show();  //Turns off the last LED
}

//============================================

void readInputs() {
  for (int x = 0; x <= inputsInUse; x++) {
    //Invert the input reading value.
    //Pin connected to ground = button pressed = logical 1 forinput[]
    input[x].Read = !digitalRead(input[x].Pin);
    /*
            if(x==1)
            {
            Serial.print("Input[");
            Serial.print(x);
            Serial.print("].Read = ");
            Serial.print(input[x].Read);

            Serial.print("      ");
            Serial.print("Input[");
            Serial.print(x);
            Serial.print("].Time = ");
            Serial.println(input[x].Time);
            }
    */
    if (input[x].Read == input[x].Prev)  // Input did not change state
    {
      if (input[x].Time < debounceTime) {
        input[x].Time += timeDiff;
      } else {
        input[x].Accepted = input[x].Read;
      }
    } else  // Input changed state
    {
      input[x].Time = 0;
      input[x].Prev = input[x].Read;
    }
  }  //End of FOR
}
//============================================
