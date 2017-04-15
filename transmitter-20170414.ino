// Bluetooth Remote Smart Car Transmitter
// By Jason Groce
// 2017-04-14

#include <SPI.h>
#include <Adafruit_GFX.h>  //https://learn.adafruit.com/adafruit-gfx-graphics-library
#include <Adafruit_PCD8544.h>  //https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
#include <SoftwareSerial.h>

//See the below links for information regarding Bluetooth pairing between Arduinos
//http://www.martyncurrey.com/connecting-2-arduinos-by-bluetooth-using-a-hc-05-and-a-hc-06-pair-bind-and-link/
//http://www.martyncurrey.com/arduino-to-arduino-by-bluetooth/

SoftwareSerial BTSerial(0, 1); // TX | RX
#define DATABUFFERSIZE 24  //max number expected incoming characters
char dataBuffer[DATABUFFERSIZE+1]; //Add 1 for NULL terminator
byte dataBufferIndex = 0;

//characters used to parse serial string
char startChar = '<';
char endChar = '>';
char delimiters[] = "<,>";

boolean storeString = false; //This will be our flag to put the data in our buffer
char* incomingData;  //string of data from serial bluetooth

int command[10];  // 10 element array holding integers
int response[6];
int distance = 0;
int color = 0;
int pos = 90;
int onlineTimer = 0;
boolean automode = false;
boolean timeout = false;

// LCD Screen Connection using Software SPI (slower updates, more flexible pin options):
// pin 9 Serial clock out (SCLK)
// pin 10 Serial data out (DIN)
// pin 11 Data/Command select (D/C)
// pin 13 LCD chip select (CS)
// pin 12 LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(9, 10, 11, 13, 12);

// settings to configure nokia 5110 display
#define NUMFLAKES 10 
#define XPOS 0 
#define YPOS 1 
#define DELTAY 2

// size settings for logo
#define LOGO48_GLCD_HEIGHT 48
#define LOGO48_GLCD_WIDTH  48

