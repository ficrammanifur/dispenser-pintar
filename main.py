"""
REFILLX MQTT SYSTEM - MAIN CONTROLLER
1. Subscribe ke topic "success" dari remote broker (Midtrans payment)
2. Parse payment notification
3. Kirim command ke ESP32 via local broker
4. Support manual command dengan volume
5. Monitor status ESP32 (online/offline, filling progress)
"""

import os
import json
import time
import subprocess
import paho.mqtt.client as mqtt
from datetime import datetime

# ============================================================
# KONFIGURASI
# ============================================================
PROJECT_DIR = r"C:\Users\muham\Project"
MQTT_DIR = os.path.join(PROJECT_DIR, "mqtt")
MOSQUITTO_DIR = r"C:\Program Files\mosquitto"

# LOCAL MQTT (untuk ESP32)
LOCAL_BROKER = "127.0.0.1"
LOCAL_PORT = 1883

# REMOTE MQTT (untuk payment dari Midtrans)
REMOTE_BROKER = ""
REMOTE_PORT = 1883
REMOTE_USERNAME = ""
REMOTE_PASSWORD = ""

# Topics
TOPIC_SUCCESS = "success"              # Dari remote broker (payment)
TOPIC_COMMAND = "refillx/command"      # Ke ESP32 (local)
TOPIC_STATUS = "refillx/status"        # Dari ESP32
TOPIC_TELEMETRY = "refillx/telemetry"  # Dari ESP32

# ============================================================
# 🔥 PRODUCT MAPPING - YANG BENAR!
# ============================================================
PRODUCT_MAP = {
    "COFFE_BREW": 2,        # Line 3 (index 2)
    "MILK_TEA": 1,          # Line 2 (index 1)
    "FRUIT_JUICE": 2,       # Line 3 (index 2)
    "MINERAL_WATER": 0,     # Line 1 (index 0)
    "ISOTONIK_WATER": 1     # Line 2 (index 1)
}

LINE_NAMES = {
    0: "Line 1 (Mineral Water)",
    1: "Line 2 (Milk Tea/Isotonik)",
    2: "Line 3 (Coffee/Fruit Juice)"
}

