// Stair Lights
// Manages the lighting on the stairs, providing AUTO/ON/OFF functionality based on ambient
// conditions and press-and-hold single-button switch.
// by: Jason Thrasher 4/11/2015
//
// Note: the CC3000 is unstable as an always-on wifi unit, for a hack to help, see:
// http://thefactoryfactory.com/wordpress/?p=1140

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
    unsigned long poll;
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
    Serial.print(F("Poll: "));
    Serial.print(r.poll);
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

d::reading p;
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino

#define    READ_INTERVAL 300000
unsigned long timeToRead = READ_INTERVAL;

void setup(void) {
  // Start Serial
  Serial.begin(115200);
  Serial.println(F("Starting StairLight"));

  pinMode(LED_GATE_PIN, OUTPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set
  pinMode(BUTTON_PIN, INPUT);

  p.mode=0;
  p.ledset=THRESHOLD;
  mode_auto();

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

  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
  //Serial.println(F("\nClosing the connection"));
  ///cc3000.disconnect();

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
  case 3:
    takeReading();
  default:
    p.mode = 0;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    int reps = 0;
    while (reps < 4) {
      flash(reps);
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

  delay(100);
  
  if (millis() < READ_INTERVAL) { // handle roll-over
    timeToRead = READ_INTERVAL;
  }
  if (timeToRead < millis()) {
    takeReading();
    timeToRead += READ_INTERVAL;
  }
  
}

void takeReading () {
  Serial.println(F("Take Reading"));
  p.poll += 1;
  acquire();
  d::print(p);
  get();
  //flash(0);
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

#define WEBSITE "bitmenu-app.appspot.com"
void get() {
  char path[74];//74=20 + 10 + 8 + 8 + 8 + 5 + 5 + 5 + 5
  sprintf(path, "/stairlight/%lu/%i/%i/%i/%i/%i/%i/%i",
  p.poll, p.mode, p.lightsense, p.ledset,
  int(100*p.humidity), int(100*p.tempc), int(100*p.tempf), int(100*p.heatindex));
  Serial.print(F("get: "));
  Serial.println(path);

  uint32_t ip = 0;
  // Try looking up the website's IP address
  Serial.print(F(WEBSITE)); 
  Serial.print(F(" -> "));
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
    www.fastrprint(F("Host: ")); 
    www.fastrprint(WEBSITE); 
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  }
  else {
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

void flash(int reps) {
  for (int i = 0; i < reps+1; i++) {
    digitalWrite(LED_GATE_PIN, 1);
    delay(100);
    digitalWrite(LED_GATE_PIN, 0);
    delay(100);
  }
  update_light();
  delay(600);
}


