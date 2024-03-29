/*

  Plant Care

  Control irrigation according to soil moisture conten
  See doc/timing.txt for usage

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

int pumpPin = 5;        // pump driver HIGH : Pump ON

int ledPin = 13;        // yellow LED to flash on Leonardo board

int ledon = 1;          // cache LED state
int nLoop = 0;
int nDry;               // the number of consecutive dry minutes
int nPump;              // the total number of irrigations in this run

int sensorValueH, sensorValueL, sensorMoisture = 0;  // variable to store the value coming from the sensor
int sensorDelay = 100;


const int owdtaPin = 4;      // IO4: 1 wire data

const int sLen = 20;      // string represantation of timestamp



// DS18S20 Temperature chip i/o
OneWire ds(owdtaPin);


byte i, incomingByte;

byte nsensor;  // the actual number of sensors
byte type_s;
byte data[12];
// addresses are stored in contingous array of 8 bytes
byte addr[8 * nSENSOR];
int rawtemp[nSENSOR];
// the highest and lowest temperature
int tempHi, tempLo;

volatile unsigned int activecycles, totalcycles;  // the number of cycles while heater is on (active) and
// the total number of cycles in current control macro cycle
// totalcycles is up until PWNMLEN

volatile boolean inDump;    // dump initiated in main loop
volatile boolean sdOK;   // SD card is available

File sdFile;
byte rtSec, rtMin, rtHour, rtDay, rtMonth, rtYear;
byte thisMin;   // store the last minute being recognised
String tStamp;




// get the temperature of n-th sensor
// n should be within limits [0..nSENSOR-1]

void GetTemp(byte n) {

  int i;

  ds.reset();
  ds.select(addr + 8 * n);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  ds.reset();
  ds.select(addr + 8 * n);
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      type_s = 0;
      //      Serial.println("Device is not a DS18x20 family device.");
  }


  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  rawtemp[n] = raw;

}




void GetDate() {

  Wire.beginTransmission(RTC); // transmit to device #4
  Wire.write(0);        // set address to 0
  Wire.endTransmission();    // stop transmitting

  Wire.requestFrom(RTC, 7);    // request 7 bytes from slave device #x68 (DS1307 RTC)
  rtSec = Wire.read();
  rtMin = Wire.read();
  rtHour = Wire.read();
  rtDay = Wire.read();
  rtDay = Wire.read();  // drop day of week data
  rtMonth = Wire.read();
  rtYear = Wire.read();

  tStamp = String(rtYear, HEX) + " " + String(rtMonth, HEX) + " " + String(rtDay, HEX)
           + " " + String(rtHour, HEX) + " " + String(rtMin, HEX) + " " + String(rtSec, HEX);

}


/*
   Dumb SetDate: the date is hardcoded - just for Setup
   values are BCD format
*/

void SetDate() {

  Wire.beginTransmission(RTC); // transmit to device #4
  Wire.write(0);        // set address to 0
  Wire.write(0);        // set s to 0
  Wire.write(0x11);        // set min
  Wire.write(0x20);        // set hour 24 hour format: bit 6 = 0!
  Wire.write(0);        // set day of week (just placeholder)

  Wire.write(0x31);        // set day
  Wire.write(0x10);        // set month
  Wire.write(0x20);        // set year

  Wire.endTransmission();    // stop transmitting


}

void StoreData() {

  if (sdOK) {

    sdFile = SD.open("data.txt", FILE_WRITE);

    sdFile.print(tStamp);
    sdFile.print('\t');

    sdFile.print(nLoop);
    sdFile.print("\t");
    sdFile.print(nDry);
    sdFile.print("\t");
    sdFile.print(nPump);
    sdFile.print(",\t");

    sdFile.print(sensorValueH);
    sdFile.print("\t");
    sdFile.print(sensorValueL);
    sdFile.print("\t");
    sdFile.print(sensorMoisture);
    sdFile.print("\t");

    for (int i = 0; i < nsensor; i++) {
      sdFile.print((rawtemp[i] & 0xff0) >> 4);
      sdFile.print(',');
      sdFile.print(rawtemp[i] & 0xf);
      sdFile.print("\t");
    }

    sdFile.println();
    sdFile.close();
  }

}

/*
  Fill rtYear, rtMonth... and tStamp
*/


// fill the array of sensor addresses
void listSensors() {
  byte n;

  // iteration count over sensors
  nsensor = 0;
  ds.reset_search();
  while (ds.search(addr + 8 * nsensor)) {
    nsensor++;
  }

  Serial.print(nsensor);
  Serial.print(" sensor(s) found.\n");

  for (n = 0; n < nsensor; n++) {
    for (i = 0; i < 8; i++) {
      if (addr[n * 8 + i] < 0x10) Serial.print('0');
      Serial.print(addr[n * 8 + i], HEX);
    }

    if (OneWire::crc8(addr + 8 * n, 7) != addr[8 * n + 7]) {
      Serial.print(" CRC is not valid!");
    }
    else Serial.print(" CRC is OK!");

    Serial.print("\n");
  }

  // if SD card is available, write sensor data to startup.txt
  if (sdOK) {
    sdFile = SD.open("startup.txt", FILE_WRITE);

    sdFile.println(" ");
    sdFile.println(tStamp);

    for (n = 0; n < nsensor; n++) {
      for (i = 0; i < 8; i++) {
        if (addr[n * 8 + i] < 0x10) sdFile.print('0');
        sdFile.print(addr[n * 8 + i], HEX);
      }

      if (OneWire::crc8(addr + 8 * n, 7) != addr[8 * n + 7]) {
        sdFile.print(" CRC is not valid!");
      }
      else sdFile.print(" CRC is OK!");

      sdFile.print("\n");
    }


    sdFile.close();

  }


  Serial.print("\n");
}




