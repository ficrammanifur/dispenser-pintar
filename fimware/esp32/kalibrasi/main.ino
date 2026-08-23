#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <HX711.h>
#include <ArduinoJson.h>

// ============================================================
//  KONFIGURASI JARINGAN — SESUAIKAN SEBELUM UPLOAD
// ============================================================
const char* WIFI_SSID = "BOLTSuper4G-CC06";
const char* WIFI_PASS = "d5ddyrqd";

// === SESUAIKAN DENGAN IP KOMPUTER ANDA ===
const char* MQTT_HOST = "192.168.8.104";   // IP komputer/laptop
const int   MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASS = "";
const char* MQTT_ID = "refillx-esp32";

// MQTT Topics
const char* TOPIC_TELEMETRY = "refillx/telemetry";
const char* TOPIC_COMMAND   = "refillx/command";
const char* TOPIC_STATUS    = "refillx/status";
const char* TOPIC_SUCCESS   = "success";

// ============================================================
//  PIN DEFINISI
// ============================================================
#define FLOW1_PIN 13
#define FLOW2_PIN 12
#define FLOW3_PIN 14
#define LOADCELL_DT  33
#define LOADCELL_SCK 32
#define TOF_SDA 26
#define TOF_SCL 25
#define PUMP1_PIN  15
#define VALVE1_PIN 5
#define PUMP2_PIN  2
#define VALVE2_PIN 18
#define PUMP3_PIN  27
#define VALVE3_PIN 19

// ============================================================
//  KALIBRASI
// ============================================================
const float CAL_FACTOR[3]    = {302.81, 298.78, 542.47};
const float LOADCELL_FACTOR  = -91.14;
const float TARE_THRESHOLD   = 50.0;
const int   TOF_MAX_DISTANCE = 100;
const bool PAKAI_HX711 = true;
const bool PAKAI_TOF   = true;

// ============================================================
//  OBJEK & STATE GLOBAL
// ============================================================
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

HX711   scale;
VL53L0X tof;
bool tofOK = false;
bool hxOK  = false;

volatile long pulseCount[3] = {0, 0, 0};
bool  lineActive[3]    = {false, false, false};
float initialWeight[3] = {0, 0, 0};
float lastWeight = 0;

long cmdCount = 0;
unsigned long lastTelemetry = 0;
unsigned long lastReconnect = 0;
unsigned long lastHeartbeat = 0;

int pumpPins[3]  = {PUMP1_PIN, PUMP2_PIN, PUMP3_PIN};
int valvePins[3] = {VALVE1_PIN, VALVE2_PIN, VALVE3_PIN};

// ============================================================
//  INTERRUPT HANDLER
// ============================================================
void IRAM_ATTR onFlow1() { pulseCount[0]++; }
void IRAM_ATTR onFlow2() { pulseCount[1]++; }
void IRAM_ATTR onFlow3() { pulseCount[2]++; }

// ============================================================
//  FUNGSI AKTUATOR
// ============================================================
bool adaLineAktif() {
  for (int i = 0; i < 3; i++) if (lineActive[i]) return true;
  return false;
}

void allActuatorsOff() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(pumpPins[i], LOW);
    digitalWrite(valvePins[i], LOW);
    lineActive[i] = false;
  }
  Serial.println(">> SEMUA AKTUATOR OFF");
}

void bukaLine(int line) {
  if (line < 0 || line > 2) {
    Serial.println("[ERR] Line invalid");
    return;
  }
  
  Serial.print(">> BUKA LINE "); Serial.println(line + 1);
  digitalWrite(valvePins[line], HIGH);
  delay(150);
  digitalWrite(pumpPins[line], HIGH);
  lineActive[line] = true;
  initialWeight[line] = lastWeight;
  pulseCount[line] = 0;
}

void tutupLine(int line) {
  if (line < 0 || line > 2) {
    Serial.println("[ERR] Line invalid");
    return;
  }
  
  Serial.print(">> TUTUP LINE "); Serial.println(line + 1);
  digitalWrite(pumpPins[line], LOW);
  digitalWrite(valvePins[line], LOW);
  lineActive[line] = false;
}

