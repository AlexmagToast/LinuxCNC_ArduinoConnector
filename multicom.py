import threading
import serial
import time
import yaml
from queue import Queue

# Globale Variablen für eingehende und ausgehende Daten
incoming_data = {
    "mcu_1": Queue(),
    "mcu_2": Queue(),
    "mcu_3": Queue(),
}

outgoing_data = {
    "mcu_1": Queue(),
    "mcu_2": Queue(),
    "mcu_3": Queue(),
}

# Thread-Stop-Event
stop_event = threading.Event()

# Funktion zur Kommunikation mit einem Arduino
def communicate_with_arduino(port, baudrate, incoming_queue, outgoing_queue, stop_event):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            while not stop_event.is_set():
                # Senden von Daten
                if not outgoing_queue.empty():
                    data_to_send = outgoing_queue.get()
                    ser.write(data_to_send.encode('utf-8'))
                    print(f"[{port}] Gesendet: {data_to_send}")

                # Empfangen von Daten
                if ser.in_waiting > 0:
                    received_data = ser.readline().decode('utf-8', errors='ignore').strip()
                    incoming_queue.put(received_data)
                    print(f"[{port}] Empfangen: {received_data}")

                time.sleep(0.001)  # Reduziert die CPU-Last
    except serial.SerialException as e:
        print(f"Fehler bei {port}: {e}")

# Erstellen und Starten der Threads
arduino_threads = []
arduino_ports = ["COM5","COM6"]  # Passe die Ports an dein System an
baudrate = 9600

for i, port in enumerate(arduino_ports):
    thread = threading.Thread(
        target=communicate_with_arduino,
        args=(port, baudrate, incoming_data[f"mcu_{i+1}"], outgoing_data[f"mcu_{i+1}"], stop_event),
        daemon=True
    )
    arduino_threads.append(thread)
    thread.start()

# Hauptprogramm
try:
    print("Drücke STRG+C zum Beenden...")
    while True:
        # Beispiel: Daten in die Warteschlange einfügen
        outgoing_data["mcu_1"].put("PING_1\n")
        outgoing_data["mcu_2"].put("PING_2\n")
        outgoing_data["mcu_3"].put("PING_3\n")

        # Auslesen der empfangenen Daten
        for i in range(1, 4):
            if not incoming_data[f"mcu_{i}"].empty():
                received = incoming_data[f"mcu_{i}"].get()
                print(f"Master empfängt von Arduino {i}: {received}")

        time.sleep(1)

except KeyboardInterrupt:
    print("\nBeenden des Programms...")

# Stoppe alle Threads
stop_event.set()
for thread in arduino_threads:
    thread.join()

print("Programm beendet.")

