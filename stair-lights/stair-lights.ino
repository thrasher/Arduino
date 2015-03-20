// Connect LED strip as:
// - LED to MOSFET drain
// + LED to +12 volt supply
// MOSFET source to GND
// MOSFET gate to LED_GATE_PIN


#define LED_GATE_PIN 5

#define FADE_SPEED 5     // make this higher to slow down

//#define BUTTON_PIN 2  // interrupt pin
volatile int state = HIGH;

#define A_POT_PIN 3  // analog pot pin
int pot_val = 0;


void setup() {
  pinMode(LED_GATE_PIN, OUTPUT);

  Serial.begin(115200);
  Serial.println("Hello world");
  delay(2000);// Give reader a chance to see the output.

  attachInterrupt(0, toggle, FALLING);
}


void loop() {

  pot_val = analogRead(A_POT_PIN) / 4;    // read the value from the sensor
  Serial.println(pot_val);
  delay(100);// Give reader a chance to see the output.

  if (state == HIGH) {
    Serial.println("high");
    Serial.println(state);
    analogWrite(LED_GATE_PIN, pot_val);
  } else {
    Serial.println("low");
    analogWrite(LED_GATE_PIN, 0);
  }

}

void toggle() {
  state = !state;
}