// ============================================================
//  FUNGSI PROSES PAYLOAD DARI TOPIC "success"
// ============================================================
void prosesSuccessPayload(const char* payload, unsigned int len) {
  Serial.println("[SUCCESS] Processing payment notification...");
  
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload, len);
  
  if (err) {
    Serial.print("[ERR] JSON gagal parse: ");
    Serial.println(err.c_str());
    return;
  }
  
  const char* order_id = doc["order_id"];
  const char* transaction_status = doc["transaction_status"];
  const char* payment_type = doc["payment_type"];
  const char* gross_amount = doc["gross_amount"];
  const char* transaction_time = doc["transaction_time"];
  
  Serial.println("========================================");
  Serial.println("📦 PAYMENT RECEIVED!");
  Serial.print("   Order ID    : "); Serial.println(order_id);
  Serial.print("   Status      : "); Serial.println(transaction_status);
  Serial.print("   Payment     : "); Serial.println(payment_type);
  Serial.print("   Amount      : Rp "); Serial.println(gross_amount);
  Serial.print("   Time        : "); Serial.println(transaction_time);
  Serial.println("========================================");
  
  if (strcmp(transaction_status, "settlement") == 0) {
    Serial.println("✅ Payment SUCCESS! Menjalankan logika...");
    
    String orderId = String(order_id);
    int firstDash = orderId.indexOf('-');
    int secondDash = orderId.indexOf('-', firstDash + 1);
    int thirdDash = orderId.indexOf('-', secondDash + 1);
    int fourthDash = orderId.indexOf('-', thirdDash + 1);
    int fifthDash = orderId.indexOf('-', fourthDash + 1);
    
    if (firstDash != -1 && secondDash != -1 && thirdDash != -1 && fourthDash != -1 && fifthDash != -1) {
      String product = orderId.substring(thirdDash + 1, fourthDash);
      String volumeStr = orderId.substring(fourthDash + 1, fifthDash);
      String priceStr = orderId.substring(fifthDash + 1);
      
      Serial.print("   Product : "); Serial.println(product);
      Serial.print("   Volume  : "); Serial.println(volumeStr);
      Serial.print("   Price   : Rp "); Serial.println(priceStr);
      
      int line = -1;
      if (product == "COFFE_BREW") {
        line = 0;
      } else if (product == "MILK_TEA") {
        line = 1;
      } else if (product == "FRUIT_JUICE") {
        line = 2;
      }
      
      if (line != -1) {
        Serial.print("▶️ Menjalankan LINE "); Serial.println(line + 1);
        bukaLine(line);
        
        int volumeMl = volumeStr.toInt();
        Serial.print("   Target Volume: "); Serial.print(volumeMl); Serial.println(" ml");
        mqtt.publish(TOPIC_STATUS, "filling", true);
      } else {
        Serial.println("❌ Product tidak dikenal!");
      }
    } else {
      Serial.println("❌ Format order_id tidak sesuai!");
    }
  } else {
    Serial.print("⚠️ Status transaksi: ");
    Serial.println(transaction_status);
  }
}

// ============================================================
//  PEMROSESAN PERINTAH DARI MQTT
// ============================================================
void prosesPerintah(const char* payload, unsigned int len) {
  cmdCount++;

  StaticJsonDocument<256> cmd;
  DeserializationError err = deserializeJson(cmd, payload, len);
  
  if (err) {
    Serial.print("[ERR] JSON gagal parse: ");
    Serial.println(err.c_str());
    return;
  }
  
  if (!cmd.containsKey("action")) {
    Serial.println("[ERR] key 'action' tidak ada");
    return;
  }

  String action = cmd["action"].as<String>();
  int line = cmd["line"] | -1;

  Serial.print("[CMD] action="); Serial.print(action);
  Serial.print(" line="); Serial.println(line);

  if (action == "open") {
    bukaLine(line);
  }
  else if (action == "close") {
    tutupLine(line);
  }
  else if (action == "stop_all") {
    allActuatorsOff();
  }
  else {
    Serial.print("[ERR] action tidak dikenal: ");
    Serial.println(action);
  }
}

