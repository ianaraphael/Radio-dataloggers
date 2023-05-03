/*

client_barrow.ino

for Utqiagvik 2022.11 deployment. modifications to alarm match

Radio enabled temperature sensor station with deep sleep and alarm functionality.
Samples temperature sensors and saves to a file on SD card. Enters standby
(low power) mode. Wakes up by scheduled RTC alarm interrupt. Sends data to server.

Ian Raphael
ian.th@dartmouth.edu
2022.08.09
*/

// debug
bool DEBUG = false;
bool volatile trigger = true;

#define Serial SerialUSB

/***************!!!!!! Station settings !!!!!!!***************/
#define SERVER_ADDRESS 0
#define STATION_ID 1 // station ID
#define NUM_TEMP_SENSORS 1 // number of sensors
// uint8_t SAMPLING_INTERVAL_HOUR = 0;// number of hours between samples
uint8_t SAMPLING_INTERVAL_MIN = 1; // number of minutes between samples
uint8_t ALARM_MINUTES = 0;
// uint8_t SAMPLING_INTERVAL_SEC = 0; // number of seconds between samples

/*************** packages ***************/
#include <OneWire.h>
#include <SerialFlash.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <RTCZero.h>
#include <TimeLib.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>


/*************** defines, macros, globals ***************/
// misc.
#define Serial SerialUSB
RTCZero rtc; // real time clock object

// Temp sensors
#define ONE_WIRE_BUS 7 // temp probe data line
const int flashChipSelect = 4;
#define TEMP_POWER 8 // temp probe power line

// pinger
#define PINGER_BUS Serial1 // serial port to read pinger
#define PINGER_POWER 11 // pinger power line
#define PINGER_TIMEOUT 100 // timeout to prevent hang in pinger reading (ms)
#define N_PINGERSAMPLES 5 // number of samples to average for each pinger reading
#define MAX_PINGER_SAMPLE_ATTEMPTS 30 // maximum number of samples to attempt in order to achieve N_PINGERSAMPLES successfully
// SD card
#define SD_CS 10 // SD card chip select
#define SD_POWER 6 // SD card power
#define SD_CD NAN // SD card chip DETECT. Indicates presence/absence of SD card. High when card is inserted.

