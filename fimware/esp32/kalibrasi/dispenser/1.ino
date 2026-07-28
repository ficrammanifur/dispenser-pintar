/*
  ==========================================
  KALIBRASI YF-S401 FLOW SENSOR 1
  TARGET : 1 LITER
  ==========================================

  Pompa DC 1    -> GPIO 15
  Selenoid 1    -> GPIO 5
  Flow Sensor 1 -> GPIO 13

  ==========================================
*/

#define POMPA_PIN     15
#define SELENOID_PIN   5
#define FLOW_PIN      13


// Relay kamu sebelumnya Active HIGH
#define RELAY_ON   HIGH
#define RELAY_OFF  LOW


volatile uint32_t pulseCount = 0;
volatile unsigned long lastPulseTime = 0;


// ================================
// Interrupt Flow Sensor
// ================================
void IRAM_ATTR flowISR() {

  unsigned long now = micros();

  // filter noise
  // minimal jarak pulsa 2ms
  if (now - lastPulseTime > 2000) {

    pulseCount++;
    lastPulseTime = now;

  }

}



void setup() {

  Serial.begin(115200);


  pinMode(POMPA_PIN, OUTPUT);
  pinMode(SELENOID_PIN, OUTPUT);
  pinMode(FLOW_PIN, INPUT_PULLUP);



  // Matikan awal
  digitalWrite(POMPA_PIN, RELAY_OFF);
  digitalWrite(SELENOID_PIN, RELAY_OFF);



  attachInterrupt(
    digitalPinToInterrupt(FLOW_PIN),
    flowISR,
    RISING
  );



  Serial.println();
  Serial.println("=================================");
  Serial.println("  KALIBRASI FLOW SENSOR YF-S401");
  Serial.println("=================================");
  Serial.println("Pompa     : GPIO 15");
  Serial.println("Selenoid  : GPIO 5");
  Serial.println("Flow      : GPIO 13");
  Serial.println();
  Serial.println("Target air : 1000 mL");
  Serial.println();



  delay(2000);



  // ===========================
  // Buka selenoid dahulu
  // ===========================

  Serial.println("SELENOID OPEN");

  digitalWrite(SELENOID_PIN, RELAY_ON);



  delay(1000);



  // ===========================
  // Pompa ON
  // ===========================

  Serial.println("POMPA ON");

  digitalWrite(POMPA_PIN, RELAY_ON);



  Serial.println();
  Serial.println("Silahkan tampung air 1 Liter");
  Serial.println("--------------------------------");

}




void loop() {


  static unsigned long lastPrint = 0;


  if (millis() - lastPrint >= 1000) {


    lastPrint = millis();


    noInterrupts();

    uint32_t total = pulseCount;

    interrupts();



    Serial.print("Total Pulse = ");

    Serial.println(total);


  }

}
