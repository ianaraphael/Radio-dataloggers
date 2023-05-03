/*

client_arcwatch_v0.ino

Client code for ArcWatch SnowTATOS

Ian Raphael
ian.th@dartmouth.edu
2023.05.01
*/

// debug
bool DEBUG = false;
bool volatile trigger = true;

/***************!!!!!! Station settings !!!!!!!***************/
#define STATION_ID 1 // station ID
#define NUM_TEMP_SENSORS 1 // number of sensors

// uint8_t SAMPLING_INTERVAL_MIN = 240; // four hour sampling
uint8_t SAMPLING_INTERVAL_MIN = 1; // one minute sampling
uint8_t ALARM_MINUTES = 0; // minute to sample on

/*************** packages ***************/
#include <SnowTATOS.h>


/*************** defines, macros, globals ***************/

// misc.
#define Serial SerialUSB

// Temp sensors
#define ONE_WIRE_BUS 7 // temp probe data line
#define TEMP_POWER 8 // temp probe power line

// pinger
#define PINGER_BUS Serial1 // serial port to read pinger
#define PINGER_POWER 11 // pinger power line
#define N_PINGERSAMPLES 5 // number of samples to average for each pinger reading
#define MAX_PINGER_SAMPLE_ATTEMPTS 30 // maximum number of samples to attempt in order to achieve N_PINGERSAMPLES successfully


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
  String tempDataString = tempSensors_object.readTempSensors_dateStamp(getDateString(), getTimeString());
  String pingerDataString = pinger_object.readSnowPinger_dateStamp(getDateString(),getTimeString(),N_PINGERSAMPLES,MAX_PINGER_SAMPLE_ATTEMPTS);

  if (DEBUG == true) {
    // test print the data
    Serial.println(tempDataString);
    Serial.println(pingerDataString);
  }

  /************ file write ************/

  // write temp data to file
  writeToFile(tempSensors_object.filename, tempDataString);

  // write pinger data to file
  writeToFile(pinger_object.filename,pingerDataString);

  /************ radio transmission ************/
  // send the temp sensors file to server
  sendData_fromClient(tempSensors_object.filename);

  // send the snow pinger file to server
  sendData_fromClient(pinger_object.filename);


  /************ alarm schedule ************/
  // schedule the next alarm
  alarm_one();

  if (Serial){
    USBDevice.detach();
  }

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

  // write the led low
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

/************ alarm_one ************/
/*
Function to set RTC alarm

takes:
bool initFlag: indicates whether this is the initial instance of the alarm.
If it is, then set the sample times as zero to sample first at the top of the interval
*/
// bool initFlag
void alarm_one() {

  // always sample on the 0th second
  rtc.setAlarmSeconds(0);

  // if we're sampling at less than an hourly rate
  if (SAMPLING_INTERVAL_MIN < 60) {

    // set for the next elapsed interval
    ALARM_MINUTES = rtc.getMinutes() + SAMPLING_INTERVAL_MIN;
    rtc.setAlarmMinutes(ALARM_MINUTES);

    // and enable the alarm to match
    rtc.enableAlarm(rtc.MATCH_MMSS);

    // otherwise
  } else if(SAMPLING_INTERVAL_MIN >= 60) {

    // figure out how long our interval is in hours
    uint8_t SAMPLING_INTERVAL_HOUR = SAMPLING_INTERVAL_MIN/60;

    // now figure out which sampling interval we're in
    uint8_t currSamplingHour = (floor(rtc.getHours()/SAMPLING_INTERVAL_HOUR)*SAMPLING_INTERVAL_HOUR);

    // and add an interval to get to the next one
    uint8_t ALARM_HOURS = SAMPLING_INTERVAL_HOUR;

    // if it's supposed to be midnight
    if (ALARM_HOURS == 24) {
      // make it 0
      ALARM_HOURS = 0;
    }

    // sample on the 0th minute
    rtc.setAlarmMinutes(0);
    // and set hours
    rtc.setAlarmHours(ALARM_HOURS);

    // and enable the alarm to match
    rtc.enableAlarm(rtc.MATCH_HHMMSS);
  }

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
