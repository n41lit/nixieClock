// Librarys to include
// Wire.h for I2C
#include <Wire.h>
// for Timekeeping
#include "RTClib.h"
//for Temp, Hum
#include <Adafruit_BME280.h>

// Define DS1307 and BM280 I2C
RTC_DS1307 rtc;
Adafruit_BME280 bme;

//Pin connected to SH_CP of 74HC595 - pin 11 on ic
int ClockPin = 6;
//Pin connected to ST_CP of 74HC595 - pin 12 on ic
int LatchPin = 5;
//Pin connected to DS of 74HC595 - pin 14 on ic
int DataPin = 1;
//Pin connected to LED
int ledRPin = 5;
int ledGPin = 6;
int ledBPin = 7;
//Pin connected to Switch 1
int switchOne = 11;
//Pin connected to Switch 2
int switchTwo = 12;
//Pin connected to Buzzer
int buzzerPin = 13;


int PlusPin = A1;
int MinusPin = A2;

//Define Pins and Registers
#define REGISTERS 4
#define PINS REGISTERS * 8

//pointer
byte * registerPattern;

//define Alarmvar
long AlarmTime = 0;

//define "Page" in which clock is at this time - standard in clock
int page = 0;


void setup() {

  //Serial Output
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Booting...");
  //set pins to output so you can control the shift register
  pinMode(DataPin, OUTPUT);
  pinMode(ClockPin, OUTPUT);
  pinMode(LatchPin, OUTPUT);
  pinMode(switchOne, INPUT);
  pinMode(switchTwo, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledRPin, OUTPUT);
  pinMode(ledRPin, OUTPUT);
  pinMode(ledRPin, OUTPUT);

  //Create Array of bites for all registers
  registerPattern = new byte[REGISTERS];

  clearRegisters();

  Serial.println("Checking Nixies...");
  checkDigits();

  Serial.println("Getting Time...");
  if (! rtc.begin()) {
    Serial.println("RTC not found!");
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is not running!");
    rtc.adjust(DateTime(__DATE__, __TIME__));
  }
  if (! bme.begin()) {
    Serial.println("Temperature and HumiditySenory faulty!");
  }
  Serial.println("Booted successfully...");

}

//Give out a code for the user to see the failiure and print to serial
void handleFail(long FaliureCode) {

  Serial.println(FaliureCode);
  notifyUser(1);

  FaliureCode = FaliureCode * 10 + 9;
  sendToClock(FaliureCode);
}

void clearRegisters() {
  for (size_t i = 0; i < REGISTERS; i++) {
    registerPattern[i] = 0;
  }
}

//figure out which pin on which register should be rewritten (state, bool)
void writeStateToPin(int pin, bool state) {

  //figure out shift reg nr (round down)
  int shiftregnr = pin / 8;

  //figure out pin on shift reg (remainder of div)
  int pinnr = pin % 8;

  //rewrite pattern
  for (size_t i = 0; i < REGISTERS; i++) {
    byte * pattern = &registerPattern[i];

    if (i == shiftregnr) {
      bitWrite(*pattern, pinnr, state);
    }
  }

}

//Write Pattern to Registers
void writeToRegisters() {
  digitalWrite(LatchPin, LOW);

  //Go through stored patterns and write them in order
  for (size_t i = 0; i < REGISTERS; i++) {

    byte * pattern = &registerPattern[i];

    shiftOut(DataPin, ClockPin, MSBFIRST, *pattern);
  }
  digitalWrite(LatchPin, HIGH);
}

void checkDigits() {
  int i = 0;
  for (i = 0; i < REGISTERS; i++) {
    clearRegisters();
    int x = 0;
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      registerPattern[i] = y;
      writeToRegisters();
      delay(100);
    }
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      y = y << 4;
      registerPattern[i] = y;
      writeToRegisters();
      delay(100);
    }
    delay(1000);
  }
  clearRegisters();
}

