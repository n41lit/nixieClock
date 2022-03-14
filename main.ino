//Define Input-Pins

int ClockPin = 6;
int LatchPin = 5;
int DataPin = 2;
int OePin = 3;
int MrPin = 4;
int ledRPin = 10;
int ledGPin = 9;
int ledBPin = 8;
int switchOne = 15;
int switchTwo = 14;
int buzzerPin = 7;
int UsTrgPin = 11;
int UsEchPin = 12;

//Define/Include Libraries
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_BME280.h>

//Define RTC and BME
RTC_DS1307 rtc;
Adafruit_BME280 bme;

//Define Pins and Registers
#define REGISTERS 4
#define PINS REGISTERS * 8

int nowPage;
int alarmTime;

//pointer
byte * registerPattern;

void setup() {
  //Serial Output
  Serial.begin(9600);

  Serial.println("Setup...");
  //set pins to output so you can control the shift register
  pinMode(DataPin, OUTPUT);
  pinMode(ClockPin, OUTPUT);
  pinMode(LatchPin, OUTPUT);
  pinMode(OePin, OUTPUT);
  pinMode(MrPin, OUTPUT);
  pinMode(ledRPin, OUTPUT);
  pinMode(ledGPin, OUTPUT);
  pinMode(ledBPin, OUTPUT);
  pinMode(switchOne, INPUT);
  pinMode(switchTwo, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(UsTrgPin, OUTPUT);
  pinMode(UsEchPin, INPUT);
  //set SHR pins
  digitalWrite(MrPin, HIGH);
  digitalWrite(OePin, LOW);

  //Create Array of bites for all registers
  registerPattern = new byte[REGISTERS];

  //clear Clock
  sendBlankToClock();
  clearRegisters();

  //start clock
  if (! bme.begin())
  {
    handleFail(300001);
    delay(1000);
  }
  //start clock
  Wire.begin();
  rtc.begin();
  if (! rtc.isrunning())
  {
    handleFail(300002);
    delay(1000);
  }
  //Check digits for cathode poisoning
  checkDigits();

  nowPage = 0;

}

//Give out a code for the user to see the failiure and print to serial
void handleFail(long FaliureCode) {

  Serial.println(FaliureCode);
  notifyUser(1);

  FaliureCode = FaliureCode * 10 + 7;
  sendToClock(FaliureCode);
}

void clearRegisters() {
  for (size_t i = 0; i < REGISTERS; i++) {
    registerPattern[i] = 0b00000000;
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
void writeToShiftRegisters() {
  digitalWrite(LatchPin, LOW);

  //Go through stored patterns and write them in order
  for (size_t i = 0; i < REGISTERS; i++) {

    if (i < 4) {
      byte a = registerPattern[i];
      byte b = a << 4;
      byte c = a >> 4;
      byte switchedPattern = b | c;
      registerPattern[i] = switchedPattern;
    }
    byte * pattern = &registerPattern[i];

    shiftOut(DataPin, ClockPin, MSBFIRST, *pattern);
  }
  digitalWrite(LatchPin, HIGH);
}

//run through each digit as protection against cathode poisoning
void checkDigits() {
  int i = 0;
  for (i = 0; i < REGISTERS; i++) {
    clearRegisters();
    int x = 0;
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      registerPattern[i] = y;
      writeToShiftRegisters();
      delay(50);
    }
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      y = y << 4;
      registerPattern[i] = y;
      writeToShiftRegisters();
      delay(50);
    }
  }
  clearRegisters();
}

//convert a seven digit number to bytecode, 6 Digits, 1 X
void sendToClock(long number) {

  byte sendBytes[] = {0b00000000, 0b00000000, 0b00000000, 0b00000000};
  int firstDigits = number / 100000;
  int secondDigits = number / 1000 % 100;
  int thirdDigits = number / 10 % 100;
  int lastDigit = number % 10;

  int nr = firstDigits / 10;
  firstDigits = nr * 6 + firstDigits;

  nr = 0;

  nr = secondDigits / 10;
  secondDigits = nr * 6 + secondDigits;

  nr = 0;

  nr = thirdDigits / 10;
  thirdDigits = nr * 6 + thirdDigits;

  byte bLastDigit = byte(lastDigit) << 4;
  bLastDigit = bLastDigit | 0b00001111; //add 1111 to activate transistors

  sendBytes[3] = byte(firstDigits);
  sendBytes[2] = byte(secondDigits);
  sendBytes[1] = byte(thirdDigits);
  sendBytes[0] = bLastDigit;

  for (int i = 0; i < 4; i++) {
    registerPattern[i] = sendBytes[i];
  }
  writeToShiftRegisters();


}

void sendBlankToClock() {
  //blank the clock
  byte sendBytes[] {0b11111111, 0b11111111, 0b11111111, 0b11111111};

  for (int i = 0; i < 4; i++) {
    registerPattern[i] = sendBytes[i];
  }

  writeToShiftRegisters();
}




void notifyUser(int type) {
  //types 0: info (short) 1: warn (long) 2: error (until solved) 3: alarm (until reset/button)
  if (type = 0) {
    digitalWrite(buzzerPin, HIGH);
    delay(500);
    digitalWrite(buzzerPin, LOW);
  }
  else if (type = 1) {
    digitalWrite(buzzerPin, HIGH);
    delay(2000);
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

unsigned long userInputTime() {
  int userTime[2];
  long timeResult = 0;
  int c = 0; 
  while (c < 2)
  {
    if (digitalRead(switchOne) == HIGH)
    {
      userTime[c] += 1;
      if (userTime[c] == 60)
      {
        userTime[c] = 0;
      }
    }

    if (digitalRead(switchTwo) == HIGH)
    {
      c += 1;
    }

    //print the userTime
    delay (70);
    timeResult = (userTime[0] * 100000) + (userTime[1] * 1000) + (userTime[2] * 10);
    sendToClock(timeResult);
  }

  return timeResult;

}



//function to Display Time
void displayTime() {
  DateTime now = rtc.now();
  int hourNow = now.hour();
  int minuteNow = now.minute();
  int secondNow = now.second();
  long timeNow = 0;
  timeNow = (hourNow * 100000) + (minuteNow * 1000) + (secondNow * 10);
  //Serial.println(timeNow);
  sendToClock(timeNow);
}

void timer () {
  int a;
  a = userInputTime();
  sendToClock(a);
}

void settingsLoop() {
}

void setAlarm() {

}

void displayAlarm() {

}

//MAIN LOOP
void loop() {

  //detect input from switchTwo and set to next page
  if (digitalRead(switchTwo) == HIGH) {
    nowPage += 1;
    if (nowPage > 5) {
      nowPage = 0;
    }
  }

  //page to function
  if (nowPage == 0)
  {
    displayTime();
  }
  if (nowPage == 1)
  {
    timer();
    sendToClock(0000001);
  }
  if (nowPage == 2)
  {
    sendToClock(2222220);
  }
  if (nowPage == 3)
  {
    sendToClock(3333330);
  }
  if (nowPage == 4)
  {
    sendToClock(4444440);
  }
  if (nowPage == 5)
  {
    sendToClock(5555550);
  }

  delay(200);
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