// ============================================================
//  CALLBACK MQTT
// ============================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("[RX] Topic: "); Serial.print(topic);
  Serial.print(" | Payload: ");
  
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  if (strcmp(topic, TOPIC_COMMAND) == 0) {
    prosesPerintah((const char*)payload, length);
  }
  else if (strcmp(topic, TOPIC_SUCCESS) == 0) {
    prosesSuccessPayload((const char*)payload, length);
  }
}

// ============================================================
//  FUNGSI KONEKSI
// ============================================================
void sambungWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("[WiFi] Menyambung ke "); Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Terhubung. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] GAGAL menyambung");
  }
}

bool sambungMQTT() {
  if (WiFi.status() != WL_CONNECTED) return false;

  Serial.print("[MQTT] Menyambung ke "); Serial.print(MQTT_HOST);
  Serial.print(":"); Serial.println(MQTT_PORT);

  bool ok;
  if (strlen(MQTT_USER) > 0) {
    ok = mqtt.connect(MQTT_ID, MQTT_USER, MQTT_PASS,
                      TOPIC_STATUS, 1, true, "offline");
  } else {
    ok = mqtt.connect(MQTT_ID, TOPIC_STATUS, 1, true, "offline");
  }

  if (ok) {
    Serial.println("[MQTT] Terhubung");
    mqtt.publish(TOPIC_STATUS, "online", true);
    mqtt.subscribe(TOPIC_COMMAND, 1);
    mqtt.subscribe(TOPIC_SUCCESS, 1);
    Serial.print("[MQTT] Subscribe: "); Serial.println(TOPIC_COMMAND);
    Serial.print("[MQTT] Subscribe: "); Serial.println(TOPIC_SUCCESS);
  } else {
    Serial.print("[MQTT] Gagal, rc=");
    Serial.println(mqtt.state());
  }
  return ok;
}

void testConnection() {
  Serial.println("=== TEST KONEKSI ===");
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MQTT Host: ");
  Serial.println(MQTT_HOST);
  Serial.print("MQTT Connected: ");
  Serial.println(mqtt.connected() ? "Yes" : "No");
  Serial.println("===================");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("   REFILLX ESP32 — MQTT GATEWAY CLIENT");
  Serial.println("==========================================");

  // 1. Setup aktuator pins
  for (int i = 0; i < 3; i++) {
    pinMode(pumpPins[i], OUTPUT);
    pinMode(valvePins[i], OUTPUT);
    digitalWrite(pumpPins[i], LOW);
    digitalWrite(valvePins[i], LOW);
  }
  Serial.println("[1/5] Relay pin OUTPUT + LOW");

  // 2. Setup flow meter
  pinMode(FLOW1_PIN, INPUT_PULLUP);
  pinMode(FLOW2_PIN, INPUT_PULLUP);
  pinMode(FLOW3_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW1_PIN), onFlow1, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW2_PIN), onFlow2, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOW3_PIN), onFlow3, FALLING);
  Serial.println("[2/5] Flow meter interrupt OK");

  // 3. Setup HX711
  Serial.println("[3/5] Cek HX711...");
  if (PAKAI_HX711) {
    scale.begin(LOADCELL_DT, LOADCELL_SCK);
    unsigned long t0 = millis();
    while (!scale.is_ready() && millis() - t0 < 2000) delay(10);
    hxOK = scale.is_ready();
    if (hxOK) {
      scale.set_scale(LOADCELL_FACTOR);
      scale.tare();
      Serial.println("      HX711 TERDETEKSI");
    } else {
      Serial.println("      HX711 TIDAK SIAP — dilewati");
    }
  } else {
    hxOK = false;
    Serial.println("      HX711 DINONAKTIFKAN");
  }

  // 4. Setup VL53L0X
  Serial.println("[4/5] Cek VL53L0X...");
  if (PAKAI_TOF) {
    Wire.begin(TOF_SDA, TOF_SCL);
    Wire.setTimeout(200);
    delay(100);
    tof.setTimeout(500);
    tofOK = tof.init();
    Serial.println(tofOK ? "      ToF TERDETEKSI" : "      ToF TIDAK ADA — interlock bypass");
  } else {
    tofOK = false;
    Serial.println("      ToF DINONAKTIFKAN");
  }

  // 5. Setup koneksi jaringan
  Serial.println("[5/5] Menyiapkan jaringan...");
  sambungWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(15);
  sambungMQTT();

  testConnection();

  Serial.println("==========================================");
  Serial.println("Sistem siap! Menunggu perintah...");
  Serial.println();
}

