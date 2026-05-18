# nfc_to_mqtt.py
from smartcard.System import readers
from smartcard.util import toHexString
from smartcard.CardMonitoring import CardMonitor, CardObserver
import paho.mqtt.client as mqtt
import time

# ── Config ────────────────────────────────────────────────────────────────────
MQTT_BROKER = "broker.emqx.io"   # your broker IP or hostname
MQTT_PORT   = 1883
MQTT_TOPIC  = "pico/cin/picoB_cin/color"

UID_TO_COLOR = {
    "5A15557F1B4189": "yellow",    # nfc_a
    "5A15A2821B4189": "white",   # nfc_c
}

# ── MQTT setup ────────────────────────────────────────────────────────────────
client = mqtt.Client()
client.connect(MQTT_BROKER, MQTT_PORT)
client.loop_start()

# ── NFC callback ──────────────────────────────────────────────────────────────
last_uid = None
last_time = 0
DEBOUNCE_SECONDS = 2

class TagHandler(CardObserver):
    def update(self, observable, actions):
        global last_uid, last_time
        added, removed = actions
        for card in added:
            try:
                card.connection = card.createConnection()
                card.connection.connect()
                GET_UID = [0xFF, 0xCA, 0x00, 0x00, 0x00]
                data, sw1, sw2 = card.connection.transmit(GET_UID)
                uid = toHexString(data).replace(" ", "")

                now = time.time()
                if uid == last_uid and (now - last_time) < DEBOUNCE_SECONDS:
                    return

                last_uid = uid
                last_time = now

                color = UID_TO_COLOR.get(uid)
                if color:
                    result = client.publish("pico/cin/nfc", color)
                    print(f"Published '{color}' to pico/cin/nfc on {MQTT_BROKER} — result: {result.rc}")
                else:
                    print(f"[{uid}] → unknown tag")

            except Exception as e:
                print(f"Error reading tag: {e}")

print(f"Listening for NFC tags → publishing to {MQTT_BROKER}/{MQTT_TOPIC}")
print("Ctrl+C to stop.\n")

monitor = CardMonitor()
handler = TagHandler()
monitor.addObserver(handler)

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    monitor.deleteObserver(handler)
    client.loop_stop()
    print("Stopped.")