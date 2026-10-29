#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import threading
import serial
import serial.tools.list_ports


# Mots-clés fréquents dans la description USB des ESP32
ESP32_HINTS = (
    "CP210", "Silicon Labs",
    "CH340", "CH341", "WCH",
    "FTDI",
    "Espressif", "ESP32"
)

DEFAULT_BAUD = 115200


def list_ports():
    """Retourne la liste des ports disponibles (ListPortInfo)."""
    return list(serial.tools.list_ports.comports())


def score_port(p):
    """
    Donne un score de "probabilité ESP32" au port.
    Plus le score est haut, plus on pense que c'est l'ESP32.
    """
    text = " ".join([
        str(p.device or ""),
        str(p.description or ""),
        str(p.manufacturer or ""),
        str(p.hwid or ""),
    ]).lower()

    score = 0
    for hint in ESP32_HINTS:
        if hint.lower() in text:
            score += 10

    # Bonus si VID/PID est présent (souvent USB-série)
    if "vid:pid" in text:
        score += 2

    return score


def autodetect_port():
    """
    Tente de choisir automatiquement le port le plus probable.
    Renvoie un string (ex: 'COM5' / '/dev/ttyUSB0') ou None.
    """
    ports = list_ports()
    if not ports:
        return None

    ranked = sorted(ports, key=score_port, reverse=True)
    best = ranked[0]
    # Si le meilleur score est 0, on n'est pas sûr : on le prend quand même
    return best.device


def open_serial(port, baud, timeout=0.1):
    """
    Ouvre le port série. timeout court pour lecture non bloquante.
    """
    return serial.Serial(
        port=port,
        baudrate=baud,
        timeout=timeout,
        write_timeout=1
    )


def reader_loop(ser, stop_event):
    """
    Lit en continu la sortie série et l'affiche en "console".
    """
    buf = bytearray()
    while not stop_event.is_set():
        try:
            data = ser.read(1024)
            if data:
                buf.extend(data)
                # On flush ligne par ligne si possible
                while b"\n" in buf:
                    line, _, rest = buf.partition(b"\n")
                    buf = bytearray(rest)
                    try:
                        print(line.decode(errors="replace"))
                    except Exception:
                        print(line)
            else:
                time.sleep(0.01)
        except (serial.SerialException, OSError) as e:
            print(f"[!] Lecture série interrompue: {e}")
            stop_event.set()
            break

    # Affiche le reste du buffer
    if buf:
        print(buf.decode(errors="replace"), end="")


def writer_loop(ser, stop_event):
    """
    Permet de taper au clavier et d'envoyer à l'ESP32.
    - Entrée envoie la ligne + \\n
    - Ctrl+C pour quitter
    """
    try:
        while not stop_event.is_set():
            line = sys.stdin.readline()
            if line == "":  # EOF
                stop_event.set()
                break
            # Envoi tel quel (souvent les firmwares attendent \n ou \r\n)
            if not line.endswith("\n"):
                line += "\n"
            ser.write(line.encode())
            ser.flush()
    except KeyboardInterrupt:
        stop_event.set()
    except (serial.SerialException, OSError) as e:
        print(f"[!] Écriture série interrompue: {e}")
        stop_event.set()


def main():
    # Port optionnel en argument: python esp32_console.py COM5 115200
    port = None
    baud = DEFAULT_BAUD

    if len(sys.argv) >= 2:
        port = sys.argv[1]
    if len(sys.argv) >= 3:
        baud = int(sys.argv[2])

    if port is None:
        port = autodetect_port()

    if port is None:
        print("[!] Aucun port série détecté.")
        print("Ports disponibles :")
        for p in list_ports():
            print(f"  - {p.device} | {p.description} | {p.manufacturer} | {p.hwid}")
        sys.exit(1)

    print(f"[*] Port sélectionné: {port}")
    print(f"[*] Baudrate: {baud}")
    print("[*] Connexion... (Ctrl+C pour quitter)\n")

    try:
        ser = open_serial(port, baud)
    except serial.SerialException as e:
        print(f"[!] Impossible d'ouvrir {port}: {e}")
        sys.exit(1)

    stop_event = threading.Event()

    t_reader = threading.Thread(target=reader_loop, args=(ser, stop_event), daemon=True)
    t_writer = threading.Thread(target=writer_loop, args=(ser, stop_event), daemon=True)
    t_reader.start()
    t_writer.start()

    try:
        while not stop_event.is_set():
            time.sleep(0.1)
    except KeyboardInterrupt:
        stop_event.set()

    try:
        ser.close()
    except Exception:
        pass

    print("\n[*] Fermé.")


if __name__ == "__main__":
    main()
