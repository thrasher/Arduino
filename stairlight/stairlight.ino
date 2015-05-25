// Stair Lights
// Manages the lighting on the stairs, providing AUTO/ON/OFF functionality based on ambient
// conditions and press-and-hold single-button switch.
// by: Jason Thrasher 4/11/2015
/*
 */

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#include <Wire.h>
#include "RTClib.h"

//#include "RunningAverage.h" // for sensor averaging calculation
#include "DHT.h" // for DHT22 temp/humidity sensor
#include "sha256.h" // for Amazon Dynamo DB

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
ADAFRUIT_CC3000_IRQ, 
ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIVIDER);
// We get the SSID & Password from memory thanks to SmartConfigCreate!

// *** pins ***
#define DHTPIN 8 // pin for DHT22 temp/humidity sensor
#define LED_GATE_PIN 9
#define A_POT_PIN A0
#define BUTTON_PIN 7

#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define THRESHOLD 70 // sensor readings above threshold will turn light off

#define     READING_DELAY_MINS     1      // Number of minutes to wait between readings.
#define     DHT_DELAY_MS           2000   // Number of milliseconds to wait between DHT 22 sensor readings
#define     TIMEOUT_MS             15000  // How long to wait (in milliseconds) for a server connection to respond (for both AWS and NTP calls).
#define     DEVICE_NAME "CC3000" // should match value set using TI SmartConfig for auto-WiFi connection

// DynamoDB table configuration
#define     TABLE_NAME             "Sensors"  // The name of the table to write results.
#define     ID_VALUE               "stair-lights"          // The value for the ID/primary key of the table.  Change this to a 
// different value each time you start a new measurement.

// Amazon AWS configuration
#define     AWS_ACCESS_KEY         "AKIAJ5GGA6DLZAVQ6C6A"                         // Put your AWS access key here.  
#define     AWS_SECRET_ACCESS_KEY  "l90OJJ1b8VKPx2A5WkW5EmrrjU3hT4bLH4aVajRb"                  // Put your AWS secret access key here.
#define     AWS_REGION             "us-west-1"                                   // The region where your dynamo DB table lives.
#define     AWS_HOST               "dynamodb.us-west-1.amazonaws.com"            // The endpoint host for where your dynamo DB table lives.

// Don't modify the below constants unless you want to play with calling other DynamoDB APIs
#define     AWS_TARGET             "DynamoDB_20120810.PutItem" // 20121017.PutItem
#define     AWS_SERVICE            "dynamodb"
#define     AWS_SIG_START          "AWS4" AWS_SECRET_ACCESS_KEY
#define     SHA256_HASH_LENGTH     32

namespace d {
  typedef struct {
    unsigned long currentTime;
    unsigned int mode; // auto=0, on=1, off=2
    unsigned int lightsense;
    unsigned int ledset;
    float humidity;
    float tempf; // Fahrenheit
    float tempc; // Celcius
    float heatindex;
  } 
  reading;

  void print(reading r) {
    Serial.print("CurrentTime: ");
    Serial.print(r.currentTime);
    Serial.print(" Mode: ");
    Serial.print(r.mode);
    Serial.print(" Lightsense: ");
    Serial.print(r.lightsense);
    Serial.print(" Ledset: ");
    Serial.print(r.ledset);
    Serial.print(" Humidity: ");
    Serial.print(r.humidity);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(r.tempc);
    Serial.print(" *C ");
    Serial.print(r.tempf);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(r.heatindex);
    Serial.println(" *F");    
  }
}

// State used to keep track of the current time and time since last temp reading.
unsigned long lastPolledTime = 0;   // Last value retrieved from time server
unsigned long sketchTime = 0;       // CPU milliseconds since last server query
unsigned long lastReading = 0;      // Time of last temperature reading.

int button_state = 1;
d::reading p;

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
//RunningAverage brRA(10); // averages sensor readings for smoother led changes

