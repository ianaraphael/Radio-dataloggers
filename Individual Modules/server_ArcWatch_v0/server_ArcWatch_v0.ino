/*

server_arcwatch_v0.ino

Server code for ArcWatch SnowTATOS

Ian Raphael
ian.th@dartmouth.edu
2023.05.02
*/

#include <SnowTATOS_server.h>

// indicates whether we're with a client and which client it is. default to server_address when on standby.
int volatile withClient = SERVER_ADDRESS;

// indicates whether we've synchronized our local time with the network
bool synchronizedWithNetwork = false;

// declare a global file object to write stuff onto
File dataFile;

// and a global filename
String filename;

// // timer variable to keep track of when we started with a client
// unsigned long beganWithClientAt_millis = 0;

/****** setup ******/
void setup() {
  // Wait for serial port to be available
  // while (!SerialUSB);

  // start serial comms with the computer
  SerialUSB.begin(9600);

  // init the radio
  init_Radio();

  // init SD
  init_SD();

  // init the realtime clock
  init_RTC();

  // init i2c comms with the simb
  init_I2C_scSide();

  // create the

  // print success
  SerialUSB.println("Server init success");
}


void loop() {

  // ********** radio comms with clients ********** //

  // SerialUSB.print("With client for (ms): ");
  // SerialUSB.println((millis() - beganWithClientAt_millis),DEC);
  //
  // // if we ever exceed our timeout (5 mins)
  // if ((millis() - beganWithClientAt_millis) > 300000) {
  //   // reset our state
  //   withClient = SERVER_ADDRESS;
  //
  //   SerialUSB.print("transaction with client timed out.");
  //   SerialUSB.println("----------");
  //   SerialUSB.println("");
  // }

  // allocate a buffer with max packet size
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];

  // allocate same sized buffer but with char
  char msg[sizeof(buf)];

  // wipe it clean
  buf[0] = '\0';

  // if the manager isn't busy right now
  if (manager.available()) {

    // copy the length of the buffer
    uint8_t len = sizeof(buf);
    // allocate a char to store station id
    uint8_t from;

    // if we've gotten a message, receive and store it, its length, and station id
    if (manager.recvfromAck(buf, &len, &from)) {

      // if we don't know what time it is
      if (!synchronizedWithNetwork) {

        // just set our time as 00:00:00 since we're sampling on an even hourly time
        // basis. in the future, we should grab the actual time from a gps unit/SIMB/or read
        // from client data. for now, this is okay.
        rtc.setTime(0,0,0);

        // change our flag to indicate that we're synchronized
        synchronizedWithNetwork = true;
      }

      // if the message is from a client we're already dealing with *or* if we're not with a client at all (standby mode)
      // otherwise we're just going to ignore the message
      if (from == withClient || withClient == SERVER_ADDRESS) {

        // add safeguard to exit client service mode after timeout, even without recieving termination message

        // get the first char
        uint8_t messageType = buf[0];

        // and switch on the message contents
        switch (messageType) {

          // if it's a transaction begin/end message
          case 0:

          // if it's from a client we're already dealing with
          if (withClient == from) {

            // then it's a transaction termination message

            // write the newest data to the simb buffer
            getNewestData(simbData,filename);

            SerialUSB.println("finished getting the newest data");

            // switch withClient variable back to server address to indicate "standby" mode
            withClient = SERVER_ADDRESS;

            SerialUSB.print("transaction with client #");
            SerialUSB.print(from, DEC);
            SerialUSB.println(" terminated.");
            SerialUSB.println("----------");
            SerialUSB.println("");

            // if we're in standby mode
          } else if (withClient == SERVER_ADDRESS) {

            // it's a transaction initiation message; change withClient to remember which client we're talking to
            // TODO: add an alarmed timeout here so that we're not stuck dealing with a client forever
            withClient = from;

            // get the filename
            filename = "";
            filename = String((char*)&buf[1]);

            SerialUSB.print("Initiating transaction with client #");
            SerialUSB.println(from, DEC);
            SerialUSB.print("Creating/opening file: ");
            SerialUSB.println(filename);

            // init the SD card
            init_SD();

            // get the file size on our end
            unsigned long numBytesServer = getFileSize(filename);

            // and send it back as a handshake to the client
            SerialUSB.print("Sending handshake back to client. File size: ");
            SerialUSB.println(numBytesServer);
            // if the client fails to acknowledge handshake
            if (!manager.sendtoWait((uint8_t *) &numBytesServer, sizeof(&numBytesServer), from)) {
              SerialUSB.println("Client failed to acknowledge reply");
              // return to standby mode
              withClient = SERVER_ADDRESS;
            }
          }
          break;

          // case 1 means it's data
          case 1:

          // if we're in a transaction with this client
          if (withClient == from) {

            // print the data out
            SerialUSB.print((char*) &buf[1]);

            // write the data to the file
            writeToFile(filename,String((char*) &buf[1]));

            break;

            // otherwise our transaction with this client has timed out and they
            // will have to try again later. ignore this message.
          } else if (withClient == SERVER_ADDRESS) {
            break;
          }
        }
      }
    }
  }

  // ********** i2c comms with simb ********** //

  // if simb asked for data
  if (simbRequestedData()) {

    // TODO: set timeout

    // send data to simb
    sendDataToSimb(simbData);

    // now reset the buffer to all zeros
    memset(simbData,0, sizeof(simbData));
  }

  // ********** set sleep alarm ********** //

  // // if we know what time it is
  // if (synchronizedWithNetwork) {
  //
  //   // if we've been awake for long enough
  //   if (rtc.getMinutes() >= ceil(AWAKE_PERIOD/2)) {
  //
  //     // schedule the next sleep alarm
  //     setSleepAlarm_server();
  //
  //     // and go to sleep
  //     rtc.standbyMode();
  //   }
  // }
}


