// ===============================
// Test Pompa & Selenoid Berpasangan
// ===============================

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

const int pompaPins[]    = {15, 2, 4};
const int selenoidPins[] = {5, 18, 19};

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 3; i++) {
    pinMode(pompaPins[i], OUTPUT);
    pinMode(selenoidPins[i], OUTPUT);

    digitalWrite(pompaPins[i], RELAY_OFF);
    digitalWrite(selenoidPins[i], RELAY_OFF);
  }

  Serial.println("Test Pompa & Selenoid");
}

void loop() {

  for (int i = 0; i < 3; i++) {

    Serial.printf("Pompa %d ON\n", i + 1);
    digitalWrite(pompaPins[i], RELAY_ON);
    delay(2000);

    Serial.printf("Selenoid %d ON\n", i + 1);
    digitalWrite(selenoidPins[i], RELAY_ON);
    delay(2000);

    Serial.printf("Pompa %d OFF\n", i + 1);
    digitalWrite(pompaPins[i], RELAY_OFF);
    delay(1000);

    Serial.printf("Selenoid %d OFF\n", i + 1);
    digitalWrite(selenoidPins[i], RELAY_OFF);
    delay(1000);
  }

  Serial.println("--------------------");
  delay(3000);
}
