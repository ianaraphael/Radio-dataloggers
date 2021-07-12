**Temperature datalogger pseudocode**

*Initialize*

```
Initialize the GPS
Initialize the RTC
Initialize the sensors
  Address all therms
  Address pinger
  Read all therms
  Read pinger
Initialize the memory
  Init flash/SD card
  Create a test file
  Write the data to the test file
Initialize the radio
  Connect to the basestation
  Transmit initialization/test data
```

*Loop*

```
Init the GPS
Init the sensors
Read all of the thermistors
Read the pinger
Write to the data file
Initialize the radio
  Connect to base station
  Transmit to base station
Decom everything
Go back to sleep
Wait for alarm
```

*Read thermistors*


*Read pinger*

*Create a file given filename*

*Write to a file given filename*

*Transmit to the base station given ID*
