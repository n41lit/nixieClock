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
long alarmTime;
long timerTime;
bool settingK; //setting for Kelvin (true)
int settingMode;
int settingDistance; //setting for distance

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
  settingK = false;
  settingMode = 0;
  settingDistance = 50;

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
    tone(buzzerPin, 523, 500);
  }
  else if (type = 1) {
    tone(buzzerPin, 523, 2000);
  }
  else if (type = 2)
  {
    /* code */
  }
  else if (type = 3)
  {
    bool t;
    while (t = true)
    {
      tone(buzzerPin, 523, 1000);
      delay(2000);
      if (digitalRead(switchOne) == HIGH | digitalRead(switchTwo) == HIGH)
      {
        t = true;
      }
    }
  }
}

int userInputTime(int userTime[]) {
  userTime[2];
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

  return userTime[0]; //DELETE AAA
}
unsigned int userInputTimeFormatted() {
  int arr[2];
  long timeResult = 0;
  int c = 0;
  while (c < 2)
  {
    if (digitalRead(switchOne) == HIGH)
    {
      arr[c] += 1;
      if (arr[c] == 60)
      {
        arr[c] = 0;
      }
    }

    if (digitalRead(switchTwo) == HIGH)
    {
      c += 1;
    }

    //print the userTime
    delay (70);
    timeResult = (arr[0] * 100000) + (arr[1] * 1000) + (arr[2] * 10);
    sendToClock(timeResult);
  }
    return timeResult;
}

//function to Display Time
void displayTime()
{
  DateTime now = rtc.now();
  int hourNow = now.hour();
  int minuteNow = now.minute();
  int secondNow = now.second();
  long timeNow = 0;
  timeNow = (hourNow * 100000) + (minuteNow * 1000) + (secondNow * 10);
  sendToClock(timeNow);
}

void displayDate()
{
  DateTime now = rtc.now();
  long DateNow = 0;
  DateNow = (now.day() * 100000) + (now.month() * 1000) + (now.year() * 10);
  //Serial.println(DateNow);
  sendToClock(DateNow);
}

void displayTemperature(bool format) //true for Kelvin
{
  int a = bme.readTemperature();
  if (format == 0)
  {
    sendToClock((a * 10) + 8);
  }
  if (format == 1)
  {
    sendToClock(((a + 273) * 10) + 5); //convert to Kelvin
  }
}

void settingsLoop() {
  int l = true;
  int settingsPage = 0;

  while (l)
  {
    if (digitalRead(switchTwo) == HIGH)
    {
      settingsPage += 1;
    }
    if (settingsPage > 5) {
      settingsPage = 0;
    }

    //Setting 0: Adjust Time/Date
    if (settingsPage == 0)
    {
      sendToClock(0000006);
      if (digitalRead(switchOne) == HIGH)
      {
        int b[2];
        int c[2];
        b[0] = userInputTime(b[0]);
        c[0] = userInputTime(c[0]);

        rtc.adjust(DateTime(c[2], c[1], c[0], b[0], b[1], b[2]));
      }
    }
    //Setting 1: Auto/Hand/1
    if (settingsPage == 1)
    {
      int v = (settingMode * 10 + 1000006);
      sendBlankToClock();

      if (digitalRead(switchOne) == HIGH)
      {
        settingMode += 1;
        if (settingMode > 3)
        {
          settingMode = 0;
        }

      }
    }
    //Setting 2: Temperature Mode
    if (settingsPage == 2)
    {
      if (settingK == true)
      {
        sendToClock(2000016);
      }
      if (settingK)
      {
        sendToClock(2000006);
      }

      if (digitalRead(switchOne) == HIGH)
      {
        settingK = !settingK;
      }

      sendToClock(2000006);

    }
    if (settingsPage == 3)
    {
      int distances[] = {5, 10, 50, 100, 200};
         sendToClock((settingDistance * 10) + 2000006);
         if (digitalRead(switchOne) == HIGH)
      {
        int v = 0;
        settingDistance = distances[v];
        v += 1;
      }

    }


    //Last "page" exits to main loop
    if (settingsPage == 4)
    {
      l = false;
    }

    delay(100);

  }
}

void setAlarm() {
  alarmTime = userInputTimeFormatted();
}

void checkAlarms()
{
  DateTime now = rtc.now();
  int hourNow = now.hour();
  int minuteNow = now.minute();
  int secondNow = now.second();
  long timeNow = 0;
  timeNow = (hourNow * 100000) + (minuteNow * 1000) + (secondNow * 10);

  if (alarmTime == timeNow)
  {
    notifyUser(3);
  }
  if (timerTime == timeNow)
  {
    notifyUser(1);
  }
}

//MAIN LOOP
void loop() {

  checkAlarms();

  //detect input from switchTwo and set to next page
  if (digitalRead(switchTwo) == HIGH) {
    nowPage += 1;
    if (nowPage > 8) {
      nowPage = 0;
    }
  }

  //check alarm and timer

  //page to function
  if (nowPage == 0)
  {
    if (settingMode = 0)
    {
      long duration;
      int distance;

      digitalWrite(UsTrgPin, LOW);
      delayMicroseconds(2);
      digitalWrite(UsTrgPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(UsTrgPin, LOW);
      duration = pulseIn(UsEchPin, HIGH);
      distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
      if (distance < settingDistance)
      {
        displayTime();
      }

    }
    if (settingMode == 1)
    {
      if (digitalRead(switchOne) == HIGH)
      {
        displayTime();
      }

    }
    if (settingMode == 2)
    {
      displayTime();
    }

  }
  if (nowPage == 1)
  {
    sendToClock(0000001);
  }
  if (nowPage == 2)
  {
    displayTemperature(settingK); //display Temp in C
  }
  if (nowPage == 3)
  {
    sendToClock((bme.readPressure() * 10) + 3);
  }
  if (nowPage == 4)
  {
    sendToClock((bme.readAltitude(1013) * 10) + 4);
  }
  if (nowPage == 5)
  {
    sendToClock((bme.readHumidity() * 10) + 1);
  }
  if (nowPage == 6)
  {
    displayDate();
  }
  if (nowPage == 7)
  {
    sendToClock(alarmTime);

    if (digitalRead(switchOne) == HIGH)
    {
      setAlarm();
    }
  }
  if (nowPage == 8)
  {
    sendToClock(1010106);
    if (digitalRead(switchOne) == HIGH)
    {
      settingsLoop();
    }
  }

  delay(200);
}
