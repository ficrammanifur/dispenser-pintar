// ===============================
// Test Relay ESP32
// ===============================

// Pin Relay
#define POMPA_DC_1   15
#define POMPA_DC_2   2
#define POMPA_DC_3   4

#define SELENOID_1   5
#define SELENOID_2   18
#define SELENOID_3   19

const int relayPins[] = {
  POMPA_DC_1,
  POMPA_DC_2,
  POMPA_DC_3,
  SELENOID_1,
  SELENOID_2,
  SELENOID_3
};

const char* relayNames[] = {
  "Pompa DC 1",
  "Pompa DC 2",
  "Pompa DC 3",
  "Selenoid 1",
  "Selenoid 2",
  "Selenoid 3"
};

const int relayCount = sizeof(relayPins) / sizeof(relayPins[0]);

// Ubah sesuai jenis relay:
// LOW  = Relay Active LOW
// HIGH = Relay Active HIGH
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < relayCount; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], RELAY_OFF);
  }

  Serial.println("=== TEST RELAY DIMULAI ===");
}

void loop() {
  for (int i = 0; i < relayCount; i++) {
    Serial.print("Menyalakan: ");
    Serial.println(relayNames[i]);

    digitalWrite(relayPins[i], RELAY_ON);
    delay(2000);

    digitalWrite(relayPins[i], RELAY_OFF);

    Serial.print("Mematikan: ");
    Serial.println(relayNames[i]);

    delay(1000);
  }

  Serial.println("---------------------------");
}