// Radio
#define MAX_TRANSMISSION_ATTEMPTS 3 // maximumum number of times to attempt transmission per sampling cycle
#define TIMEOUT 20000 // max wait time for radio transmissions in ms
#define RADIO_CS 5 // radio chip select pin
#define RADIO_INT 2 // radio interrupt pin
#define RADIO_FREQ 915.0 // radio frequency
RH_RF95 driver(RADIO_CS, RADIO_INT); // Singleton instance of the radio driver
RHReliableDatagram manager(driver, STATION_ID); // Class to manage message delivery and receipt, using the driver declared above

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
  int numHeaderLines = 4;


  /*************** TempSensors object destructor ***************/
  // destroy a TempSensors object. Frees the dynamically allocated memory (called
  // automatically once object goes out of scope)
  ~TempSensors() {

    // // deallocate the address array
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
  // TODO: fully sleep the sensors at the end of this function
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
    addresses = new DeviceAddress[num_tempSensors];

    // // and point the object's addresses attribute here
    // addresses = curr_addresses;


    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin , HIGH);


    // init temp sensors themselves
    this->sensors.begin();

    // for every sensor
    for (int i = 0; i < numSensors; i++) {

      // if error getting the address
      if (!(this->sensors.getAddress(addresses[i], i))) {

        // print error
        Serial.print("Couldn't find sensor at index ");
        Serial.println(Serial.print(i), DEC);
      }
    }

    // now create the data filename
    filename += "stn";
    filename += stationID;
    filename += "_t.txt";
    // filename += "_tempArray.txt";

    // define the header information
    headerInformation = new String[numHeaderLines];

    headerInformation[0] = "DS18B20 array";

    headerInformation[1] = "Station ID: ";
    headerInformation[1] += stationID;

    headerInformation[2] = "Sensor addresses: {";
    appendAddrToStr(addresses[0], &headerInformation[2]);
    for (int i = 1; i < numSensors; i++) {
      headerInformation[2] += ", ";
      appendAddrToStr(addresses[i], &headerInformation[2]);
    }
    headerInformation[2] += "}";

    headerInformation[3] = "Date, Time";
    // for every sensor in the array
    for (int i = 0; i < numSensors; i++) {

      // add a header column for each sensor
      headerInformation[3] += ", Sensor";
      headerInformation[3] += i + 1;
      headerInformation[3] += " [°C]";
    }

    // if a file for the station doesn't already exist
    if (!SD.exists(filename)) {

      Serial.print("Creating datafile: ");
      Serial.println(filename);

      // create the file
      File dataFile = SD.open(filename, FILE_WRITE);

      // write the header information
      // if the file is available
      if (dataFile) {

        // print the header information to the file
        for (int i = 0; i < numHeaderLines; i++) {

          dataFile.println(headerInformation[i]);

          // also print to serial to confirm
          Serial.println(headerInformation[i]);
        }

        // close the file
        dataFile.close();
      }
    }


    // shut down the sensors until we need them
    digitalWrite(powerPin, LOW);

  }


  /*************** TempSensors readTempSensors ***************/
  // Reads an array of temp sensors, returning a string with a timestamp and the
  // reading for each sensor.
  // inputs: timestamp
  // TODO: wake/sleep the sensors in this function
  String readTempSensors(String date, String time) {

    // write the power pin high
    digitalWrite(powerPin, HIGH);


    // Call sensors.requestTemperatures() to issue a global temperature request to all devices on the bus
    sensors.requestTemperatures();

    // declare a string to hold the read data
    String readString = "";

    // throw the timestamp on there
    readString = date + ", " + time;

    // for every sensor on the line
    for (int i = 0; i < numSensors; i++) {

      // add its data to the string
      readString += ", ";
      readString += this->sensors.getTempC(addresses[i]);
    }

    // write the power pin low
    digitalWrite(powerPin, LOW);

    // return the readstring. TODO: add timestamp to the string
    return readString;
  }

  /************ print array ************/
  // void printArr(uint8_t *ptr, int len) {
  //   for (int i=0; i<len; i++) {
  //     Serial.print(ptr[i], HEX);
  //     Serial.print(" ");
  //   }
  // }

  void appendAddrToStr(uint8_t *addrPtr, String *strPtr) {
    for (int i = 0; i < 8; i++) {
      // *strPtr += "0x";
      *strPtr += String(addrPtr[i], HEX);
      if (i < 7) {
        *strPtr += " ";
      }
    }
  }
};


/*************** SnowPinger class ***************/
// define a class for the snow pinger
class SnowPinger {

  // make public access for everything
public:

  /* Attributes */
  int powerPin; // power pin for the pinger
  HardwareSerial *pingerBus; // Serial pin for the pinger

  int stationID; // measurement station ID

  String filename = ""; // a filename for the data file
  String* headerInformation; // the header information for the data file
  int numHeaderLines = 3;

  /*************** SnowPinger object destructor ***************/
  // destroy a SnowPinger object. Frees the dynamically allocated memory (called
  // automatically once object goes out of scope)
  ~SnowPinger() {

    // deallocate header info memory
    delete [] headerInformation;
    // zero out the address
    headerInformation = 0;
  }


  /*************** SnowPinger object constructor ***************/
  // Constructor. This takes in the input information and creates the header
  // information for the data file.
  // IMPT: This uses dynamically allocated memory via `new`! You _must_ free
  // the header information via the SnowPinger destructor method when you are done.
  SnowPinger(HardwareSerial *pinger_Bus, int power_pin, int station_ID) {

    // save the pinger bus
    this->pingerBus = pinger_Bus;

    // save the power pin
    powerPin = power_pin;

    // save the station id
    stationID = station_ID;

    // power up the pinger
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);

    // try talking to the pinger
    pingerBus->begin(9600);
    delay(800);

    if (!pingerBus->available()) {
      // print error
      Serial.println("Error opening snow pinger serial comms.");
    }

    // now create the data filename
    filename += "stn";
    filename += stationID;
    filename += "_p.txt"; // p for pinger

