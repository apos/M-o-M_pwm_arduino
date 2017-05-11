/***
  
FUN with your Mirror-o-Matic (12/24V)
 
Program for a custom made "Mirror-o-Matic" machine controlling two motors 
(turntable and ExtenderWheel) with and arduino (nano) and pwm.
We are using I2C for input devices like the 4x4 keypad and the 16x2 lcd screen.
We are using HM-10 for bluetooth communication
We are using TCST2103 transmissive optical sensors for controlling the speed of the motors.
We are using BTS7960 43A H-bridges for controlling an 12V and an 24V motor (3A)with PWM.

Homepage http://wiki.blue-it.org
Copyright (GNU public licence v. 2.0): 
Axel Pospischil
apos@gmx.de

Read the Changelog.md and Readme.md files contained in the source folder.

*/

/////////////////////////////////////////////////////////////////
// INCLUDES
/////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <EEPROMAnything.h>

// not used yet because of problems with setting the address
// #include <EEPROMAnything_A.h>
// #include <EEPROMAnything_B.h>


// BLUETOOTH
/* Include the software serial port library */
#include <SoftwareSerial.h>
/* to communicate with the Bluetooth module's TXD pin */
#define BT_SERIAL_TX 10
/* to communicate with the Bluetooth module's RXD pin */
#define BT_SERIAL_RX 11
/* Initialise the software serial port */
SoftwareSerial BluetoothSerial(BT_SERIAL_TX, BT_SERIAL_RX);
char c;

/////////////////////////////////////////////////////////////////
// VARIABLES THAT CAN (SHOULD) BE ALTLERLED
/////////////////////////////////////////////////////////////////

int DEBUG = 0;
int SIMULATION = 0;

/////////////////////////////////////////////////////////////////
// PINS
/////////////////////////////////////////////////////////////////

// defines
// KEYPAD 4x4
// I2C adress from PCF8574
#define I2CADDR 0x38

const int pwmPin_Motor_ExtenderWheel = 9;
const int pwmPin_Motor_Turntable = 5;

//const int ledPin = 13;

const int sensorPin_LS_ExtenderWheel = A0;
const int sensorPin_LS_Turntable = A1;

// MOTOR SETTINGS

// Scale the motor according due to its possibilities - why?:
// - var actualRPMExtenderWheel:  is limited to values from 0 to 9
// - var pulseCountExtenderWheel: depends on the motor - you should reach a value
//           slightly above 200 at maximum (== 9) - to have reserves at maximum speed 
//           Example: you are using a speed value of 9. But the resistence of your
//           medium is strong, so the motor has a little reserve to hold the speed of 9.
// a value of 1.2 gives me around 200 pwm with a big 12V/10A motor
// for a bigger 24V/10A motor I did not 
const int scaleMotorFactorExtenderWheel = 1.2; 
const int scaleMotorFactorTurntable = 1.0; 

// min pwm for the used motor at which it will begin to rotate
// It turns out, that for wiper motors a minimum of 20 to 30 is necessary, to get the motor running.
int minSpeed_Motor_ExtenderWheel = 30;
int minSpeed_Motor_Turntable = 20;

// This is: how fast the motor will switch from one pwm state to the other and vice versa
//          when slowing up or down on changing speed values (you should leave this at 5.
int increment_Motor_ExtenderWheel = 5;
int increment_Motor_Turntable = 5;


/////////////////////////////////////////////////////////////////
/// END OF USER DEFINED VARIABLES
/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
// VARIABLES FIXED
/////////////////////////////////////////////////////////////////

// Software Version
float version = 0.5;

// ExtenderWheel
int actualRPMExtenderWheel = 0;
int actualPWMExtenderWheel = 0;   // range: 0 - 255;
int pulseCountExtenderWheel = 0;   // max. Interval 1-9
int prevLSLevelExtenderWheel = 0;

// Turntable
int actualRPMTurntable = 0;
int actualPWMTurntable = 0;   // range: 0 - 255;
int pulseCountTurntable = 0;   // max. Interval 1-9
int prevLSLevelTurntable = 0;


// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time of action was updated
// unsigned long timerStartValueMicros = 0;   // for exact timer values

