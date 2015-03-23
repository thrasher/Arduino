/* 
 This a simple example of the aREST Library for Arduino (Uno/Mega/Due/Teensy)
 using the CC3000 WiFi chip. See the README file for more details.
 
 Written in 2014 by Marco Schwartz under a GPL license. 
 */

// Import required libraries
#include <Adafruit_CC3000.h>
#include <SPI.h>
//#include <CC3000_MDNS.h>
#include <aREST.h>
#include <avr/wdt.h>

// These are the pins for the CC3000 chip if you are using a breakout board
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

//#define LIGHTWEIGHT 1

// Create CC3000 instance
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIV2);
// Create aREST instance
aREST rest = aREST();

// Your WiFi SSID and password                                         
//#define WLAN_SSID       "yourSSID"
//#define WLAN_PASS       "yourPassword"
//#define WLAN_SECURITY   WLAN_SEC_WPA2

// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80

// Server instance
Adafruit_CC3000_Server restServer(LISTEN_PORT);

// DNS responder instance
//MDNSResponder mdns;

#define LED_GATE_PIN 6
#define ONOFF_PIN INT0
#define A_POT_PIN A0

// Variables to be exposed to the API
int temperature;
int humidity;
volatile int state = HIGH;
int brightness = 0;

void setup(void)
{  
  // Start Serial
  Serial.begin(115200);

  // Init variables and expose them to REST API
  temperature = 24;
  humidity = 40;
  rest.variable("temperature",&temperature);
  rest.variable("humidity",&humidity);
  rest.variable("brightness",&brightness);

  // Function to be exposed
  rest.function("led",ledControl);

  // Give name and ID to device
  rest.set_id("008");
  rest.set_name("mighty_cat");

  // Set up CC3000 and get connected to the wireless network.
  if (!cc3000.begin(false, true, "CC3000"))
  {
    while(1);
  }
  //  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
  //    while(1);
  //  }
  while (!cc3000.checkDHCP())
  {
    delay(100);
  }
  Serial.println();

  // Print CC3000 IP address. Enable if mDNS doesn't work
  //while (! displayConnectionDetails()) {
  // delay(1000);
  //}

  // Start multicast DNS responder
  //  if (!mdns.begin("arduino", cc3000)) {
  //    while(1); 
  //  }

  pinMode(LED_GATE_PIN, OUTPUT);
  pinMode(ONOFF_PIN, INPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set
  attachInterrupt(INT0, toggle, FALLING);

  // Start server
  restServer.begin();
  Serial.println(F("Listening for connections..."));

  // Enable watchdog
  wdt_enable(WDTO_4S);
}

void loop() {

  // Handle any multicast DNS requests
  //  mdns.update();

  // Handle REST calls
  Adafruit_CC3000_ClientRef client = restServer.available();
  rest.handle(client);
  wdt_reset();

  // Check connection, reset if connection is lost
  if(!cc3000.checkConnected()){
    while(1){
    }
  }
  wdt_reset();

  brightness = analogRead(A_POT_PIN) / 4;    // read the value from the sensor
  if (state == HIGH) {
    // note: light can be on, but with 0 brightness
    analogWrite(LED_GATE_PIN, brightness);
  } 
  else {
    // light is off
    digitalWrite(LED_GATE_PIN, LOW); 
  }  

}

// Print connection details of the CC3000 chip
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
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

// Custom function accessible by the API
int ledControl(String command) {

  // Get brightness from command
  brightness = command.toInt() % 256;
  Serial.print("led command: ");
  Serial.println(brightness);

  Serial.print("dimmer: ");
  Serial.println(analogRead(A_POT_PIN));

  Serial.print("on/off: ");
  Serial.println(digitalRead(ONOFF_PIN));

  //  digitalWrite(LED_GATE_PIN, brightness);
  analogWrite(LED_GATE_PIN, brightness);
  return 1;
}

void toggle() {
  state = !state;
}