    // define the header information
    headerInformation = new String[numHeaderLines];

    headerInformation[0] = "Snow pinger";

    headerInformation[1] = "Station ID: ";
    headerInformation[1] += stationID;

    headerInformation[2] = "Date, Time";
    headerInformation[2] += ", Snow pinger range [cm]";

    // if the file doesn't already exist
    if (!SD.exists(filename)) {

      Serial.print("Creating datafile: ");
      Serial.println(filename);

      // create the file
      File dataFile = SD.open(filename, FILE_WRITE);

      // write the header information
      // if the file is available
      if (dataFile) {

        // print the header information to the file
        for (int i = 0; i < numHeaderLines; i++) {

          dataFile.println(headerInformation[i]);

          // also print to serial to confirm
          Serial.println(headerInformation[i]);
        }

        // close the file
        dataFile.close();
      }
    }

    // shut down the pinger until we need it
    digitalWrite(powerPin, LOW);
  }


  /*************** SnowPinger readSnowPinger ***************/
  // Reads a snow pinger, returning a string with a timestamp and the
  // range value [cm] for the snow pinger
  // inputs: timestamp
  String readSnowPinger(String date, String time, int nPingerSamples, int maxSampleAttempts) {

    // write the power pin high
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);

    // establish serial comms with the pinger
    pingerBus->begin(9600);
    delay(1000);

    // declare a string to hold the read data
    String readString = "";

    // throw the timestamp on there
    readString = date + ", " + time;

    // add our comma
    readString += ", ";

    // query the pinger
    if (pingerBus->available()) {

      String pingerReadout; // a string to hold what's read off the pinger
      String pingerReturn; // a string to return the cleaned pinger info

      // TODO: Include a timer here to protect against errors. We don't want to get into
      // a situation where the pinger is just spitting back garbage (so Serial1.available() still
      // returns true) but this just hangs the board since it never spits out '\r'
      // and possibly runs us out of memory.
      // set a timeout for reading the serial line
      pingerBus->setTimeout(PINGER_TIMEOUT);

      // declare a float to hold the pinger value samples before averaging
      float runningSum = 0;

      // keep track of how many samples we've successfully collected
      int nGoodSamples = 0;

      int nLoops = 0;

      // loop while we have fewer than nPingerSamples
      do {

        // increment our loop counter
        nLoops++;

        // get the pinger readout
        // The format of the pingers output is ascii: Rxxxx\r where x is a digit
        // Thus we can read the streaming input until we catch '\r'
        pingerReadout = pingerBus->readStringUntil('\r');

        // copy pinger readout into the return string starting after the first char
        pingerReturn = pingerReadout.substring(1);

        // if the readout doesn't conform to the R + 4 digits format
        if ((pingerReadout[0] != 'R') || (pingerReadout.length()!= 5)) {

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

        // if it's a nan reading
        if (pingerReturn == "NaN") {
          // // add it directly to the readString
          // readString += pingerReturn;

          // continue to the next reading
          continue;

          // else it's numerical
        } else {

          // convert the pinger reading to float cm
          unsigned long pingerReturn_long_mm = pingerReturn.toInt();
          float pingerReturn_float_cm = (float)pingerReturn_long_mm/10;

          // and add to the running sum
          runningSum += pingerReturn_float_cm;

          // increment our counter
          nGoodSamples++;
        }

        // while we have fewer than nPingerSamples and we haven't timed out
      } while (nGoodSamples < nPingerSamples & nLoops < maxSampleAttempts);

      // if we failed to get tht req'd number of samples before timing out
      if (nGoodSamples < nPingerSamples) {

        // set readstring to NaN
        readString += "NaN";

        Serial.println("Failed to get req'd samples.");

      } else {

        // average the running sum
        float pingerAverage_float_cm = runningSum/nPingerSamples;

        // round off and convert to int
        int pingerAverage_int_cm = (int) round(pingerAverage_float_cm);

        // then add to the string
        readString += pingerAverage_int_cm;
      }

      // if we weren't able to talk to the pinger
    } else {

      // write a nan to the data string
      readString += "NaN";
    }

    // write the power pin low
    digitalWrite(powerPin, LOW);

    // return the data string
    return readString;
  }
};



