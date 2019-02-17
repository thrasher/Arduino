/**
 * Generic framework to allow for ESP8266 IoT devices, including:
 * 
 * 1) boot up, and scan for available WiFi networks
 * 2) present soft-AP connection, to allow selection and saving of WiFi details for later reboots
 * 3) ability to reset configuration by holding pin D0 (GPIO16) to ground on boot, clearing saved WiFi
 * 4) webserver hosting telemetry data about device
 * 
 * Created using a WeMos D1 Mini ESP8266 board.
 * 
 */
#include <ESP8266WiFi.h>
#include <AddrList.h>
#include <ESP8266WebServer.h>
#include <Ticker.h> // used for async scanning of SSIDs

#define D0 16 // GPIO16 INPUT_PULLDOWN_16

const char *ssid_prefix = "esp-";
byte mac[6]; // used to hold the mac address

ESP8266WebServer server(80);
//String scanJson = "[]"; // networks found in the last WiFi.scan.
int scanResults = 0;
Ticker scanner; // async timer that initiates network scans for APs to connect to

// preinit() is called before system startup
// from nonos-sdk's user entry point user_init()

void preinit() {
  // Global WiFi constructors are not called yet
  // (global class instances like WiFi, Serial... are not yet initialized)..
  // No global object methods or C++ exceptions can be called in here!
  //The below is a static class method, which is similar to a function, so it's ok.
  ESP8266WiFiClass::preinitWiFiOff();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);   // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);// turn on the LED to show we're doing work
  pinMode(D0, INPUT_PULLDOWN_16); // special pin 16 uses a pull-down, not a pull-up!

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("sleeping 5s");

  // during this period, a simple amp meter shows
  // an average of 20mA with a Wemos D1 mini
  // a DSO is needed to check #2111
  delay(5000);

  // test if reset requested on powerup
  if (digitalRead(D0) == 1) {
    Serial.println("resetting wifi now");
    digitalWrite(LED_BUILTIN, HIGH); // turn off the LED
    resetAndRestart();
  }

  Serial.println("waking WiFi up, sleeping 5s");
  WiFi.forceSleepWake();

  // amp meter raises to 75mA
  delay(5000);

  Serial.print("mac address: ");
  WiFi.macAddress(mac);
  Serial.println(macToString(mac));
  Serial.print("ap ssid: ");
  Serial.println(getApSsid(mac));

  Serial.println(WiFi.SSID().c_str());
  Serial.println(sizeof(WiFi.SSID().c_str())); // 4 when empty

  Serial.println(WiFi.SSID());
  Serial.println(sizeof(WiFi.SSID())); // 12 when empty

  for(int i=0; i<sizeof(WiFi.SSID().c_str()); i++) {
    Serial.print("'");
    Serial.print(WiFi.SSID().c_str()[i], HEX);
    Serial.println("'");
  }
  

  // print out the ssid and psk saved in the flash
  if (sizeof(WiFi.SSID().c_str()) == 4) { // it seems a size of 4 means it's empty, but don't understand why
    Serial.println("network not configured, preparing ap mode to scan");
    //WiFi.mode(WIFI_STA);
    WiFi.mode(WIFI_AP); // Both in Station and Access Point Mode
    WiFi.softAP(getApSsid(mac));
    //WiFi.disconnect();
    scanner.attach(15, asyncScan); // start async scanning for SSIDs
    delay(100);
  } else {
    Serial.println("connecting to AP");
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("PSK: %s\n", WiFi.psk().c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(); // uses SSID and PSK stored in flash
    //    WiFi.begin("gogoair", "thrasher2004");

    for (bool configured = false; !configured;) {
      for (auto addr : addrList)
        if ((configured = !addr.isLocal() && addr.ifnumber() == STATION_IF)) {
          Serial.printf("STA: IF='%s' hostname='%s' addr= %s\n",
                        addr.ifname().c_str(),
                        addr.ifhostname(),
                        addr.toString().c_str());
          break;
        }
      Serial.print('.');
      delay(500);
    }

    // amp meter cycles within 75-80 mA
  }

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.onNotFound([](){
    server.send(404, "text/plain", "404: Not found");
  });  
  server.begin();
  Serial.println("HTTP server started");

}