// interval at which to check pulses (milliseconds)
// be careful changing: affects pulseCountExtenderWheel!
const unsigned long interval = 1000;


// Typing "E"  changes to true. 
// Typing "T" changes to false.
// E: Extender, T: Turntable (default)
char modusSetRPM = 'T';

// EEPROM stored values
const int memActual = 0; // configurationActual
// const int memA = sizeof(configurationA); // ???
// const int memB = sizeof(configurationB); // ???

// Stores the actual values in EEPROM
// Will be restored after new start, Power off, stoppong with '0', pressing 'C'
struct configurationActual_t
{
    float version;
    int firstRun;
    boolean timerEnabled;    
    char memory;
    // TIMER values are in minutes !
    float timerValue; // given by user
    float actualTimerValue;
    int speedTurntable;
    int speedExtenderWheel;
    int simulation;

} configurationActual = { 
  0.0, 1, true, 'X', 0.0, 0.0, 0, 0 
};

// not used yet
struct configurationA_t
{
    float version;
    int firstRun;
    boolean timerEnabled;
    char memory;
    // TIMER values are in minutes !
    float timerValue; // given by user
    float actualTimerValue;
    int speedTurntable;
    int speedExtenderWheel;

// not used yet
} configurationA = { 
  0.0, 1, true, 'A', 0.0, 0.0, 0, 0 
};

struct configurationB_t
{
    float version;
    int firstRun;
    boolean timerEnabled;
    char memory;
    // TIMER values are in minutes !
    float timerValue; // given by user
    float actualTimerValue;
    int speedTurntable;
    int speedExtenderWheel;

} configurationB = {
  0.0, 1, true, 'B', 0.0, 0.0, 0, 0 
};


///////////////////////////////////////////////////////////////// 
// LCD
// set the LCD address to 0x27 for a 16 chars 2 line display
// some LCD use 0x3F as address ...    
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

///////////////////////////////////////////////////////////////// 
// KEYPAD 4x4 matrix (foiled pad)
const byte numRows= 4; //number of rows on the keypad
const byte numCols= 4; //number of columns on the keypad