/************ TEST SCRIPT ************/

// setup function
void setup() {

  // setup the board
  boardSetup();

  // ***** IMPORTANT DELAY FOR CODE UPLOAD BEFORE USB PORT DETACH DURING SLEEP *****
  delay(5000);

  // Start serial communications
  Serial.begin(9600);
  while (!Serial); // Wait for serial comms

  // init the SD
  init_SD();

  // init the radio
  init_Radio();

  // init RTC
  init_RTC();

  Serial.println("Board initialized successfully. Initial file creation and test data-write to follow.");
  // Serial.println("–––––––––––––––");
  // alarm_one_routine();
  Serial.println("–––––––––––––––");
  Serial.println("Subsequent data sampling will occur beginning on the 0th multiple of the sampling period.");
  Serial.println("For example, if you are sampling every half hour, the next sample will occur at the top of the next hour, and subsequently every 30 minutes.");
  Serial.println("If you are sampling every 30 seconds, the next sample will occur at the top of the next minute, and subsequently every 30 seconds.");
  Serial.println("–––––––––––––––");
  Serial.println("Have a great field deployment :)");
  Serial.println("");
}

/************ main loop ************/
void loop(void) {

  init_SD();
  init_Radio();

  /************ object instantiation ************/

  // get a static temp sensors object. Created once, will persist throughout program lifetime.
  static TempSensors tempSensors_object = TempSensors(ONE_WIRE_BUS, TEMP_POWER, NUM_TEMP_SENSORS, STATION_ID);
  // get a static pinger object. Created once, will persist throughout program lifetime.
  static SnowPinger pinger_object = SnowPinger(&PINGER_BUS, PINGER_POWER, STATION_ID);

  /************ data collection ************/

  // read the data
  String tempDataString = tempSensors_object.readTempSensors(getDateString(), getTimeString());
  String pingerDataString = pinger_object.readSnowPinger(getDateString(),getTimeString(),N_PINGERSAMPLES,MAX_PINGER_SAMPLE_ATTEMPTS);

  // test print the data
  Serial.println(tempDataString);
  Serial.println(pingerDataString);


  /************ file write ************/

  // write temp data to file
  writeToFile(tempSensors_object.filename, tempDataString);

  // write pinger data to file
  writeToFile(pinger_object.filename,pingerDataString);


  /************ radio transmission ************/
  // send the temp sensors file to server
  radio_transmit(tempSensors_object.filename);

  // send the snow pinger file to server
  radio_transmit(pinger_object.filename);


  /************ alarm schedule ************/
  // schedule the next alarm
  alarm_one();

  if (Serial){
    USBDevice.detach();
  }

  // deinit everything
  // deinit_SD();

  // Switch unused pins as input and enabled built-in pullup
  for (int pinNumber = 0; pinNumber < 23; pinNumber++)
  {
    pinMode(pinNumber, INPUT_PULLUP);
  }

  for (int pinNumber = 32; pinNumber < 42; pinNumber++)
  {
    pinMode(pinNumber, INPUT_PULLUP);
  }

  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

  if (!driver.init()){}
  driver.sleep();

  // Sleep until next alarm match
  rtc.standbyMode();

  // debug
  if (DEBUG == true) {
    trigger = false;
    while (!trigger);
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

  // put the flash chip to sleep since we are using SD
  SerialFlash.begin(flashChipSelect);
  SerialFlash.sleep();
}

void deinit_SD(){

  // digitalWrite(SD_POWER,LOW);

}

/************ init_SD ************/
void init_SD() {

  delay(100);
  // set SS pins high for turning off radio
  pinMode(RADIO_CS, OUTPUT);
  delay(500);
  digitalWrite(RADIO_CS, HIGH);
  delay(500);
  //
  // pinMode(SD_POWER, OUTPUT);
  // digitalWrite(SD_POWER,HIGH);

  pinMode(SD_CS, OUTPUT);
  delay(1000);
  digitalWrite(SD_CS, LOW);
  delay(2000);
  SD.begin(SD_CS);
  delay(2000);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD initialization failed!");
  }
}


