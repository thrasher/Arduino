// Stair Lights
// Manages the lighting on the stairs, providing AUTO/ON/OFF functionality based on ambient
// conditions and press-and-hold single-button switch.
// by: Jason Thrasher 4/11/2015
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>

#include <Wire.h>
#include "RTClib.h"

#include "DHT.h" // for DHT22 temp/humidity sensor

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, 
    ADAFRUIT_CC3000_IRQ, 
    ADAFRUIT_CC3000_VBAT,
    SPI_CLOCK_DIVIDER);

// We get the SSID & Password from memory thanks to SmartConfigCreate!
//#define WLAN_SSID       "gogoair"           // cannot be longer than 32 characters!
//#define WLAN_PASS       "password"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define     DEVICE_NAME "CC3000" // should match value set using TI SmartConfig for auto-WiFi connection

// *** pins ***
#define DHTPIN 8 // pin for DHT22 temp/humidity sensor
#define LED_GATE_PIN 9
#define A_POT_PIN A0
#define BUTTON_PIN 7

#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define THRESHOLD 70 // sensor readings above threshold will turn light off

#define     READING_DELAY_SEC      60      // Number of seconds to wait between readings.
#define     DHT_DELAY_MS           2000   // Number of milliseconds to wait between DHT 22 sensor readings
#define     TIMEOUT_MS             15000  // How long to wait (in milliseconds) for a server connection to respond (for both AWS and NTP calls).

namespace d {
  typedef struct {
    unsigned long currentTime;
    byte mode; // auto=0, on=1, off=2
    byte lightsense;
    byte ledset;
    float humidity;
    float tempf; // Fahrenheit
    float tempc; // Celcius
    float heatindex;
  } 
  reading;

  void print(reading r) {
    Serial.print(F("CurrentTime: "));
    Serial.print(r.currentTime);
    Serial.print(F(" Mode: "));
    Serial.print(r.mode);
    Serial.print(F(" Lightsense: "));
    Serial.print(r.lightsense);
    Serial.print(F(" Ledset: "));
    Serial.print(r.ledset);
    Serial.print(F(" Humidity: "));
    Serial.print(r.humidity);
    Serial.print(F("% Temperature: "));
    Serial.print(r.tempc);
    Serial.print(F("*C "));
    Serial.print(r.tempf);
    Serial.print(F("*F Heat index: "));
    Serial.print(r.heatindex);
    Serial.println("*F");    
  }
}

// handle smoothing of sensor readings by averaging 100 of them
#define AVG_CNT 100 // number of values to average together, must be less than 256
int byteSum = 0;
byte idx = 0;
byte byteArray[AVG_CNT];
// function returns average of bytes put into buffer
byte addValue(byte f) {
  byteSum -= byteArray[idx];
  byteArray[idx] = f;
  byteSum += byteArray[idx];
  idx++;
  if (idx == AVG_CNT) idx = 0;  // faster than %
  return byteSum / AVG_CNT;
}

// State used to keep track of the current time and time since last temp reading.
unsigned long lastPolledTime = 0;   // Last value retrieved from time server
unsigned long sketchTime = 0;       // CPU milliseconds since last server query
unsigned long lastReading = 0;      // Time of last temperature reading.
unsigned long lastTimeUpdate = 0;   // Time of last time update
d::reading p;
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino

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
  
  if (!cc3000.begin(false, true, DEVICE_NAME))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    while(1);
  }
  
  Serial.println(F("Re-Connected!"));

  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  while (!cc3000.checkDHCP()) {
    Serial.println(F("\nWaiting for DHCP..."));
    delay(1000); // ToDo: Insert a DHCP timeout!
  }

  dht.begin(); // start the DHT library

  updateTimers();

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

  if (digitalRead(BUTTON_PIN) == LOW) {
    int reps = 0;
    while (reps < 3) {
      flash();
      if (digitalRead(BUTTON_PIN) == HIGH) {
        p.mode = reps;
        break;
      }
      reps++;
      delay(1500);
    }
    Serial.print(F("mode is now: "));
    Serial.println(p.mode);
  }

  // Update the current time.
  // Note: If the sketch will run for more than ~24 hours, you probably want to query the time
  // server again to keep the current time from getting too skewed.
  p.currentTime = lastPolledTime + (millis() - sketchTime) / 1000;

  if ((p.currentTime - lastReading) >= (READING_DELAY_SEC)) {
    lastReading = p.currentTime;

    // Get a temp reading
    acquire();
    d::print(p);

    // Write the result to the database.
    Serial.print(F("Writing data to web at time: ")); 
    Serial.println(p.currentTime);
    //dynamoDBWrite(TABLE_NAME, ID_VALUE, p.currentTime, p.tempf);
    get();
  }
  
  if ((p.currentTime - lastTimeUpdate) >= (60 * 60 * 6)) { // every 6 hours
    Serial.print("Updating time at: ");
    Serial.println(p.currentTime);
    lastTimeUpdate = p.currentTime;
    updateTimers();
  }

  delay(100);
}

void updateTimers() {
  // Get an initial time value by querying an NTP server.
  unsigned long t = getTime();
  while (t == 0) {
    // Failed to get time, try again in a minute.
    Serial.print(F("Getting NTP time, waiting (ms): ")); 
    Serial.println(TIMEOUT_MS);
    delay(TIMEOUT_MS);
    t = getTime();
  }
  lastPolledTime = t;
  sketchTime = millis();
}

#define WEBSITE "bitmenu-app.appspot.com"
void get() {
  char path[74];//74=20 + 10 + 8 + 8 + 8 + 5 + 5 + 5 + 5
  sprintf(path, "/stairlight/%lu/%i/%i/%i/%i/%i/%i/%i",
    p.currentTime, p.mode, p.lightsense, p.ledset,
    int(100*p.humidity), int(100*p.tempc), int(100*p.tempf), int(100*p.heatindex));
  Serial.print(F("get: "));
  Serial.println(path);

  uint32_t ip = 0;
  // Try looking up the website's IP address
  Serial.print(F(WEBSITE)); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
  }
  cc3000.printIPdotsRev(ip);

  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(path);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  while (www.connected() && (millis() - lastRead < TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      //Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  Serial.println(F("Success, Connection closed")); 
}

// call every 2 seconds, maximum
void acquire() {
  p.humidity = dht.readHumidity();
  // Read temperature as Celsius
  p.tempc = dht.readTemperature();
  // Read temperature as Fahrenheit
  p.tempf = dht.readTemperature(true);
  // Compute heat index, Must send in temp in Fahrenheit!
  p.heatindex = dht.computeHeatIndex(p.tempf, p.humidity);
}

void mode_auto() {
  // read the value from the sensor
  p.lightsense = analogRead(A_POT_PIN) / 4;

  if (p.lightsense < 1 ) {
    // minimum dim-ness
    p.ledset = 1;
  } 
  else if (p.lightsense < THRESHOLD) {
    p.ledset = p.lightsense;
  } 
  else if (THRESHOLD <= p.lightsense) {
    // too bright, turn off
    p.ledset = 0;
  }

  //brRA.addValue(p.ledset);
//  p.ledset = p.lightsense;
  p.ledset = addValue(p.ledset);
  update_light();
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
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

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