//keymap defines the key pressed according to the row and columns just as appears on the keypad
char keymap[numRows][numCols] = {
  {'1','2','3','A'}, 
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

//Code that shows the the keypad connections to the arduino with PCF8574 
byte rowPins[numRows] = {0, 1, 2, 3};   
byte colPins[numCols] = {4, 5, 6, 7};    

 
// Initialize an instance of the Keypad_I2C class
Keypad_I2C myKeypad( makeKeymap(keymap), rowPins, colPins, numRows, numCols, I2CADDR);


///////////////////////////////////////////////////////////////// 
// SETUP
void setup() {

  /////////////////////////////////////////////////////////////////////////////
  // INIT ON FIRST RUN (EEPROM empty)
  /////////////////////////////////////////////////////////////////////////////

  // Get stored values from last session from EEPROM
  EEPROM_readAnything(memActual, configurationActual);
  SIMULATION = configurationActual.simulation;
  
  //  if ( configurationActual.timerValue > 0 ) {    
  //      timerStartValueMicros = (unsigned long)(configurationActual.timerValue); 
  //  }
  
  if ( configurationActual.firstRun == 1 ) {

    configurationActual.firstRun = 0;
    configurationActual.version = version;
    configurationActual.memory = 'A';
    configurationActual.timerEnabled = true; 
    configurationActual.actualTimerValue = 0.0;
    configurationActual.timerValue = 0.0;
    configurationActual.speedTurntable = 0;
    configurationActual.speedExtenderWheel = 0;
    configurationActual.simulation = 0;

  } else {
    // EEPROM_readAnything(memActual, configurationActual);
  }

  // Arduino bord led
  pinMode(13, OUTPUT);

  // I2C for LCD and Keypad  
  lcd.begin(16,2);

  // 4x4 keypad - special settings
  // http://playground.arduino.cc/Code/Keypad#Functions
  myKeypad.addEventListener(keypadEvent);   // Add an event listener.
  // You might use other values
  myKeypad.setHoldTime(2000);               // Default is 1000mS
  myKeypad.setDebounceTime(100);            // Default is 50mS
  myKeypad.begin(); 

  Serial.begin(9600);


  // http://www.scynd.de/tutorials/arduino-tutorials/3-luefter-steuern/3-1-pwm-ohne-pfeifen.html
  // Timer1 (Pin 9 and 10) to 31300Hz - much smoother pwm with bigger motor
  //TCCR1B = TCCR1B & 0b11111000 | 0x02; 
  pinMode(pwmPin_Motor_ExtenderWheel, OUTPUT); 
  pinMode(pwmPin_Motor_Turntable, OUTPUT);
  //pinMode(ledPin, OUTPUT);   

  pinMode(sensorPin_LS_ExtenderWheel, INPUT);
  pinMode(sensorPin_LS_Turntable, INPUT);

  // INIT MOTOR
  digitalWrite(pwmPin_Motor_ExtenderWheel, LOW);
  digitalWrite(pwmPin_Motor_Turntable, LOW);
  
  analogWrite(pwmPin_Motor_Turntable, actualPWMExtenderWheel);
  analogWrite(pwmPin_Motor_ExtenderWheel, actualPWMTurntable);


  ///////////////////////////////////////////////////////
  // Show, if simulation mode is on
  if ( SIMULATION == 1 ) {
    lcd.clear();
    lcd.setCursor(3,0);
    lcd.print("SIMULATION");
    lcd.setCursor(3,1);
    lcd.print(" MODE ON! ");
    delay(2000);
  }


  ///////////////////////////////////////////////////////
  // BLUETOOTH - not implemented yet
  ///////////////////////////////////////////////////////

  // LCD info
  //lcd.setCursor(0,0);
  //lcd.print ( "Bluetooth setup ");

  // Set the baud rate for the software serial port
  BluetoothSerial.begin(9600);
  delay(100);

  /////////////////////////////////////////
  // Initial Setup of module HM10
  /////////////////////////////////////////
  // Reset all settings
  BluetoothSerial.println("AT+RENEW");
  waitForResponse();
  BluetoothSerial.println("AT+VERSION");
  waitForResponse();
  // This is neccessary to connect to android or other devices 
  BluetoothSerial.println("AT+TYPE2");
  waitForResponse();
  BluetoothSerial.println("AT+ROLE0");
  waitForResponse();
  // Set the name to M-O-M
  BluetoothSerial.println("AT+NAMEM-O-M");
  waitForResponse();
  // BluetoothSerial.println("AT+PASS000000");
  //waitForResponse();

  /////////////////////////////////////////
  // Other ALT+HELP

  // Should respond with its version
  // Soft reboot of device
  // BluetoothSerial.println("AT+RESET");
  // waitForResponse();
  // delay(3000);
  // BluetoothSerial.println("AT");
  // waitForResponse();
  // Set baudrate from 9600 (default) to 57600
  // * Note of warning * - many people report issues after increasing JY-MCU
  // baud rate upwards from the default 9,600bps rate (e.g. 'AT+BAUD4')
  // so you may want to leave this and not alter the speed!!
  // BluetoothSerial.print("AT+BAUD9600"); // 9600
  // BluetoothSerial.print("AT+BAUD57600"); // 57600
  // waitForResponse();
  // BluetoothSerial.println("AT+HELP");
  // waitForResponse();

  
  // LCD info
  //lcd.setCursor(0,1);
  //lcd.print ( "Bluetooth finish");
  //delay(2000);
  //lcd.clear();

  
}

// BLUETOOTH
void waitForResponse() {

  delay(20);  

  //read from HM-10 and write to Serial
  if(BluetoothSerial.available()) Serial.write(BluetoothSerial.read());

  //read from Serial and print to the HM-10
  if(Serial.available()) BluetoothSerial.write(Serial.read());
  
}

// EVENT HANDLER FOR KEYPAD
// Take care of some special HOLD (long press) events 
// see code above: myKeypad.addEventListener(keypadEvent);
void keypadEvent(KeypadEvent key){

  // LONG PRESS
  switch (myKeypad.getState()){
    
    case HOLD:
      switch (key){       

        case '#':
          // clearEEPROM();

          configurationActual.actualTimerValue = 0.0;
          configurationActual.timerValue = 0.0;
          configurationActual.speedExtenderWheel = 0;
          configurationActual.speedTurntable = 0;
          // timerStartValueMicros = micros();
          
          if ( isAnyMotorRunning() ) {
            
              lcd.clear();
              lcd.setCursor(2,0);
              lcd.print("Resetting all");
              lcd.setCursor(2,1);
              lcd.print("values! Stop!");
              
              eepromWriteActual();
              speedStopAll();
              
          } else {
            
              lcd.clear();
              lcd.setCursor(2,0);
              lcd.print("#: resetted");
              lcd.setCursor(2,1);
              lcd.print("   values.");
              delay(1500);   

              eepromWriteActual();  
          }
          
        break; 

        case '*':

          if ( SIMULATION == 0 ) {
              SIMULATION = 1;
              configurationActual.simulation = 1;
              lcd.clear();
              lcd.setCursor(3,0);
              lcd.print("SIMULATION");
              lcd.setCursor(3,1);
              lcd.print(" MODE ON! ");
              delay(2000);             
          } else {
              SIMULATION = 0;
              configurationActual.simulation = 0;
              lcd.clear();
              lcd.setCursor(3,0);
              lcd.print("Simulation");
              lcd.setCursor(3,1);
              lcd.print(" mode off");
              delay(2000);             
          }

          eepromWriteActual(); 

        break;

        case 'A':
          configurationActual.timerValue += 5;
          eepromWriteAll();
        break;

        case 'B':          
            if ( configurationActual.timerValue >= 5 ) configurationActual.timerValue -= 5;
            if ( configurationActual.timerValue <= 5 ) configurationActual.timerValue = 0;
            eepromWriteAll();
        break;

      }
      
    break;
  }
}

///////////////////////////////////////////////////////////////// 
// LOOP
void loop() {

  // Bluetooth communication
  // waitForResponse();

  ////////////////////////////////////////
  // read keypad
  char key = myKeypad.getKey();

  if ( key != NO_KEY ){

    // SHORT PRESS
    switch (key) {
        
        // Short press: write config and switch to ...
        case 'A':
          configurationActual.timerValue += 0.5;
          eepromWriteAll();
        break;

        case 'B':          
            if ( configurationActual.timerValue >= 0.5 ) configurationActual.timerValue -= 0.5;
            if ( configurationActual.timerValue <= 0.5 ) configurationActual.timerValue = 0;
            eepromWriteAll();
        break;

        case 'C':
          resumeActual();
        break;

        case 'D':
          if ( configurationActual.timerEnabled == true ) {
            configurationActual.timerEnabled = false;
          } else {
            configurationActual.timerEnabled = true;
          }
          eepromWriteAll();
        break;


        case '#':
          // do nothing see long pressed HOLD.
        break;

        // Short sets E/T
        case '*':
          if ( modusSetRPM == 'E') {
            modusSetRPM = 'T';
          } else {
            modusSetRPM = 'E'; 
          }
          eepromWriteAll();
        break;

        case '0':
          eepromWriteAll();
          speedStopAll();
        break;

        default:
        
          int num = num * 10 + (key - '0');

          if ( modusSetRPM == 'E' ) {
            actualRPMExtenderWheel = num;
            configurationActual.speedExtenderWheel = actualRPMExtenderWheel;
          } else {
            actualRPMTurntable = num;
            configurationActual.speedTurntable = actualRPMTurntable;
          }

          eepromWriteAll();
      }
  } 


  ////////////////////////////////////////
  // Messurement of real Turntable speed (pulseCount)
  // This is for the calculation of the speed with the speed messurement wheel and the light sensor

  // The light diode emitts values between 740 and 900 (sensorPin_LS_...).
  // These will be computed to a Level from 0 to 9 which result in ON and OFF.
  //  < 5: OFF
  // >= 5: ON
  int actLSLevelTurntable = map(analogRead(sensorPin_LS_Turntable),740,900,0,9);

  if ( SIMULATION == 0 ) {

    if ( prevLSLevelTurntable == actLSLevelTurntable )
    {
      // wait until the next segment is comming ...
    }
    else
    {  
      if (actLSLevelTurntable >= 5 && prevLSLevelTurntable < 3 ) // actual Level is High (Sensor is hidden)
      {   
         pulseCountTurntable +=1;
         prevLSLevelTurntable = actLSLevelTurntable;
      }

      if (actLSLevelTurntable < 3 && prevLSLevelTurntable >= 5 ) // actual Level is LOW (Sensor is free)
      {   
         pulseCountTurntable +=1;
         prevLSLevelTurntable = actLSLevelTurntable;
      }
    }

  }

  ////////////////////////////////////////
  // Messurement of real ExtenderWheel speed (pulseCount)
  // This is for the calculation of the speed with the speed messurement wheel and the light sensor

  if ( SIMULATION == 0 ) {

    int actLSLevelExtenderWheel = map(analogRead(sensorPin_LS_ExtenderWheel),740,900,0,9);
    
    if ( prevLSLevelExtenderWheel == actLSLevelExtenderWheel )
    {
      // wait until the next segment is comming ...
    }
    else
    {  
      if (actLSLevelExtenderWheel >= 5 && prevLSLevelExtenderWheel < 3 ) // actual Level is High (Sensor is hidden)
      {   
         pulseCountExtenderWheel +=1;
         prevLSLevelExtenderWheel = actLSLevelExtenderWheel;
      }

      if (actLSLevelExtenderWheel < 3 && prevLSLevelExtenderWheel >= 5 ) // actual Level is LOW (Sensor is free)
      {   
         pulseCountExtenderWheel +=1;
         prevLSLevelExtenderWheel = actLSLevelExtenderWheel;
      }
    }
  }

 
//////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////
  // CHECK EVERY 1000ms (usigned long millis() interval)
  // check to see if it's time to do something
  // the difference between the current time and last time
  // unsigned long currentTimerMillis = millis();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {

    // save the last time
    previousMillis = currentMillis;

   // Simulation mode: don't check sensor, just add speed ...
   if ( SIMULATION == 1 ) {

    pulseCountExtenderWheel += 1;
    pulseCountTurntable += 1;
    // if ( actualRPMExtenderWheel == 0 ) actualRPMExtenderWheel = 1;
    // if ( actualRPMTurntable == 0 ) actualRPMTurntable = 1;

    }

    ////////////////////////////////////////////////////////////////
    // Scale the motor according due to its possibilities
    // actualRPMExtenderWheel is limited to values from 0 to 9
    // pulseCountExtenderWheel depends on the motor - you should reach a value
    // slightly above 200 at maximum (== 9)- to have reserves at maximum speed
    pulseCountExtenderWheel = (int)(pulseCountExtenderWheel/scaleMotorFactorExtenderWheel);
    pulseCountTurntable = (int)(pulseCountTurntable/scaleMotorFactorTurntable);
    ////////////////////////////////////////////////////////////////

    if ( isAnyMotorRunning() )
    {

      // CHECK TIMER

      // 1. attempt: (not good)
      // set the time according to the setting of this timerStartValue in milli seconds
      // configurationActual.actualTimerValue = ((float)((unsigned long)(micros() - timerStartValueMicros) / 10000000)) / 10;
      // 2. attempt: (good)
      // Set the actual values according to the state of the machine and store them
      // This is a little bit unaccurate and gives an error about a few seconds
      // That's why it is 59,9 and not 60 which should prolong one second a little bit on the long run.
      // millis() on the arduino is in any case not entirely exact - but the mission is not that
      // time critical here.
      configurationActual.actualTimerValue += (1.0/59.9);

      // If timer mode is enabled and timerValue is reached, stop the table
      if ( configurationActual.timerEnabled == true ) {
        if ( configurationActual.actualTimerValue >= configurationActual.timerValue ) {
          speedStopAll();
        }
      } else {
        // count up the timer value corresponding to the actual time
        if ( configurationActual.timerValue <= configurationActual.actualTimerValue ) {
          configurationActual.timerValue = configurationActual.actualTimerValue;
        }
      }

      // eepromWriteAll();

    } // end only if motors running
    
    ////////////////////////////////////////////////////////////////


    ////////////////////////////////////////////////////////////////
    // ExtenderWheel
    int increment = increment_Motor_ExtenderWheel;

    if (actualPWMExtenderWheel <= 255 && actualPWMExtenderWheel >= 0) 
    { 
        if ( pulseCountExtenderWheel == actualRPMExtenderWheel )
        {
          // Motor speed is reached - do nothing
        }
        else 
        {
          // beschleunigen
          if (pulseCountExtenderWheel < actualRPMExtenderWheel) 
          {
            if ( actualPWMExtenderWheel < minSpeed_Motor_ExtenderWheel )
            {              
                actualPWMExtenderWheel += minSpeed_Motor_ExtenderWheel;
            }
            else if ( pulseCountExtenderWheel >= actualRPMExtenderWheel-1 )
            {
               actualPWMExtenderWheel += 1; 
            }
            else if (actualPWMExtenderWheel <= ( 255 - increment ) ) 
            {
                  actualPWMExtenderWheel += increment;
            }

          }
          
          // oder verlangsamen
          else
          {
            if ( actualPWMExtenderWheel >= increment )
            {
              if (  pulseCountExtenderWheel <= actualRPMExtenderWheel+1 )
                actualPWMExtenderWheel -= 1;
              else
                actualPWMExtenderWheel -= increment;
            }
          }
        }     
    }

    ////////////////////////////////////////////////////////////////
    // Turntable
    increment = increment_Motor_Turntable; 

    if (actualPWMTurntable <= 255 && actualPWMTurntable >= 0) 
    { 
        if ( pulseCountTurntable == actualRPMTurntable )
        {
          // Motor speed is reached - do nothing
        }
        else 
        {
          // beschleunigen
          if (pulseCountTurntable < actualRPMTurntable) 
          {
            if ( actualPWMTurntable < minSpeed_Motor_Turntable )
            {              
                actualPWMTurntable += minSpeed_Motor_Turntable;
            }
            else if ( pulseCountTurntable >= actualRPMTurntable-1 )
            {
               actualPWMTurntable += 1; 
            }
            else if (actualPWMTurntable <= ( 255 - increment ) ) 
            {
                  actualPWMTurntable += increment;
            }

          }
          
          // oder verlangsamen
          else
          {
            if ( actualPWMTurntable >= increment )
            {
              if (  pulseCountTurntable <= actualRPMTurntable+1 )
                actualPWMTurntable -= 1;
              else
                actualPWMTurntable -= increment;
            }
          }
        }     
    }

    analogWrite(pwmPin_Motor_ExtenderWheel, actualPWMExtenderWheel);
    analogWrite(pwmPin_Motor_Turntable, actualPWMTurntable);

    // LCD
    showInfoOn_LCD_16x2();

    // RESET pulseCountExtenderWheel
    pulseCountExtenderWheel = 0;
    pulseCountTurntable = 0;
  
  } // END INTERVAL (1000ms)
  
} // END loop()

