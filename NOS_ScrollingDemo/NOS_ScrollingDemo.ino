/*
   11/23/22
   Program to drive two WESIRI 32x8 LED displays connected in series
   with a scrolling message and other patterns.

   Scroll is from right to left

   The controller is a Teensy 4.0.

   Several default display patterns (both monochrome and RGB) are
   defined within the program.

   For testing purposes, a command string can be entered through
   the serial monitor.


*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
//#include <Adafruit_NeoPixel.h>

#define PIN 12

const char defaultText[][41] PROGMEM =    //Default Messages to be displayed
{
  "GO FAIRPORT ROBOTICS!!",
  "GO BLUE ALLIANCE",
  "GO JJ",
};



// Max brightness is 255, 32 is a conservative value to not overload
// a USB power supply (500mA) for 12x12 pixels.
#define BRIGHTNESS 60

// Define matrix display panel width and height.
#define mw 16
#define mh 16

// MATRIX DECLARATION:
// Parameter 1 = width of each matrix (panel)
// Parameter 2 = height of each matrix
// Parameter 3 = number of matrices arranged horizontally
// Parameter 4 = number of matrices arranged vertically
// Parameter 5 = pin number (most are valid)
// Parameter 6 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the FIRST MATRIX; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs WITHIN EACH MATRIX are
//     arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns WITHIN
//     EACH MATRIX proceed in the same order, or alternate lines reverse
//     direction; pick one.
//   NEO_TILE_TOP, NEO_TILE_BOTTOM, NEO_TILE_LEFT, NEO_TILE_RIGHT:
//     Position of the FIRST MATRIX (tile) in the OVERALL DISPLAY; pick
//     two, e.g. NEO_TILE_TOP + NEO_TILE_LEFT for the top-left corner.
//   NEO_TILE_ROWS, NEO_TILE_COLUMNS: the matrices in the OVERALL DISPLAY
//     are arranged in horizontal rows or in vertical columns, respectively;
//     pick one or the other.
//   NEO_TILE_PROGRESSIVE, NEO_TILE_ZIGZAG: the ROWS/COLUMS OF MATRICES
//     (tiles) in the OVERALL DISPLAY proceed in the same order for every
//     line, or alternate lines reverse direction; pick one.  When using
//     zig-zag order, the orientation of the matrices in alternate rows
//     will be rotated 180 degrees (this is normal -- simplifies wiring).
//   See example below for these values in action.
// Parameter 7 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 pixels)
//   NEO_GRB     Pixels are wired for GRB bitstream (v2 pixels)
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA v1 pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(mw, mh, 1, 2, PIN,
                            NEO_MATRIX_TOP    + NEO_MATRIX_RIGHT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);

// This could also be defined as matrix.color(255,0,0) but those defines
// are meant to work for adafruit_gfx backends that are lacking color()
#define LED_BLACK    0

#define LED_RED_VERYLOW   (3 <<  11)
#define LED_RED_LOW     (7 <<  11)
#define LED_RED_MEDIUM    (15 << 11)
#define LED_RED_HIGH    (31 << 11)

#define LED_GREEN_VERYLOW (1 <<  5)
#define LED_GREEN_LOW     (15 << 5)
#define LED_GREEN_MEDIUM  (31 << 5)
#define LED_GREEN_HIGH    (63 << 5)

#define LED_BLUE_VERYLOW  3
#define LED_BLUE_LOW    7
#define LED_BLUE_MEDIUM   15
#define LED_BLUE_HIGH     31

#define LED_ORANGE_VERYLOW  (LED_RED_VERYLOW + LED_GREEN_VERYLOW)
#define LED_ORANGE_LOW    (LED_RED_LOW     + LED_GREEN_LOW)
#define LED_ORANGE_MEDIUM (LED_RED_MEDIUM  + LED_GREEN_MEDIUM)
#define LED_ORANGE_HIGH   (LED_RED_HIGH    + LED_GREEN_HIGH)

#define LED_PURPLE_VERYLOW  (LED_RED_VERYLOW + LED_BLUE_VERYLOW)
#define LED_PURPLE_LOW    (LED_RED_LOW     + LED_BLUE_LOW)
#define LED_PURPLE_MEDIUM (LED_RED_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_PURPLE_HIGH   (LED_RED_HIGH    + LED_BLUE_HIGH)

#define LED_CYAN_VERYLOW  (LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_CYAN_LOW    (LED_GREEN_LOW     + LED_BLUE_LOW)
#define LED_CYAN_MEDIUM   (LED_GREEN_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_CYAN_HIGH   (LED_GREEN_HIGH    + LED_BLUE_HIGH)

#define LED_WHITE_VERYLOW (LED_RED_VERYLOW + LED_GREEN_VERYLOW + LED_BLUE_VERYLOW)
#define LED_WHITE_LOW   (LED_RED_LOW     + LED_GREEN_LOW     + LED_BLUE_LOW)
#define LED_WHITE_MEDIUM  (LED_RED_MEDIUM  + LED_GREEN_MEDIUM  + LED_BLUE_MEDIUM)
#define LED_WHITE_HIGH    (LED_RED_HIGH    + LED_GREEN_HIGH    + LED_BLUE_HIGH)

uint16_t bmpcolor[] =   //Used in display_scrollText and display_FixedText
{ LED_RED_HIGH, 
  LED_BLUE_HIGH, 
  LED_WHITE_HIGH
};

//==========================================================
void display_scrollText(uint8_t messageNum, uint16_t color)
//==========================================================
{
#define SCROLL_DELAY 40      //Number of ms between display updates
#define MESSAGE_DELAY 500    //Number of ms between message repeat
  char message[41];

  // read the default message text stored in program memory
  for (byte k = 0; k < strlen_P(defaultText[messageNum]); k++) {
    message[k] = pgm_read_byte_near(defaultText[messageNum] + k);
    message[k + 1] = 0;
  }

  //Calculated number of display cycles needed
  // for the message to scroll off the display...
  int displayCycles;

  /*
    Serial.print("message = ");
    Serial.println(message);
    Serial.print("message Length = ");
    Serial.println(strlen(message));

    Serial.print("color = ");
    Serial.print(color);
    Serial.print("  ");
    Serial.println(color, HEX);
  */
  displayCycles = strlen(message) * 12;
  matrix.setTextColor(color);

  for (int x = matrix.width(); x > -displayCycles; x-- ) //One message scroll is complete
  {
    matrix.fillScreen(0);    //Turn off all the LEDs
    matrix.setCursor(x, 1);
    matrix.setTextSize(2);
    matrix.print(message);
    matrix.show();
    delay(SCROLL_DELAY);
  }
  delay(MESSAGE_DELAY);
  matrix.fillScreen(0);    //Turn off all the LEDs
  matrix.show();
}


//==============================================================
//==============================================================
void setup()
//==============================================================
//==============================================================
{
  Wire.begin();
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(BRIGHTNESS);   //0 (off) to 255 (max brightness)

  matrix.show();

}

//==============================================================
//==============================================================
void loop()         //=============== LOOP =====================
//==============================================================
//==============================================================
{


  for (uint8_t y = 0; y < 3; y++)
  {
    display_scrollText(y, bmpcolor[y % 8]);
  }
}
