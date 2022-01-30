// First we include the libraries
//#include <Wire.h> activate for Arduino

//Pin connected to SH_CP of 74HC595 - pin 11 on ic
int ClockPin = 2;
//Pin connected to ST_CP of 74HC595 - pin 12 on ic
int LatchPin = 3;
//Pin connected to DS of 74HC595 - pin 14 on ic
int DataPin = 4;

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

    //set pins to output so you can control the shift register
    pinMode(DataPin, OUTPUT);
    pinMode(ClockPin, OUTPUT);
    pinMode(LatchPin, OUTPUT);


    //Create Array of bites for all registers
    registerPattern = new byte[REGISTERS];

    clearRegisters();

    checkDigits();

}

//Give out a code for the user to see the failiure and print to serial
void handleFail(long FaliureCode){

    Serial.println(FaliureCode);

    FaliureCode= FaliureCode*10 + 9;
    convertToByte(FaliureCode);
}

void clearRegisters(){
    for (size_t i=0; i < REGISTERS; i++){
        registerPattern[i] = 0;
    }
}

//figure out which pin on which register should be rewritten (state, bool)
void writeStateToPin(int pin, bool state){
  
	//figure out shift reg nr (round down)
  	int shiftregnr = pin / 8;
  	
  	//figure out pin on shift reg (remainder of div)
  	int pinnr = pin % 8;
  
  	//rewrite pattern
  	for(size_t i = 0; i < REGISTERS; i++){
    	byte * pattern = &registerPattern[i];
      	
      	if (i == shiftregnr){
          	bitWrite(*pattern, pinnr, state);
        }
 	}

}

//Write Pattern to Registers
void writeToRegisters(){
	digitalWrite(LatchPin, LOW);
  	
  	//Go through stored patterns and write them in order
  	for(size_t i= 0; i < REGISTERS; i++){

        byte * pattern = &registerPattern[i];

      	shiftOut(DataPin, ClockPin, MSBFIRST, *pattern);
    }
  	digitalWrite(LatchPin, HIGH);
}

void checkDigits(){
    int i = 0;
    for(i = 0; i < REGISTERS; i++){
        clearRegisters();
        int x = 0;
        for (x = 0; x < 10; x++){
            byte y = byte(x);
            registerPattern[i] = y;
            writeToRegisters();
            delay(100);
        }
        for (x = 0; x < 10; x++){
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
void convertToByte(long number){

    byte sendBytes[] = {0};
    int firstDigits = number/100000;
    int secondDigits = number/1000%100;
    int thirdDigits = number/10 % 100;
    int lastDigit = number % 10;

    if (number < 1000000)
    {
        handleFail(900001);
    }
    
    else if (number <10000000)
    {

        int nr = firstDigits/10;
        firstDigits = nr*6+firstDigits;

        nr = secondDigits/10;
        secondDigits = nr*6+secondDigits;

        nr = thirdDigits/10;
        thirdDigits = nr*6+thirdDigits;

        byte bLastDigit = byte(lastDigit) << 4;

        

        sendBytes[3]= byte(firstDigits);
        sendBytes[2]= byte(secondDigits);
        sendBytes[1]= byte(thirdDigits);
        sendBytes[0]= bLastDigit;

        for(int i = 0; i < 4; i++){
            registerPattern[i]= sendBytes[i];
        }
        writeToRegisters();


    }
    
    else{ 
        handleFail(900001);
    }
}

//timer function, with int time in s
void Timer(int time){

}

//alarm function
void Alarm(long time){

}


//MAIN LOOP
void loop() {

    convertToByte(4544122);

    delay(10000);
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

//1313135