///////////////////////////////////////////////////////////////// 
// FUNCTIONS

void showInfoOn_LCD_16x2() {

    ///////////////////////////////////////////////////////////////
    // LCD show modus
    // ExtenderWheel mode (true)
    if ( modusSetRPM == 'E' ) {
        lcd.setCursor(0,0);
        lcd.print(">EXT: ");
        lcd.setCursor(0,1);
        lcd.print(" Tbl: ");
    } else {

    // Turntable mode (false)
    //if ( modusSetRPM == 'T' ) {
        lcd.setCursor(0,0);
        lcd.print(" Ext: ");
        lcd.setCursor(0,1);
        lcd.print(">TBL: ");
     }

    // LCD - show the status 
    // LCD LINE 1
    lcd.setCursor(6, 0);
    String lcdViewActTimerValue = "";
    
    if ( isAnyMotorRunning() ) {
      if ( SIMULATION == 0 ) {
        lcdViewActTimerValue = String(actualRPMExtenderWheel) + "/" + String(pulseCountExtenderWheel) + " " + String(configurationActual.actualTimerValue,1) + "min  "; //  + ":" + String(configurationActual.memory) + "   ";
      } else {
        lcdViewActTimerValue = String(actualRPMExtenderWheel) + "/" + String(actualRPMExtenderWheel) + " " + String(configurationActual.actualTimerValue,1) + "min  "; //  + ":" + String(configurationActual.memory) + "   ";
      }
    } else {
        lcdViewActTimerValue = String(actualRPMExtenderWheel) + "/" + String(configurationActual.speedExtenderWheel) + " " + String(configurationActual.actualTimerValue,1) + "min  "; //  + ":" + String(configurationActual.memory) + "   ";
    }

    if ( isAnyMotorRunning() ) {
    // RUNNING
      if ( DEBUG == 0 ) {
        lcd.print(lcdViewActTimerValue);
      } else {
        lcd.print(String(actualRPMExtenderWheel) + "/" + String(actualPWMExtenderWheel,0) + "  ");
      }

    // STOPPED
    } else {
        lcd.print(lcdViewActTimerValue);
    }

    // LCD LINE 2
    lcd.setCursor(6, 1);

      String myTimer = "";
      String lcdViewTimerValue = "";
      if ( configurationActual.timerEnabled == true ) { myTimer = "T"; } else { myTimer = "nT"; };
      
      if ( isAnyMotorRunning() ) {
        if ( SIMULATION == 0 ) {
          lcdViewTimerValue = String(actualRPMTurntable) + "/" + String(pulseCountTurntable) +" " + String(configurationActual.timerValue,1) + myTimer + "   ";
        } else {
          lcdViewTimerValue = String(actualRPMTurntable) + "/" + String(actualRPMTurntable) +" " + String(configurationActual.timerValue,1) + myTimer + "   ";
        }
      } else {
          lcdViewTimerValue = String(actualRPMTurntable) + "/" + String(configurationActual.speedTurntable) +" " + String(configurationActual.timerValue,1) + myTimer + "   ";
      }

      if ( isAnyMotorRunning() ) { 
      // RUNNING
        if ( DEBUG == 0 ) {
          lcd.print(lcdViewTimerValue);
        } else {
          lcd.print(String(actualRPMTurntable) + " PC:" + String(actualPWMTurntable,0) + "  ");
        }
      // STOPPED
      } else {
        lcd.print(lcdViewTimerValue);
      }

}

