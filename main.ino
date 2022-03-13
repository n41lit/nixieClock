//Pin connected to SH_CP of 74HC595(SR) - pin 11
int ClockPin = 6;
//Pin connected to ST_CP of SR pin 12
int LatchPin = 5;
//Pin connected to DS of SR pin 14
int DataPin = 2;
//Pin connected to -OE of SR pin 13
int OePin = 3;
//Pin connected to -MR of SR pin 10
int MrPin = 4;
//Pin connected to LED
int ledRPin = 10;
int ledGPin = 9;
int ledBPin = 8;
//Pin connected to Switch 1
int switchOne = 15;
//Pin connected to Switch 2
int switchTwo = 14;
//Pin connected to Buzzer
int buzzerPin = 7;
//Pin to for US-Sensory
int UsTrgPin = 11;
int UsEchPin = 12;

#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_BME280.h>
RTC_DS1307 rtc;
Adafruit_BME280 bme;

int PlusPin = A1;
int MinusPin = A2;

//Define Pins and Registers
#define REGISTERS 4
#define PINS REGISTERS * 8

//pointer
byte * registerPattern;


void setup() {

    //Serial Output
    Serial.begin(9600);

    while (!Serial) {
        ; //idle
    }

    Serial.println("Starting Test Sequence:");

    Serial.println("Test-Block #0 - Setup");

    Serial.println("Test-Block #0.1 - Setting Up...");
    registerPattern = new byte[REGISTERS];

    Serial.println("Test-Block #0.2 - Enabling Pins...");

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

    digitalWrite(MrPin, HIGH);
    digitalWrite(OePin, LOW);

    Serial.println("Pins enabled...");

    Serial.println("Test-Block #0 - Setup done");

    Serial.println("Test-Block #1 - External Sensory");

    Serial.println("Test-Block #1.1 - Testing LED...");
    Serial.println("Red...");
    digitalWrite(ledRPin, HIGH);
    delay(1000);
    digitalWrite(ledRPin, LOW);
    delay(1000);
    Serial.println("Green...");
    digitalWrite(ledGPin, HIGH);
    delay(1000);
    digitalWrite(ledGPin, LOW);
    delay(1000);
    Serial.println("Blue...");
    digitalWrite(ledBPin, HIGH);
    delay(1000);
    digitalWrite(ledBPin, LOW);
    delay(500);

    Serial.println("Test-Block #1.2 - Testing Ultrasonic Sensor...");
    long duration;
    int distance;

    Serial.println("Taking Distance in 1 Second...");
    digitalWrite(UsTrgPin, LOW);
    delayMicroseconds(2);
    digitalWrite(UsTrgPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(UsTrgPin, LOW);
    duration = pulseIn(UsEchPin, HIGH);
    distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    Serial.println("Test-Block #1.3 - Testing Switches...");
    int buttonTime = 2000;

    Serial.println("Press Button 1...");
    while (digitalRead(switchOne) != LOW)
    {
        ;
    }
    Serial.println("Button 1 pressed...");

    Serial.println("Press Button 2...");
    while (digitalRead(switchTwo) != LOW)
    {
        ;
    }
    Serial.println("Button 2 pressed...");
    
    Serial.println("Press both buttons for 2 sec...");
    while (pulseIn(switchOne, HIGH) <= buttonTime && pulseIn(switchTwo, HIGH) <= buttonTime)
    {
        ;
    }
    Serial.println("Both buttons pressed...");

    Serial.println("Test-Block #1.4 - Testing Buzzer...");

    digitalWrite(buzzerPin, HIGH);
    delay(1000);
    digitalWrite(buzzerPin, LOW);

    Serial.println("Test-Block #1 - External Sensory done");

    Serial.println("Test-Block #2 - Internal Sensory");

    Serial.println("Test-Block #2.1 - BME280 Test");
    if (!bme.begin()) {
        Serial.println("BME280 not found");
        while (1);
    }
    Serial.print("Pressure:");
    Serial.print(bme.readPressure());
    Serial.print("Temperature:");
    Serial.print(bme.readTemperature());    
    Serial.print("Humidity:");
    Serial.print(bme.readHumidity());
    Serial.print("Altitude(calc):");
    Serial.print(bme.readAltitude(1013));

    Serial.println("Test-Block #2.2 - DS1307 Test"); 
    Wire.begin();
    rtc.begin();
    if (! rtc.isrunning()){
        Serial.println("RTC Time not running...");
        Serial.println("Setting to compiler time...");
        rtc.adjust(DateTime(__DATE__,__TIME__));
    }
    DateTime now = rtc.now();
    Serial.println("Time from RTC:");
    Serial.print(now.day(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.year(), DEC);
    Serial.print("");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.print(now.minute(), DEC);
    Serial.print(":");
    Serial.print(now.second(), DEC); 
    Serial.println();

    Serial.println("Test-Block #2 - Internal Sensory done");

    Serial.println("Test-Block #3 - Nixie-Tubes");

    Serial.println("Check Digits...");

    checkDigits();

    Serial.println("Hope all digits were in order :-)");

    Serial.println("Test-Block #3 - Nixie-Tubes done");
}

void clearRegisters(){
    for (size_t i=0; i < REGISTERS; i++){
        registerPattern[i] = 0;
    }
}

void writeToShiftRegisters() {
  digitalWrite(LatchPin, LOW);

  //Go through stored patterns and write them in order
  for (size_t i = 0; i < REGISTERS; i++) {

    byte * pattern = &registerPattern[i];

    shiftOut(DataPin, ClockPin, MSBFIRST, *pattern);
  }
  digitalWrite(LatchPin, HIGH);
}

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

void checkDigits() {
  int i = 0;
  for (i = 0; i < REGISTERS; i++) {
    clearRegisters();
    int x = 0;
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      registerPattern[i] = y;
      writeToShiftRegisters();
      delay(100);
    }
    for (x = 0; x < 10; x++) {
      byte y = byte(x);
      y = y << 4;
      registerPattern[i] = y;
      writeToShiftRegisters();
      delay(100);
    }
    delay(1000);
  }
  clearRegisters();
}

//MAIN LOOP
void loop() {
    Serial.println("completed. Restart or unplug...");
    delay(10000);
}