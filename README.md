# 🚰 RefillX - Dispenser Pintar (Smart Dispenser)

Sistem otomatis untuk penjualan minuman menggunakan wadah *refillable* dengan pembayaran online terintegrasi via **Midtrans** dan komunikasi IoT menggunakan **MQTT**.

## 📋 Daftar Isi

- [Fitur Utama](#-fitur-utama)
- [Arsitektur Sistem](#-arsitektur-sistem)
- [Komponen Teknologi](#-komponen-teknologi)
- [Instalasi](#-instalasi)
- [Konfigurasi](#-konfigurasi)
- [Penggunaan](#-penggunaan)
- [Struktur Proyek](#-struktur-proyek)
- [Topik MQTT](#-topik-mqtt)
- [Format Pesan](#-format-pesan)
- [Troubleshooting](#-troubleshooting)
- [Lisensi](#-lisensi)

---

## 🎯 Fitur Utama

✅ **Integrasi Pembayaran Midtrans**
- Menerima notifikasi pembayaran dari remote broker
- Parsing order otomatis dari Midtrans

✅ **Kontrol ESP32 via MQTT**
- Komunikasi lokal dengan mikrokontroler ESP32
- Perintah pembukaan/penutupan saluran (lines) cairan
- Monitoring status pengisian real-time

✅ **Manajemen 3 Saluran Produk**
- Line 1: Mineral Water
- Line 2: Milk Tea / Isotonik Water
- Line 3: Coffee / Fruit Juice

✅ **Monitoring dan Telemetri**
- Status online/offline ESP32
- Progress pengisian (volume, target, persentase)
- Deteksi wadah dan timeout handling

✅ **Command Manual**
- Interface CLI interaktif
- Kontrol manual pembukaan/penutupan saluran
- Cek status real-time ESP32

---

## 🏗️ Arsitektur Sistem

```
┌─────────────────────┐
│   Midtrans Server   │  (Payment Gateway)
│   Remote Broker     │
└──────────────┬──────┘
               │
               │ MQTT (103.168.146.179:1883)
               │
┌──────────────▼──────────────┐
│  RefillX Controller         │
│  (Python + paho-mqtt)       │
│  main.py                    │
└──────────────┬──────────────┘
               │
               │ MQTT (127.0.0.1:1883)
               │ Mosquitto Local Broker
               │
┌──────────────▼──────────────┐
│      ESP32 + Hardware       │
│  - Pump Control             │
│  - Weight Sensor            │
│  - Container Detection      │
└─────────────────────────────┘
```

---

## 💻 Komponen Teknologi

| Komponen | Versi | Fungsi |
|----------|-------|--------|
| **Python** | 3.7+ | Bahasa pemrograman utama |
| **paho-mqtt** | 2.1.0 | Client MQTT untuk Python |
| **Mosquitto** | 2.x | MQTT Broker lokal (Windows) |
| **ESP32** | - | Mikrokontroler untuk kontrol hardware |
| **Midtrans** | - | Gateway pembayaran |

---

## 📦 Instalasi

### Prasyarat
- Python 3.7 atau lebih tinggi
- Mosquitto MQTT Broker (terinstal di `C:\Program Files\mosquitto`)
- ESP32 dengan firmware MQTT
- Koneksi internet untuk remote broker

### Langkah-Langkah

#### 1. Clone Repository
```bash
git clone https://github.com/ficrammanifur/dispenser-pintar.git
cd dispenser-pintar
```

#### 2. Buat Virtual Environment (Opsional)
```bash
python -m venv venv
venv\Scripts\activate
```

#### 3. Install Dependencies
```bash
pip install -r requirements.txt
```

Atau jalankan script installer (Windows):
```bash
INSTALL.bat
```

#### 4. Konfigurasi Mosquitto
Pastikan Mosquitto sudah terinstal dan berjalan. Konfigurasi biasanya di:
```
C:\Program Files\mosquitto\mosquitto.conf
```

---

## ⚙️ Konfigurasi

Buka file `main.py` dan sesuaikan konfigurasi berikut:

### Direktori Proyek
```python
PROJECT_DIR = r"C:\Users\muham\Project"
MOSQUITTO_DIR = r"C:\Program Files\mosquitto"
```

### Local MQTT (untuk ESP32)
```python
LOCAL_BROKER = "127.0.0.1"
LOCAL_PORT = 1883
```

### Remote MQTT (Midtrans Payment)
```python
REMOTE_BROKER = "103.168.146.179"
REMOTE_PORT = 1883
REMOTE_USERNAME = "refillx"
REMOTE_PASSWORD = "Password1!"
```

### Pemetaan Produk
```python
PRODUCT_MAP = {
    "COFFE_BREW": 2,        # Line 3
    "MILK_TEA": 1,          # Line 2
    "FRUIT_JUICE": 2,       # Line 3
    "MINERAL_WATER": 0,     # Line 1
    "ISOTONIK_WATER": 1     # Line 2
}
```

---

## 🚀 Penggunaan

### Jalankan Aplikasi
```bash
python main.py
```

### Output Awal
```
==============================================================
  REFILLX CONTROLLER
  Payment → ESP32 Gateway
==============================================================
[HH:MM:SS] [INFO] Starting Mosquitto Broker...
[HH:MM:SS] [INFO] ✅ Mosquitto started
[HH:MM:SS] [INFO] Connecting to Local Broker...
[HH:MM:SS] [INFO] ✅ Connected to Local Broker
[HH:MM:SS] [INFO] Connecting to Remote Broker...
[HH:MM:SS] [INFO] ✅ Connected to Remote Broker
==============================================================
[HH:MM:SS] [INFO] ✅ SYSTEM READY!
[HH:MM:SS] [INFO]    Menunggu payment dari Midtrans...
[HH:MM:SS] [INFO]    Atau ketik command manual
==============================================================
```

### Perintah Manual CLI

#### 1. **Status ESP32**
```
> status

==================================================
📡 ESP32 STATUS
==================================================
   Online     : ✅ YES
   Filling    : NO
   Line       : -1
   Volume     : 0.0 ml
   Target     : 0 ml
   Progress   : 0%
   RSSI       : 0 dBm
   Last Update: 2026-08-23 12:45:30
==================================================
```

#### 2. **Buka Saluran (Open Line)**
```
> open 0
   Target volume (ml): 300
✅ open line 0 sent
```

Opsi line:
- `0` = Line 1 (Mineral Water)
- `1` = Line 2 (Milk Tea / Isotonik)
- `2` = Line 3 (Coffee / Fruit Juice)

#### 3. **Tutup Saluran (Close Line)**
```
> close 0
✅ close line 0 sent
```

#### 4. **Stop Semua**
```
> stop_all
✅ Stop all sent
```

#### 5. **Keluar**
```
> exit
```

---

## 📁 Struktur Proyek

```
dispenser-pintar/
├── main.py                 # Controller utama
├── requirements.txt        # Python dependencies
├── INSTALL.bat            # Script instalasi Windows
├── RUN.bat                # Script untuk menjalankan aplikasi
├── README.md              # Dokumentasi (file ini)
├── fimware/               # Firmware ESP32
└── mqtt/                  # Konfigurasi MQTT
```

---

## 📡 Topik MQTT

### Remote Broker (Midtrans)
| Topik | Arah | Format | Keterangan |
|-------|------|--------|-----------|
| `success` | IN | JSON | Notifikasi pembayaran dari Midtrans |

### Local Broker (ESP32)
| Topik | Arah | Format | Keterangan |
|-------|------|--------|-----------|
| `refillx/command` | OUT | JSON | Perintah ke ESP32 |
| `refillx/status` | IN | String | Status pengisian dari ESP32 |
| `refillx/telemetry` | IN | JSON | Data telemetri real-time |

---

## 📨 Format Pesan

### 1. Notifikasi Pembayaran (Remote)
**Topik:** `success`  
**Format:**
```json
{
  "order_id": "REFILLX-20260818-215507-MINERAL_WATER-300ML-1500",
  "transaction_status": "settlement",
  "gross_amount": "15000"
}
```

**Format Order ID:**
```
REFILLX-[DATE]-[TIME]-[PRODUCT]-[VOLUME]-[PRICE]
REFILLX-20260818-215507-MINERAL_WATER-300ML-1500

Parts Index:
- parts[0] = REFILLX (identifier)
- parts[1] = 20260818 (tanggal YYYYMMDD)
- parts[2] = 215507 (waktu HHMMSS)
- parts[3] = MINERAL_WATER (nama produk)
- parts[4] = 300ML (volume)
- parts[5] = 1500 (harga)
```

### 2. Perintah ke ESP32 (Local)
**Topik:** `refillx/command`  
**Format:**
```json
{
  "action": "open",
  "line": 0,
  "volume": 300
}
```

**Aksi Tersedia:**
- `open` - Buka saluran (perlu line dan volume)
- `close` - Tutup saluran (perlu line)
- `stop_all` - Stop semua

### 3. Telemetri dari ESP32 (Local)
**Topik:** `refillx/telemetry`  
**Format:**
```json
{
  "volume": 150.5,
  "target_ml": 300,
  "progress": 50,
  "is_filling": true,
  "line_active": 0,
  "weight": 450,
  "rssi": -65
}
```

### 4. Status dari ESP32 (Local)
**Topik:** `refillx/status`  
**Format:** String simpel
```
online              # ESP32 online
filling             # Sedang mengisi
done                # Pengisian selesai
no_container        # Wadah tidak terdeteksi
timeout             # Timeout pengisian
stopped             # Pengisian dihentikan
```

---

## 🔍 Troubleshooting

### ❌ Mosquitto Gagal Dijalankan
**Error:**
```
❌ Failed to start Mosquitto: ...
```

**Solusi:**
1. Pastikan Mosquitto terinstal di `C:\Program Files\mosquitto`
2. Jalankan Command Prompt sebagai Administrator
3. Periksa apakah port 1883 tidak digunakan program lain
   ```bash
   netstat -ano | findstr :1883
   ```

### ❌ Remote Broker Connection Failed
**Error:**
```
❌ Failed to connect remote: ...
```

**Solusi:**
1. Periksa koneksi internet
2. Verifikasi IP broker: `103.168.146.179`
3. Cek username/password remote broker
4. Pastikan firewall tidak memblokir port 1883

### ❌ ESP32 OFFLINE
**Error:**
```
❌ ESP32 OFFLINE! Tidak bisa memproses payment
```

**Solusi:**
1. Periksa koneksi WiFi ESP32
2. Pastikan ESP32 terhubung ke local broker (127.0.0.1:1883)
3. Cek firmware ESP32 sudah mendukung MQTT
4. Restart ESP32

### ⚠️ Payment Tidak Diproses
**Checklist:**
- ✅ Transaction status harus "settlement"
- ✅ Order ID format harus benar (6 bagian)
- ✅ Produk harus ada di PRODUCT_MAP
- ✅ Sistem tidak sedang mengisi (status is_filling)
- ✅ ESP32 harus online

### ⏰ Timeout Pengisian
**Solusi:**
1. Periksa kondisi hardware (pump, sensor weight)
2. Sesuaikan timeout value di firmware ESP32
3. Verifikasi volume target sesuai kapasitas wadah

---

## 📝 Format Pesan Error

Semua error log menggunakan format:
```
[HH:MM:SS] [ERROR] <pesan error>
```

Contoh:
```
[14:30:45] [ERROR] ❌ Product tidak dikenal: UNKNOWN_DRINK
[14:31:20] [ERROR] ❌ Failed to send command: Connection lost
```

---

## 🔐 Keamanan

⚠️ **Penting:**
- Jangan commit file dengan credentials asli
- Gunakan environment variables untuk:
  - `REMOTE_USERNAME`
  - `REMOTE_PASSWORD`
  - `REMOTE_BROKER`
- Proteksi akses lokal broker dengan firewall

---

## 📊 Monitoring Log

Log otomatis ditampilkan di console dengan format:
```
[TIMESTAMP] [LEVEL] MESSAGE
```

Level yang ada:
- `INFO` - Informasi umum
- `WARNING` - Peringatan
- `ERROR` - Error

---

## 🤝 Kontribusi

Untuk berkontribusi:
1. Fork repository
2. Buat branch fitur (`git checkout -b feature/AmazingFeature`)
3. Commit perubahan (`git commit -m 'Add some AmazingFeature'`)
4. Push ke branch (`git push origin feature/AmazingFeature`)
5. Buat Pull Request

---

## 📄 Lisensi

Proyek ini belum memiliki lisensi resmi. Silakan hubungi owner untuk informasi lebih lanjut.

---

## 👤 Author

**ficrammanifur**  
GitHub: [@ficrammanifur](https://github.com/ficrammanifur)

---

## 📞 Support

Untuk pertanyaan atau laporan bug, silakan buat issue di repository ini.

---

**Last Updated:** 23 Agustus 2026  
**Version:** 1.0.0
