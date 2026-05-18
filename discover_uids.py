# discover_uids.py
from smartcard.System import readers
from smartcard.util import toHexString
from smartcard.CardMonitoring import CardMonitor, CardObserver
import time

class TagPrinter(CardObserver):
    def update(self, observable, actions):
        added, removed = actions
        for card in added:
            card.connection = card.createConnection()
            card.connection.connect()
            GET_UID = [0xFF, 0xCA, 0x00, 0x00, 0x00]
            data, sw1, sw2 = card.connection.transmit(GET_UID)
            uid = toHexString(data).replace(" ", "")
            print(f"Tag UID: {uid}")

print("Tap your NFC tags one by one. Ctrl+C to stop.\n")
monitor = CardMonitor()
observer = TagPrinter()
monitor.addObserver(observer)

try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    monitor.deleteObserver(observer)