/************ init_RTC() ************/
/*
Function to sync the RTC to compile-time timestamp
*/
bool init_RTC() {

  // get the date and time the compiler was run
  uint8_t dateArray[3];
  uint8_t timeArray[3];

  if (getDate(__DATE__, dateArray) && getTime(__TIME__, timeArray)) {

    rtc.begin();
    rtc.setTime(timeArray[1], timeArray[2], timeArray[3]);
    rtc.setDate(dateArray[3], dateArray[2], dateArray[1]);

  } else { // if failed, hang
    Serial.println("failed to init RTC");
    while (1);
  }
}


/************ init_Radio() ************/
/*
Function to initialize the radio
*/
void init_Radio() {

  // wait while the radio initializes
  while (!manager.init()) {
  }

  manager.setTimeout(5000); // set timeout to 1 second

  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  driver.setTxPower(23, false);
  driver.setFrequency(RADIO_FREQ);

  // print surccess
  Serial.println("Client radio initialized successfully");
}

/************ alarm_one ************/
/*
Function to set RTC alarm

TODO: at this point, we are incrementing all intervals at once. This is only
accurate as long as we are not doing a mixed definition sampling interval,
e.g. as long as we define our sampling interval as some number of minutes
ONLY (every 15 mins, e.g.), and not as some number of hours and minutes

takes:
bool initFlag: indicates whether this is the initial instance of the alarm.
If it is, then set the sample times as zero to sample first at the top of the interval
*/
// bool initFlag
void alarm_one() {

  // if any of the intervals are defined as zero, redefine them as their max value
  // if (SAMPLING_INTERVAL_SEC == 0) {
  //   SAMPLING_INTERVAL_SEC = 60;
  // }
  // if (SAMPLING_INTERVAL_MIN == 0) {
  //   SAMPLING_INTERVAL_MIN = 60;
  // }
  // if (SAMPLING_INTERVAL_HOUR == 0) {
  //   SAMPLING_INTERVAL_HOUR = 24;
  // }


  // static uint8_t sampleSecond;
  // static uint8_t sampleMinute;
  // static uint8_t sampleHour;
  // static uint8_t nSecSamples = 0;
  // static uint8_t nMinSamples = 0;
  // static uint8_t nHourSamples = 0;

  // if our sample definitions are null, create them
  // if (initFlag == TRUE){
  // if (sampleSecond == NULL){
  //   sampleSecond = 0;
  //   sampleMinute = 0;
  //   sampleHour = 0;
  // } else {
  //   // otherwise define the next sample to be taken. e.g. if we are sampling every 15
  //   // seconds, and the next will be the second sample (n=1), then we will have:
  //   // sampleSecond = (1*15)%60 = 15.
  //   sampleSecond = (nSecSamples * SAMPLING_INTERVAL_SEC) % 60;
  //   sampleMinute = (nMinSamples * SAMPLING_INTERVAL_MIN) % 60;
  //   sampleHour = (nHourSamples * SAMPLING_INTERVAL_HOUR) % 24;
  // }

  // sampleSecond = (nSecSamples * SAMPLING_INTERVAL_SEC) % 60;
  // sampleMinute = (nMinSamples * SAMPLING_INTERVAL_MIN) % 60;
  // sampleHour = (nHourSamples * SAMPLING_INTERVAL_HOUR) % 24;
  //
  // // increment the counter for the number of subinterval samples we've taken
  // nSecSamples = ((sampleSecond + SAMPLING_INTERVAL_SEC) % 60) / SAMPLING_INTERVAL_SEC;
  // nMinSamples = ((sampleMinute + SAMPLING_INTERVAL_MIN) % 60) / SAMPLING_INTERVAL_MIN;
  // nHourSamples = ((sampleHour + SAMPLING_INTERVAL_HOUR) % 24) / SAMPLING_INTERVAL_HOUR;

  // then set the alarm and define the interrupt
  // rtc.setAlarmTime(sampleHour, sampleMinute, sampleSecond);
  rtc.setAlarmMinutes(ALARM_MINUTES);
  // rtc.enableAlarm(rtc.MATCH_MMSS);
  rtc.enableAlarm(rtc.MATCH_SS);
  // // if we're sampling at some nHour interval
  // if (SAMPLING_INTERVAL_HOUR != 24) {
  //   rtc.enableAlarm(rtc.MATCH_HHMMSS);
  // }
  // // if we're sampling at some nMin interval
  // else if (SAMPLING_INTERVAL_MIN != 60) {
  //   rtc.enableAlarm(rtc.MATCH_MMSS);
  // }
  // // if we're sampling at some nSec interval
  // else if (SAMPLING_INTERVAL_SEC != 60) {
  //   rtc.enableAlarm(rtc.MATCH_SS);
  // }

  rtc.attachInterrupt(alarm_one_routine);
}

