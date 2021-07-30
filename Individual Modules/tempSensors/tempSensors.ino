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

  OneWire oneWire; // onewire obect for the dallas temp object to hold
  DallasTemperature sensors; // the temp sensor object
  uint8_t (*addresses)[8]; // pointer to array of device addresses

  int numSensors; // number of temperature sensors in the array
  int stationID; // measurement station ID

  String filename = ""; // a filename for the data file
  String* headerInformation; // the header information for the data file
  int numHeaderLines = 3;


  /*************** TempSensors object destructor ***************/
  // destroy a TempSensors object. Frees the dynamically allocated memory (called
  // automatically once object goes out of scope)
  ~TempSensors() {

    // deallocate the address array
    delete [] addresses;
    // zero out the address
    addresses = 0;

    delete [] headerInformation;
    // zero out the address
    headerInformation = 0;
  }


  /*************** TempSensors object constructor ***************/
  // Constructor. This takes in the input information, addresses all of the sensors,
  // and creates the header information for the data file.
  // IMPT: This uses dynamically allocated memory via `new`! You _must_ free the address
  // array and the header information via the tempsensors destructor method when you are done.
  TempSensors(int data_pin, int power_pin, int num_tempSensors, int station_ID) {

    // Setup a oneWire instance to communicate with any OneWire devices
    OneWire currOneWire = OneWire(data_pin);

    this->oneWire = currOneWire;

    // Pass our oneWire reference to create a Dallas Temp object
    DallasTemperature currDallasTemp = DallasTemperature(&oneWire);

    this->sensors = currDallasTemp;

    // copy over the other stuff that we need
    powerPin = power_pin;
    numSensors = num_tempSensors;
    stationID = station_ID;

    // init the address array
    addresses = new uint8_t[num_tempSensors][8];

    // // and point the object's addresses attribute here
    // addresses = curr_addresses;

    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin , HIGH);

    // init temp sensors themselves
    this->sensors.begin();

    // for every sensor
    for (int i = 0; i < numSensors; i++) {

      // if error getting the address
      if (!(this->sensors.getAddress(&addresses[i][0], i))) {

        // print error
        Serial.print("Couldn't find sensor at index ");
        Serial.println(Serial.print(i));
      }
    }

    // now create the data filename
    filename += "station_";
    filename += stationID;
    filename += "_tempArray.txt";


    // define the header information
    int numHeaderLines = 3;

    headerInformation = new String[numHeaderLines];

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
  // reading for each sensor.
  // inputs: timestamp
  String readTempSensors(String date, String time) {

    // Call sensors.requestTemperatures() to issue a global temperature request to all devices on the bus
    sensors.requestTemperatures();

    // declare a string to hold the read data
    String readString = "";

    // throw the timestamp on there
    readString = date + ", " + time;

    // for every sensor on the line
    for (int i=0; i<numSensors; i++){

      // add its data to the string
      readString += ", ";
      readString += this->sensors.getTempC(&addresses[i][0]);
    }

    // return the readstring. TODO: add timestamp to the string
    return readString;
  }

  /************ print array ************/
  void printArr(uint8_t *ptr, int len) {
    for (int i=0; i<len; i++) {
      Serial.print(ptr[i], HEX);
      Serial.print(" ");
    }
  }
};



/************ TEST SCRIPT ************/

// // declare temp sensors pointer as a global
// TempSensors *test;

// setup function
void setup() {

  // setup the board
  boardSetup();

  // Start serial communications
  Serial.begin(9600);
  while (!Serial); // Wait for serial comms

}

/************ main loop ************/
void loop(void)
{

  // get a static temp sensors object. Should persist throughout program lifetime
  static TempSensors test = TempSensors(ONE_WIRE_BUS, TEMP_POWER, NUM_TEMP_SENSORS, STATION_ID);

  // read the data
  String testRead = test.readTempSensors("2021.07.30","16:52:21");

  // print the header information
  for (int i=0;i<test.numHeaderLines;i++) {

    Serial.println(test.headerInformation[i]);
  }

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
