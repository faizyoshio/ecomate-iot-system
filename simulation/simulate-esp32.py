"""
Simulator Telemetri ESP32 untuk Komposter Rotari ECO-MATE
Mensimulasikan data pembacaan sensor secara realistis ke MQTT Broker (Node-RED)
"""

import time
import json
import random
import paho.mqtt.client as mqtt

BROKER_HOST = "127.0.0.1"
BROKER_PORT = 1883
TOPIC_TELEMETRY = "ecomate/telemetry"
TOPIC_CMD_MOTOR = "ecomate/cmd/motor"

motor_running = 0
door_safe = 1

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("[SIMULATOR] Terhubung ke Broker MQTT Node-RED (127.0.0.1:1883)!")
        client.subscribe(TOPIC_CMD_MOTOR)
        print(f"[SIMULATOR] Berlangganan ke topik perintah: {TOPIC_CMD_MOTOR}")
    else:
        print(f"[SIMULATOR] Gagal terhubung ke broker, rc={rc}")

def on_message(client, userdata, msg):
    global motor_running
    payload = msg.payload.decode('utf-8')
    print(f"\n[SIMULATOR DITERIMA] Perintah Motor: '{payload}' di topic: {msg.topic}")
    if payload in ["1", "true", 1]:
        motor_running = 1
        print(">>> [STATUS HARDWARE] MOTOR DRUM 17.5 RPM MENYALA (ON)")
    else:
        motor_running = 0
        print(">>> [STATUS HARDWARE] MOTOR DRUM MATI (OFF)")

def main():
    try:
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=f"Simulator_ESP32_{random.randint(100, 999)}")
    except AttributeError:
        client = mqtt.Client(client_id=f"Simulator_ESP32_{random.randint(100, 999)}")

    client.on_connect = on_connect
    client.on_message = on_message

    print("[SIMULATOR] Menghubungkan ke Node-RED...")
    try:
        client.connect(BROKER_HOST, BROKER_PORT, 60)
    except Exception as e:
        print(f"[ERROR] Gagal konek ke broker: {e}. Pastikan Node-RED sudah berjalan.")
        return

    client.loop_start()

    # Simulasi dinamika bioproses
    suhu_base = 52.0
    kelembaban_base = 58.0
    ph_base = 6.8

    try:
        step = 0
        while True:
            step += 1
            suhu = round(suhu_base + random.uniform(-0.8, 1.2), 1)
            kelembaban = round(kelembaban_base + random.uniform(-1.0, 1.0), 1)
            ph = round(ph_base + random.uniform(-0.08, 0.08), 2)

            telemetry = {
                "suhu": suhu,
                "kelembaban": kelembaban,
                "ph": ph,
                "motor": motor_running,
                "interlock": door_safe
            }

            payload_json = json.dumps(telemetry)
            client.publish(TOPIC_TELEMETRY, payload_json)
            print(f"[SIMULATOR KIRIM] Telemetri -> Suhu: {suhu}°C | Kelembaban: {kelembaban}% | pH: {ph} | Motor: {motor_running} | Pintu: {door_safe}")

            time.sleep(5)
    except KeyboardInterrupt:
        print("\n[SIMULATOR] Dihentikan.")
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