// ============================================================
//  LOOP UTAMA
// ============================================================
void loop() {
  // ---- JAGA KONEKSI + FAILSAFE ----
  if (WiFi.status() != WL_CONNECTED) {
    if (adaLineAktif()) {
      Serial.println("[FAILSAFE] WiFi putus saat mengisi — aktuator dimatikan");
      allActuatorsOff();
    }
    if (millis() - lastReconnect > 5000) {
      lastReconnect = millis();
      sambungWiFi();
    }
  }
  else if (!mqtt.connected()) {
    if (adaLineAktif()) {
      Serial.println("[FAILSAFE] MQTT putus saat mengisi — aktuator dimatikan");
      allActuatorsOff();
    }
    if (millis() - lastReconnect > 3000) {
      lastReconnect = millis();
      sambungMQTT();
    }
  }
  else {
    mqtt.loop();
  }

  // ---- BACA SENSOR ----
  float weight = hxOK ? scale.get_units(5) : 0;
  lastWeight = weight;

  int  distance;
  bool containerPresent;
  if (tofOK) {
    distance = tof.readRangeSingleMillimeters();
    containerPresent = (distance <= TOF_MAX_DISTANCE) && !tof.timeoutOccurred();
  } else {
    distance = -1;
    containerPresent = true;
  }

  // ---- SAFETY INTERLOCK ----
  if (!containerPresent && adaLineAktif()) {
    Serial.println("[INTERLOCK] Wadah tidak terdeteksi — aktuator dimatikan");
    allActuatorsOff();
  }

  // ---- HITUNG VOLUME ----
  float volumeFlow[3], weightDelta[3];
  for (int i = 0; i < 3; i++) {
    volumeFlow[i] = (pulseCount[i] / CAL_FACTOR[i]) * (1000.0 / 60.0);
    weightDelta[i] = lineActive[i] ? (weight - initialWeight[i]) : 0;
  }

  // ---- KIRIM TELEMETRI (setiap 200ms) ----
  if (millis() - lastTelemetry > 200) {
    lastTelemetry = millis();

    StaticJsonDocument<512> doc;
    doc["weight"]            = weight;
    doc["distance_mm"]       = distance;
    doc["tof_ok"]            = tofOK;
    doc["hx_ok"]             = hxOK;
    doc["cmd_count"]         = cmdCount;
    doc["rssi"]              = WiFi.RSSI();
    doc["container_present"] = containerPresent;
    doc["valid_weight"]      = (weight >= TARE_THRESHOLD);

    JsonArray vol    = doc.createNestedArray("vol_flow_ml");
    JsonArray wdelta = doc.createNestedArray("weight_delta_g");
    JsonArray active = doc.createNestedArray("line_active");
    for (int i = 0; i < 3; i++) {
      vol.add(volumeFlow[i]);
      wdelta.add(weightDelta[i]);
      active.add(lineActive[i]);
    }

    char buf[512];
    size_t n = serializeJson(doc, buf);
    if (mqtt.connected()) {
      mqtt.publish(TOPIC_TELEMETRY, buf, n);
    }

    // Heartbeat setiap 1 detik
    if (millis() - lastHeartbeat > 1000) {
      lastHeartbeat = millis();
      Serial.print("[HB] w="); Serial.print(weight, 1);
      Serial.print(" d="); Serial.print(distance);
      Serial.print(" cmd="); Serial.print(cmdCount);
      Serial.print(" wifi="); Serial.print(WiFi.status() == WL_CONNECTED ? "OK" : "--");
      Serial.print(" mqtt="); Serial.print(mqtt.connected() ? "OK" : "--");
      Serial.print(" act=");
      for (int i = 0; i < 3; i++) Serial.print(lineActive[i] ? "1" : "0");
      Serial.println();
    }
  }

  delay(20);
}