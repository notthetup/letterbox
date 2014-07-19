// Example sketch for talking to the VCNL4000 i2c proximity/light sensor
// Written by Adafruit! Public domain.
// To use: Connect:

// VCC to 3.3-5V (5V is best if it is available)
// GND to ground
// SDA to i2c data (on classic arduinos Analog 4)
// SCL to i2c clock (on classic arduinos, Analog 5)
// The 3.3v pin is an ouptut if you need 3.3V

// This sensor is 5V compliant so you can use it with 3.3 or 5V micros

// You can pick one up at the Adafruit shop: www.adafruit.com/products/466

#include <Wire.h>
#include <SPI.h>
#include "Adafruit_BLE_UART.h"

// Connect CLK/MISO/MOSI to hardware SPI
// e.g. On UNO & compatible: CLK = 13, MISO = 12, MOSI = 11
#define ADAFRUITBLE_REQ 10
#define ADAFRUITBLE_RDY 2     // This should be an interrupt pin, on Uno thats #2 or #3
#define ADAFRUITBLE_RST 9

// the i2c address
#define VCNL4000_ADDRESS 0x13

// commands and constants
#define VCNL4000_COMMAND 0x80
#define VCNL4000_PRODUCTID 0x81
#define VCNL4000_IRLED 0x83
#define VCNL4000_AMBIENTPARAMETER 0x84
#define VCNL4000_AMBIENTDATA 0x85
#define VCNL4000_PROXIMITYDATA 0x87
#define VCNL4000_SIGNALFREQ 0x89
#define VCNL4000_PROXINITYADJUST 0x8A

#define VCNL4000_3M125 0
#define VCNL4000_1M5625 1
#define VCNL4000_781K25 2
#define VCNL4000_390K625 3

#define VCNL4000_MEASUREAMBIENT 0x10
#define VCNL4000_MEASUREPROXIMITY 0x08
#define VCNL4000_AMBIENTREADY 0x40
#define VCNL4000_PROXIMITYREADY 0x20

Adafruit_BLE_UART BTLEserial = Adafruit_BLE_UART(ADAFRUITBLE_REQ, ADAFRUITBLE_RDY, ADAFRUITBLE_RST);

int postmanLDRPin = 0;
int ownerLDRPin = 1;

int proximitySensorVal;
float proximityFilterVal = .9;
int prevProximitySmoothedVal;
int proximitySmoothedVal = 2310; // smoothed starting value for the proximity sensor

int ownerSensorVal;
float ownerFilterVal = .5;
int ownerSmoothedVal = 4; // smoothed starting value for the owner ldr sensor

int postmanSensorVal;
float postmanFilterVal = .5;
int postmanSmoothedVal = 0;

int bothSidesOpenThreshold = 20;

int previousMail = 2472; // no mail proximity value
boolean postmanOpened = false;
boolean ownerOpened = false;

uint8_t deliveries = 0; // number sent as notification to owner's device

uint8_t sendbuffer[] = {0};
char sendbuffersize = 1;
aci_evt_opcode_t laststatus = ACI_EVT_DISCONNECTED;

void setup() {
  Serial.begin(9600);

  Serial.println(F("Letter Box Ready!"));
  // Serial.println("VCNL");
  Wire.begin();

  BTLEserial.begin();

  uint8_t rev = read8(VCNL4000_PRODUCTID);

  if ((rev & 0xF0) != 0x10) {
    Serial.println("Sensor not found :(");
    while (1);
  }


  write8(VCNL4000_IRLED, 20);

  write8(VCNL4000_PROXINITYADJUST, 0x81);
  Serial.print("Proximity adjustment register = ");
  Serial.println(read8(VCNL4000_PROXINITYADJUST), HEX);
}

uint16_t readProximity() {
  write8(VCNL4000_COMMAND, VCNL4000_MEASUREPROXIMITY);
  while (1) {
    uint8_t result = read8(VCNL4000_COMMAND);
    if (result & VCNL4000_PROXIMITYREADY) {
      return read16(VCNL4000_PROXIMITYDATA);
    }
    delay(1);
  }
}

