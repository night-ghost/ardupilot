/*
  SD card basic file example

 This example shows how to create and destroy an SD card file
 The circuit:
 * SD card attached

 This example code is in the public domain.

 */
#include <SD.h>

File myFile;

void setup()
{
  // Open serial communications and wait for port to open:
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  SerialUSB.print("Initializing SD card...");

  while (SD.begin(SD_DETECT_PIN) != TRUE)
  {
    delay(10);
  }
  SerialUSB.println("initialization done.");

  if (SD.exists("example.txt")) {
    SerialUSB.println("example.txt exists.");
  }
  else {
    SerialUSB.println("example.txt doesn't exist.");
  }

  // open a new file and immediately close it:
  SerialUSB.println("Creating example.txt...");
  myFile = SD.open("example.txt", FILE_WRITE);
  myFile.close();

  // Check to see if the file exists:
  if (SD.exists("example.txt")) {
    SerialUSB.println("example.txt exists.");
  }
  else {
    SerialUSB.println("example.txt doesn't exist.");
  }

  // delete the file:
  SerialUSB.println("Removing example.txt...");
  SD.remove("example.txt");

  if (SD.exists("example.txt")) {
    SerialUSB.println("example.txt exists.");
  }
  else {
    SerialUSB.println("example.txt doesn't exist.");
  }
}

void loop()
{
  // nothing happens after setup finishes.
}
