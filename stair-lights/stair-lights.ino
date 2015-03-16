// Connect LED strip as:
// - LED to MOSFET drain
// + LED to +12 volt supply
// MOSFET source to GND
// MOSFET gate to LED_GATE_PIN


#define LED_GATE_PIN 5

#define FADE_SPEED 10     // make this higher to slow down

void setup() {
  pinMode(LED_GATE_PIN, OUTPUT);
}


void loop() {
  int r;

  // fade on
  for (r = 0; r < 256; r++) {
    analogWrite(LED_GATE_PIN, r);
    delay(FADE_SPEED);
  }
  // fade off
  for (r = 255; r > 0; r--) {
    analogWrite(LED_GATE_PIN, r);
    delay(FADE_SPEED);
  }

}