void setup(void)
{
  p.mode=0;
  p.ledset=0;

  // Start Serial
  Serial.begin(115200);
  Serial.println(F("Starting StairLight"));

  pinMode(LED_GATE_PIN, OUTPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set
  pinMode(BUTTON_PIN, INPUT);

  dht.begin(); // start the DHT library
//  brRA.clear(); // explicitly start buffer clean

  /* Try to reconnect using the details from SmartConfig          */
  /* This basically just resets the CC3000, and the auto connect  */
  /* tries to do it's magic if connections details are found      */
  Serial.println(F("Trying to reconnect using SmartConfig values ..."));

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!! Note the additional arguments in .begin that tell the   !!! */
  /* !!! app NOT to deleted previously stored connection details !!! */
  /* !!! and reconnected using the connection details in memory! !!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  if (!cc3000.begin(false, true, DEVICE_NAME))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    while(1);
  }
  Serial.println(F("WiFi Reconnected via SmartConfig!"));

  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  while (!cc3000.checkDHCP()) {
    Serial.println(F("\nWaiting for DHCP..."));
    delay(1000); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  // Get an initial time value by querying an NTP server.
  unsigned long t = getTime();
  while (t == 0) {
    // Failed to get time, try again in a minute.
    Serial.print("Getting NTP time, waiting (ms): "); 
    Serial.println(TIMEOUT_MS);
    delay(TIMEOUT_MS);
    t = getTime();
  }
  lastPolledTime = t;
  sketchTime = millis();

  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  //Serial.println(F("\nClosing the connection"));
  ///cc3000.disconnect();

  // do one temp acquisition to set values
  acquire();

  Serial.println(F("Started..."));
}

void loop() {
  switch (p.mode) {
  case 0:
    mode_auto();
    break;
  case 1:
    digitalWrite(LED_GATE_PIN, 1);
    break;
  case 2:
    digitalWrite(LED_GATE_PIN, 0);
    break;
  default:
    p.mode = 0;
  }

  button_state = digitalRead(BUTTON_PIN);
  if (button_state == 0) {
  Serial.print("button_state: ");
  Serial.println(button_state);
    //    unsigned long time;
    //    time = millis();
    int reps = 0;
    while (reps < 3) {
      flash();
      if (digitalRead(BUTTON_PIN) == 1) {
        p.mode = reps;
        break;
      }
      reps++;
      delay(1500);
    }
    Serial.print("mode is now: ");Serial.println(p.mode);
  }

  // Update the current time.
  // Note: If the sketch will run for more than ~24 hours, you probably want to query the time
  // server again to keep the current time from getting too skewed.
  p.currentTime = lastPolledTime + (millis() - sketchTime) / 1000;

  if ((p.currentTime - lastReading) >= (READING_DELAY_MINS * 60)) {
    lastReading = p.currentTime;

    // Get a temp reading
    acquire();

    // Write the result to the database.
    Serial.print(F("Writing data to DynamoDB at time: ")); 
    Serial.println(p.currentTime);
    //dynamoDBWrite(TABLE_NAME, ID_VALUE, currentTime, t);
  }

  d::print(p);
  delay(100);
}

// call every 2 seconds, maximum
void acquire() {
  p.humidity = dht.readHumidity();
  // Read temperature as Celsius
  p.tempc = dht.readTemperature();
  // Read temperature as Fahrenheit
  p.tempf = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(p.humidity) || isnan(p.tempc) || isnan(p.tempf)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  p.heatindex = dht.computeHeatIndex(p.tempf, p.humidity);
}

void mode_auto() {
  p.lightsense = analogRead(A_POT_PIN);
  int br = p.lightsense / 4;    // read the value from the sensor
  Serial.print("br: ");Serial.print(br);

  if (br < 1 ) {
    // minimum dim-ness
    br = 1;
  } 
  else if (br < THRESHOLD) {
    // accept value
  } 
  else if (THRESHOLD <= br) {
    // too bright, turn off
    br = 0;
    //mode = 2; // hysteresis hack
  }

  Serial.print(" br2: ");Serial.print(br);

//  brRA.addValue(br);
//  Serial.print(" brRAvg: ");Serial.println(brRA.getAverage());
//  p.ledset = brRA.getAverage();
  p.ledset = br;

  update_light();

  Serial.print("Measurment: ");
  Serial.println(br);

//  Serial.print("Running Average: ");
//  Serial.println(brRA.getAverage(), 3);
//
//  Serial.print("Running Count: ");
//  Serial.println(brRA.getCount(), 3);
//
//  Serial.print("Running Size: ");
//  Serial.println(brRA.getSize(), 3);

  Serial.print("led brightness: ");
  Serial.println(p.ledset);
}

void update_light() {
  if (p.ledset == 0) {
    digitalWrite(LED_GATE_PIN, 0); // note: analogWrite may glitch-flash on, so we use digital here
  } 
  else {
    analogWrite(LED_GATE_PIN, p.ledset);
  }
}

void flash() {
  digitalWrite(LED_GATE_PIN, 1);
  delay(100);
  digitalWrite(LED_GATE_PIN, 0);
  delay(200);
  digitalWrite(LED_GATE_PIN, 1);
  delay(100);
  digitalWrite(LED_GATE_PIN, 0);
  delay(200);
  digitalWrite(LED_GATE_PIN, 1);
  delay(100);
  update_light();
  delay(600);
}

// Write a temperature reading to the DynamoDB table.
void dynamoDBWrite(char* table, char* id, unsigned long currentTime, float currentTemp) {
  // Generate time and date strings
  DateTime dt(currentTime);
  // Set dateTime to the ISO8601 simple date format string.
  char dateTime[17];
  memset(dateTime, 0, 17);
  dateTime8601(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second(), dateTime);
  // Set date to just the year month and day of the ISO8601 simple date string.
  char date[9];
  memset(date, 0, 9);
  memcpy(date, dateTime, 8);
  // Set currentTimeStr to the string value of the current unix time (seconds since epoch).
  char currentTimeStr[8 * sizeof(unsigned long) + 1];
  memset(currentTimeStr, 0, 8 * sizeof(unsigned long) + 1);
  ultoa(currentTime, currentTimeStr, 10);

  // Generate string for the temperature reading.
  char temp[8 * sizeof(unsigned long) + 5];
  memset(temp, 0, 8 * sizeof(unsigned long) + 5);
  // Convert to fixed point string.  Using a proper float to string function
  // like dtostrf takes too much program memory (~1.5kb) to use in this sketch.
  ultoa((unsigned long)currentTemp, temp, 10);
  int n = strlen(temp);
  temp[n] = '.';
  temp[n + 1] = '0' + ((unsigned long)(currentTemp * 10)) % 10;
  temp[n + 2] = '0' + ((unsigned long)(currentTemp * 100)) % 10;
  temp[n + 3] = '0' + ((unsigned long)(currentTemp * 1000)) % 10;

  // Generate string with payload length for use in the signing and request sending.
  char payloadlen[8 * sizeof(unsigned long) + 1];
  memset(payloadlen, 0, 8 * sizeof(unsigned long) + 1);
  ultoa(71 + strlen(table) + strlen(id) + strlen(currentTimeStr) + strlen(temp), payloadlen, 10);

  // Generate the signature for the request.
  // For details on the AWS signature process, see:
  //   http://docs.aws.amazon.com/general/latest/gr/signature-version-4.html

  // First, generate signing key to use in later signature generation.
  // Note: This could be optimized to generate just once per day (when the date value changes),
  // but since calls are only made every few minutes it's simpler to regenerate each time.
  char signingkey[SHA256_HASH_LENGTH];
  Sha256.initHmac((uint8_t*)AWS_SIG_START, strlen(AWS_SIG_START));
  Sha256.print(date);
  memcpy(signingkey, Sha256.resultHmac(), SHA256_HASH_LENGTH);
  Sha256.initHmac((uint8_t*)signingkey, SHA256_HASH_LENGTH);
  Sha256.print(AWS_REGION);
  memcpy(signingkey, Sha256.resultHmac(), SHA256_HASH_LENGTH);
  Sha256.initHmac((uint8_t*)signingkey, SHA256_HASH_LENGTH);
  Sha256.print(AWS_SERVICE);
  memcpy(signingkey, Sha256.resultHmac(), SHA256_HASH_LENGTH);
  Sha256.initHmac((uint8_t*)signingkey, SHA256_HASH_LENGTH);
  Sha256.print(F("aws4_request"));
  memcpy(signingkey, Sha256.resultHmac(), SHA256_HASH_LENGTH);

  // Second, generate hash of the payload data.
  Sha256.init();
  Sha256.print(F("{\"TableName\":\""));
  Sha256.print(table);
  Sha256.print(F("\",\"Item\":{\"Id\":{\"S\":\""));
  Sha256.print(id);
  Sha256.print(F("\"},\"Date\":{\"N\":\""));
  Sha256.print(currentTimeStr);
  Sha256.print(F("\"},\"Temp\":{\"N\":\""));
  Sha256.print(temp);
  Sha256.print(F("\"}}}"));
  char payloadhash[2 * SHA256_HASH_LENGTH + 1];
  memset(payloadhash, 0, 2 * SHA256_HASH_LENGTH + 1);
  hexString(Sha256.result(), SHA256_HASH_LENGTH, payloadhash);

  // Third, generate hash of the canonical request.
  Sha256.init();
  Sha256.print(F("POST\n/\n\ncontent-length:"));
  Sha256.print(payloadlen);
  Sha256.print(F("\ncontent-type:application/x-amz-json-1.0\nhost:"));
  Sha256.print(AWS_HOST);
  Sha256.print(F(";\nx-amz-date:"));
  Sha256.print(dateTime);
  Sha256.print(F("\nx-amz-target:"));
  Sha256.print(AWS_TARGET);
  Sha256.print(F("\n\ncontent-length;content-type;host;x-amz-date;x-amz-target\n"));
  Sha256.print(payloadhash);
  char canonicalhash[2 * SHA256_HASH_LENGTH + 1];
  memset(canonicalhash, 0, 2 * SHA256_HASH_LENGTH + 1);
  hexString(Sha256.result(), SHA256_HASH_LENGTH, canonicalhash);

  // Finally, generate request signature from the string to sign and signing key.
  Sha256.initHmac((uint8_t*)signingkey, SHA256_HASH_LENGTH);
  Sha256.print(F("AWS4-HMAC-SHA256\n"));
  Sha256.print(dateTime);
  Sha256.print(F("\n"));
  Sha256.print(date);
  Sha256.print(F("/"));
  Sha256.print(AWS_REGION);
  Sha256.print(F("/"));
  Sha256.print(AWS_SERVICE);
  Sha256.print(F("/aws4_request\n"));
  Sha256.print(canonicalhash);
  char signature[2 * SHA256_HASH_LENGTH + 1];
  memset(signature, 0, 2 * SHA256_HASH_LENGTH + 1);
  hexString(Sha256.resultHmac(), SHA256_HASH_LENGTH, signature);

  // Make request to DynamoDB API.
  uint32_t ip = 0;
  while (ip == 0) {
    if (!cc3000.getHostByName(AWS_HOST, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("POST / HTTP/1.1\r\nhost: "));
    www.fastrprint(AWS_HOST);
    www.fastrprint(F(";\r\nx-amz-date: "));
    www.fastrprint(dateTime);
    www.fastrprint(F("\r\nAuthorization: AWS4-HMAC-SHA256 Credential="));
    www.fastrprint(AWS_ACCESS_KEY);
    www.fastrprint(F("/"));
    www.fastrprint(date);
    www.fastrprint(F("/"));
    www.fastrprint(AWS_REGION);
    www.fastrprint(F("/"));
    www.fastrprint(AWS_SERVICE);
    www.fastrprint(F("/aws4_request, SignedHeaders=content-length;content-type;host;x-amz-date;x-amz-target, Signature="));
    www.fastrprint(signature);
    www.fastrprint(F("\r\ncontent-type: application/x-amz-json-1.0\r\ncontent-length: "));
    www.fastrprint(payloadlen);
    www.fastrprint(F("\r\nx-amz-target: "));
    www.fastrprint(AWS_TARGET);
    www.fastrprint(F("\r\n\r\n{\"TableName\":\""));
    www.fastrprint(table);
    www.fastrprint(F("\",\"Item\":{\"Id\":{\"S\":\""));
    www.fastrprint(id);
    www.fastrprint(F("\"},\"Date\":{\"N\":\""));
    www.fastrprint(currentTimeStr);
    www.fastrprint(F("\"},\"Temp\":{\"N\":\""));
    www.fastrprint(temp);
    www.fastrprint(F("\"}}}"));
  }
  else {
    Serial.println(F("Connection failed"));
    www.close();
    return;
  }

  // Read data until either the connection is closed, or the idle timeout is reached.
  Serial.println(F("AWS response:"));
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
}

// Convert an array of bytes into a lower case hex string.
// Buffer MUST be two times the length of the input bytes array!
void hexString(uint8_t* bytes, size_t len, char* buffer) {
  for (int i = 0; i < len; ++i) {
    btoa2Padded(bytes[i], &buffer[i * 2], 16);
  }
}

// Fill a 16 character buffer with the date in ISO8601 simple format, like '20130101T010101Z'.
// Buffer MUST be at least 16 characters long!
void dateTime8601(int year, byte month, byte day, byte hour, byte minute, byte seconds, char* buffer) {
  ultoa(year, buffer, 10);
  btoa2Padded(month, buffer + 4, 10);
  btoa2Padded(day, buffer + 6, 10);
  buffer[8] = 'T';
  btoa2Padded(hour, buffer + 9, 10);
  btoa2Padded(minute, buffer + 11, 10);
  btoa2Padded(seconds, buffer + 13, 10);
  buffer[15] = 'Z';
}

// Print a value from 0-99 to a 2 character 0 padded character buffer.
// Buffer MUST be at least 2 characters long!
void btoa2Padded(uint8_t value, char* buffer, int base) {
  if (value < base) {
    *buffer = '0';
    ultoa(value, buffer + 1, base);
  }
  else {
    ultoa(value, buffer, base);
  }
}

// getTime function adapted from CC3000 ntpTest sketch.
// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {
  Adafruit_CC3000_Client client;
  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  if (cc3000.getHostByName("pool.ntp.org", &ip)) {
    static const char PROGMEM
      timeReqA[] = { 
      227,  0,  6, 236                 }
    ,
    timeReqB[] = {  
      49, 78, 49,  52                 };

    startTime = millis();
    do {
      client = cc3000.connectUDP(ip, 123);
    } 
    while ((!client.connected()) &&
      ((millis() - startTime) < TIMEOUT_MS));

    if (client.connected()) {
      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      client.write(buf, sizeof(buf));

      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while ((!client.available()) &&
        ((millis() - startTime) < TIMEOUT_MS));
      if (client.available()) {
        client.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
          ((unsigned long)buf[41] << 16) |
          ((unsigned long)buf[42] <<  8) |
          (unsigned long)buf[43]) - 2208988800UL;
      }
      client.close();
    }
  }
  return t;
}
/**************************************************************************/
/*!
 @brief  Tries to read the IP address and other connection details
 */
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if (!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); 
    cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); 
    cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); 
    cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); 
    cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}




