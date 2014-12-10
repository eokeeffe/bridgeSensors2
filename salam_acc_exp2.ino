// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include <Wire.h>
#include "RTClib.h"

// SD card datalogger headers
#include <SPI.h>
#include <SD.h>
// RF24 
#include <RF24_config.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>


// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 10;

RTC_DS1307 rtc;
File dataFile;
DateTime now;
long time_start=0,time_end=0;

//accelerometer zeroing
const static int _zeroOffset = 478;

void setup () {
  Serial.begin(57600);
  while(!Serial){}//wait for port connection
  
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino Due
#endif
  rtc.begin();

  if (! rtc.isrunning()) 
  {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  now = rtc.now();
  time_start = now.unixtime();
  
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SS, OUTPUT);
  
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) 
  {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1) ;
  }
  Serial.println("card initialized.");
  
  // Open up the file we're going to log to!
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  }
  
   dataFile.print("x");
   dataFile.print(",");
   dataFile.print("y");
   dataFile.print(",");
   dataFile.print("z");
   dataFile.print(",");
   dataFile.print("Total Seconds");
   dataFile.println();
  
}

void loop () 
{
    float acc1_x_value = (float)(analogRead(A0) - _zeroOffset)/96;
    float acc1_y_value = (float)(analogRead(A1) - _zeroOffset)/96;
    float acc1_z_value = (float)(analogRead(A2) - _zeroOffset)/96;
  
    now = rtc.now();
    time_end = now.unixtime();
    long total = time_end - time_start;
    
    dataFile.print(acc1_x_value);
    dataFile.print(",");
    dataFile.print(acc1_y_value);
    dataFile.print(",");
    dataFile.print(acc1_z_value);
    dataFile.print(",");
    dataFile.print(total);
    dataFile.println();
    
    // The following line will 'save' the file to the SD card after every
    // line of data - this will use more power and slow down how much data
    // you can read but it's safer! 
    // If you want to speed up the system, remove the call to flush() and it
    // will save the file only every 512 bytes - every time a sector on the 
    // SD card is filled with data.
    dataFile.flush();
    
    delay(50);
}
