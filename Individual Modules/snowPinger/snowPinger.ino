/*

snowPinger.ino

Class definition to initialize, read, and write maxbotix snow pinger class

Ian Raphael
ian.th@dartmouth.edu
2021.07.30
*/

#include <MemoryFree.h>

#define Serial SerialUSB

#define STATION_ID 1 // station ID

HardwareSerial* PINGER_BUS = &Serial1; // serial port for reading the data
#define PINGER_POWER 7 // digital pin for pinger power

/*************** SnowPinger class ***************/
// define a class for the snow pinger
class SnowPinger {

  // make public access for everything
public:

  /* Attributes */
  int powerPin; // power pin for the pinger
  HardwareSerial* dataBus; // data pin for the pinger
  int stationID; // measurement station ID

  int timeout = 100; // max time to wait for a pinger reading
  String filename = ""; // a filename for the data file
  String* headerInformation; // the header information for the data file
  int numHeaderLines = 3;


  /*************** TempSensors object destructor ***************/
  // destroy a TempSensors object. Frees the dynamically allocated memory (called
  // automatically once object goes out of scope)
  ~SnowPinger() {

    // // deallocate the address array
    // delete [] addresses;
    // // zero out the address
    // addresses = 0;

    // // deallocate the header info array
    delete [] headerInformation;
    // zero out the address
    headerInformation = 0;
  }


  /*************** SnowPinger object constructor ***************/
  // Constructor. This holds all of the bus information for the sensor
  // and creates the header information for the data file.
  // IMPT: This uses dynamically allocated memory via `new`! You _must_ free the address
  // array and the header information via the tempsensors destructor method when you are done.
  SnowPinger(HardwareSerial* data_bus, int power_pin, int station_ID) {

    // copy over the other stuff that we need
    dataBus = data_bus;
    powerPin = power_pin;
    stationID = station_ID;

    // now create the data filename
    filename += "station_";
    filename += stationID;
    filename += "_snowPinger.txt";

    // define the header information
    headerInformation = new String[numHeaderLines];

    headerInformation[0] = "Snow pinger";

    headerInformation[1] = "Station ID: ";
    headerInformation[1] += stationID;

    headerInformation[2] = "Date, Time";

    headerInformation[2] += ", Snow pinger range (mm)";


  }


  /*************** SnowPinger readPinger ***************/
  // Reads the snow pinger and returns a string with "date, time, range"
  // inputs: date, time strings
  String readPinger(String date, String time) {

    // write power to the pinger
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin , HIGH);

    // start communicating with the pinger
    dataBus->begin(9600);

    String pingerReturn = "NaN"; // a string to return the cleaned pinger info

    // If we're connected to the PINGER_BUS get a reading. Since the pinger is constantly
    // spewing data when powered we only need the RX pin (we never TX to pinger)
    if (dataBus->available()) {

      String pingerReadout; // a string to hold what's read off the pinger

      // set a timeout for reading the serial line
      Serial.setTimeout(timeout);

      // The format of the pingers output is ascii: Rxxxx\r where x is a digit
      // Thus we can read the streaming input until we catch '\r'
      // get the pinger readout
      pingerReadout = dataBus->readStringUntil('\r');

      // copy the pinger readout into the return string starting after the first char
      pingerReturn = pingerReadout.substring(1);

      // if the readout doesn't conform to the R + 4 digits format
      if ((pingerReadout[0] != 'R') || (pingerReadout.length()!= 5)) { // first check if the first char is caps R

        // set the output to string "NaN"
        pingerReturn = "NaN";

      } else {
        // then for the following four chars
        for (int i=1; i<5; i++) {

          // if any of them aren't digits
          if (!isdigit(pingerReadout[i])) {

            // set the output to string "NaN"
            pingerReturn = "NaN";

            // and escape
            break;
          }
        }
      }
    }

    // declare a string to hold the read data
    String readString = "";

    // throw the timestamp and data on there
    readString = date + ", " + time + ", " + pingerReturn;

    // return the readstring.
    return readString;
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
  static SnowPinger test = SnowPinger(PINGER_BUS, PINGER_POWER, STATION_ID);

  // read the data
  String testRead = test.readPinger("2021.07.30","16:52:21");

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