# ============================================================
# CLASS: Refillx Controller
# ============================================================
class RefillxController:
    def __init__(self):
        self.running = True
        self.is_filling = False
        self.current_order = None
        self.esp32_last_seen = None
        
        # MQTT Clients
        self.remote_client = None
        self.local_client = None
        
        # Status ESP32
        self.esp32_status = {
            "online": False,
            "weight": 0,
            "volume": 0,
            "target_ml": 0,
            "progress": 0,
            "is_filling": False,
            "line_active": -1,
            "rssi": 0,
            "last_update": None
        }
        
    def log(self, msg, level="INFO"):
        timestamp = datetime.now().strftime("%H:%M:%S")
        print(f"[{timestamp}] [{level}] {msg}")
        
    def start_mosquitto(self):
        """Start Mosquitto broker lokal"""
        self.log("Starting Mosquitto Broker...")
        try:
            subprocess.run(["taskkill", "/f", "/im", "mosquitto.exe"], 
                          capture_output=True, shell=True)
            
            proc = subprocess.Popen(
                [r"C:\Program Files\mosquitto\mosquitto.exe", "-v", "-c", "mosquitto.conf"],
                cwd=MOSQUITTO_DIR,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                creationflags=subprocess.CREATE_NEW_CONSOLE
            )
            time.sleep(2)
            self.log("✅ Mosquitto started")
            return True
        except Exception as e:
            self.log(f"❌ Failed to start Mosquitto: {e}", "ERROR")
            return False
            
    def connect_remote(self):
        """Connect ke remote broker untuk subscribe payment"""
        self.log("Connecting to Remote Broker...")
        self.remote_client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="refillx-controller"
        )
        self.remote_client.username_pw_set(REMOTE_USERNAME, REMOTE_PASSWORD)
        self.remote_client.on_connect = self.on_remote_connect
        self.remote_client.on_message = self.on_remote_message
        
        try:
            self.remote_client.connect(REMOTE_BROKER, REMOTE_PORT, 60)
            self.remote_client.loop_start()
            self.log("✅ Connected to Remote Broker")
            return True
        except Exception as e:
            self.log(f"❌ Failed to connect remote: {e}", "ERROR")
            return False
            
    def connect_local(self):
        """Connect ke local broker untuk kirim command ke ESP32"""
        self.log("Connecting to Local Broker...")
        self.local_client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id="refillx-controller-local"
        )
        self.local_client.on_connect = self.on_local_connect
        self.local_client.on_message = self.on_local_message
        
        try:
            self.local_client.connect(LOCAL_BROKER, LOCAL_PORT, 60)
            self.local_client.loop_start()
            self.log("✅ Connected to Local Broker")
            return True
        except Exception as e:
            self.log(f"❌ Failed to connect local: {e}", "ERROR")
            return False
            
    def on_remote_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.log("✅ Remote: Connected!")
            client.subscribe(TOPIC_SUCCESS)
            self.log(f"✅ Remote: Subscribed to {TOPIC_SUCCESS}")
        else:
            self.log(f"❌ Remote: Failed, rc={rc}", "ERROR")
            
    def on_remote_message(self, client, userdata, msg):
        """🔥 Menerima payment dari remote broker"""
        try:
            payload = msg.payload.decode()
            self.log("📩 Payment received from remote!")
            
            data = json.loads(payload)
            self.process_payment(data)
            
        except json.JSONDecodeError as e:
            self.log(f"❌ Invalid JSON: {e}", "ERROR")
        except Exception as e:
            self.log(f"❌ Error processing: {e}", "ERROR")
            
    def on_local_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self.log("✅ Local: Connected!")
            client.subscribe(TOPIC_STATUS)
            client.subscribe(TOPIC_TELEMETRY)
            self.log(f"✅ Local: Subscribed to {TOPIC_STATUS}, {TOPIC_TELEMETRY}")
        else:
            self.log(f"❌ Local: Failed, rc={rc}", "ERROR")
            
    def on_local_message(self, client, userdata, msg):
        """Menerima status dari ESP32"""
        try:
            payload = msg.payload.decode()
            
            if msg.topic == TOPIC_TELEMETRY:
                data = json.loads(payload)
                
                # Update status ESP32
                self.esp32_status["online"] = True
                self.esp32_status["volume"] = data.get('volume', 0)
                self.esp32_status["target_ml"] = data.get('target_ml', 0)
                self.esp32_status["progress"] = data.get('progress', 0)
                self.esp32_status["is_filling"] = data.get('is_filling', False)
                self.esp32_status["line_active"] = data.get('line_active', -1)
                self.esp32_status["last_update"] = datetime.now()
                self.esp32_last_seen = time.time()
                
                # Display progress
                self.display_progress()
                
            elif msg.topic == TOPIC_STATUS:
                status = payload
                self.log(f"📡 ESP32 Status: {status}")
                
                if status == "online":
                    self.esp32_status["online"] = True
                    self.esp32_last_seen = time.time()
                elif status == "done":
                    self.log("✅ PENGISIAN SELESAI!")
                    self.is_filling = False
                    self.current_order = None
                    self.esp32_status["is_filling"] = False
                    self.esp32_status["target_ml"] = 0
                    self.esp32_status["progress"] = 0
                elif status == "no_container":
                    self.log("❌ Wadah tidak terdeteksi!")
                elif status == "timeout":
                    self.log("⏰ Timeout! Pengisian dihentikan")
                    self.is_filling = False
                    self.current_order = None
                    self.esp32_status["is_filling"] = False
                elif status == "filling":
                    self.esp32_status["is_filling"] = True
                    self.is_filling = True
                elif status == "stopped":
                    self.esp32_status["is_filling"] = False
                    self.is_filling = False
                    
        except Exception as e:
            pass
            
    def display_progress(self):
        """Display progress pengisian"""
        status = self.esp32_status
        if status.get("is_filling", False):
            volume = status.get("volume", 0)
            target = status.get("target_ml", 0)
            progress = status.get("progress", 0)
            line = status.get("line_active", -1)
            
            if target > 0:
                bar_length = 20
                filled = int(bar_length * progress / 100)
                bar = "█" * filled + "░" * (bar_length - filled)
                
                line_name = LINE_NAMES.get(line, f"Line {line}")
                print(f"\r🔵 {line_name} | {volume:.0f}ml/{target}ml [{bar}] {progress}%", end='')
                
    def process_payment(self, data):
        """🔥 PROSES PAYMENT DAN KIRIM COMMAND KE ESP32"""
        self.log("=" * 60)
        self.log("💳 PROCESSING PAYMENT")
        
        # Cek status
        transaction_status = data.get("transaction_status", "")
        if transaction_status != "settlement":
            self.log(f"⚠️ Status: {transaction_status} (bukan settlement)", "WARNING")
            return
            
        # Cek sedang filling
        if self.is_filling:
            self.log("⚠️ Sedang mengisi, skip", "WARNING")
            return
            
        # Cek ESP32 online
        if not self.esp32_status.get("online", False):
            self.log("❌ ESP32 OFFLINE! Tidak bisa memproses payment", "ERROR")
            return
            
        # Ambil order_id
        order_id = data.get("order_id", "")
        gross_amount = data.get("gross_amount", "0")
        
        self.log(f"   Order ID: {order_id}")
        self.log(f"   Amount  : Rp {gross_amount}")
        
        # 🔥 PARSE ORDER_ID
        # Format: REFILLX-20260818-215507-MINERAL_WATER-300ML-1500
        # Index:  0       1         2        3             4     5
        parts = order_id.split('-')
        
        if len(parts) < 6:
            self.log(f"❌ Format order_id salah: {order_id}", "ERROR")
            return
        
        # 🔥 INDEX YANG BENAR:
        # parts[0] = "REFILLX"
        # parts[1] = "20260818" (date)
        # parts[2] = "215507" (time)
        # parts[3] = "MINERAL_WATER" (PRODUCT)
        # parts[4] = "300ML" (VOLUME)
        # parts[5] = "1500" (PRICE)
        
        product = parts[3]      # INDEX 3 = PRODUCT
        volume_str = parts[4]   # INDEX 4 = VOLUME (contoh: "300ML")
        price = parts[5]        # INDEX 5 = PRICE
        
        # Hapus "ML" dari volume
        volume_ml = int(volume_str.replace('ML', ''))
        
        self.log(f"   Product : {product}")
        self.log(f"   Volume  : {volume_ml} ml")
        self.log(f"   Price   : Rp {price}")
        
        # 🔥 CARI LINE
        if product not in PRODUCT_MAP:
            self.log(f"❌ Product tidak dikenal: {product}", "ERROR")
            self.log(f"   Produk yang tersedia: {list(PRODUCT_MAP.keys())}")
            return
            
        line = PRODUCT_MAP[product]
        line_name = LINE_NAMES[line]
        
        self.log(f"   Line    : {line_name}")
        
        # 🔥 KIRIM COMMAND KE ESP32
        self.log("▶️ Mengirim command ke ESP32...")
        self.send_command("open", line, volume_ml)
        
        self.is_filling = True
        self.current_order = {
            "order_id": order_id,
            "product": product,
            "volume": volume_ml,
            "line": line
        }
        
        self.log("✅ Command sent! Menunggu pengisian selesai...")
        self.log("=" * 60)
        
    def send_command(self, action, line=None, volume=None):
        """Kirim command ke ESP32 via local broker"""
        try:
            payload = {"action": action}
            if line is not None:
                payload["line"] = line
            if volume is not None:
                payload["volume"] = volume
                
            self.local_client.publish(TOPIC_COMMAND, json.dumps(payload))
            self.log(f"📤 Command: {json.dumps(payload)}")
            return True
        except Exception as e:
            self.log(f"❌ Failed to send command: {e}", "ERROR")
            return False
            
    def check_esp32_online(self):
        """Cek apakah ESP32 masih online"""
        if self.esp32_last_seen is None:
            return False
        if time.time() - self.esp32_last_seen > 15:
            self.esp32_status["online"] = False
            return False
        return True
            
    def run(self):
        """Run the controller"""
        self.log("=" * 60)
        self.log("  REFILLX CONTROLLER")
        self.log("  Payment → ESP32 Gateway")
        self.log("=" * 60)
        
        # Start Mosquitto
        if not self.start_mosquitto():
            self.log("❌ Mosquitto failed", "ERROR")
            return
            
        # Connect ke local broker
        if not self.connect_local():
            self.log("❌ Local broker failed", "ERROR")
            return
            
        # Connect ke remote broker
        if not self.connect_remote():
            self.log("❌ Remote broker failed", "ERROR")
            return
            
        self.log("=" * 60)
        self.log("✅ SYSTEM READY!")
        self.log("   Menunggu payment dari Midtrans...")
        self.log("   Atau ketik command manual")
        self.log("=" * 60)
        
        # Command sender
        self.start_commander()
        
    def start_commander(self):
        """Interactive command sender"""
        print("\n" + "=" * 60)
        print("  COMMAND SENDER (Manual)")
        print("=" * 60)
        print("Commands:")
        print("  open 0/1/2   - Buka line (akan minta volume)")
        print("  close 0/1/2  - Tutup line")
        print("  stop_all     - Stop semua")
        print("  status       - Cek status ESP32")
        print("  exit         - Keluar")
        print("=" * 60)
        
        while self.running:
            try:
                cmd = input("\n> ").strip().lower()
                if not cmd:
                    continue
                    
                parts = cmd.split()
                action = parts[0]
                
                if action == "exit":
                    self.running = False
                    break
                elif action == "status":
                    s = self.esp32_status
                    online = self.check_esp32_online()
                    
                    print("\n" + "=" * 50)
                    print("📡 ESP32 STATUS")
                    print("=" * 50)
                    print(f"   Online     : {'✅ YES' if online else '❌ NO'}")
                    print(f"   Filling    : {'YES' if s.get('is_filling') else 'NO'}")
                    print(f"   Line       : {s.get('line_active', -1)}")
                    print(f"   Volume     : {s.get('volume', 0):.1f} ml")
                    print(f"   Target     : {s.get('target_ml', 0)} ml")
                    print(f"   Progress   : {s.get('progress', 0)}%")
                    print(f"   RSSI       : {s.get('rssi', 0)} dBm")
                    print(f"   Last Update: {s.get('last_update', 'Never')}")
                    print("=" * 50)
                    continue
                elif action == "stop_all":
                    self.send_command("stop_all")
                    print("✅ Stop all sent")
                elif action in ["open", "close"]:
                    if len(parts) < 2:
                        print("❌ Specify line: open 0 / close 0")
                        continue
                    try:
                        line = int(parts[1])
                        if line < 0 or line > 2:
                            print("❌ Line must be 0, 1, or 2")
                            continue
                            
                        if action == "open":
                            if not self.check_esp32_online():
                                print("❌ ESP32 OFFLINE! Tidak bisa mengirim command")
                                continue
                                
                            try:
                                volume_input = input("   Target volume (ml): ").strip()
                                volume = int(volume_input)
                                if volume <= 0:
                                    print("❌ Volume harus > 0")
                                    continue
                            except ValueError:
                                print("❌ Volume harus angka")
                                continue
                            self.send_command(action, line, volume)
                        else:
                            self.send_command(action, line)
                            
                        print(f"✅ {action} line {line} sent")
                    except ValueError:
                        print("❌ Line must be a number")
                else:
                    print("❌ Unknown command")
                    
            except KeyboardInterrupt:
                self.running = False
                break
                
    def stop(self):
        """Stop all"""
        self.log("Stopping...")
        self.running = False
        
        if self.remote_client:
            self.remote_client.loop_stop()
            self.remote_client.disconnect()
        if self.local_client:
            self.local_client.loop_stop()
            self.local_client.disconnect()
            
        subprocess.run(["taskkill", "/f", "/im", "mosquitto.exe"], capture_output=True)
        self.log("✅ Stopped")

# ============================================================
# MAIN
# ============================================================
if __name__ == "__main__":
    controller = RefillxController()
    try:
        controller.run()
    except KeyboardInterrupt:
        print("\n🛑 Shutting down...")
    finally:
        controller.stop()