// get all temperatures
void GetTemp() {
  int i;

  for (i = 0; i < nsensor; i++)
    GetTemp(i);
  tempLo = rawtemp[0];
  tempHi = rawtemp[0];
  for (i = 1; i < nsensor; i++) {
    if (rawtemp[i] > tempHi) tempHi = rawtemp[i];
    if (rawtemp[i] < tempLo) tempLo = rawtemp[i];
  }

}




void printStatus() {

  if (sdOK) Serial.print("SD_OK    ");
  else  Serial.print  ("!!SD_FAIL");
  Serial.print("\t");

  // there is no watchdog actually
  //  if (digitalRead(wmonPin) == HIGH) Serial.print("WDen ");
  //    else  Serial.print("WDdis");
  //  Serial.print("\t");

  Serial.print(tStamp);
  Serial.print('\t');

  Serial.print(nLoop);
  Serial.print("\t");
  Serial.print(nDry);
  Serial.print("\t");
  Serial.print(nPump);
  Serial.print(",\t");

  Serial.print(sensorMoisture);
  Serial.print("\t");

  Serial.print((tempLo & 0xff0) >> 4);
  Serial.print(',');
  Serial.print(tempLo & 0xf);
  Serial.print("\t");

  // there is one tempereture sensor only
  //  Serial.print((tempHi & 0xff0) >> 4);
  //  Serial.print(',');
  //  Serial.print(tempHi & 0xf);
  //  Serial.print("\t");

  Serial.println();
}


// dump acquired data
void dumpData() {
  char c;

  //  Serial.println("*****************************************");
  //  Serial.println("***          startup.txt              ***");
  //  Serial.println("*****************************************");
  Serial.println("*S");
  sdFile = SD.open("startup.txt", FILE_READ);
  while (sdFile.available()) {
    c = sdFile.read();
    Serial.write(c);
  }
  sdFile.close();

  //  Serial.println("*****************************************");
  //  Serial.println("***            data.txt               ***");
  //  Serial.println("*****************************************");
  Serial.println("*D");
  sdFile = SD.open("data.txt", FILE_READ);
  while (sdFile.available()) {
    c = sdFile.read();
    Serial.write(c);
  }
  sdFile.close();

  //  Serial.println("*****************************************");
  //  Serial.println("***              end                  ***");
  //  Serial.println("*****************************************");
  Serial.println("*E");

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


void MeasMoisture() {

  digitalWrite(drive1Pin, HIGH);
  digitalWrite(drive2Pin, LOW);
  // stop the program for <sensorValue> milliseconds:
  delay(sensorDelay);
  // turn the ledPin off:
  // read the value from the sensor:
  sensorValueH = analogRead(sensorPin);

  digitalWrite(drive1Pin, LOW);
  digitalWrite(drive2Pin, HIGH);
  // stop the program for for <sensorValue> milliseconds:
  delay(sensorDelay);
  // read the value from the sensor:
  sensorValueL = analogRead(sensorPin);
  sensorMoisture = sensorValueH - sensorValueL;

  // deactivate sensor if out of use
  digitalWrite(drive1Pin, LOW);
  digitalWrite(drive2Pin, LOW);

  //  Serial.print("Drive H/L - D : ");
  //  Serial.print(sensorValueH);
  //  Serial.print(" / ");
  //  Serial.print(sensorValueL);
  //  Serial.print(" - ");
  //  Serial.println(sensorMoisture);

}

void PumpOn(int t_sec) {
  digitalWrite(pumpPin, HIGH);
  delay(1000 * t_sec);
  digitalWrite(pumpPin, LOW);

}




void setup()
{

  pinMode(drive1Pin, OUTPUT);
  pinMode(drive2Pin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);

  digitalWrite(drive1Pin, LOW);
  digitalWrite(drive2Pin, LOW);
  digitalWrite(ledPin, LOW);
  digitalWrite(pumpPin, LOW);

  ledon = 0;
  nLoop = 0;
  inDump = false;

  nDry = 0;
  nPump = 0;

  Wire.begin();        // join i2c bus (address optional for master)
  // only after comissioning or changing battery
  // SetDate();

  Serial.begin(19200);

  //  Serial.print("Initializing SD card...");

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    //    Serial.println("Card failed, or not present");
    sdOK = false;
  }
  else {
    //    Serial.println("card initialized.");
    sdOK = true;
  }

  listSensors();

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

  // wait for dump comand
  if (Serial.available()) {
    // read the incoming byte:
    incomingByte = Serial.read();

    if (incomingByte == 'D') {
      inDump = true;
      dumpData();
      inDump = false;
    }
    else
      Serial.println("?");
  }

  GetDate();
  GetTemp();
  MeasMoisture();

  if (thisMin != rtMin) {
    // make certain calculations once in a minute
    thisMin = rtMin;

    if (nDry > N_TO_NEXT_IRRIGATION) {
      PumpOn(PUMP_LENGTH_S);
      nDry = 0;
      nPump++;
    }

    if (sensorMoisture > MOIST_LOW) {
      // totally dry reads 1023, totally wet ~300
      // may be confusing to call it moisture as reading is inversly proportional
      nDry++;
    }
    else {
      nDry = 0;
    }

  }

  if (sdOK) {
    StoreData();
  }


  printStatus();
  delay(1000);
  nLoop++;

}
