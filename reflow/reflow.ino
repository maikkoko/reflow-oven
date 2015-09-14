/***********************************************************
   Optimized for SMD291AX
   Sn63/ Pb37
***********************************************************/
#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "Wire.h"
#include <LiquidCrystal.h>

/*** Definitions for Max 31855 ***/
#define DO   2
#define CS   3
#define CLK  4

Adafruit_MAX31855 thermocouple(CLK, CS, DO);

double prevTemp = 0.0; //to prevent printing 0 value

/*** Definitions for RTC***/
#define DS1307_ADDRESS 0x68

byte zero = 0x00; //data holder for RTC

/*** Definitions for LCD ***/
LiquidCrystal lcd(10, 9, 8, 7, 6, 5);

/**** Definitions for Buttons and LEDs ****/
#define BTN 11 //Button
#define PWR 13 //Relay Switch

/*********************************************
/ states:
/ 0 - Starting State
/ 1 - Preheating (after button press)
/ 2 - Soaking (150 - 180)
/ 3 - Reflow (180 - 210)
/ 4 - Cooldown
*********************************************/
int state = 0; 

/******* Main Code ********/ 

void setup() {
  Wire.begin();
  Serial.begin(9600);
  
  initLCD();
  resetTime();
  state = 0;
  
  pinMode(PWR, OUTPUT);
  digitalWrite(PWR, LOW);

  pinMode(BTN, INPUT);  
}

void loop() {
  
  printSerial(); //debugging and dev purposes
  
  if(digitalRead(BTN) == HIGH) {
    stateHandler();
    printLowerLCD();
  } else {
    resetTime();
    digitalWrite(PWR, LOW);
    
    if (checkTemperature() >= 35) {
      lcd.begin(16, 2);
      lcd.print("Cooling down... ");
      printLowerLCD(); 
    } else {
      initLCD();
      state = 0;
    }
  }
  
  delay(750);
}

/**** State Handler Subroutine ****/
void stateHandler() {
  lcd.clear();
  switch(state) {
    case 0:
      if (checkTemperature() >= 37) {
        state = 1;
      }
      resetTime();
      digitalWrite(PWR, HIGH);
      lcd.print("Starting...");
      break;
    case 1:
      if (checkTemperature() >= 100) {
        resetTime();
        state = 2;
      } 
      if (printTime() <= 10) {
        lcd.print("Insert PCB...");  
      } else {
        lcd.print("Preheating..."); 
      }
      break;
    case 2:
      if (printTime() < 90) {
        if (printTime() <= 10) {
          digitalWrite(PWR, LOW);
        } else if (printTime() <= 20) {
          digitalWrite(PWR, HIGH);
        } else if (printTime() <= 30) {
          digitalWrite(PWR, LOW);
        } else if (printTime() <= 40) {
          digitalWrite(PWR, HIGH);
        } else if (printTime() <= 50) {
           digitalWrite(PWR, LOW); 
        } else if (printTime() <= 65) {
          digitalWrite(PWR, HIGH);
        } else if (printTime() <= 75) {
          digitalWrite(PWR, LOW);
        } else {
          digitalWrite(PWR, HIGH);
        }
        state = 2;       
      } else {        
        if (checkTemperature() >= 183) {
          resetTime();
          state = 3;
        }
        
        digitalWrite(PWR, HIGH);
      } 
      lcd.print("Soaking...");
      break;
    case 3:
      if (printTime() >= 60) {
        digitalWrite(PWR, LOW);
        state = 4;
      }
      lcd.print("Reflow...");
      break;
    case 4:
      if (printTime() <= 70) {
        lcd.print("Reflow...");
        state = 4;
      } else {
        resetTime();
        lcd.print("Turn Off Button!");
      }
      break; 
    default:
      state = 0;
      break;
  }
}

void initLCD() {
  lcd.begin(16, 2);
  lcd.print("  Press Button");
  lcd.setCursor(4, 1);
  lcd.print("To Start!");
}

void printLowerLCD(){
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(checkTemperature());
}

/**** Serial Data Print for Development ****/
void printSerial() {
  Serial.print(printTime());
  Serial.print(',');
  Serial.println(checkTemperature());
  
}

/******* Max 31855 Subroutines *******/
double checkTemperature() {
  double c = thermocouple.readCelsius();
  if (isnan(c)) {
    return prevTemp;
  } else {
    prevTemp = c;
    return prevTemp;
  }
}

/************* RTC Subroutines ****************/

void resetTime(){
  byte second = 0;
  byte minute = 0;

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //stop Oscillator

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));

  Wire.write(zero); //start 

  Wire.endTransmission();
}

byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

int printTime() {
  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  int second = bcdToDec(Wire.read());
  int minute = bcdToDec(Wire.read());

  return ((minute * 60) + second);
}
