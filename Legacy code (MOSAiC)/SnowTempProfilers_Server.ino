/* 
   basic server for receiving data from readWriteSleepTransmit, printing data, and
   saving to files

   Ian Raphael 06/29/19
   ianaraphael@gmail.com

*/

/************ INCLUDES ************/
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include <SD.h>

/************ MACROS ************/
#define SERVER_ADDRESS 2 // address for receiver (central server/base stn.)
#define RFM95_CS 5 // radio pin
#define RFM95_INT 2 // radio pin
#define RF95_FREQ 915.0 // radio frequency
#define cardSelect 4 // sd card chip select
#define LED 13

/************ GLOBALS ************/

// File stuff
int numSensors = 2;
int writeBufSize = 2 * numSensors; // size of the buffer for writing temp to file
int readBufSize = 2 * numSensors; // size of the buffer for reading temp off of file
int fileSize = 2 * numSensors; // size of the file for holding temps

// Radio
RH_RF95 driver(RFM95_CS, RFM95_INT); // radio driver
RHReliableDatagram manager(driver, SERVER_ADDRESS); // delivery/receipt manager
uint8_t msgBuf[RH_RF95_MAX_MESSAGE_LEN]; // buf for TX/RX handshake messages
byte msg[RH_RF95_MAX_MESSAGE_LEN];
byte snr[2];


/************ setup() ************/
void setup() {


  /************ Board setup ************/

  unsigned char pinNumber;
  // Pullup all unused pins to prevent floating vals
  for (pinNumber = 0; pinNumber < 23; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  for (pinNumber = 32; pinNumber < 42; pinNumber++) {
    pinMode(pinNumber, INPUT_PULLUP);
  }
  pinMode(25, INPUT_PULLUP);
  pinMode(26, INPUT_PULLUP);
  pinMode(LED, HIGH);


  /************ USB monitor init ************/
  while (!Serial) ; // Wait for serial port to be available
  Serial.begin(9600);
  Serial.println("Server online");
  delay(100);


  // if failure to init the manager
  if (!manager.init()) {

    // print error
    Serial.println("Unable to init server radio");
    while (1);
  }
  // otherwise print success
  Serial.println("Server radio online");

  // set parameters
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
}


void loop() {

  if (manager.available())
  {
    // Wait for a message addressed to us from the client
    uint8_t len = sizeof(msgBuf);
    uint8_t from;

    if (manager.recvfromAck(msgBuf, &len, &from)) {
      Serial.print("Logger ID ");
      Serial.print(from, DEC);
      Serial.println(": ");

    }

    // get the size of the filename
    int filenameSize = msgBuf[0]+2;

    // get the filename
    char filename[filenameSize + 1];
    int i;
    for (i = 1; i < filenameSize + 1; i++) {
      filename[i - 1] = msgBuf[i];
    }
    filename[filenameSize] = '\0';

    // create a file object for the data
    File file;

    // allocate a buffer to hold the logger id for a sec
    char charIDBuf[3];
    
    // convert uint8_t id to char, store in buffer
    itoa(from, charIDBuf, 10);

    // if it's a one digit number
    if (strlen(charIDBuf) == 1) {
  
      // shift null terminating character right
      charIDBuf[2] = '\0';
  
      // shift original digit right
      charIDBuf[1] = charIDBuf[0];
  
      // pad with a 0
      charIDBuf[0] = '0';

    }

    // allocate a buffer for the new filename (2 for ID, 1 for underscore, filename, terminal)
    char newFileName[4+filenameSize];

    // cat the logger ID
    strcat(newFileName, charIDBuf);
    
    // concatenate an underscore
    strcat(newFileName, "_");
    
    // concatenate timestamp
    strcat(newFileName,filename);

    Serial.print("Filename: ");
    Serial.println((char *)filename);

    // if unable to init the SD card
    while (!SD.begin(cardSelect)) {
      Serial.println("Unable to init SD card");
    }

    // open a new file
    file = SD.open(newFileName, FILE_WRITE);
    if(!file) {
      Serial.print("Couldnt create "); 
      Serial.println(file);
    }

    // print to the file
    file.write(msgBuf,len);

    file.close();
    
    // get the number of temp readings
    int nTemps = msgBuf[i];
    i++;

    // init a buffer to hold temps
    // TODO: error check by multiplying 2x–buffer overflow?
    int16_t tempArray[nTemps];

    // for every temperature
    for (int j = 0; j < nTemps; j++) {

      // get a temp
      tempArray[j] = (int16_t) word(msgBuf[i], msgBuf[i + 1]);

      // increment i twice (read two bytes)
      i += 2;
    }

    // get the pinger value
    int pingerOutput = word(msgBuf[i], msgBuf[i + 1]);
    i += 2;

    // get the adc reading
    short adcRead = word(msgBuf[i], msgBuf[i + 1]);
    i += 2;

    // convert to voltage
    float battVoltage = ((float)  adcRead) * 1 / 1024.0;

    // allocate array to hold the thermo addresses
    uint8_t addr[nTemps][8];

    // get the number of addresses
    int nAddr = msgBuf[i];
    i++;

    int j = i;
    
    // for every address
    for (int currAddr = 0; currAddr < nAddr; currAddr++) {

      // for every byte
      for (int currByte = 0; currByte < 8; currByte++) {

        // get the byte from the msgbuf
        addr[currAddr][currByte] = msgBuf[i];

        // increment i
        i++;
      }
    }
    
    Serial.print("  Temps: ");
    for (i = 0; i < nAddr; i++) {
      for (int currByte = 0; currByte < 8; currByte++) {
        Serial.print(addr[i][currByte], HEX);
      }
      Serial.print("|");
      Serial.println(tempArray[i], DEC);
    }

    Serial.print("  Pinger: ");
    Serial.println(pingerOutput, DEC);

    Serial.print("  Parsed battery voltage: ");
    Serial.println(battVoltage, 2);
  }
}