// 4-H Logo to display at startup
static const unsigned char PROGMEM logo48_glcd_bmp[] = 
{ B00000000, B00000000, B00000000, B00000001, B11000000, B00000000,
  B00000000, B00001111, B10000000, B00000111, B11110000, B00000000,
  B00000000, B00111111, B11100000, B00001111, B11111100, B00000000,
  B00000000, B00111111, B11110000, B00011111, B11111100, B00000000,
  B00000000, B01111111, B11111000, B00111111, B11111110, B00000000,
  B00000000, B11111111, B11111100, B00111111, B11111110, B00000000,
  B00000001, B11111111, B11111100, B01111111, B11001111, B00000000,
  B00000011, B11100111, B11111100, B01111111, B10001111, B11000000,
  B00000111, B11100011, B11111100, B01111111, B00011111, B11100000,
  B00001111, B11110001, B11111100, B01111110, B00111101, B11110000,
  B00011111, B01111000, B11111110, B01111100, B00011000, B11111000,
  B00111110, B00110000, B01111110, B01111000, B10000001, B11111000,
  B00111111, B00000010, B00111110, B01111001, B11000011, B11111000,
  B00111111, B10000111, B01111110, B01111111, B11000111, B11111000,
  B00111111, B11000011, B11111110, B01111111, B10001111, B11111000,
  B00111111, B11100001, B11111110, B01111111, B00011111, B11111000,
  B00111111, B11110001, B11111111, B11111111, B11111111, B11110000,
  B00011111, B11111111, B11111111, B11111111, B11111111, B11110000,
  B00001111, B11111111, B11111111, B11111111, B11111111, B11000000,
  B00000111, B11111111, B11111111, B11111111, B11111111, B10000000,
  B00000001, B11111111, B11111111, B11111111, B11111100, B00000000,
  B00000000, B00000000, B00111111, B11111000, B00000000, B00000000,
  B00000000, B00000000, B00111111, B11111000, B00001110, B00000000,
  B00000011, B11111111, B11111111, B11111111, B11111111, B11100000,
  B00001111, B11111111, B11111111, B11111111, B11111111, B11110000,
  B00011111, B11111111, B11111111, B11111111, B11111111, B11111000,
  B00111111, B11111111, B11111111, B11111111, B11011111, B11111100,
  B00111111, B11100011, B11111111, B11111111, B10001111, B11111100,
  B01111111, B11000011, B11111111, B11111111, B11000111, B11111100,
  B01111111, B10000111, B11111001, B10111111, B11100001, B11111100,
  B01111111, B00001111, B01111001, B10111110, B11100000, B11111100,
  B01111110, B00000110, B00111001, B10011100, B01000100, B01111100,
  B00111100, B01100000, B01111001, B10011110, B00001110, B01111000,
  B00111111, B11110000, B11111001, B10011111, B00001111, B11111000,
  B00011111, B11100001, B11111001, B10011111, B10000111, B11110000,
  B00001111, B11000111, B11110001, B10011111, B11100011, B11100000,
  B00000111, B11001111, B11110001, B10001111, B11100011, B11000000,
  B00000011, B11111111, B11110001, B10001111, B11111111, B10000000,
  B00000001, B11111111, B11100001, B10001111, B11111111, B10000000,
  B00000000, B11111111, B11100001, B11000111, B11111111, B00000000,
  B00000000, B11111111, B11000001, B11000011, B11111110, B00000000,
  B00000000, B01111111, B10000001, B11000001, B11111100, B00000000,
  B00000000, B00111111, B00000000, B11100000, B01111000, B00000000,
  B00000000, B00000000, B00000000, B11100000, B00000000, B00000000,
  B00000000, B00000000, B00000000, B11110000, B00000000, B00000000,
  B00000000, B00000000, B00000000, B01110000, B00000000, B00000000,
  B00000000, B00000000, B00000000, B01111000, B00000000, B00000000,
  B00000000, B00000000, B00000000, B00000000, B00000000, B00000000 };

void setup() {
  display.begin();          // initialize display 
  display.setContrast(50);  // this can be changed to make the print darker or lighter
  display.clearDisplay();
  display.drawBitmap(18, 0, logo48_glcd_bmp, 48, 48, 1);  //show logo
  display.display();        // prints current screen buffer
  delay(2000);              // wait 2 seconds
  display.clearDisplay();   // clears the screen and buffer   
  
  BTSerial.begin(9600);     // initialize bluetooth serial
}