void speedStopAll() {

  // PRE: make resume the after possible with "D"
  //      eepromWriteActual();

  int myDelay = 300;

  if ( actualPWMExtenderWheel <= 255 && actualPWMExtenderWheel >= 0 )
  {

      // SMOOTH STOP
      digitalWrite (13, HIGH);

      int toggleBlink = 0;
      while ( ( actualPWMExtenderWheel > 0 ) || ( actualPWMTurntable > 0 ) )
      {

        // ExtenderWheel
        if ( actualPWMExtenderWheel > 0 )
            actualPWMExtenderWheel -= increment_Motor_ExtenderWheel;

        if (actualPWMExtenderWheel <= increment_Motor_ExtenderWheel) actualPWMExtenderWheel = 0;
        analogWrite(pwmPin_Motor_ExtenderWheel,   actualPWMExtenderWheel);

        // // Turntable
        if ( actualPWMTurntable > 0 ) 
            actualPWMTurntable -= increment_Motor_Turntable;

        if ( actualPWMTurntable <= increment_Motor_Turntable) actualPWMTurntable = 0;
        analogWrite(pwmPin_Motor_Turntable,   actualPWMTurntable);

        // LCD signaling soft stop progress        
        lcd.setCursor(0,0);
        if ( toggleBlink == 0) {            
          lcd.print("s");
          lcd.setCursor(0,1);
          lcd.print("s");
          toggleBlink = 1; 
        } else {
          lcd.print(" ");
          lcd.setCursor(0,1);
          lcd.print(" ");          
          toggleBlink = 0; 
        }

        delay(myDelay);

      }
    
      // Reset global values for speed
      actualRPMExtenderWheel = 0;
      actualRPMTurntable = 0;

      digitalWrite(pwmPin_Motor_ExtenderWheel, LOW);
      digitalWrite(pwmPin_Motor_Turntable, LOW);

      digitalWrite (13, LOW);  
  
  }  
  
}

