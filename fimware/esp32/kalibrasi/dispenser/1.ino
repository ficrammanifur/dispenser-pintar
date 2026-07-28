/*
  ==========================================
  KALIBRASI FLOW SENSOR 1 (1 LITER)
  ==========================================
  Pompa DC 1   -> GPIO 15
  Selenoid 1   -> GPIO 5
  Flow Sensor1 -> GPIO 13
  ==========================================
*/

#define POMPA_PIN      15
#define SELENOID_PIN   5
#define FLOW_PIN       13

// Sesuaikan dengan relay
#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

volatile uint32_t pulseCount = 0;

void IRAM_ATTR flowISR() {
  pulseCount++;
}

void setup() {

  Serial.begin(115200);

  pinMode(POMPA_PIN, OUTPUT);
  pinMode(SELENOID_PIN, OUTPUT);
  pinMode(FLOW_PIN, INPUT_PULLUP);

  digitalWrite(POMPA_PIN, RELAY_OFF);
  digitalWrite(SELENOID_PIN, RELAY_OFF);

  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowISR, FALLING);

  Serial.println();
  Serial.println("====================================");
  Serial.println("KALIBRASI FLOW SENSOR 1");
  Serial.println("Target : 1000 mL (1 Liter)");
  Serial.println("====================================");

  delay(1000);

  // Buka selenoid dulu
  digitalWrite(SELENOID_PIN, RELAY_ON);
  Serial.println("Selenoid OPEN");

  delay(1000);

  // Baru nyalakan pompa
  digitalWrite(POMPA_PIN, RELAY_ON);
  Serial.println("Pompa ON");
  Serial.println();
  Serial.println(">>> Tampung tepat 1 Liter <<<");
  Serial.println(">>> Tekan RESET setelah selesai <<<");
}

void loop() {

  static unsigned long lastPrint = 0;

  if (millis() - lastPrint >= 1000) {

    lastPrint = millis();

    noInterrupts();
    uint32_t totalPulse = pulseCount;
    interrupts();

    Serial.print("Total Pulse = ");
    Serial.println(totalPulse);
  }
}
