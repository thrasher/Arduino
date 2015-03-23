// This program manages the 12V LED lights installed on the stairs.

// Use-cases
// 1) manually turn the device on or off by pressing a button
// 2) manually adjust the brightness by turning a pot
// 3) call a URL to turn the lights on or off (manual over-rides)
// 4) call a URL to adjust the brightness (manual over-rides)
// 6) monitor ambient light in the stairwell and automatically turn on when it's dark
// 7) monitor the ambient light in the stairwell and automatically turn off when it's daytime
// 8) 

// Connect LED strip as:
// - LED to MOSFET drain
// + LED to +12 volt supply
// MOSFET source to GND
// MOSFET gate to LED_GATE_PIN

// Orange : A0 - Potentiometer center pin
// White : 5V - red rail of breadboard
// Red-LED-strip : Vin - 12v or 9v
// Yellow : GND - blue rail of breadboard
// White: MOSFET-gate - PWM for mosfet
// Blue: D2 - interrupt 0

#define LED_GATE_PIN 5  // pwm digital pin for MOSFET gate
#define A_POT_PIN A0  // analog pot pin

volatile int state = HIGH;
int brightness = 255;

void setup() {
  pinMode(LED_GATE_PIN, OUTPUT);
  
  Serial.begin(115200);
  Serial.println("Hello world");
  delay(2000);// Give reader a chance to see the output.

  attachInterrupt(INT0, toggle, FALLING);

  setBrightness();
}

void loop() {
  if (brightness != analogRead(A_POT_PIN) / 4) {
    brightness = analogRead(A_POT_PIN) / 4;    // read the value from the sensor
    setBrightness();  
  }
  Serial.println(brightness);
  delay(20);// Give reader a chance to see the output.
}

void setBrightness() {
  Serial.print("set brightness: ");
  Serial.print(state);
  Serial.print(" ");
  Serial.print(brightness);
  
  if (state == HIGH) {
    // note: light can be on, but with 0 brightness
    analogWrite(LED_GATE_PIN, brightness);
  } else {
    // light is off
    digitalWrite(LED_GATE_PIN, LOW); 
  }  
}

void toggle() {
  state = !state;
  setBrightness();
}

