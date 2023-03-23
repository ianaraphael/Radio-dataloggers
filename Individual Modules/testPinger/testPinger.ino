/*

testPinger.ino

for testing pinger interference, range, angle with two pingers

Ian Raphael
ian.th@dartmouth.edu
2023.03.17
*/

// debug
bool DEBUG = false;
bool volatile trigger = true;

/***************!!!!!! Station settings !!!!!!!***************/
#define STATION_ID 1 // station ID
uint8_t SAMPLING_INTERVAL_MIN = 1; // number of minutes between samples
uint8_t ALARM_MINUTES = 0;
// uint8_t SAMPLING_INTERVAL_SEC = 0; // number of seconds between samples
#define RADIO_CS 5 // radio chip select pin

/*************** packages ***************/
#include <SerialFlash.h>
#include <SD.h>
#include <RTCZero.h>
#include <TimeLib.h>
#include <SPI.h>


/*************** defines, macros, globals ***************/
// misc.
RTCZero rtc; // real time clock object

const int flashChipSelect = 4;

// pinger
#define PINGER1_BUS Serial // serial port to read pinger1
#define PINGER2_BUS Serial1 // serial port to read pinger2

#define PINGER1_POWER 8 // pinger power line
#define PINGER2_POWER 11 // pinger power line

int pingerID_1 = 1; // pinger ID for filename
int pingerID_2 = 2; // pinger ID for filename

#define PINGER_TIMEOUT 100 // timeout to prevent hang in pinger reading (ms)
#define N_PINGERSAMPLES 5 // number of samples to average for each pinger reading
#define MAX_PINGER_SAMPLE_ATTEMPTS 30 // maximum number of samples to attempt in order to achieve N_PINGERSAMPLES successfully

// SD card
#define SD_CS 10 // SD card chip select
#define SD_CD NAN // SD card chip DETECT. Indicates presence/absence of SD card. High when card is inserted.

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
  SnowPinger(HardwareSerial *pinger_Bus, int power_pin, int station_ID, int pingerID) {

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
      SerialUSB.println("Error opening snow pinger serial comms.");
    }

    // now create the data filename
    filename += "stn";
    filename += stationID;
    filename += "_p_";
    filename += pingerID; // pinger number
    filename += ".txt"; // p for pinger

    // define the header information
    headerInformation = new String[numHeaderLines];

    headerInformation[0] = "Snow pinger";

    headerInformation[1] = "Station ID: ";
    headerInformation[1] += stationID;

    headerInformation[2] = "Date, Time";
    headerInformation[2] += ", Snow pinger range [cm]";

    // if the file doesn't already exist
    if (!SD.exists(filename)) {

      SerialUSB.print("Creating datafile: ");
      SerialUSB.println(filename);

      // create the file
      File dataFile = SD.open(filename, FILE_WRITE);

      // write the header information
      // if the file is available
      if (dataFile) {

        // print the header information to the file
        for (int i = 0; i < numHeaderLines; i++) {

          dataFile.println(headerInformation[i]);

          // also print to serial to confirm
          SerialUSB.println(headerInformation[i]);
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

        SerialUSB.println("Failed to get req'd samples.");

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



// setup function
void setup() {

  // setup the board
  boardSetup();

  // ***** IMPORTANT DELAY FOR CODE UPLOAD BEFORE USB PORT DETACH DURING SLEEP *****
  delay(5000);

  // Start serial communications
  SerialUSB.begin(9600);
  while (!SerialUSB); // Wait for serial comms

  // init the SD
  init_SD();

  // init RTC
  init_RTC();

  SerialUSB.println("Board initialized successfully. Initial file creation and test data-write to follow.");
  // SerialUSB.println("–––––––––––––––");
  // alarm_one_routine();
  SerialUSB.println("–––––––––––––––");
  SerialUSB.println("Subsequent data sampling will occur beginning on the 0th multiple of the sampling period.");
  SerialUSB.println("For example, if you are sampling every half hour, the next sample will occur at the top of the next hour, and subsequently every 30 minutes.");
  SerialUSB.println("If you are sampling every 30 seconds, the next sample will occur at the top of the next minute, and subsequently every 30 seconds.");
  SerialUSB.println("–––––––––––––––");
  SerialUSB.println("Have a great field deployment :)");
  SerialUSB.println("");
}

/************ main loop ************/
void loop(void) {


  /************ object instantiation ************/

  // get two static pinger objects. Created once, will persist throughout program lifetime.
  static SnowPinger pinger1 = SnowPinger(&PINGER1_BUS , PINGER1_POWER, STATION_ID, pingerID_1);
  static SnowPinger pinger2 = SnowPinger(&PINGER2_BUS, PINGER2_POWER, STATION_ID, pingerID_2);

  /************ data collection ************/

  // read the data
  String pingerDataString_1 = pinger1.readSnowPinger(getDateString(),getTimeString(),N_PINGERSAMPLES,MAX_PINGER_SAMPLE_ATTEMPTS);
  String pingerDataString_2 = pinger2.readSnowPinger(getDateString(),getTimeString(),N_PINGERSAMPLES,MAX_PINGER_SAMPLE_ATTEMPTS);

  // test print the data
  SerialUSB.print("Pinger 1 test: ");
  SerialUSB.println(pingerDataString_1);
  SerialUSB.print("Pinger 2 test: ");
  SerialUSB.println(pingerDataString_2);


  /************ file write ************/

  // write temp data to file

  // write pinger data to file
  writeToFile(pinger1.filename,pingerDataString_1);
  writeToFile(pinger2.filename,pingerDataString_2);

  /************ alarm schedule ************/
  // schedule the next alarm
  alarm_one();

  if (SerialUSB){
    USBDevice.detach();
  }

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

/************ init_SD ************/
void init_SD() {

  delay(100);
  // set SS pins high for turning off radio
  pinMode(RADIO_CS, OUTPUT);
  delay(500);
  digitalWrite(RADIO_CS, HIGH);
  delay(500);
  pinMode(SD_CS, OUTPUT);
  delay(1000);
  digitalWrite(SD_CS, LOW);
  delay(2000);
  SD.begin(SD_CS);
  delay(2000);
  if (!SD.begin(SD_CS)) {
    SerialUSB.println("SD initialization failed!");
    while (1);
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
    SerialUSB.println("failed to init RTC");
    while (1);
  }
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
