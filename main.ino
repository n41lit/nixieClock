//Define Pins
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

//define global var
int nowPage;
long alarmTime;
long timerTime;
bool settingK; //setting for Kelvin (true)
int settingMode;
long settingDistance; //setting for distance

//pointer for "internal" registers
byte * registerPattern;

//setup code
void setup() 
  {
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

  //set var for startup
  nowPage = 0;
  settingK = false;
  settingMode = 0;
  settingDistance = 50;
  alarmTime = 999999;

  //blink green for setup finish
  digitalWrite(ledGPin, HIGH);
  delay(200);
  digitalWrite(ledGPin, LOW);
  delay(100);
  digitalWrite(ledGPin, HIGH);
  delay(200);
  digitalWrite(ledGPin, LOW);
}

//Give out a code for the user to see the error code and print to serial
void handleFail(long FailureCode)
  {
    Serial.println(FailureCode);
    notifyUser(1);
    long x = FailureCode *10 + 7;
    sendToClock(x);
  }

//clear internal buffer
void clearRegisters()
  {
    //overwrite
    for (size_t i = 0; i < REGISTERS; i++) {
    registerPattern[i] = 0b00000000;
  }
}

//figure out which pin on which register should be rewritten (state, bool)
void writeStateToPin(int pin, bool state)
{
  //figure out shift reg nr (round down)
  int shiftregnr = pin / 8;

  //figure out pin on shift reg (modulo)
  int pinnr = pin % 8;

  //rewrite pattern
  for (size_t i = 0; i < REGISTERS; i++)
  {
    byte * pattern = &registerPattern[i];

    if (i == shiftregnr)
    {
      bitWrite(*pattern, pinnr, state);
    }
  }
}

//Write Pattern to Registers
void writeToShiftRegisters()
{
  digitalWrite(LatchPin, LOW);

  //Go through stored patterns and write them in order
  for (size_t i = 0; i < REGISTERS; i++)
  {
    if (i < 4) 
    {
      byte a = registerPattern[i];
      byte b = a << 4;
      byte c = a >> 4;
      byte switchedPattern = b | c;
      registerPattern[i] = switchedPattern;
    }

    //pointer and push to the shift registers of the clock
    byte * pattern = &registerPattern[i];
    shiftOut(DataPin, ClockPin, MSBFIRST, *pattern);
  }

  digitalWrite(LatchPin, HIGH);
}