void loop() {

  display.clearDisplay();        // clear display
  display.setTextSize(1);        // 5 by 7 pixel font
  display.setTextColor(BLACK);   // black text on white background
  display.setCursor(0,0);        // start in top left corner
  int currentMillis = millis();  // store current time as a variable
  
  command[0] = analogRead(A0); // read measurement of vertical Joystick
  command[1] = analogRead(A1); // read measurement of horizontal Joystick
  command[2] = digitalRead(2); // read up button
  command[3] = digitalRead(3); // read right button
  command[4] = digitalRead(4); // read down button
  command[5] = digitalRead(5); // read left button
  command[6] = digitalRead(6); // read start button
  command[7] = digitalRead(7); // read select button
  command[8] = digitalRead(8); // read joystick button

  //receive data
  if(getSerialString()){  // check for valid incoming data
    //String available for parsing.  Parse it here
    // Split the data into chunks to be used as variables
    incomingData = strtok(dataBuffer, delimiters);
    for(int i = 0; i < 6; i++){
      response[i] = atoi(incomingData);  //atoi() turns char * to int
      //Here we pass in a NULL value, which tells strtok to continue working with the previous string
      incomingData = strtok(NULL, delimiters);
    }
    if(response[5]==1234){      //Last variable received should be "1234" to indicate valid data
      distance = response[0];
      pos = response[1];
      color = response[2];
      automode = response[3];
      pos = response[4];
      onlineTimer = currentMillis;  //reset online timer
      
      //send data
      BTSerial.print("<");
      for(int i = 0; i < 9; i++){
        BTSerial.print(command[i]);
        BTSerial.print(",");
      }
      BTSerial.print("1234>");  //Last variable sent is "1234" to indicate valid data
    }
    
    if(!command[6]||!command[7]){  //"!command" used because buttons are inversed
      delay(1000);    //if a mode button was pressed, wait 1 sec
    } else if(!command[2]||!command[3]||!command[4]||!command[5]||!command[6]||!command[7]){
      delay(250);    //if a red or blue button was pressed, wait .25 sec
    }
  }

  //check online timer
  if((currentMillis-onlineTimer) > 1000){
    display.setTextColor(WHITE, BLACK); // 'inverted' text
    display.println("Offline...");
    display.setTextColor(BLACK);
  }else{
    display.println("Online");
  }

  //display mode, LED color
  //line 2-3
  display.print("Mode: ");
  if(color == 10){
    display.println("Demo");
    display.println(" ");
    //IDEA: add a picture or animation during demo mode?
  } else if(automode) {
    display.println("Auto");
    display.println(" ");
    //IDEA: in automode, add a sweeping radar graphic with distances?
  } else {
    display.println("Manual");
    display.print("Color: ");
    switch(color){
      case 0:
        display.println("white");
        break;
      case 1:
        display.println("red");
        break;
      case 2:
        display.println("green");
        break;
      case 3:
        display.println("blue");
        break;
    }
  }

  //display head position
  //line 4
  display.print("Sonar: ");
  display.print(-(pos - 90));  //return -45 to 45, left to right, for servo position
  display.print((char)247);  //0x247 or 0x0ba or (char)223
  display.setCursor(76,24);  //set to end of 4th line
  if(pos > 90) {
    display.print((char)27);  //Left, char 27?  <
  } else if(pos < 90) {
    display.print((char)26);  //Right, char 26?  <
  } else {
    display.print((char)24);  //Forward, char 24?  ^
  }
  display.setCursor(0,32);  //set to 5th line

  //display distance
  //line 5-6
  if(distance > 0 && distance < 300){  //if within 6 inches/15 cm
    display.print("Distance: ");
    display.println(distance);
    if(distance < 25){
      display.setTextColor(WHITE, BLACK); // 'inverted' text
      display.println("Sensor Alarm!");
      display.setTextColor(BLACK);
    } else {
      display.println(" ");
    }
  } else {  //if outside range 0-300, "unknown"
    display.print("Dist: ");
    display.setTextColor(WHITE, BLACK); // 'inverted' text
    display.println("Unknown");
    display.setTextColor(BLACK);
  }
  display.display();
}

boolean getSerialString(){
  storeString = false; //This will be our flag to put the data in our buffer
  dataBufferIndex = 0;
  char incomingbyte;
  while(BTSerial.available()>0){  //bytes incoming?
    incomingbyte = BTSerial.read();
    if(incomingbyte==startChar){
      dataBufferIndex = 0;  //Initialize our dataBufferIndex variable
      if(storeString){
        break; //false start
      } else {
        storeString = true;  //start recording
      }
    }
    if(storeString){  //recording data
      //Let's check our index here, and abort if we're outside our buffer size
      //We use our define here so our buffer size can be easily modified
      if(dataBufferIndex==DATABUFFERSIZE){
        //Oops, our index is pointing to an array element outside our buffer.
        dataBufferIndex = 0;
        break;
      }
      if(incomingbyte==endChar){  //all done
        dataBuffer[dataBufferIndex] = 0; //null terminate the C string
        //Our data string is complete.  return true
        return true;
      }else{  //not finished, keep chugging
        dataBuffer[dataBufferIndex++] = incomingbyte;
        dataBuffer[dataBufferIndex] = 0; //null terminate the C string
      }
    }
  }
  //We've read in all the available Serial data, and don't have a valid string yet, so return false
  return false;
}
