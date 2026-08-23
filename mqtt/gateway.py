import paho.mqtt.client as mqtt
import time
import sys


# ============================================================
# CONFIGURATION
# ============================================================

LOCAL_BROKER = "127.0.0.1"
LOCAL_PORT = 1883

REMOTE_BROKER = "103.168.146.179"
REMOTE_PORT = 1883

REMOTE_USERNAME = "refillx"
REMOTE_PASSWORD = "Password1!"

# ============================================================
# TOPIC MAPPING
# ============================================================

LOCAL_TO_REMOTE = [
    "refillx/telemetry",
    "refillx/status",
]

REMOTE_TO_LOCAL = [
    "refillx/command",
]

# ============================================================
# CLIENTS
# ============================================================

local_client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="refillx_gateway_local"
)

remote_client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="refillx_gateway_remote"
)

remote_client.username_pw_set(
    REMOTE_USERNAME,
    REMOTE_PASSWORD
)


# ============================================================
# LOCAL MQTT
# ============================================================

def local_on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("[LOCAL] Connected to Mosquitto")

        for topic in REMOTE_TO_LOCAL:
            print(f"[LOCAL] Ready to publish: {topic}")

    else:
        print(f"[LOCAL] Connection failed: {reason_code}")


def local_on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    print(f"[LOCAL] Disconnected: {reason_code}")


local_client.on_connect = local_on_connect
local_client.on_disconnect = local_on_disconnect


# ============================================================
# REMOTE MQTT
# ============================================================

def remote_on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("[REMOTE] Connected to MQTT server")

        for topic in REMOTE_TO_LOCAL:
            client.subscribe(topic)
            print(f"[REMOTE] Subscribed: {topic}")

    else:
        print(f"[REMOTE] Connection failed: {reason_code}")


def remote_on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    print(f"[REMOTE] Disconnected: {reason_code}")


remote_client.on_connect = remote_on_connect
remote_client.on_disconnect = remote_on_disconnect


# ============================================================
# LOCAL → REMOTE
# ============================================================

def local_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload

    if topic not in LOCAL_TO_REMOTE:
        return

    print(
        f"[FORWARD LOCAL → REMOTE] "
        f"{topic} : {payload.decode(errors='replace')}"
    )

    result = remote_client.publish(
        topic,
        payload,
        qos=msg.qos,
        retain=msg.retain
    )

    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print("[FORWARD] Success")
    else:
        print(f"[FORWARD] Failed: {result.rc}")


local_client.on_message = local_message


# ============================================================
# REMOTE → LOCAL
# ============================================================

def remote_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload

    if topic not in REMOTE_TO_LOCAL:
        return

    print(
        f"[FORWARD REMOTE → LOCAL] "
        f"{topic} : {payload.decode(errors='replace')}"
    )

    result = local_client.publish(
        topic,
        payload,
        qos=msg.qos,
        retain=msg.retain
    )

    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        print("[FORWARD] Success")
    else:
        print(f"[FORWARD] Failed: {result.rc}")


remote_client.on_message = remote_message


# ============================================================
# CONNECT
# ============================================================

def connect_clients():

    print("=" * 60)
    print("REFILLX MQTT GATEWAY")
    print("=" * 60)

    print(f"LOCAL  : {LOCAL_BROKER}:{LOCAL_PORT}")
    print(f"REMOTE : {REMOTE_BROKER}:{REMOTE_PORT}")
    print()

    print("[SYSTEM] Connecting LOCAL...")

    local_client.connect(
        LOCAL_BROKER,
        LOCAL_PORT,
        60
    )

    print("[SYSTEM] Connecting REMOTE...")

    remote_client.connect(
        REMOTE_BROKER,
        REMOTE_PORT,
        60
    )

    # Subscribe LOCAL topics that should be forwarded.
    for topic in LOCAL_TO_REMOTE:
        local_client.subscribe(topic)
        print(f"[LOCAL] Subscribed: {topic}")

    print()
    print("[SYSTEM] Gateway ready")
    print("=" * 60)


# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":

    try:
        connect_clients()

        local_client.loop_start()
        remote_client.loop_start()

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\n[SYSTEM] Gateway stopped")

    except Exception as e:
        print(f"[SYSTEM] ERROR: {e}")
        sys.exit(1)

    finally:
        try:
            local_client.loop_stop()
            remote_client.loop_stop()

            local_client.disconnect()
            remote_client.disconnect()

        except Exception:
            pass