boolean isAnyMotorRunning() {

    if ( actualRPMTurntable == 0 && actualRPMExtenderWheel == 0 ) {
      return false;
    } else {
      return true;
    }

}

void resumeActual() {

  actualRPMTurntable = configurationActual.speedTurntable;
  actualRPMExtenderWheel = configurationActual.speedExtenderWheel;

}


void eepromWriteActual() {
  //configurationActual.speedTurntable = (int)(actualRPMTurntable/10);
  //configurationActual.speedExtenderWheel = (int)(actualRPMExtenderWheel/10);
  EEPROM_writeAnything(memActual, configurationActual);
}

void eepromWriteAll() {

  // if ( configurationActual.memory == 'A' ) eepromWriteA();
  // if ( configurationActual.memory == 'B' ) eepromWriteB();    
  eepromWriteActual();

}

void eepromWriteA() {

  configurationA.firstRun = configurationActual.firstRun;
  configurationA.version = configurationActual.version;
  configurationA.memory = 'A';
  configurationA.timerEnabled = configurationActual.timerEnabled; 
  configurationA.actualTimerValue = configurationActual.actualTimerValue;
  configurationA.timerValue = configurationActual.timerValue;
  configurationA.speedTurntable = (int)(actualRPMTurntable);
  configurationA.speedExtenderWheel = (int)(actualRPMExtenderWheel);
  
  // readMemoryA();

  // eepromWriteActual();
  // EEPROM_A_writeAnything(memA, configurationA);
}

