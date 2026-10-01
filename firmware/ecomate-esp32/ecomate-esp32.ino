/*
  ================================================================================
  PROYEK ECO-MATE (Ecological Machine for Automated Rotary Drum Composting)
  Program Mikrokontroler NodeMCU ESP32
  Kelompok 3A - Jurusan Kesehatan Lingkungan Poltekkes Kemenkes Bandung
  ================================================================================
  Spesifikasi Hardware & Pinout:
  - ESP32 NodeMCU 32-bit Wi-Fi/Bluetooth
  - Sensor Suhu Inti: Maxim DS18B20 SUS 304 Waterproof (1-Wire Pin GPIO 4)
  - Sensor Kelembaban: Capacitive Soil Moisture Sensor v1.2 (Analog ADC Pin GPIO 34)
  - Sensor pH: Elektroda Analog E-201-C & Signal Conditioning (Analog ADC Pin GPIO 35)
  - Limit Switch Interlock K3 Pintu: GPIO 18 (Input Pull-up)
  - Tombol Emergency Stop (E-Stop): GPIO 19 (Input Pull-up)
  - Relay Koil Kontaktor Motor 1-Phasa 0.5 HP 17.5 RPM: GPIO 26 (Active LOW/HIGH)
  - Buzzer Alarm K3 85 dB: GPIO 27 (Output)
  ================================================================================
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

// --- Konfigurasi Jaringan Wi-Fi & MQTT Broker ---
const char* WIFI_SSID     = "NAMA_WIFI_KAMU";     // Ganti dengan SSID Wi-Fi
const char* WIFI_PASSWORD = "PASSWORD_WIFI_KAMU"; // Ganti dengan Password Wi-Fi
const char* MQTT_SERVER   = "192.168.1.100";      // IP Laptop yang menjalankan Node-RED
const int   MQTT_PORT     = 1883;

const char* TOPIC_TELEMETRY = "ecomate/telemetry";
const char* TOPIC_CMD_MOTOR = "ecomate/cmd/motor";

// --- Definisi Pinout GPIO ESP32 ---
const int PIN_DS18B20     = 4;   // 1-Wire Data bus
const int PIN_SOIL_ADC    = 34;  // ADC1_CH6 (Capacitive Moisture)
const int PIN_PH_ADC      = 35;  // ADC1_CH7 (pH Probe E-201-C)
const int PIN_LIMIT_DOOR  = 18;  // Limit switch pintu drum (NC / NO)
const int PIN_ESTOP       = 19;  // E-Stop button
const int PIN_RELAY_MOTOR = 26;  // Triggers Schneider LC1D09 Contactor
const int PIN_BUZZER      = 27;  // Buzzer 85 dB

// --- Kalibrasi Sensor ---
// Kalibrasi Moisture Capacitive (Sesuaikan hasil uji udara bebas vs air penuh)
const int AIR_VALUE   = 3200;  // Nilai ADC saat sensor kering (0%)
const int WATER_VALUE = 1500;  // Nilai ADC saat sensor terendam air jenuh (100%)

// Kalibrasi pH Probe E-201-C (Buffer 4.01 & 7.00)
const float PH_NEUTRAL_VOLTAGE = 2.50; // Tegangan modul pada pH 7.0
const float PH_SLOPE           = 3.50; // Sensitivitas probe (V/pH)

// --- Inisialisasi Objek ---
OneWire oneWire(PIN_DS18B20);
DallasTemperature ds18b20(&oneWire);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Status sistem
bool motorRunning = false;
bool doorSafe = true;
bool estopSafe = true;
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL = 30000; // Kirim data tiap 30 detik

void setupPins() {
  pinMode(PIN_RELAY_MOTOR, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LIMIT_DOOR, INPUT_PULLUP);
  pinMode(PIN_ESTOP, INPUT_PULLUP);

  digitalWrite(PIN_RELAY_MOTOR, LOW); // Motor mati di awal
  digitalWrite(PIN_BUZZER, LOW);
  analogReadResolution(12); // ADC 12-bit (0 - 4095)
}

void connectWiFi() {
  Serial.print("[WiFi] Menghubungkan ke: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Berhasil terhubung!");
    Serial.print("[WiFi] IP ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Gagal terhubung. Menunggu siklus berikutnya...");
  }
}

void setMotor(bool state) {
  // Verifikasi K3: Jika pintu terbuka atau E-Stop aktif, motor DILARANG menyala!
  if (state && (!doorSafe || !estopSafe)) {
    Serial.println("[K3 SAFETY] Motor dicegah hidup karena pintu terbuka atau E-Stop!");
    digitalWrite(PIN_RELAY_MOTOR, LOW);
    motorRunning = false;
    tone(PIN_BUZZER, 1000, 500);
    return;
  }

  if (state) {
    digitalWrite(PIN_RELAY_MOTOR, HIGH);
    motorRunning = true;
    Serial.println("[AKTUATOR] Motor Drum 17.5 RPM: AKTIF");
  } else {
    digitalWrite(PIN_RELAY_MOTOR, LOW);
    motorRunning = false;
    Serial.println("[AKTUATOR] Motor Drum 17.5 RPM: MATI");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("[MQTT] Pesan masuk [Topic: ");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (String(topic) == TOPIC_CMD_MOTOR) {
    if (message == "1" || message == "true") {
      setMotor(true);
    } else {
      setMotor(false);
    }
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Menghubungi Broker di ");
    Serial.print(MQTT_SERVER);
    Serial.print("...");

    String clientId = "ESP32_ECOMATE_" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" Terhubung!");
      mqttClient.subscribe(TOPIC_CMD_MOTOR);
      Serial.println("[MQTT] Berlangganan ke topik: " + String(TOPIC_CMD_MOTOR));
    } else {
      Serial.print(" Gagal, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Mencoba lagi dalam 3 detik...");
      delay(3000);
      break;
    }
  }
}

float readMoisturePercentage() {
  int rawADC = analogRead(PIN_SOIL_ADC);
  int percent = map(rawADC, AIR_VALUE, WATER_VALUE, 0, 100);
  return (float)constrain(percent, 0, 100);
}

float readPHValue() {
  int rawADC = analogRead(PIN_PH_ADC);
  float voltage = (rawADC / 4095.0) * 3.3;
  float ph = 7.0 + ((PH_NEUTRAL_VOLTAGE - voltage) * PH_SLOPE);
  return constrain(ph, 0.0, 14.0);
}

void sendTelemetry() {
  // 1. Baca Suhu Inti DS18B20
  ds18b20.requestTemperatures();
  float suhu = ds18b20.getTempCByIndex(0);
  if (suhu < -50.0 || suhu > 125.0) {
    suhu = 0.0; // Sensor error fallback
  }

  // 2. Baca Kelembaban Media
  float kelembaban = readMoisturePercentage();

  // 3. Baca pH
  float ph = readPHValue();

  // 4. Periksa Status Safety K3
  doorSafe = (digitalRead(PIN_LIMIT_DOOR) == HIGH); // HIGH = pintu tertutup
  estopSafe = (digitalRead(PIN_ESTOP) == HIGH);

  if (!doorSafe && motorRunning) {
    setMotor(false); // Cut-off seketika
  }

  // 5. Logika Alarm Lokal (Buzzer)
  if (suhu >= 70.0 || kelembaban < 40.0) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // 6. Buat Paket JSON
  StaticJsonDocument<256> doc;
  doc["suhu"] = round(suhu * 10.0) / 10.0;
  doc["kelembaban"] = round(kelembaban * 10.0) / 10.0;
  doc["ph"] = round(ph * 100.0) / 100.0;
  doc["motor"] = motorRunning ? 1 : 0;
  doc["interlock"] = (doorSafe && estopSafe) ? 1 : 0;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);

  // 7. Publish ke Node-RED
  if (mqttClient.connected()) {
    mqttClient.publish(TOPIC_TELEMETRY, jsonBuffer);
    Serial.print("[TELEMETRI] Terkirim: ");
    Serial.println(jsonBuffer);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=========================================");
  Serial.println("  SISTEM IOT KOMPOSTER ROTARI ECO-MATE   ");
  Serial.println("  Poltekkes Kemenkes Bandung Kelompok 3A ");
  Serial.println("=========================================");

  setupPins();
  ds18b20.begin();

  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Kirim data berkala
  unsigned long currentMillis = millis();
  if (currentMillis - lastTelemetryTime >= TELEMETRY_INTERVAL) {
    lastTelemetryTime = currentMillis;
    sendTelemetry();
  }
}
