/*

  Oxigen rationing

  initial guess: periode/duty -> 5/0.2 s
  
  Use with
	- Arduino Leonardo R3
	- "Adafruit SMT Datalogger Shield.pdf" - providing SD card interface and RTC
	- circuit board according to main-wleonardo.pdf - providing pump, level sensor, temperature sensor connectivity


  TODO:
	- implement level sensor check
	- implement irrigation plausibility: shut down if conductivity is not changing after (eg.) 5 irigations
	- SD card readout (via serial line) will hang the sketch!
	- add HC-12 wireless serial support - so serial wire connection is not necessary (?)
	- is it possible to eliminate RTC and SD, still preserve -some level of- data logging?



*/

#include "OneWire.h"

#include <SD.h>

// Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 10;

// On the Ethernet Shield, CS is pin 4.
// const int chipSelect = 4;

// I2C bus access

#include <Wire.h>

const int RTC = 0x68;

// data logging is done by 5 minutes
const unsigned int MEASPER = 300;

// the number of 1W temperature sensors in the chain
const unsigned int nSENSOR = 1;

// the number of minutes to the next irrigation if
const unsigned int N_TO_NEXT_IRRIGATION = 5;

// moistore below MOIST_LOW consideered dry
const unsigned int MOIST_LOW = 875;

const unsigned int PUMP_LENGTH_S = 7;

int sensorPin = A3;    // select the input pin for the potentiometer
// drive pins for soil moisture sensor
int drive1Pin = 8;      // select the pin for the LED
int drive2Pin = 9;      // select the pin for the LED

int valvePin = 5;        // valve driver HIGH : valve open

int ledPin = 13;        // yellow LED to flash on Leonardo board

int ledon = 1;          // cache LED state
int nLoop = 0;
int nPump;              // the total number of irrigations in this run

const int owdtaPin = 4;      // IO4: 1 wire data

const int sLen = 20;      // string represantation of timestamp

int vOnTime = 10;      // valve on time [ms]
int cycleTime = 500;    // main loop iteration time
int idleTime = 5000;    // delay between puffs
int passTime = 0;

byte i, incomingByte;




void printStatus() {
  
  Serial.println();
  Serial.print("On time: ");
  Serial.println(vOnTime);
  Serial.print(" oxigen puffs: ");
  Serial.println(nPump);

}


void ToggleLED() {
  if (ledon == 0) {
    ledon = 1;
    digitalWrite(ledPin, HIGH);
  }
  else {
    ledon = 0;
    digitalWrite(ledPin, LOW);
  }
}



void ValveOn(int t_ms) {

  // chopped start to reduce noise!?
  /*
  digitalWrite(valvePin, HIGH);
  delay(20);
  digitalWrite(valvePin, LOW);
  delay(20);
  */
  digitalWrite(valvePin, HIGH);
  delay(t_ms);
  digitalWrite(valvePin, LOW);
  nPump++;

}




void setup()
{

  pinMode(drive1Pin, OUTPUT);
  pinMode(drive2Pin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(valvePin, OUTPUT);

  digitalWrite(drive1Pin, LOW);
  digitalWrite(drive2Pin, LOW);
  digitalWrite(ledPin, LOW);
  digitalWrite(valvePin, LOW);

  ledon = 0;
  nLoop = 0;

  nPump = 0;

  Serial.begin(19200);

}



/*

  Main loop:
  - dump file if requested (disable logging for the time)
  - print status message to the terminal
  - WDLOOP presence toggling
  - temperature measurement (1 s * number of sensors!)
  - 1 sec delay
*/

void loop()
{

  ToggleLED();

  // wait for comand
  if (Serial.available()) {
    // read the incoming byte:
    incomingByte = Serial.read();

    if (incomingByte == 'V') {
      ValveOn(vOnTime);
      printStatus();
      passTime = 0;
    }
    else
      Serial.println("?");
  }

  if (passTime > idleTime) {
      ValveOn(vOnTime);
      printStatus();
      passTime = 0;
  }

  passTime += cycleTime;
  delay(cycleTime);
  Serial.print("*");
  nLoop++;

}