void eepromWriteB() {

  configurationB.firstRun = configurationActual.firstRun;
  configurationB.version = configurationActual.version;
  configurationB.memory = 'B';
  configurationB.timerEnabled = configurationActual.timerEnabled; 
  configurationB.actualTimerValue = configurationActual.actualTimerValue;
  configurationB.timerValue = configurationActual.timerValue;
  configurationB.speedTurntable = (int)(actualRPMTurntable);
  configurationB.speedExtenderWheel = (int)(actualRPMExtenderWheel);

  // readMemoryB();
  // eepromWriteActual();

  // EEPROM_B_writeAnything(memB, configurationB);
}

void readMemoryA() {

    // EEPROM_A_readAnything(memA,configurationA);

    configurationActual.firstRun = configurationA.firstRun;
    configurationActual.version = configurationA.version;
    configurationActual.memory = 'A';
    configurationActual.timerEnabled = configurationA.timerEnabled; 
    configurationActual.actualTimerValue = configurationA.actualTimerValue;
    configurationActual.timerValue = configurationA.timerValue;
    configurationActual.speedTurntable = configurationA.speedTurntable;
    configurationActual.speedExtenderWheel = configurationA.speedExtenderWheel;
    actualRPMTurntable = configurationA.speedTurntable;
    actualRPMExtenderWheel = configurationA.speedExtenderWheel;
   
    //eepromWriteAll();

}

