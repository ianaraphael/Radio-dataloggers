/*

tempSensors.ino

Class definition to initialize, read, and write DS18B20 temperature sensor data

Ian Raphael
ian.th@dartmouth.edu
2021.07.19
*/

#include <MemoryFree.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define Serial SerialUSB

#define ONE_WIRE_BUS 12 // temp probe data line
#define TEMP_POWER 6 // temp probe power line
#define STATION_ID 1 // station ID
#define NUM_TEMP_SENSORS 2 // number of sensors



/*************** TempSensors class ***************/
// define a class for the temp sensors
class TempSensors {

  // make public access for everything
public:

  /* Attributes */
  int powerPin; // power pin for the sensors
  int dataPin; // data pin for the sensors

  DallasTemperature* sensors; // the temp sensor object
  uint8_t (*addresses)[8]; // pointer to array of device addresses

  int numSensors; // number of temperature sensors in the array
  int stationID; // measurement station ID

  String filename = ""; // a filename for the data file
  String* headerInformation; // the header information for the data file
  int numHeaderLines = 3;

  /*************** TempSensors object constructor ***************/
  // Constructor. This takes in the input information, addresses all of the sensors,
  // and creates the header information for the data file.
  TempSensors(int data_pin, int power_pin, int num_tempSensors, int station_ID) {

    // Setup a oneWire instance to communicate with any OneWire devices
    OneWire oneWire(data_pin);

    // Pass our oneWire reference to create a Dallas Temp object
    DallasTemperature currDallasTemp = DallasTemperature(&oneWire);

    // TODO: this is just for debugging. let's init everything here and see if we can read a temp.
    // pinMode(powerPin, OUTPUT);
    // digitalWrite(powerPin , HIGH);
    // currDallasTemp.begin();
    // currDallasTemp.requestTemperatures();
    // float currTemp = currDallasTemp.getTempCByIndex(0);
    // Serial.print("Here's a test temp: ");
    // Serial.println(currTemp);

    sensors = &currDallasTemp;




    // copy over the other stuff that we need
    powerPin = power_pin;
    numSensors = num_tempSensors;
    stationID = station_ID;

    // init the address array
    uint8_t curr_addresses[num_tempSensors][8];

    // and point the object addresses attribute here
    addresses = curr_addresses;

    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin , HIGH);

    // init temp sensors themselves
    sensors->begin();

    // for every sensor
    for (int i = 0; i < numSensors; i++) {

      // if error getting the address
      if (!sensors->getAddress(&addresses[i][0], i)) {

        // print error
        Serial.print("Couldn't find sensor at index ");
        Serial.println(Serial.print(i, DEC));
      }
    }


    // now create the data filename
    filename += "station_";
    filename += stationID;
    filename += "_tempArray.txt";


    // define the header information
    int numHeaderLines = 3;
    String currHeaderInformation[numHeaderLines];
    headerInformation = currHeaderInformation;

    headerInformation[0] = "DS18B20 array";

    headerInformation[1] = "Station ID: ";
    headerInformation[1] += stationID;

    headerInformation[2] = "Date, Time";

    // for every sensor in the array
    for (int i=0; i<numSensors; i++) {

      // add a header column for each sensor
      headerInformation[2] += ", Sensor ";
      headerInformation[2] += i+1;
    }
  }


  /*************** TempSensors readTempSensors ***************/
  // Reads an array of temp sensors, returning a string with a timestamp and the
  // reading for each sensor
  String readTempSensors() {

    Serial.println("Made it to readtemps");

    Serial.print("Here's a count just to check that we can access this object: ");
    Serial.println(sensors->devices,DEC);

    Serial.print("Free ram: ");
    Serial.println(freeMemory());

    // Call sensors.requestTemperatures() to issue a global temperature request to all devices on the bus
    sensors->requestTemperatures();

    Serial.println("Made it past requesting the temps");

    // declare a string to hold the read data
    String readString = "";

    Serial.println("created the readstring");

    // for every sensor on the line
    for (int i=0; i<numSensors; i++){

      Serial.println("Made it into reading the sensors");

      // add its data to the string
      readString += sensors->getTempC(&addresses[i][0]);
      readString += ", ";

      Serial.println(readString);

    }

    Serial.println(readString);

    // return the readstring. TODO: add timestamp to the string
    return readString;
  }
};


void setup() {

  boardSetup();

  // Start serial communications
  Serial.begin(9600);
  while (!Serial); //Wait for serial comms

}


/************ main loop ************/
void loop(void)
{
  // create our temp sensors object
  TempSensors test (ONE_WIRE_BUS, TEMP_POWER, NUM_TEMP_SENSORS, STATION_ID);

  // // print an address
  // Serial.print("Here's an address: ");
  // printArr(&test.addresses[1][0],8);
  // Serial.println("");
  //
  // Serial.print("Is it connected?: ");
  // Serial.println(test.sensors->isConnected(&test.addresses[1][0]));
  //
  //
  // Serial.print("Here's another one: ");
  // printArr(&test.addresses[0][0],8);
  // Serial.println("");
  // Serial.print("Is it connected?: ");
  // Serial.println(test.sensors->isConnected(&test.addresses[0][0]));

  // read the data
  String testRead = test.readTempSensors();

  // print the data
  Serial.println(testRead);

  delay(1000);
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

/************ print array ************/
void printArr(uint8_t *ptr, int len) {
  for (int i=0; i<len; i++) {
    Serial.print(ptr[i], HEX);
    Serial.print(" ");
  }
}
