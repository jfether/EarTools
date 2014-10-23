
/*
 Mouse Ear Recorder
 
 (C) 2014 Jonathan Fether
 
 --
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 --
 
 This program allows recording of IR signals using a 2400 baud format.
 
 The IR signals are recieved via a IR receiver IC (Several models available).
 
 Generally you will want a 5V model with 3-pins: Ground, Power, and Signal
 
 Connect Signal to the IR Rx pin of your Arduino, and power and ground respectively.
 
 The data file is recorded to an SD card as text using the Arduino SD library with this format:
 
 XXXXXXXX: YY YY YY YY ...
 
 Where XXXXXXXX is a timestamp in milliseconds and YY YY YY YY are the contents of the buffer.

 */

// An arbitrary number to ensure the
#define NO_STAMP 3103730031

#include <SD.h>
#include <stdio.h>

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 4;

// Data buffer
byte data[256];
// Current byte index in buffer
int currentbyte = 0;
// Timeout before a line is written.
int timeout=30;
unsigned long lastime;
// Variables to compare millis() values against the top of the file.
unsigned long timestamp = NO_STAMP, basestamp=NO_STAMP;
File datafile;

void setup() {
    // initialize serial:
    Serial.begin(2400);
    Serial.print("Initializing SD card...");
    // make sure that the default chip select pin is set to
    // output, even if you don't use it:
    pinMode(10, OUTPUT);
    
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
        Serial.println("Card failed, or not present");
        // Cannot continue.
        return;
    }
    Serial.println("card initialized.");
    char filename[15];
    unsigned int filenumber=0;
    boolean s=true;
    while(s)
    {
        sprintf(filename, "mrdf%04X.txt", filenumber++);  // The four letters may be configured as desired.
        s=SD.exists(filename);
    }
    Serial.print("Filename: ");
    Serial.println(filename);
    
    datafile=SD.open(filename, FILE_WRITE);
    
    // You may place an optional header here, but you will have to skip it when reading the file back.
    // datafile.println("// Mouse Ear Recorder Text Data File");    
    lastime=millis();
}

void loop() {
    if(((millis()-lastime)>timeout)&&(currentbyte>0)) // Run only if data is in buffer and timeout (300) is reached
    {
        printbuf();  // Dump the data buffer after a timeout
    }
}

void printbuf()
{  
    // Output string buffer
    char outprint[10];
    
    // This is a noise reduction routine. If only 0xf0 is received it's probably single bit noise.
    if(currentbyte<3)
    {
        boolean abort=true;
        for(int i=0; i<currentbyte; i++) if(data[i]<0xf0) abort=false;
        if(abort) 
        {
            currentbyte=0;
            lastime=millis();    // Reset last time
            timestamp=NO_STAMP;  // Reset timestamp
            return;
        }
    }
    
    // Format the timestamp
    sprintf(outprint, "%08lX: ", timestamp);
    timestamp=NO_STAMP; // Reset timestamp
    Serial.print(outprint); // Send to serial port
    datafile.print(outprint); // Store to file

    // Format the received data
    for(int i=0; i<currentbyte; i++) {
        sprintf(outprint, "%02X ", data[i]);
        Serial.print(outprint);
        datafile.print(outprint);
    }
    Serial.println();   // Send to serial port
    datafile.println(); // Store to file
    lastime=millis();   // Reset lastime
    datafile.flush();   // Flush the file to card, in case of power loss
    currentbyte = 0;    // Reset the buffer index
}

/*
 SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */

void serialEvent() {
    if(basestamp==NO_STAMP) basestamp=millis();    // The first transmission sets the base stamp so file begins with 0msec.
    if(timestamp==NO_STAMP) timestamp=millis()-basestamp;  // Set timestamp for this buffer
    while (Serial.available()) {
        // get the new byte:
        byte inChar = (byte)Serial.read(); 
        // add it to the inputString:
        data[currentbyte++] = inChar;
        lastime=millis(); // So we don't timeout
        if(currentbyte>250) printbuf(); // Extra overflow protection
    }
}