//run through each digit as protection against cathode poisoning
void checkDigits()
{
  int i = 0;
  //for loop with registers
  for (i = 0; i < REGISTERS; i++)
  {
    clearRegisters();

    //count up from small to high int
    int x = 0;
    for (x = 0; x < 10; x++)
    {
      byte y = byte(x);
      registerPattern[i] = y;
      writeToShiftRegisters();
      delay(50);
    }

    for (x = 0; x < 10; x++)
    {
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
void sendToClock(long number)
{
  //make byte array filled with 0
  byte sendBytes[] = {0b00000000, 0b00000000, 0b00000000, 0b00000000};

  //put the number in var
  int firstDigits = number / 100000;
  int secondDigits = number / 1000 % 100;
  int thirdDigits = number / 10 % 100;
  int lastDigit = number % 10;

  //convert byte number into 2*4bit numbers
  int nr = firstDigits / 10;
  firstDigits = nr * 6 + firstDigits;
  nr = 0;
  nr = secondDigits / 10;
  secondDigits = nr * 6 + secondDigits;
  nr = 0;
  nr = thirdDigits / 10;
  thirdDigits = nr * 6 + thirdDigits;

  //shift the last digits (for transistors)
  byte bLastDigit = byte(lastDigit) << 4;
  bLastDigit = bLastDigit | 0b00001111; //add 1111 to activate transistors

  //fill array
  sendBytes[3] = byte(firstDigits);
  sendBytes[2] = byte(secondDigits);
  sendBytes[1] = byte(thirdDigits);
  sendBytes[0] = bLastDigit;

  //push array
  for (int i = 0; i < 4; i++)
  {
    registerPattern[i] = sendBytes[i];
  }

  //send to registers
  writeToShiftRegisters();
}

//blank the clock
void sendBlankToClock() {
  //just overwrite with ones
  for (int i = 0; i < 4; i++)
  {
    registerPattern[i] = 0b11111111;
  }
  writeToShiftRegisters();
}

//get the attention from user
void notifyUser(int type)
{
  //types 0: info (short) 1: warn (long) 2: error(to be implemented) 3: alarm (until reset/button)
  if (type = 0)
  {
    tone(buzzerPin, 523, 500);
  }
  else if (type = 1)
  {
    tone(buzzerPin, 523, 2000);
  }
  else if (type = 2)
  {
    //to be implemented
  }
  else if (type = 3)
  {
    bool t = false;
    while (t = true)
    {
      tone(buzzerPin, 523, 1000);
      digitalWrite(ledRPin, HIGH);
      delay(2000);
      digitalWrite(ledRPin, LOW);

      if (digitalRead(switchOne) == HIGH | digitalRead(switchTwo) == HIGH)
      {
        t = true;
      }
    }
  }
}

//get a time formatted for the output from the user
unsigned long userInputTimeFormatted()
{
  //init array and var
  long arr[] = {0,0,0};
  unsigned long timeResult = 0;
  int c = 0;
  long x = 0;
  long y = 0;
  long z = 0;

  //loop for 3 digits
  while (c < 3)
  {
    if (digitalRead(switchOne) == HIGH)
    {
      arr[c] += 1;
      if (arr[c] > 59 && c > 0)
      {
        arr[c] = 0;
      }

      if (arr[c] > 23 && c == 0)
      {
        arr[c] = 0;
      }
    }
    if (digitalRead(switchTwo) == HIGH)
    {
      c += 1;
    }

    //format and send
    delay (150);
    timeResult = 0;
    x = arr[0] * 100000;
    y = arr[1] * 1000;
    z = arr[2] * 10;
    timeResult = x + y + z;
    sendToClock(timeResult);
  }
  return timeResult;
}

//get a second interval from the user
unsigned long userInputTimeSeconds()
{
  //init var
  int c = 0;
  long x = 0;

  //loop through until switch two is pressed and increment x for each press
  while (c < 1)
  {
    if (digitalRead(switchOne) == HIGH)
    {
      x += 1;
    }
    if (digitalRead(switchTwo) == HIGH)
    {
      c += 1;
    }

    //send to clock
    delay (150);
    sendToClock(x*10);
  }

  return x;
}

//get a date formatted for the output from the user
unsigned long userInputDateFormatted()
{
  //init array and var
  long arr[] = {1,1,1};
  unsigned long dateResult = 0;
  int c = 0;
  long x = 0;
  long y = 0;
  long z = 0;

  // loop for days, month, and years with validation
  while (c < 3)
  {
    if (digitalRead(switchOne) == HIGH)
    {
      arr[c] += 1;
      if (arr[c] > 59 && c == 2)
      {
        arr[c] = 1;
      }
      if (arr[c] > 12 && c == 1)
      {
        arr[c] = 1;
      }      
      if (arr[c] > 31 && c == 0)
      {
        arr[c] = 1;
      }
    }
    if (digitalRead(switchTwo) == HIGH)
    {
      c += 1;
    }

    //display and format the resulting input
    delay (150);
    dateResult = 0;
    x = arr[0] * 100000;
    y = arr[1] * 1000;
    z = arr[2] * 10;
    dateResult = x + y + z;
    sendToClock(dateResult);
  }

  return dateResult;
}

//display the time to the user
void displayTime()
{
  //get time from rtc and put into DateTime obj
  DateTime now = rtc.now();
  long hourNow = now.hour();
  long minuteNow = now.minute();
  long secondNow = now.second();
  long timeNow = 0;

  //format and display
  timeNow = (hourNow * 100000) + (minuteNow * 1000) + (secondNow * 10);
  sendToClock(timeNow);
}

//display the date to the user
void displayDate()
{
  //get date from rtc and put in DateTime obj
  DateTime now = rtc.now();
  long DateNow = 0;

  //format and output
  DateNow = (now.day() * 100000) + (now.month() * 1000) + (now.year()%100 * 10);
  sendToClock(DateNow);
}

//display the temperature for the user
void displayTemperature(bool format) //true for Kelvin
{
  //read from bme
  int a = bme.readTemperature();

  //check which format the temp should be displayed, if K -> calculate
  if (format == 0)
  {
    sendToClock((a * 10) + 8);
  }

  if (format == 1)
  {
    sendToClock(((a + 273) * 10) + 5); //convert to Kelvin
  }
}

//display the Timer
void displayTimer()
{
  //get unixseconds and put in DateTime object
  DateTime now = rtc.now();
  long unixtimeNow = now.unixtime();
  long delta = timerTime - unixtimeNow;

  //dont push if delta time < 0
  if (delta < 0)
  {
    ;
  }
  else
  {
      sendToClock(delta*10);
  }
}

//let the user set the timer
void setTimer()
{
  //get the delta from user input
  long delta = userInputTimeSeconds();

  //get unixseconds from now, format and save in global var
  DateTime now = rtc.now();
  long unixtimeNow = now.unixtime();
  timerTime = unixtimeNow + delta;
}

//loop for settings
void settingsLoop()
{
  //init the var
  int l = true;
  long j = 0;
  int settingsPage = 0;

  //loop while l is true
  while (l)
  {
    //flip the pages from settings with switch two
    if (digitalRead(switchTwo) == HIGH)
    {
      settingsPage += 1;
    }

    //pull back to zero if too high
    if (settingsPage > 5)
    {
      settingsPage = 0;
    }

    //Setting 0: Adjust Time/Date
    if (settingsPage == 0)
    {
      //send code to clock
      sendToClock(0000006);
      delay(300);

      //change time/date if switchOne is pushed
      if (digitalRead(switchOne) == HIGH)
      {
        //get user Input and format
        long dt = userInputDateFormatted();
        long tm = userInputTimeFormatted();
        long dy = dt / 100000;
        long mo = dt % 100000 / 1000;
        long yr = dt % 1000 / 10;
        long hh = tm / 100000;
        long mm = tm % 100000 /1000;
        long ss = tm % 1000 / 10;
        DateTime adjusted(yr, mo, dy, hh, mm, ss);

        //Check if DtTm is valid
        if (adjusted.isValid() == true)
        {
          //then adjust
          rtc.adjust(adjusted);
        }
        else
        {
          //or send fail
          handleFail(000001);
        }
      }
    }

    //Setting 1: Auto/Hand/1
    if (settingsPage == 1)
    {
      //send setting to clock
      long v = (settingMode * 10 + 1000006);
      sendToClock(v);

      //push one up if switch one is pressed
      if (digitalRead(switchOne) == HIGH)
      {
        settingMode += 1;
        if (settingMode > 2)
        {
          settingMode = 0;
        }

      }
    }

    //Setting 2: set Distance
    if (settingsPage == 2)
    {
      //array init
      int distances[] = {10, 50, 100, 200};

      //if button is pressed change distance
      if (digitalRead(switchOne) == HIGH)
      {
        j += 1;
        settingDistance = distances[j];
        if (j > 3)
        {
          j=0;
        }
      }

      //format and send to clock
      sendToClock(settingDistance * 10 + 2000006);
    }

    //Setting 3: Kathode Poisoning
    if (settingsPage == 3)
    {
      //if confirmed check digits
      if (digitalRead(switchOne) == HIGH )      {
        checkDigits();
      }

      sendToClock(3000006);
    }

    //if settings pages are at end -> exit loop 
    if (settingsPage == 4)
    {
      l = false;
    }

    delay(300);
  }
}

void setAlarm() 
{
  //get user Input and save to var
  alarmTime = userInputTimeFormatted();
}

void checkAlarms()
{
  //get time now and format
  DateTime now = rtc.now();
  int hourNow = now.hour();
  int minuteNow = now.minute();
  int secondNow = now.second();
  long timeNow = 0;
  timeNow = (hourNow * 100000) + (minuteNow * 1000) + (secondNow * 10);

  //if times are matching send notifyuser
  if (alarmTime == timeNow)
  {
    notifyUser(3);
  }

  if (timerTime == now.unixtime())
  {
    notifyUser(1);
  }
}

//MAIN LOOP
void loop()
{

  //check the alarms
  checkAlarms();

  //detect input from switchTwo and set to next page
  if (digitalRead(switchTwo) == HIGH)
  {
    nowPage += 1;
    if (nowPage > 8)
    {
      nowPage = 0;
    }
  }

  //page to function
  if (nowPage == 0)
  {

    //detect which mode shoud be displayed
    if (settingMode == 0)
    {

      //get distance from sensory
      long duration;
      int distance;
      digitalWrite(UsTrgPin, LOW);
      delayMicroseconds(2);
      digitalWrite(UsTrgPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(UsTrgPin, LOW);
      duration = pulseIn(UsEchPin, HIGH);

      //calculate cm
      distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)

      //detect if distance is over or under limit and act accordingly
      if (distance < settingDistance)
      {
        displayTime();
      }

      else
      {
        sendBlankToClock();
      }
    }

    if (settingMode == 1)
    {
      //blank and only display time if button is pressed
      sendBlankToClock();
      if (digitalRead(switchOne) == HIGH)
      {
        displayTime();
      }
    }


    if (settingMode == 2)
    {
      //display time all times
      displayTime();
    }
  }

  // display date on page one
  if (nowPage == 1)
  {
    displayDate();
  }

  // display temperature 
  if (nowPage == 2)
  {
    // change setting if switch is pressed
    if (digitalRead(switchOne) == HIGH)
    {
      settingK = !settingK;
    }
    //display the temp with setting
    displayTemperature(settingK); //display Temp in C
  }

  // display pressure
  if (nowPage == 3)
  {
    // format to hPa and send to clock
    int a = bme.readPressure()/ 100.0F;
    a *= 10;
    a += 3;
    sendToClock(a);
  }

  // display Altitude
  if (nowPage == 4)
  {
    // read altitude and format for clock
    int a = bme.readAltitude(1013);
    a*= 10;
    a += 4;
    sendToClock(a);
  }

  //display Humidity
  if (nowPage == 5)
  {
    int a = bme.readHumidity();
    a *= 10;
    a += 1;
    sendToClock(a);
  }

  //Alarm Page
  if (nowPage == 6)
  {
    // show the alarm Time
    sendToClock(alarmTime);

    //let user set alarm time if button one is pressed
    if (digitalRead(switchOne) == HIGH)
    {
      delay(50);
      setAlarm();
    }
  }

  //Timer page
  if (nowPage == 7)
  {
    //show the timer
    displayTimer();

    //let user set timer if button one is pressed
    if (digitalRead(switchOne) == HIGH)
    {
      delay(50);
      setTimer();
    }
  }

  //settings page
  if (nowPage == 8)
  {
    //display settings page on clock
    sendToClock(1010106);

    //let user go into settings if button one is pressed
    if (digitalRead(switchOne) == HIGH)
    {
      settingsLoop();
    }
  }

  //master delay for main loop
  delay(300);
  }