void loop() {

  // Tell the nRF8001 to do whatever it should be working on.
  BTLEserial.pollACI();

  // Ask what is our current status
  aci_evt_opcode_t status = BTLEserial.getState();
  // If the status changed....
  if (status != laststatus) {
    // print it out!
    if (status == ACI_EVT_DEVICE_STARTED) {
        Serial.println(F("* Advertising started"));
    }
    if (status == ACI_EVT_CONNECTED) {
        Serial.println(F("* Connected!"));
    }
    if (status == ACI_EVT_DISCONNECTED) {
        Serial.println(F("* Disconnected or advertising timed out"));
    }
    // OK set the last status change to this one
    laststatus = status;
  }

  // read ambient light!
  write8(VCNL4000_COMMAND, VCNL4000_MEASUREAMBIENT | VCNL4000_MEASUREPROXIMITY);

  // postman ldr value
  postmanSensorVal = analogRead(postmanLDRPin);
  postmanSmoothedVal = smooth(postmanSensorVal, postmanFilterVal, postmanSmoothedVal);
  Serial.print("Postman = ");
  Serial.print(postmanSmoothedVal);

  // owner ldr value
  ownerSensorVal = analogRead(ownerLDRPin);
  ownerSmoothedVal = smooth(ownerSensorVal, ownerFilterVal, ownerSmoothedVal);
  Serial.print("\tOwner = ");
  Serial.print(ownerSmoothedVal);

  while (1) {
    uint8_t result = read8(VCNL4000_COMMAND);
    if ((result & VCNL4000_AMBIENTREADY)&&(result & VCNL4000_PROXIMITYREADY)) {

      // proximity value
      proximitySensorVal = read16(VCNL4000_PROXIMITYDATA);
      proximitySmoothedVal = smooth(proximitySensorVal, proximityFilterVal, proximitySmoothedVal);
      Serial.print("\tMail = ");
      Serial.print(proximitySmoothedVal);

      break;
    }
    delay(10);
  }

  if(ownerSmoothedVal > bothSidesOpenThreshold && postmanSmoothedVal > bothSidesOpenThreshold) { // letterbox opened
    if(ownerSmoothedVal > postmanSmoothedVal) {                                                  // owner opens letterbox
        Serial.println("\t => Owner opened letterbox");
        ownerOpened = true;
    } else {                                                                                     // postman opens letterbox
        Serial.println("\t => Postman opened letterbox");
        postmanOpened = true;
    }
  } else { // letterbox closed
    if(postmanOpened || ownerOpened) {

      if(postmanOpened) {                                       // postman just closed
         if(proximitySmoothedVal > 10 + previousMail) {
            Serial.println("\t => NEW MAIL DELIVERED !!!");
            deliveries++; // send notification to owner's device -  mails delivered
            sendBLEAlert(deliveries);
            postmanOpened = false;
            previousMail = proximitySmoothedVal;
         } else {
           postmanOpened = false;
           previousMail = proximitySmoothedVal;
           Serial.println("\t => no mail delivered");
         }
      }
      if(ownerOpened){                                        // owner just closed
        ownerOpened = false;
        previousMail = proximitySmoothedVal;
        deliveries = 0;   // send notification to owner's device -  mails cleared
        sendBLEAlert(deliveries);
        Serial.println("\t => mails cleared");
      }
    } else {
      Serial.println("\t => letterbox closed");
    }
  }

}

void sendBLEAlert(uint8_t value){
  sendbuffer[0] = value;
  BTLEserial.write(sendbuffer, sendbuffersize);
  Serial.print(F("* Sending -> \""));
  Serial.println((char *)sendbuffer);
}

// Read 1 byte from the VCNL4000 at 'address'
uint8_t read8(uint8_t address)
{
  uint8_t data;

  Wire.beginTransmission(VCNL4000_ADDRESS);
#if ARDUINO >= 100
  Wire.write(address);
#else
  Wire.send(address);
#endif
  Wire.endTransmission();

  delayMicroseconds(170);  // delay required

  Wire.requestFrom(VCNL4000_ADDRESS, 1);
  while(!Wire.available());

#if ARDUINO >= 100
  return Wire.read();
#else
  return Wire.receive();
#endif
}


// Read 2 byte from the VCNL4000 at 'address'
uint16_t read16(uint8_t address)
{
  uint16_t data;

  Wire.beginTransmission(VCNL4000_ADDRESS);
#if ARDUINO >= 100
  Wire.write(address);
#else
  Wire.send(address);
#endif
  Wire.endTransmission();

  Wire.requestFrom(VCNL4000_ADDRESS, 2);
  while(!Wire.available());
#if ARDUINO >= 100
  data = Wire.read();
  data <<= 8;
  while(!Wire.available());
  data |= Wire.read();
#else
  data = Wire.receive();
  data <<= 8;
  while(!Wire.available());
  data |= Wire.receive();
#endif

  return data;
}

// write 1 byte
void write8(uint8_t address, uint8_t data)
{
  Wire.beginTransmission(VCNL4000_ADDRESS);
#if ARDUINO >= 100
  Wire.write(address);
  Wire.write(data);
#else
  Wire.send(address);
  Wire.send(data);
#endif
  Wire.endTransmission();
}

int smooth(int data, float filterVal, float smoothedVal){

  if (filterVal > 1){      // check to make sure param's are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);
  return (int)smoothedVal;
}
