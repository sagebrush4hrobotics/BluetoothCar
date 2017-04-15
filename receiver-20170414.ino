// Bluetooth Remote Smart Car Receiver
// By Jason Groce
// 2017-04-14

#include <Wire.h>
#include <Adafruit_MotorShield.h>  //https://github.com/adafruit/Adafruit_Motor_Shield_V2_Library
#include "utility/Adafruit_PWMServoDriver.h"
#include <Servo.h>
#include <NewPing.h>  //https://bitbucket.org/teckel12/arduino-new-ping/wiki/Home
#include <TimerFreeTone.h>  //https://bitbucket.org/teckel12/arduino-timer-free-tone/wiki/Home
#include <SoftwareSerial.h>

// Motor Shield
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_DCMotor *firstMotor = AFMS.getMotor(1); //front right
Adafruit_DCMotor *secondMotor = AFMS.getMotor(2); //front left
Adafruit_DCMotor *thirdMotor = AFMS.getMotor(3); //back left
Adafruit_DCMotor *fourthMotor = AFMS.getMotor(4); //back right

// Ultrasonic Sensor & Servo
NewPing sonar(7, 6, 300);  //trigger, echo, max distance
Servo servo;
int pos = 90;
boolean sweep = true;  //TRUE=left, FALSE=right
// The servo position appears reversed; larger values mean left, smaller means right
int mn = 45;  //right min
int mx = 135;  //left max
int spd = 100;  //speed when running on auto
int maxdist = 25;
unsigned int distance = 100;

// Bluetooth
SoftwareSerial BTSerial(4, 5); // TX | RX
#define DATABUFFERSIZE 35  //max number expected incoming characters
char dataBuffer[DATABUFFERSIZE+1]; //Add 1 for NULL terminator
byte dataBufferIndex = 0;

//characters used to parse serial string
char startChar = '<';
char endChar = '>';
char delimiters[] = "<,>";

// Communication Variables
boolean storeString = false; //This will be our flag to put the data in our buffer
char* incomingData;  //string of data from serial bluetooth
int command[10];        // 10 element array holding integers
int response[6];
boolean automode = false;

// Motor Variables
int JoystickX = 400;    // horizontal joystick measurement
int JoystickY = 400;    // vertical joystick measurement
int speedX = 0;     // used to calculate speed based on JoystickX
int speedY = 0;     // used to calculate speed based on JoystickY
int speedR = 0;     // calculated speed for right wheels
int speedL = 0;     // calculated speed for left wheels
int Lmin = 0;       // minimum measurement for lower joystick half
int Lmax = 275;     // maximum measurement for lower joystick half
int Hmin = 425;     // minimum measurement for upper joystick half
int Hmax = 700;   // maximum measurement for upper joystick half

// Inputs & Outputs
// NOTE: Buttons are inversed; 0=on, 1=off
int aButton = 1;  // up buttton
int bButton = 1;  // right buttton
int cButton = 1;  // down buttton
int dButton = 1;  // left buttton
int eButton = 1;  // select buttton
int fButton = 1;  // start buttton
int gButton = 1;  // joystick buttton
int color = 0;    // color mode
int redpin = A0;    // red LED
int greenpin = A1;  // green LED
int bluepin = A2;   // blue LED
int gndPin = 17;    // pin A3, rgb LED ground
int horn = 11;      // piezo buzzer

void setup() {
  pinMode(gndPin, OUTPUT);    // artificial ground pin for LED
  digitalWrite(gndPin, LOW);  // set to ground
  pinMode(redpin, OUTPUT);    // red LED
  pinMode(greenpin, OUTPUT);  // green LED
  pinMode(bluepin, OUTPUT);   // blue LED
  pinMode(horn, OUTPUT);      // piezo buzzer
  
  AFMS.begin();                // initialize Adafruit Motor Shield
  firstMotor->setSpeed(spd);   // front right motor
  firstMotor->run(FORWARD);
  secondMotor->setSpeed(spd);  // front left motor
  secondMotor->run(FORWARD);
  thirdMotor->setSpeed(spd);   // back left motor
  thirdMotor->run(FORWARD);
  fourthMotor->setSpeed(spd);  // back right motor
  fourthMotor->run(FORWARD);
  
  servo.attach(10);            // initialize servo
  servo.write(pos);            // set servo to starting position
  delay(3000);                 // pause for 3 seconds to allow for position calibration
  servo.detach();              // turn off servo to avoid bluetooth serial interference
  
  BTSerial.begin(9600);        // initialize bluetooth serial
}

