#define LED_GATE_PIN 9
#define A_POT_PIN A0
#define BUTTON_PIN 12
#define THRESHOLD 30 // sensor readings above threshold will turn light off

int brightness = 0;
int button_state = 1;
int mode = 0;  // auto=0, on=1, off=2

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  pinMode(LED_GATE_PIN, OUTPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set
  pinMode(BUTTON_PIN, INPUT);

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

  delay(100);
}

void mode_auto() {
  brightness = analogRead(A_POT_PIN) / 4;    // read the value from the sensor

  //  analogWrite(LED_GATE_PIN, brightness);
  // light is off
  //    digitalWrite(LED_GATE_PIN, LOW);

  if (brightness < 1 ) {
    // minimum dim-ness
    brightness = 1;
  } else if (brightness < THRESHOLD) {
    // accept value
  } else if (THRESHOLD <= brightness) {
    // too bright, turn off
    brightness = 0;
    mode = 2; // hysteresis hack
  }

  update_light();

  //  analogWrite(LED_GATE_PIN, brightness);
  Serial.print("led command: ");
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

