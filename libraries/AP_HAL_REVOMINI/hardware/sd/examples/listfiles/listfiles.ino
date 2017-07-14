/*
  Listfiles

 This example shows how print out the files in a
 directory on a SD card

 The circuit:
 * SD card attached

 This example code is in the public domain.

 */
#include <SD.h>

File root;

void setup()
{
  // Open serial communications and wait for port to open:
  SerialUSB.begin(9600);
  while (!SerialUSB) {
    ; // wait for Serial port to connect. Needed for Leonardo only
  }

  SerialUSB.print("Initializing SD card...");
  while (SD.begin(SD_DETECT_PIN) != TRUE)
  {
    delay(10);
  }
  SerialUSB.println("initialization done.");

  root = SD.open("/");
  if(root)
    printDirectory(root, 0);
  else
    SerialUSB.println("Could not open root");
  delay(2000);
  SerialUSB.println();
  SerialUSB.println("Rewinding, and repeating below:" );
  SerialUSB.println();
  delay(2000);
  root.rewindDirectory();
  printDirectory(root, 0);
  root.close();
}

void loop()
{
  // nothing happens after setup finishes.
}

void printDirectory(File dir, int numTabs) {
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       SerialUSB.print('\t');

     }
     SerialUSB.print(entry.name());
     if (entry.isDirectory()) {

       SerialUSB.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       SerialUSB.print("\t\t");
       SerialUSB.println(entry.size(), DEC);
     }
     entry.close();
   }
}
