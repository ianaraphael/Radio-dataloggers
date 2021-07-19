/*********
  Code to demonstrate how to read temperature data off of the DS18B20
  As found on: https://randomnerdtutorials.com/guide-for-ds18b20-temperature-sensor-with-arduino/

  Modified by David Clemens-Sewall on 2019-06-23
*********/

#include <OneWire.h>
#include <DallasTemperature.h>

#define Serial SerialUSB

void(* resetFunc) (void) = 0;

// Data wire is connected to the Arduino digital pin 12
#define ONE_WIRE_BUS 12

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);

// Array to hold device addresses
uint8_t addr[8];

void setup(void)
{
  // Start serial communication for debugging purposes
  Serial.begin(9600);
  while (!Serial); //Wait for serial comms
  // Start up the library
  sensors.begin();
  // Get the address of the first sensor
  if (sensors.getAddress(addr, 0)) {
    Serial.print("Sensor 0 address: ");
    printArr(addr, 8);
    Serial.println(" ");
    sensors.setResolution(12);
  } else {
    Serial.println("Couldn't find sensor at address 0");
    resetFunc();
  }
}


void loop(void){
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures();
  Serial.print("Raw Values: ");
  Serial.print(sensors.getTemp(addr), BIN);


  Serial.print(" - Celsius temperature: ");
  Serial.print(sensors.getTempC(addr));
  Serial.print(" - Celsius temperature by index: ");
  Serial.println(sensors.getTempCByIndex(0));
  delay(1000);
}

void printArr(uint8_t *ptr, int len) {
  for (int i=0; i<len; i++) {
    Serial.print(ptr[i], HEX);
    Serial.print(" ");
  }
}
