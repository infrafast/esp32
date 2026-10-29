import sys
import subprocess
import threading
import re
import tkinter as tk
from tkinter import ttk, messagebox

# ============================================================
# ESP32-S3 Flash GUI — Version 1.2
# Modes: data | application | normal | full
# ============================================================

# ---------- Configuration fixe ----------
ESPTOOL_BASE = [
    sys.executable, "-m", "esptool",
    "--chip", "esp32s3",
    "--baud", "921600",
    "--before", "default-reset",
    "--after", "hard-reset",
    "write-flash",
    "--flash-mode", "dio",
    "--flash-freq", "80m",
    "--flash-size", "detect",
]

OFFSETS = {
    "bootloader": ("0x0000", "bootloader.bin"),
    "application": ("0x10000", "firmware.bin"),
    "data": ("0x670000", "littlefs.bin"),
}

PERCENT_RE = re.compile(r'(\d+\.\d+)%')

# ---------- GUI ----------
root = tk.Tk()
root.title("ESP32-S3 Flash Tool")
root.resizable(False, False)

mode_var = tk.StringVar(value="normal")

frame = ttk.Frame(root, padding=15)
frame.pack()

# Mode selector
ttk.Label(frame, text="Flash mode:").grid(row=0, column=0, sticky="w")

mode_combo = ttk.Combobox(
    frame,
    textvariable=mode_var,
    values=["data", "application", "normal", "full"],
    state="readonly",
    width=15
)
mode_combo.grid(row=0, column=1, pady=5, sticky="w")

# Progress bar
progress = ttk.Progressbar(frame, length=420, mode="determinate")
progress.grid(row=1, column=0, columnspan=2, pady=10)

status_label = ttk.Label(frame, text="Idle")
status_label.grid(row=2, column=0, columnspan=2, sticky="w")

# Log output
log = tk.Text(frame, width=60, height=15, state="disabled")
log.grid(row=3, column=0, columnspan=2, pady=10)

# Flash button
flash_btn = ttk.Button(frame, text="Flash", width=20)
flash_btn.grid(row=4, column=0, columnspan=2, pady=5)

# ---------- UI helpers ----------
def append_log(text):
    log.configure(state="normal")
    log.insert("end", text)
    log.see("end")
    log.configure(state="disabled")

def show_success_and_exit():
    messagebox.showinfo(
        "Flash terminé",
        "Le flash s'est terminé avec succès.\n\nVous pouvez débrancher l'ESP32."
    )
    root.destroy()

# ---------- Flash logic ----------
def build_flash_args(mode: str):
    args = []

    if mode == "data":
        args += OFFSETS["data"]

    elif mode == "application":
        args += OFFSETS["application"]

    elif mode == "normal":
        args += OFFSETS["application"]
        args += OFFSETS["data"]

    elif mode == "full":
        args += OFFSETS["bootloader"]
        args += OFFSETS["application"]
        args += OFFSETS["data"]

    return args


def run_flash():
    flash_btn.config(state="disabled")
    progress["value"] = 0
    status_label.config(text="Flashing...")
    log.configure(state="normal")
    log.delete("1.0", "end")
    log.configure(state="disabled")

    mode = mode_var.get()
    cmd = ESPTOOL_BASE + build_flash_args(mode)

    append_log("Executing:\n" + " ".join(cmd) + "\n\n")

    try:
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        for line in process.stdout:
            append_log(line)

            m = PERCENT_RE.search(line)
            if m:
                progress["value"] = float(m.group(1))
                root.update_idletasks()

        ret = process.wait()

        if ret == 0:
            progress["value"] = 100
            status_label.config(text="Flash completed successfully")
            root.after(150, show_success_and_exit)
        else:
            status_label.config(text="Flash failed")
            messagebox.showerror("Erreur", "Le flash a échoué.\nVoir les logs.")

    except Exception as e:
        messagebox.showerror("Exception", str(e))

    flash_btn.config(state="normal")


def on_flash_clicked():
    threading.Thread(target=run_flash, daemon=True).start()


flash_btn.config(command=on_flash_clicked)

# ---------- Main loop ----------
root.mainloop()
