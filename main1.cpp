#include <Arduino.h>

int temperature;

void setup() {
  Serial.begin(115200);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  randomSeed(micros());

}

void loop() {
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);

  temperature = random(20, 46);

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  if (temperature < 25) {
    Serial.println("Status: Cold");
    digitalWrite(10, HIGH);
  }

  else if( temperature < 35) {
    Serial.println("Status: Warm");
    digitalWrite(12, HIGH);
  }

  else if( temperature < 40) {
    Serial.println("Status: Hot");
    digitalWrite(13, HIGH);
  }

  else {
    Serial.println("Status: Dangerously Hot");
    digitalWrite(11, HIGH);
  }

  Serial.println("-------------------------");
  delay(2000);
}