/************ alarm_one_routine ************/
/*
Dummy routine for alarm match. Nothing happens here, just kicks back to main
loop upon alarm
*/
void alarm_one_routine() {
  if (DEBUG == true) {
    trigger = true;
  }
}


/************ writeToFile ************/
/*
function for writing generic data to generic file
*/
void writeToFile(String filename, String dataString) {

  // open the file for writing
  File dataFile = SD.open(filename, FILE_WRITE);

  // if the file is available
  if (dataFile) {

    // write the data
    dataFile.println(dataString);

    // close the file
    dataFile.close();

    // flash LED to indicate successful data write
    for (int i = 0; i < 5; i++) {
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
    }
  }
}


/************ radio transmission ************/
/*
function to transmit a generic data file to the server
*/
void radio_transmit(String filename) {

  // send a handshake message (1 byte for message type, then size of filename)
  int filenameLength = (filename).length() + 1; // get the length of the filename
  char charFilename[filenameLength]; // allocate array to hold filename as chars
  (filename).toCharArray(charFilename,filenameLength); // convert to a char array
  uint8_t handShake[1 + filenameLength]; // allocate memory for handshake message
  handShake[0] = (uint8_t) 0; // write our message type code in (0 for handshake, 1 for data)
  memcpy(&handShake[1], charFilename, filenameLength); // copy the filename in

  // Dont put this on the stack:
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  // char msg[sizeof(buf)];

  // set number of transmission attempts to 0
  int nTransmissionAttempts = 0;

  // // goto label for radio transmission block
  // handshake:

  do {

    Serial.println("Attempting to send a message to the server");

    // manager.sendtoWait((uint8_t*) dataString.c_str(), dataString.length()+1, SERVER_ADDRESS))

    // disable intterupts during comms
    // noInterrupts();

    // send the initial handshake message to the server
    if (manager.sendtoWait((uint8_t*) handShake, sizeof(handShake), SERVER_ADDRESS)) {

      // Now wait for the reply from the server telling us how much of the data file it has
      uint8_t len = sizeof(buf);
      uint8_t from;
      if (manager.recvfromAckTimeout(buf, &len, TIMEOUT, &from)) {

        // print the server reply
        // strcpy(msg, (char*)buf);
        // strcat(msg, " ");
        // convert to an integer
        // unsigned long serverFileLength = *msg;
        Serial.print("Received a handshake from server with length: ");
        Serial.println(len,DEC);
        Serial.println("");

        unsigned long serverFileLength = *(unsigned long*)buf;

        Serial.print("Server file length: ");
        Serial.println(serverFileLength, DEC);

        // open the file for writing
        File dataFile = SD.open(filename, FILE_READ);

        // if the file is available
        if (dataFile) {

          // seek to the end of the file
          // get the file length
          unsigned long stationFileLength = dataFile.size();

          Serial.print("Station file length: ");
          Serial.println(stationFileLength, DEC);

          // if the file is longer than what the server has
          if (serverFileLength < stationFileLength) {

            Serial.println("Sending: ");

            // seek to that point in the file
            dataFile.seek(serverFileLength);

            // create a buffer with max message length
            uint8_t sendBuf[RH_RF95_MAX_MESSAGE_LEN];

            // leave some room to prevent buffer overflow
            uint8_t sendLength = RH_RF95_MAX_MESSAGE_LEN - 4;

            // while we haven't sent all information
            while (stationFileLength > dataFile.position()) {

              memset(sendBuf, 0, sizeof(sendBuf));

              // put our message type code in (1 for data)
              sendBuf[0] = (uint8_t) 1;

              // read a 256 byte chunk
              int numBytesRead = dataFile.readBytes(&sendBuf[1], sendLength-1);

              // print the data that we're going to send
              // Serial.println("Sending the following data to the server: ");
              Serial.print((char*) sendBuf);
              // Serial.println("");

              // send the data to the server
              manager.sendtoWait((uint8_t*) sendBuf, sendLength, SERVER_ADDRESS);
            }
          }

          // close the file
          dataFile.close();
        }

        // send a closure message
        uint8_t closureMessage = 0;
        manager.sendtoWait((uint8_t*) &closureMessage, sizeof(&closureMessage), SERVER_ADDRESS);

        // break out of the loop
        break;
      }
      else {

        // in this case, the server received a message but we haven't gotten a
        // handshake back. the server is busy with another client. try again later
        Serial.println("Server's busy, gonna wait a few seconds.");

        // increment the number of attempts we've made
        nTransmissionAttempts++;

        // build in a random delay between 10 and 20 seconds
        // TODO: in the future, make this a multiple of station ID to prevent collision?
        delay(1000*random(10,21));

        // jump back to the handshake and try again
        // goto handshake;

        // continue next cycle of the loop
        continue;
      }
    }
    else {
      // in this case, the server did not recieve the message. we'll try again at
      // the next sampling instance.
      Serial.println("Server failed to acknowledge receipt");
      break;
    }

    // try again if we failed and haven't reached max attempts
  } while (nTransmissionAttempts < MAX_TRANSMISSION_ATTEMPTS);
}


