/*
  SD card file dump

 This example shows how to read a file from the SD card using the
 SD library and send it over the serial port.

 The circuit:
 * SD card attached

 This example code is in the public domain.

 */

#include <SD.h>

void setup()
{
  // Open serial communications and wait for port to open:
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  SerialUSB.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  while (SD.begin(SD_DETECT_PIN) != TRUE)
  {
    delay(10);
  }
  delay(100);
  SerialUSB.println("card initialized.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt");

  // if the file is available, write to it:
  if (dataFile) {
    while (dataFile.available()) {
      SerialUSB.write(dataFile.read());
    }
    dataFile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    SerialUSB.println("error opening datalog.txt");
  }
}

void loop()
{
}
