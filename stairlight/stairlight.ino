// Stair Lights
// Manages the lighting on the stairs, providing AUTO/ON/OFF functionality based on ambient
// conditions and press-and-hold single-button switch.
// by: Jason Thrasher 4/11/2015
//
// Note: the CC3000 is unstable as an always-on wifi unit, for a hack to help, see:
// http://thefactoryfactory.com/wordpress/?p=1140

// *** pins ***
#define LED_GATE_PIN 9
#define A_POT_PIN A0
#define BUTTON_PIN 7

#define THRESHOLD 70 // sensor readings above threshold will turn light off

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

  Serial.println(F("Started..."));
}

byte glow = 0;
byte dir = 1;

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
    digitalWrite(LED_GATE_PIN, 1);
    delay(50);
    digitalWrite(LED_GATE_PIN, 0);
    delay(80);
    break;
  case 4:
    if (glow == 0) {
      glow += dir;
    } else if (glow == 255) {
      glow -= dir;
    }
    analogWrite(LED_GATE_PIN, glow);
    break;
  default:
    p.mode = 0;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    int reps = 0;
    while (reps < 5) {
      flash(reps);
      delay(1500);
      if (digitalRead(BUTTON_PIN) == HIGH) {
        p.mode = reps;
        break;
      }
      reps++;
    }
    Serial.print(F("mode is now: "));
    Serial.println(p.mode);
  }

  delay(20);
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


