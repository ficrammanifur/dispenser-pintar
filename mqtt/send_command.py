import paho.mqtt.client as mqtt
import json
import sys

def send_command(action, line=None):
    client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    
    try:
        client.connect("127.0.0.1", 1883, 60)
        
        # Buat payload JSON
        payload = {"action": action}
        if line is not None:
            payload["line"] = line
            
        # Kirim command
        topic = "refillx/command"
        client.publish(topic, json.dumps(payload))
        print(f"✅ Command sent: {payload}")
        
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        client.disconnect()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python send_command.py <action> [line]")
        print("  action: open, close, stop_all")
        print("  line: 0, 1, 2 (for open/close)")
        sys.exit(1)
    
    action = sys.argv[1]
    line = int(sys.argv[2]) if len(sys.argv) > 2 else None
    
    send_command(action, line)