#define LED_GATE_PIN 9
#define BUTTON_PIN 7
byte mode = 0;

unsigned char myChar = 0;

void setup() {
  // Start Serial
  Serial.begin(115200);
  Serial.println(F("Starting Rosie The Riviter"));

  pinMode(LED_GATE_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
}

void loop() {
  Serial.print(F("do mode "));
  Serial.println(mode);
  switch (mode) {
    case 0:
      digitalWrite(LED_GATE_PIN, 0);
      break;
    case 1:
      digitalWrite(LED_GATE_PIN, 1);
      break;
    case 2:
      digitalWrite(LED_GATE_PIN, 0);
      delay(100);
      digitalWrite(LED_GATE_PIN, 1);
      break;
    case 3:
      myChar += 4;
      if (myChar == 0) {
        flash();
      }
      analogWrite(LED_GATE_PIN, myChar);
      break;
    default:
      mode = 0;
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    int reps = 0;
    while (reps < 4) {
      flash();
      if (digitalRead(BUTTON_PIN) == HIGH) {
        mode = reps;
        break;
      }
      reps++;
      delay(1500);
    }
    Serial.print(F("mode is now: "));
    Serial.println(mode);
  }

  delay(20);
}


void flash() {
  digitalWrite(LED_GATE_PIN, 0);
  delay(100);
  digitalWrite(LED_GATE_PIN, 1);
  delay(100);
  digitalWrite(LED_GATE_PIN, 0);
  delay(100);
  digitalWrite(LED_GATE_PIN, 1);
  delay(100);
  digitalWrite(LED_GATE_PIN, 0);
  delay(100);
}

