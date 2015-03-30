#define LED_GATE_PIN 9
#define A_POT_PIN A0

int brightness = 0;

void setup(void)
{  
  // Start Serial
  Serial.begin(115200);

  pinMode(LED_GATE_PIN, OUTPUT);
  //  pinMode(A_POT_PIN, INPUT); // does not need to be set

  Serial.println(F("Started..."));
}

void loop() {
  brightness = analogRead(A_POT_PIN) / 4;    // read the value from the sensor

  analogWrite(LED_GATE_PIN, brightness);
    // light is off
//    digitalWrite(LED_GATE_PIN, LOW); 

  if (brightness <=1 ) {
     analogWrite(LED_GATE_PIN, 1);
  } else if (brightness < 50) {
     analogWrite(LED_GATE_PIN, brightness);
  } else if (50 <= brightness) {
     digitalWrite(LED_GATE_PIN, 0);
  }

//  analogWrite(LED_GATE_PIN, brightness);
  Serial.print("led command: ");
  Serial.println(brightness);
 

  delay(2000);
}

