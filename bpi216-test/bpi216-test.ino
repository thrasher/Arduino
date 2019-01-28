/***
 * BPI216 LCD Display Test
 * Works with this part: https://www.seetron.com/bpi216/index.html
 * 
 * LCD pin Connections:
 * white (serial in) to Arduino pin 3
 * red (5V) to Arduino 5V
 * black (GND) to Arduion GND
 */
#include <SoftwareSerial.h> // for BPI216 LCD display

#define rxPin 255       // Not used, so set to invalid pin #
#define txPin 3         // Connect BPI/BPK's SER input to this pin.
#define inverted 1     // In setup, 1=inverted, 0=noninverted

const char clearScreen[]  = {254,1,254,128,0};
const char bpiLine1[] = {254,128,0};
const char bpiLine2[] = {254,192,0};

int count=0;

/*
   Set up a new serial output using the pin definitions above. Note the
 argument "inverted," which instructs SoftwareSerial to output BPI/BPK-
 compatible inverted-TTL serial (like RS-232, but without the +/-
 voltage swing).*/
SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin, inverted);

void setup() {
  // put your setup code here, to run once:
  // define pin mode for tx:
  digitalWrite(txPin, LOW); // Stop bit state for inverted serial
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600); // Set the data rate
  mySerial.write("\xff"); // write a dummy character to fix setTX bug
  //  delay(10);              // wait (may not be needed w/ Arduino v1.0)
  mySerial.print(clearScreen);
  mySerial.print("Starting Arduino");
}

void loop() {
  // put your main code here, to run repeatedly:
  mySerial.print(clearScreen);
  
  mySerial.print(bpiLine1);
  mySerial.print("Checking Sensor");
  
  mySerial.print(bpiLine2);
  mySerial.print("Waiting ");
  mySerial.print(count);
  
  delay(1000);
  
  count ++;
}