//convert a seven digit number to bytecode, 6 Digits, 1 X
void sendToClock(long number) {

  byte sendBytes[] = {0};
  int firstDigits = number / 100000;
  int secondDigits = number / 1000 % 100;
  int thirdDigits = number / 10 % 100;
  int lastDigit = number % 10;

  if (number < 1000000)
  {
    handleFail(900001);
  }

  else if (number < 10000000)
  {

    int nr = firstDigits / 10;
    firstDigits = nr * 6 + firstDigits;

    nr = secondDigits / 10;
    secondDigits = nr * 6 + secondDigits;

    nr = thirdDigits / 10;
    thirdDigits = nr * 6 + thirdDigits;

    byte bLastDigit = byte(lastDigit) << 4;



    sendBytes[3] = byte(firstDigits);
    sendBytes[2] = byte(secondDigits);
    sendBytes[1] = byte(thirdDigits);
    sendBytes[0] = bLastDigit;

    for (int i = 0; i < 4; i++) {
      registerPattern[i] = sendBytes[i];
    }
    writeToRegisters();


  }

  else {
    handleFail(900002);
  }
}
//notify user

void notifyUser(int type) {
  //types 0: info (short) 1: warn (long) 2: error (until solved) 3: alarm (until reset/button)
  if (type = 0) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
  }
  else if (type = 1) {
    digitalWrite(buzzerPin, HIGH);
    delay(5000);
    digitalWrite(buzzerPin, LOW);
  }
  else if (type = 2)
  {
    /* code */
  }
  else if (type = 3)
  {
    for (size_t i = 0; i < 10000; i++)
    {
      digitalWrite(buzzerPin, HIGH);
      delay(1000);
      digitalWrite(buzzerPin, LOW);
      delay(1000);
    }
  }
}

DateTime now = rtc.now();

void printTime() {
  int Hour = now.hour();
  int Min = now.minute();
  int Sec = now.second();
  long Time = 0;
  Time = (Hour * 100000) + (Min * 1000) + (Sec * 10);
  sendToClock(Time);
}

void timer() {

}

void alarm() {

}

void temper() {
  int temperature = int(bme.readTemperature());
  sendToClock(temperature * 10 + 2); //Number for C
}

void date() {
  int Day = now.day();
  int Mon = now.month();
  int Yr = now.year();
  long Date = 0;
  Date = (Day * 100000) + (Mon * 1000) + ((Yr % 100) * 10) + 1; //1 muss angepasst werden -> Signalleuchte
  sendToClock(Date);
}

void settings() {
  
}

void adjustTime() {
  
  }

//MAIN LOOP
void loop() {
  bool buttonOneVal = digitalRead(switchOne);
  
  if (buttonTwoVal == HIGH) {
    page += 1;
    if (page > 5){
      page = 0;  
    }
  }

  if (page = 0) {
    printTime();
    if (digitalRead(SwitchTwo)== HIGH){
    adjustTime();  
    }
  }
  if (page = 1) {
    timer();
  }
  if (page = 2) {
    alarm();
  }
  if (page = 3) {
    temper();
  }
  if (page = 4) {
    date();
  }
  if (page = 5) {
    sendToClock(1010101);
    if (digitalRead(SwitchTwo) == HIGH){
      settings();
    }
  }


}

// 0000 0000 0000 0000 0000 0000 0000
// hhhh hhhh mmmm mmmm ssss ssss xxxx
//
// 1 0000 0001
// 2 0000 0010
// 3 0000 0011
// 4 0000 0100
// 5 0000 0101
// 6 0000 0110
// 7 0000 0111
// 8 0000 1000
// 9 0000 1001

// 1-9 = 1-9
// 16-25 =  10-19
// 32-41= 20 - 29
// 48-57 = 30 - 39
// 64-73= 40 - 49
// 80-89= 50 - 59
// 96-105 = 60 - 69
// 112-121= 70 - 79
// 128-137= 80 - 89
// 144-153= 90 - 99
