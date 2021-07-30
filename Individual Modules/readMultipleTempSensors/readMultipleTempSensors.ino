/*

readMultipleTempSensors.ino

Read multiple temp sensors

Ian Raphael
ian.th@dartmouth.edu
2021.07.19
*/

#include <OneWire.h>
#include <DallasTemperature.h>

#define Serial SerialUSB


#define ONE_WIRE_BUS 12 // temp probe data line
#define TEMP_POWER 6 // temp probe power line

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature tempSensors(&oneWire);

const int ADDRESS_SIZE = 8; // size of a thermistor address
const int NUM_TEMP_SENSORS = 2;
uint8_t tempAddrx[NUM_TEMP_SENSORS][ADDRESS_SIZE]; // 2D array of device addresses

/************ setup ************/
void setup(void)
{
  // setup the board
  boardSetup();

  // Start serial communications
  Serial.begin(9600);
  while (!Serial); //Wait for serial comms

  // init all the temp sensors
  tempInit();
}

/************ main loop ************/
void loop(void)
{

  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  for (int i = 0; i<5; i++) {
    tempSensors.requestTemperatures();
    Serial.print("Sensor 0 address: ");
    printArr(&tempAddrx[0][0], 8);
    Serial.println("");
    Serial.print("Sensor 0: Raw Values: ");
    Serial.print(tempSensors.getTemp(&tempAddrx[0][0]), BIN);

    Serial.print(" - Celsius temperature: ");
    Serial.print(tempSensors.getTempC(&tempAddrx[0][0]));
    Serial.print(" - Celsius temperature by index: ");
    Serial.println(tempSensors.getTempCByIndex(0));
  }

  for (int i = 0; i<5; i++) {
    tempSensors.requestTemperatures();
    Serial.print("Sensor 1 address: ");
    printArr(&tempAddrx[1][0], 8);
    Serial.println("");
    Serial.print("Sensor 1: Raw Values: ");
    Serial.print(tempSensors.getTemp(&tempAddrx[1][0]), BIN);

    Serial.print(" - Celsius temperature: ");
    Serial.print(tempSensors.getTempC(&tempAddrx[1][0]));
    Serial.print(" - Celsius temperature by index: ");
    Serial.println(tempSensors.getTempCByIndex(1));
  }
  delay(1000);
}


/************ print array ************/
void printArr(uint8_t *ptr, int len) {
  for (int i=0; i<len; i++) {
    Serial.print(ptr[i], HEX);
    Serial.print(" ");
  }
}

/************ Board setup ************/
void boardSetup() {

  unsigned char pinNumber;

  // Pull up all unused pins to prevent floating vals
  for (pinNumber = 0; pinNumber < 23; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  for (pinNumber = 32; pinNumber < 42; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  analogReadResolution(12);
}

/************ Temp sensors init ************/
void tempInit() {

  pinMode(TEMP_POWER, OUTPUT);
  digitalWrite(TEMP_POWER, HIGH);

  // init temp sensors
  tempSensors.begin();

  // for every sensor
  for (int i = 0; i < NUM_TEMP_SENSORS; i++) {

    // if error getting the address
    if (!tempSensors.getAddress(&tempAddrx[i][0], i)) {

      // print error
      Serial.print("Couldn't find sensor at address ");
      Serial.println(Serial.print(i, DEC));
    }

    // otherwise print address
    Serial.print("Sensor ");
    Serial.print(i, DEC);
    Serial.print(" online, address: ");
    printArr(&tempAddrx[i][0], 8);
    Serial.println(" ");
  }
}
