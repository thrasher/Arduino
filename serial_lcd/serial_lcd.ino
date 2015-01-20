/*This program uses the updated SoftwareSerial distributed in v1.0 of 
the Arduino software. Users of earlier versions must download 
NewSoftSerial. */

#include <SoftwareSerial.h>

#define rxPin 255       // Not used, so set to invalid pin #
#define txPin 3         // Connect BPI/BPK's SER input to this pin.
#define inverted 1     // In setup, 1=inverted, 0=noninverted

const char clearScreen[ ] = {254,1,254,128,0};
const char message[ ] = "Hi Noah" ;
const char bpiLine1[] = {254,128,0};
const char bpiLine2[] = {254,192,0};

/* 
Set up a new serial output using the pin definitions above. Note the 
argument "inverted," which instructs SoftwareSerial to output BPI/BPK-
compatible inverted-TTL serial (like RS-232, but without the +/- 
voltage swing).*/

SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin, inverted);

void setup()  {
  // define pin mode for tx:
  digitalWrite(txPin, LOW);   // Stop bit state for inverted serial
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);    // Set the data rate
  mySerial.write("\xff") ;  // write a dummy character to fix setTX bug
//  delay(10);              // wait (may not be needed w/ Arduino v1.0)
  mySerial.print(clearScreen);
  mySerial.print("Starting Arduino");
}

void loop() {
  delay(2000);
  mySerial.print(bpiLine2);
  mySerial.print("Hi NOAH!");
  delay(2000);
  mySerial.print(clearScreen);
  mySerial.print(bpiLine1);
  mySerial.print("I'm gonna...");
  delay(2000);
  mySerial.print(bpiLine2);
  mySerial.print("WRECK IT!");
  exit(0);
}