/************ getTime() ************/
/*
Function to parse time from system __TIME__ string. From Paul Stoffregen
DS1307RTC library example SetTime.ino
*/
bool getTime(const char *str, uint8_t* timeArray) {
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;

  // pack everything into a return array
  timeArray[1] = (uint8_t) Hour;
  timeArray[2] = (uint8_t) Min;
  timeArray[3] = (uint8_t) Sec;

  return true;
}

/************ getDate() ************/
/*
Function to parse date from system __TIME__ string. Modified from Paul
Stoffregen's DS1307RTC library example SetTime.ino
*/
bool getDate(const char *str, uint8_t *dateArray) {
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;

  // convert the int year into a string
  String twoDigitYear = String(Year);
  // extract the last two digits of the year
  twoDigitYear = twoDigitYear.substring(2);

  // pack everything into a return array as uint8_ts
  dateArray[1] = (uint8_t) twoDigitYear.toInt();
  dateArray[2] = monthIndex + 1;
  dateArray[3] = (uint8_t) Day;

  return true;
}

/************ getDateString() ************/
/*
Function to get the current datestring from RTC
*/
String getDateString(void) {

  // get date and timestamps
  String dateString = "";
  dateString = String(20) + String(rtc.getYear()) + ".";

  int month = rtc.getMonth();
  int day = rtc.getDay();

  // append months and days to datestamp, 0 padding if necessary
  if (month < 10) {
    dateString += String(0) + String(month) + ".";
  } else {
    dateString += String(month) + ".";
  }

  if (day < 10) {
    dateString += String(0) + String(day);
  } else {
    dateString += String(day);
  }

  // return the datestring
  return dateString;
}

/************ getTimeString() ************/
/*
Function to get the current timestring from RTC
*/
String getTimeString(void) {

  // allocate an empty string
  String timeString = "";
  int hours = rtc.getHours();
  int mins = rtc.getMinutes();
  int secs = rtc.getSeconds();

  // append hours, mins, secs to the timestamp, 0 padding if necessary
  if (hours < 10) {
    timeString = String(0) + String(hours) + ":";
  } else {
    timeString = String(hours) + ":";
  }

  if (mins < 10) {
    timeString += String(0) + String(mins) + ":";
  } else {
    timeString += String(mins) + ":";
  }

  if (secs < 10) {
    timeString += String(0) + String(secs);
  } else {
    timeString += String(secs);
  }

  // return the timestring
  return timeString;
}
