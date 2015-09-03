#include <SPI.h>
#include "Adafruit_MAX31855.h"
#include "Wire.h"
#include <LiquidCrystal.h>

/*** Definitions for Max 31855 ***/
#define DO   2
#define CS   3
#define CLK  4

Adafruit_MAX31855 thermocouple(CLK, CS, DO);

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
/ 3 - Reflow (180 - 210+ - 180)
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
    
    if (checkTemperature() >= 80) {
      lcd.begin(16, 2);
      lcd.print("Cooling down... ");
      printLowerLCD(); 
    } else {
      initLCD();
      state = 0;
    }
  }
  
  delay(800);
}

/**** State Handler Subroutine ****/
void stateHandler() {
  lcd.clear();
  switch(state) {
    case 0:
      resetTime();
      digitalWrite(PWR, HIGH);
      state = 1;
      break;
    case 1:
      if (checkTemperature() >= 150) {
        resetTime();
        state = 2;
      } else {
        lcd.print("Preheating..."); 
      }
      break;
    case 2:
      if (checkTemperature() >= 180) {
        resetTime();
        state = 3;
      } else {
        lcd.print("Soaking...");
      }
      break;
    case 3:
      if (checkTemperature() >= 200) {
        digitalWrite(PWR, LOW);
        state = 4;
      } else {
        lcd.print("Reflow...");
      }
      break;
    case 4:
      if (checkTemperature() >= 180) {
        lcd.print("Reflow...");
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
    return 0.0;
  } else {
    return c;
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