void readMemoryB() {

    // EEPROM_B_readAnything(memB,configurationB);

    configurationActual.firstRun = configurationB.firstRun;
    configurationActual.version = configurationB.version;
    configurationActual.memory = 'B';
    configurationActual.timerEnabled = configurationB.timerEnabled; 
    configurationActual.actualTimerValue = configurationB.actualTimerValue;
    configurationActual.timerValue = configurationB.timerValue;
    configurationActual.speedTurntable = configurationB.speedTurntable;
    configurationActual.speedExtenderWheel = configurationB.speedExtenderWheel;
    actualRPMTurntable = configurationB.speedTurntable;
    actualRPMExtenderWheel = configurationB.speedExtenderWheel;
    
    //eepromWriteAll();

}

void clearEEPROM() {

  /***
    Source: https://www.arduino.cc/en/Tutorial/EEPROMClear

    Iterate through each byte of the EEPROM storage.

    Larger AVR processors have larger EEPROM sizes, E.g:
    - Arduno Duemilanove: 512b EEPROM storage.
    - Arduino Uno:        1kb EEPROM storage.
    - Arduino Mega:       4kb EEPROM storage.

    Rather than hard-coding the length, you should use the pre-provided length function.
    This will make your code portable to all AVR processors.
  ***/

  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}



