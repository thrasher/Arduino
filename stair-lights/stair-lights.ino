// Stair Lights
// Manages the lighting on the stairs, providing AUTO/ON/OFF functionality based on ambient
// conditions and press-and-hold single-button switch.
// by: Jason Thrasher 4/11/2015

#include "RunningAverage.h" // for sensor averaging calculation
#include "DHT.h" // for DHT22 temp/humidity sensor

// *** pins ***
#define DHTPIN 8 // pin for DHT22 temp/humidity sensor
#define LED_GATE_PIN 9
#define A_POT_PIN A0
#define BUTTON_PIN 12

#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define THRESHOLD 30 // sensor readings above threshold will turn light off

long time = millis();
int brightness = 0;
int button_state = 1;
int mode = 0;  // auto=0, on=1, off=2
float h, t, f, hi;

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino
RunningAverage brRA(100); // averages sensor readings for smoother led changes

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  pinMode(LED_GATE_PIN, OUTPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set
  pinMode(BUTTON_PIN, INPUT);

  dht.begin(); // start the DHT library
  brRA.clear(); // explicitly start buffer clean

  Serial.println(F("Started..."));
}

void loop() {
  switch (mode) {
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
      mode = 0;
  }

  button_state = digitalRead(BUTTON_PIN);
  if (button_state == 0) {
    //    unsigned long time;
    //    time = millis();
    int reps = 0;
    while (reps < 3) {
      flash();
      if (digitalRead(BUTTON_PIN) == 1) {
        mode = reps;
        break;
      }
      reps++;
      delay(1500);
    }
  }
  acquire();

  delay(100);
}

void acquire() {
  // only allow acquire to run every two seconds, otherwise return old values
  if (millis() - time < 2000) {
    //return previous value
    Serial.println("skipping old value");
    return;
  }
  time = millis();
  
  h = dht.readHumidity();
  // Read temperature as Celsius
  t = dht.readTemperature();
  // Read temperature as Fahrenheit
  f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  hi = dht.computeHeatIndex(f, h);

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hi);
  Serial.println(" *F");

}

void mode_auto() {
  int br = analogRead(A_POT_PIN) / 4;    // read the value from the sensor
  Serial.print("raw sensor brightness: ");
  Serial.println(br);

  if (br < 1 ) {
    // minimum dim-ness
    br = 1;
  } else if (br < THRESHOLD) {
    // accept value
  } else if (THRESHOLD <= br) {
    // too bright, turn off
    br = 0;
    //mode = 2; // hysteresis hack
  }

  brRA.addValue(br);
  brightness = brRA.getAverage();

  update_light();

  //  Serial.print("Measurment: ");
  //  Serial.println(br);
  //
  //  Serial.print("Running Average: ");
  //  Serial.println(brRA.getAverage(), 3);
  //
  //  Serial.print("Running Count: ");
  //  Serial.println(brRA.getCount(), 3);
  //
  //  Serial.print("Running Size: ");
  //  Serial.println(brRA.getSize(), 3);
  //
  Serial.print("led brightness: ");
  Serial.println(brightness);
}

void update_light() {
  if (brightness == 0) {
    digitalWrite(LED_GATE_PIN, 0); // note: analogWrite may glitch-flash on, so we use digital here
  } else {
    analogWrite(LED_GATE_PIN, brightness);
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

