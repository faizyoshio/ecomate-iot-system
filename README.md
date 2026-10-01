# ECO-MATE: Sistem IoT & Monitoring Komposter Rotari Otomatis

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://www.espressif.com/)
[![Engine: Node--RED](https://img.shields.io/badge/Engine-Node--RED-darkred.svg)](https://nodered.org/)
[![Protocol: MQTT](https://img.shields.io/badge/Protocol-MQTT%201883-orange.svg)](https://mqtt.org/)
[![UI: Mobile--First](https://img.shields.io/badge/UI-Mobile--First%20Responsive-emerald.svg)](https://nodered.org/)

Sistem akuisisi data bioproses dan otomasi kendali aktuator komposter rotari drum horizontal berbasis **NodeMCU ESP32** dan **Node-RED Dashboard**. Dirancang untuk penelitian efektivitas ko-komposting aerobik serasah daun dan sisa makanan kantin di **Jurusan Kesehatan Lingkungan, Poltekkes Kemenkes Bandung (Kelompok 3A)**.

---

## Tampilan Antarmuka Dashboard (UI/UX)

Dashboard dibangun dengan prinsip **Mobile-First Responsive**, tipografi industrial modern, dan palet warna semantik *Deep Slate* (60-30-10 rule) untuk pemantauan parameter bioproses secara *real-time*.

### 1. Tampilan Desktop & Tablet
Tampilan dua kolom yang efisien untuk memantau indikator bioproses, status pengaduk, kurva dinamika suhu harian, dan entri rekaman data riset secara simultan.

![Tampilan Dashboard Desktop](docs/images/dashboard-desktop.png)

### 2. Tampilan Mobile Smartphone
Tampilan satu kolom (*single-column*) yang dioptimalkan untuk pengoperasian lapangan dengan satu jempol, dial radial kontras tinggi, tombol aksi minimum $48\text{ px}$, dan nol pergeseran horizontal (*zero horizontal scroll* pada viewport $375\text{--}390\text{ px}$).

![Tampilan Dashboard Mobile](docs/images/dashboard-mobile.png)

---

## Fitur Utama Sistem

1. **Embedded MQTT Broker (Aedes Port 1883)**:
   - Node-RED langsung bertindak sebagai MQTT Broker lokal tanpa memerlukan instalasi aplikasi broker pihak ketiga (seperti Mosquitto eksternal).
2. **Monitoring Multi-Sensor Bioproses**:
   - **Suhu Inti Biomassa**: Probe SUS 304 waterproof DS18B20 ($0\text{--}80^\circ\text{C}$) dengan zona termofilik hijau ($45\text{--}65^\circ\text{C}$).
   - **Kadar Air Media**: Capacitive Soil Moisture v1.2 ($0\text{--}100\%$, target ideal $40\text{--}70\%$).
   - **Derajat Keasaman (pH)**: Elektroda analog kaca E-201-C ($0\text{--}14\text{ pH}$).
3. **Kendali Aktuator Pengadukan Drum (17.5 RPM)**:
   - **Indikator Operasional**: Status real-time motor penggerak (`Aktif (17.5 RPM)` vs `Standby`).
   - **Saklar Manual**: Toggle saklar ON/OFF seketika dari dashboard.
   - **Tombol Cepat**: Pemicu putaran pengadukan drum selama tepat 10 menit.
   - **Otomasi Siklus Terjadwal**: Siklus putar otomatis 10 menit setiap 4 jam.
4. **Pencatatan Data Otomatis (Datalogger 21 Hari)**:
   - Setiap paket telemetri otomatis dicatat ke berkas CSV lokal untuk analisis data statistik, pembuatan grafik kinetika dekomposisi, dan lampiran laporan akhir.

---

## Arsitektur Sistem

```text
[ Sensor-Sensor Drum ]
  - DS18B20 (Suhu Inti SUS 304)
  - Capacitive v1.2 (Kadar Air)
  - E-201-C (Derajat Keasaman pH)
         │
         ▼
  [ NodeMCU ESP32 ] ──(GPIO 26)──> [ Modul Relay & Kontaktor Motor 17.5 RPM ]
         │
         │ (Wi-Fi 2.4 GHz / MQTT JSON Telemetry Port 1883)
         ▼
 [ Laptop: Node-RED Server ]
   ├── Aedes MQTT Broker (Port 1883)
   ├── Bioprocess Decision Engine
   ├── CSV Datalogger (data-ecomate-log.csv)
   └── Web UI Dashboard (Port 1880/ui) ──> Dapat diakses di Laptop & Smartphone
```

---

## Struktur Direktori Repositori

```text
ecomate-iot-system/
├── .gitignore                      # Pengecualian berkas temporer, kredensial, & build
├── LICENSE                         # Lisensi sumber terbuka MIT
├── README.md                       # Dokumentasi resmi proyek
├── docs/
│   └── images/
│       ├── dashboard-desktop.png   # Tangkapan layar antarmuka desktop
│       └── dashboard-mobile.png    # Tangkapan layar antarmuka mobile
├── firmware/
│   └── ecomate-esp32/
│       └── ecomate-esp32.ino       # Kode sumber C++ Arduino untuk NodeMCU ESP32
├── nodered/
│   └── flows.json                  # Konfigurasi flow aktif Node-RED Dashboard
└── simulation/
    └── simulate-esp32.py           # Simulator telemetri Python untuk uji coba tanpa alat fisik
```

---

## Pengkabelan & Pinout ESP32

Sambungkan sensor dan modul ke pin NodeMCU ESP32 mengikuti konfigurasi tabel di bawah:

| Komponen | Pin Modul | Pin ESP32 | Keterangan & Catatan Wiring |
| :--- | :--- | :--- | :--- |
| **Sensor Suhu DS18B20** | VCC | 3.3V | Beri resistor *pull-up* $4.7\text{ k}\Omega$ antara VCC dan DATA |
| | GND | GND | |
| | DATA | **GPIO 4** | Jalur komunikasi 1-Wire |
| **Capacitive Moisture v1.2** | VCC | 3.3V | Gunakan rel 3.3V agar output aman bagi ADC ESP32 |
| | GND | GND | |
| | AOUT | **GPIO 34** | Kanal ADC1 (bebas dari interferensi Wi-Fi) |
| **pH Probe E-201-C** | VCC | 5V | Modul analog signal conditioning |
| | GND | GND | Wajib satukan ground (*common ground*) |
| | PO | **GPIO 35** | Kanal ADC1 |
| **Modul Relay Motor** | VCC | 5V | Jalur daya koil relay |
| | GND | GND | |
| | IN | **GPIO 26** | Mentrigger koil kontaktor Schneider LC1D09 |
| **Buzzer Alarm 85 dB** | (+) | **GPIO 27** | Alarm lokal saat suhu $>70^\circ\text{C}$ |
| | (-) | GND | |

---

## Panduan Pemasangan & Menjalankan (Step-by-Step)

### 1. Persiapan Komputer / Laptop Pengendali
1. Pasang **Node.js** (LTS) dari situs resmi `nodejs.org`.
2. Pasang **Node-RED** secara global:
   ```bash
   npm install -g --unsafe-perm node-red
   ```
3. Pasang modul dependensi di folder data Node-RED (`~/.node-red`):
   ```bash
   cd ~/.node-red
   npm install node-red-dashboard node-red-contrib-aedes
   ```
4. Salin berkas flow dari repositori ini ke folder Node-RED:
   - Salin file `nodered/flows.json` ke direktori user Node-RED kamu (`C:\Users\<user>\.node-red\flows.json` di Windows).
5. Jalankan server Node-RED:
   ```bash
   node-red
   ```

### 2. Mengetahui Alamat IP Lokal Laptop
Buka terminal/PowerShell dan jalankan:
```powershell
ipconfig
```
Catat nilai **IPv4 Address** (contoh: `192.168.1.15`). Alamat ini yang dimasukkan ke firmware ESP32.

### 3. Konfigurasi & Upload Firmware ESP32
1. Buka berkas `firmware/ecomate-esp32/ecomate-esp32.ino` di **Arduino IDE**.
2. Pasang library yang dibutuhkan via **Library Manager**:
   - `PubSubClient`
   - `OneWire`
   - `DallasTemperature`
   - `ArduinoJson`
3. Sesuaikan parameter jaringan pada bagian atas kode:
   ```cpp
   const char* WIFI_SSID     = "NAMA_WIFI_KAMU";
   const char* WIFI_PASSWORD = "PASSWORD_WIFI_KAMU";
   const char* MQTT_SERVER   = "192.168.1.15"; // Ganti dengan IP laptop kamu
   const int   MQTT_PORT     = 1883;
   ```
4. Pilih Board **ESP32 Dev Module**, pilih port COM yang sesuai, dan klik **Upload**.

### 4. Mengakses Dashboard
- **Melalui Laptop**: Buka peramban ke `http://localhost:1880/ui`
- **Melalui Smartphone**: Sambungkan ponsel ke Wi-Fi yang sama, lalu buka `http://<IP_LAPTOP>:1880/ui` (contoh: `http://192.168.1.15:1880/ui`).

---

## Pengujian Tanpa Perangkat Keras (Simulator Python)

Jika unit komposter atau modul sensor masih dalam proses perakitan mekanis, kamu dapat memverifikasi visualisasi dashboard menggunakan skrip simulasi:

1. Pasang pustaka klien MQTT:
   ```bash
   pip install paho-mqtt
   ```
2. Jalankan skrip simulator:
   ```bash
   python simulation/simulate-esp32.py
   ```
3. Simulator akan menyuntikkan data sensor ke broker MQTT setiap 5 detik dan merespons perintah pengadukan motor secara real-time.

---

## Format Data Telemetri & Datalogger

### Payload MQTT JSON (`ecomate/telemetry`)
```json
{
  "suhu": 56.0,
  "kelembaban": 57.1,
  "ph": 6.95,
  "motor": 1
}
```

### Rekaman Berkas CSV (`data-ecomate-log.csv`)
Setiap data yang masuk secara otomatis dicatat untuk kebutuhan analisis eksperimen 21 hari:
```csv
Timestamp,Suhu,Kelembaban,pH,StatusMotor
2026-10-01 20:41:38,56.0,57.1,6.95,1
2026-10-01 20:42:08,56.2,57.0,6.96,1
```

---

## Lisensi

Proyek ini dilisensikan di bawah lisensi terbuka [MIT License](LICENSE).
Dikembangkan untuk tujuan akademik, penelitian sanitasi lingkungan, dan inovasi teknologi tepat guna.