// // watchdog
// void setWatchDog(uint8_t minutes) {
//
// }


/************ setAlarm_server ************/
// Function to set RTC alarm for server side sleep
void setSleepAlarm_server() {

  // always wake up on the 0th second
  rtc.setAlarmSeconds(0);

  // if we're sampling at less than an hourly rate
  if (SAMPLING_INTERVAL_MIN < 60) {

    // if we've rolled over
    if (ALARM_MINUTES >= 60) {
      // reset
      ALARM_MINUTES = ALARM_MINUTES - 60;
    }

    // get the next elapsed interval
    ALARM_MINUTES = ALARM_MINUTES + SAMPLING_INTERVAL_MIN;

    // then set the alarm with awake period centered around ALARM_MINUTES
    rtc.setAlarmMinutes(ALARM_MINUTES-floor(AWAKE_PERIOD/2));

    // finally, enable the alarm to match
    rtc.enableAlarm(rtc.MATCH_MMSS);

    // otherwise
  } else if(SAMPLING_INTERVAL_MIN >= 60) {

    // figure out how long our interval is in hours
    uint8_t SAMPLING_INTERVAL_HOUR = SAMPLING_INTERVAL_MIN/60;

    // now figure out which sampling interval we're in
    uint8_t currSamplingHour = (floor(rtc.getHours()/SAMPLING_INTERVAL_HOUR)*SAMPLING_INTERVAL_HOUR);

    // and add an interval to get to the next one
    uint8_t ALARM_HOURS = SAMPLING_INTERVAL_HOUR;

    // now subtract 1 because we're going to wake up AWAKE_PERIOD/2 minutes before the hour
    ALARM_HOURS = ALARM_HOURS - 1;

    // if it's supposed to be midnight
    if (ALARM_HOURS == 24) {
      // make it 0
      ALARM_HOURS = 0;
    }

    // now get the alarm minutes as 60 - AWAKE_PERIOD/2
    ALARM_MINUTES = 60 - ceil(AWAKE_PERIOD/2);

    // set minutes
    rtc.setAlarmMinutes(ALARM_MINUTES);
    // set hours
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
}