void loop() {
  if(getSerialString()){  // check for valid incoming data
    radioListen();        // if data valid, parse data
  } else {                // if no valid data, set default input values
    JoystickX = 500;
    JoystickY = 500;
    aButton = 1; 
    bButton = 1;
    cButton = 1;
    dButton = 1;
    eButton = 1;
    fButton = 1;
    gButton = 1;
  }
  radioSend();
  delay(80);  //need delay to avoid corrupt read after send

  //TimerFreeTone used due to conflicts with timer usage by Servo
  //See the below links for information regarding conflicts between the built-in tone() and servo() libraries
  //http://forum.arduino.cc/index.php?topic=106043.msg1312639.html#msg1312639
  //https://bitbucket.org/teckel12/arduino-timer-free-tone/wiki/Home

  if(gButton == 0){  // joystick button pushed
    TimerFreeTone(horn,262,200);
  }
  if(eButton == 0){  // start button pushed
    automode=!automode;
    delay(2000);
    eButton = 1;
  }
  if(fButton == 0){  //select button pushed
    dance();
    fButton = 1;
  } else if(automode){
    autobot();
  } else {
    manualbot();
  }
}

//Sets RGB LED color
void light(int r, int g, int b){
    analogWrite(redpin,r);
    analogWrite(greenpin,g);
    analogWrite(bluepin,b);
}

void manualbot(){
  //Self-Calibration code.  If joystick is outside range, extend existing range to fit
  if( (JoystickY < 1024) && (JoystickY > Hmax) ) {  //joystick recalibrate
    Hmax = JoystickY;
    Hmin = (Hmax / 2) + 75;
    Lmax = (Hmax / 2) - 75;
  }
  if( (JoystickX < 1024) && (JoystickX > Hmax) ) {  //joystick recalibrate
    Hmax = JoystickX;
    Hmin = (Hmax / 2) + 75;
    Lmax = (Hmax / 2) - 75;
  }

  if( JoystickY > Hmin ) {              // if pointing up, move forward
    speedY = map(JoystickY,Hmin,Hmax,0,255);    // map vertical joystick to motor speed
    turn();                               // check to see if pointing left or right
    firstMotor->run(FORWARD);
    secondMotor->run(FORWARD);
    thirdMotor->run(FORWARD);
    fourthMotor->run(FORWARD);
  } else if( JoystickY < Lmax ) {       // if pointing down, move backward
    speedY = map(JoystickY,Lmax,Lmin,0,255);    // map vertical joystick to motor speed
    turn();                               // check to see if pointing left or right
    firstMotor->run(BACKWARD);
    secondMotor->run(BACKWARD);
    thirdMotor->run(BACKWARD);
    fourthMotor->run(BACKWARD);
  } else if ( JoystickX < Lmax ){       // if pointing due left, spin left
    speedL = map(JoystickX,Lmax,Lmin,0,255);    // map horizontal joystick to motor speed
    speedR = speedL;
    firstMotor->run(FORWARD);
    secondMotor->run(BACKWARD);
    thirdMotor->run(BACKWARD);
    fourthMotor->run(FORWARD);
  } else if ( JoystickX > Hmin ){       // if pointing due right, spin right
    speedR = map(JoystickX,Hmin,Hmax,0,255);    // map horizontal joystick to motor speed
    speedL = speedR;
    firstMotor->run(BACKWARD);
    secondMotor->run(FORWARD);
    thirdMotor->run(FORWARD);
    fourthMotor->run(BACKWARD);
  } else {                                // centered, stop
    speedX = 0;
    speedY = 0;
    speedR = 0;
    speedL = 0;
  }
  
  if ( speedL < 0 ) {   // check for incorrect reading and clear bad numbers
    speedL = 0;
  }
  if ( speedR < 0 ) {
    speedR = 0;
  }
  
  firstMotor->setSpeed(speedR);
  secondMotor->setSpeed(speedL);
  thirdMotor->setSpeed(speedL);
  fourthMotor->setSpeed(speedR);

  // Moving the Ultrasonic Sensor Servo left and right
  if(bButton == 0){  // right button
    if(pos > mn){  // position is not at minimum yet, so move right
      pos-=5;
    }
    servo.attach(10);
    delay(80);       //delay needed to clear interference from bluetooth
    servo.write(pos);
    servo.detach();  //detach needed to avoid interference with bluetooth
    delay(100);
  } else if(dButton == 0){  // left button
    if(pos < mx){  // position is not at maximum yet, so move left
      pos+=5;
    }
    servo.attach(10);
    delay(80);       //delay needed to clear interference from bluetooth
    servo.write(pos);
    servo.detach();  //detach needed to avoid interference with bluetooth
    delay(100);
  }
  //see link below for details on servo connectivity with SoftwareSerial:
  //http://tutorial.cytron.com.my/2015/09/04/softwareserial-conflicting-with-servo-interrupt/

  // Read color buttons, set the RGB LED color
  if(aButton == 0){  // top button
    color++;
    if(color>3){
      color=0;
    }
    delay(100);
  } else if(cButton == 0){  // bottom button
    color--;
    if(color<0){
      color=3;
    }
    delay(100);
  }
  switch(color){  //lights set to user color selection
    case 0:
      light(255,255,255);  //white
      break;
    case 1:
      light(255,0,0);  //red
      break;
    case 2:
      light(0,255,0);  //green
      break;
    case 3:
      light(0,0,255);  //blue
      break;
  }
}