void loop() {
  delay(500);
  Serial.println(digitalRead(D0));
  server.handleClient();
}

// clear the wifi ssid and psk stored in flash, and restart the device
void resetAndRestart() {
  Serial.println(ESP.eraseConfig());
  Serial.println("waiting 5s before ESP reset");
  delay(5000);
  Serial.println("resetting now");
  ESP.reset(); // this will reboot the whole device
}

// initiate an asynchronous scan
void asyncScan() {
  Serial.println("starting async scan");
  scanResults = 0;
  boolean findHidden = false; // false = do not scan for hidden SSIDs
  WiFi.scanNetworksAsync(onScanComplete, findHidden);
}

// handle completed asynchronous scans
void onScanComplete(int networksFound) {
  Serial.println("onScanComplete");
  scanResults = networksFound;
}

// build the JSON representation of scan results
// note: if called right before scan completes, some or all results may be empty
String getScanJson(int n) {
  String json = "[\n"; 
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      if (i > 0)
        json += ",\n";
      json += "\"";
      json += WiFi.SSID(i);
      json += "\": { \"rssi\": ";
      json += WiFi.RSSI(i); // Received Signal Strength Indication
      json += ", \"encryption\": \"";
      json += mapEncryptionType(WiFi.encryptionType(i));
      json += "\", \"bssid\": \"";
      json += WiFi.BSSIDstr(i);
      json += "\", \"channel\": ";
      json += WiFi.channel(i);
      json += ", \"isHidden\": ";
      json += WiFi.isHidden(i) == 1 ? "true" : "false";
      json += "}";
      delay(10);
    }
    
    json += "\n";
  }
  json += "]";
  Serial.println(json);
  return json;
}

// blocking scan for wifi networks to connect to
String scan() {
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  onScanComplete(n);
  
  return getScanJson(n);
}

// map network scan encryption types to human-readable values
String mapEncryptionType(int t) {
  switch (t) {
    case ENC_TYPE_WEP:
      return "WEP";
      break;
    case ENC_TYPE_TKIP:
      return "WPA / PSK";
      break;
    case ENC_TYPE_CCMP:
      return "WPA2 / PSK";
      break;
    case ENC_TYPE_NONE:
      return "open network";
      break;
    case ENC_TYPE_AUTO:
      return "WPA / WPA2 / PSK";
      break;
    default:
      return "unknown";
      break;
  }
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

// render scan results as plain JSON, or JSONP
void handleScan() {
  if (server.hasArg("callback")) {
    String cb = server.arg("callback");
    cb += "(";
    cb += getScanJson(scanResults);
    cb += ");";
    server.send(200, "application/javascript", cb);
  } else {
    server.send(200, "application/json", getScanJson(scanResults));
  }
}

// sets the desired network config
void handleConnect() {
  if ( !server.hasArg("ssid") || !server.hasArg("psk") ||
      server.arg("ssid") == NULL || server.arg("psk") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }

  // TODO: save the ssid and psk to EEPROM, connect to network
}

// render the current network config as JSON
void handleNetwork() {
  // TODO: return JSON of the current network configuration, device MAC addr, etc
}

// build the SSID for this device, using the last two bytes of the mac addr
String getApSsid(const unsigned char* mac) {
  char buf[5]; // 4 + one extra char for null byte
  snprintf(buf, sizeof(buf), "%02x%02x", mac[4], mac[5]);
           
  String ssid = ssid_prefix;
  ssid += String(buf);
  return ssid;
}

// convert a mac address byte array to a traditional hex encoded string
String macToString(const unsigned char* mac) {
  char buf[18]; // 17 + one extra char for the null byte
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
