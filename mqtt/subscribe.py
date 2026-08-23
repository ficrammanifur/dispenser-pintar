import paho.mqtt.client as mqtt

BROKER = "103.168.146.179"
PORT = 1883
TOPIC = "success"
USERNAME = "refillx"
PASSWORD = "Password1!"

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Berhasil connect ke broker!")
        client.subscribe(TOPIC)
        print(f"Subscribed ke topic: {TOPIC}")
    else:
        print("Gagal connect, return code:", rc)

def on_message(client, userdata, msg):
    print(f"Pesan diterima di topic '{msg.topic}': {msg.payload.decode()}")

client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(BROKER, PORT, 60)
    client.loop_forever()
except Exception as e:
    print(f"Error: {e}")