void turn() {
  if ( JoystickX < Lmax ) {                       // if joystick pointing left, go left
    speedX = map(JoystickX,Lmax,Lmin,0,255);    // map horizontal joystick to motor speed value
    speedL = speedY - speedX;                     // slow left wheels by amount joystick points left
    speedR = speedY;                              // full speed right wheels
  } else if ( JoystickX > Hmin ) {                // if joystick pointing right, go right
    speedX = map(JoystickX,Hmin,Hmax,0,255);    // map horizontal joystick to motor speed value
    speedR = speedY - speedX;                     // slow right wheels by amount joystick points right
    speedL = speedY;                              // full speed left wheels
  } else {
    speedL = speedY;                              // full speed ahead!
    speedR = speedY;
  }
}

void autobot(){
  // set motors to default speed
  firstMotor->setSpeed(spd);
  secondMotor->setSpeed(spd);
  thirdMotor->setSpeed(spd);
  fourthMotor->setSpeed(spd);

  // check distance
  distance = sonar.ping_cm();

  if(distance < maxdist && distance > 0){ //object detected, back up
    if(pos > 95){                 //if head pointing left, turn right
      firstMotor->run(BACKWARD);  //front right
      secondMotor->run(FORWARD);  //front left
      thirdMotor->run(FORWARD);   //back left
      fourthMotor->run(BACKWARD); //back right
    }else if(pos < 85){           //if head pointing right, turn left
      firstMotor->run(FORWARD);
      secondMotor->run(BACKWARD);
      thirdMotor->run(BACKWARD);
      fourthMotor->run(FORWARD);
    }else{                        //straight back
      firstMotor->run(BACKWARD); 
      secondMotor->run(BACKWARD);
      thirdMotor->run(BACKWARD); 
      fourthMotor->run(BACKWARD);
    }
    light(128,0,255);             //LED purple while backing up
  } else {
    firstMotor->run(FORWARD);
    secondMotor->run(FORWARD);
    thirdMotor->run(FORWARD);
    fourthMotor->run(FORWARD);
    if(sweep){  // if sweeping head left
      pos+=5;   // turn head left
    }else{
      pos-=5;   // turn head right
    }
    if(pos <= mn || pos >= mx){ // if head movement at min/max position
      sweep=!sweep;             // start to sweep opposite direction
    }
    if((sweep && pos>90) || (!sweep && pos<90)){  // flash lights blue and red
      light(255,0,0);  // red when sweeping left and pointing left/sweeping right and pointing right
    } else {
      light(0,0,255);  // blue when sweeping left and pointing right/sweeping right and pointing left
    }
    // turn servo on
    servo.attach(10);
    delay(50);         //delay needed to clear interference from bluetooth
    servo.write(pos);  // move servo to new position
    servo.detach();    // detach needed to avoid interference with bluetooth
  }
}

void dance(){
  //set color to "10" and tell remote so it will know demo mode
  int oldColor = color;
  color = 10;
  radioSend();
  color = oldColor;  //set color back

  //turn on servo and wait for serial interference to clear
  servo.attach(10);
  pos = 90;
  delay(250);

  //full speed
  firstMotor->setSpeed(250);
  secondMotor->setSpeed(250);
  thirdMotor->setSpeed(250);
  fourthMotor->setSpeed(250);

  //spin in a circle
  firstMotor->run(FORWARD);
  secondMotor->run(BACKWARD);
  thirdMotor->run(BACKWARD);
  fourthMotor->run(FORWARD);

  // turn head left and right
  servo.write(mx);
  delay(1000);
  servo.write(mn);
  delay(1000);
  servo.write(pos);
  delay(1000);

  // sing "la cucaracha"
  // Note reference: C4=262, D4=294, E4=330; F4=349, G4=392, A4=440, B4=494
  int melody[] =        { 262,262,262,349,440,  0,262,262,262,349,440 };  //tone to play
  int noteDurations[] = {   8,  8,  8,  2,  2,  2,  8,  8,  8,  2,  2 };  //how long to play it, in 1/Nth seconds
  int reds[] =          { 255,  0,  0,255,  0,  0,255,  0,  0,255,  0 };  //red LED value
  int greens[] =        {   0,255,  0,  0,255,  0,  0,255,  0,  0,255 };  //green LED value
  int blues[] =         {   0,  0,255,  0,  0,255,  0,  0,255,  0,  0 };  //blue LED value
  for (int i = 0; i < 11; i++) {
    int noteDuration = 1000/noteDurations[i];    // calculate nth of second to play note
    light(reds[i],greens[i],blues[i]);           // change LED color
    TimerFreeTone(horn,melody[i],noteDuration);  // play note
    delay(125);                         // wait between notes
  }

  //spin the other way
  firstMotor->run(BACKWARD);
  secondMotor->run(FORWARD);
  thirdMotor->run(FORWARD);
  fourthMotor->run(BACKWARD);

  // turn head left and right
  servo.write(mx);
  delay(1000);
  servo.write(mn);
  delay(1000);
  servo.write(pos);
  delay(1000);

  // sing "la cucaracha" again
  for (int i = 0; i < 11; i++) {
    int noteDuration = 1000/noteDurations[i];
    light(reds[i],greens[i],blues[i]);
    TimerFreeTone(horn,melody[i],noteDuration);
    delay(125);
  }
  
  servo.detach();  //detach needed to avoid interference with bluetooth
}

//See the below links for information regarding parsing serial information
//http://jhaskellsblog.blogspot.com/2011/05/serial-comm-fundamentals-on-arduino.html
//http://jhaskellsblog.blogspot.com/2011/06/parsing-quick-guide-to-strtok-there-are.html
//http://techshorts.ddpruitt.net/2013/08/remote-controlled-robotank_99.html

void radioListen(){
  // String available for parsing.  Parse it here
  // Split the data into chunks to be used as variables
  incomingData = strtok(dataBuffer, delimiters);
  for(int i = 0; i < 10; i++){
    command[i] = atoi(incomingData);  //atoi() turns char * to int, literaly "a" to #
    //Here we pass in a NULL value, which tells strtok to continue working with the previous string
    incomingData = strtok(NULL, delimiters);
  }
  if(command[9]==1234){      //Last variable received should be "1234" to indicate valid data
    JoystickX = command[0];  // Copy the commands to the Joystick variables
    JoystickY = command[1];
    aButton = command[2];    // up buttton
    bButton = command[3];    // right buttton
    cButton = command[4];    // down buttton
    dButton = command[5];    // left buttton
    eButton = command[6];    // start buttton
    fButton = command[7];    // select buttton
    gButton = command[8];    // joystick buttton
  }
}

void radioSend(){
  distance = sonar.ping_cm();
  response[0] = distance;
  response[1] = pos;
  response[2] = color;
  response[3] = automode;
  response[4] = pos;
  //send data
  BTSerial.print("<");
  for(int i = 0; i < 5; i++){
    BTSerial.print(response[i]);
    BTSerial.print(",");
  }
  BTSerial.print("1234>");  //Last variable sent is "1234" to indicate valid data
}

boolean getSerialString(){
  // This is where the magic happens; pull in info from the serial buffer,
  // and check that it is intact and valid information
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
