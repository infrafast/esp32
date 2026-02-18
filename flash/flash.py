import os
import sys
import tkinter as tk
from tkinter import ttk, messagebox
import runpy
import contextlib

# ============================================================
# ESP32-S3 Flash GUI (simplified)
# - No thread
# - No queue
# - No progress bar
# - Port auto-detected by esptool
# ============================================================

def resource_path(rel):
    base = getattr(sys, "_MEIPASS", os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base, rel)

OFFSETS = {
    "bootloader": ("0x0000", resource_path("bootloader.bin")),
    "application": ("0x10000", resource_path("firmware.bin")),
    "data": ("0x670000", resource_path("littlefs.bin")),
}

# ---------------- GUI ----------------

root = tk.Tk()
root.title("ESP32-S3 Flash Tool")
root.resizable(False, False)

mode_var = tk.StringVar(value="normal")

frame = ttk.Frame(root, padding=15)
frame.pack()

ttk.Label(frame, text="Flash mode:").grid(row=0, column=0, sticky="w")

mode_combo = ttk.Combobox(
    frame,
    textvariable=mode_var,
    values=["data", "application", "normal", "full"],
    state="readonly",
    width=15
)
mode_combo.grid(row=0, column=1, pady=5, sticky="w")

log = tk.Text(frame, width=70, height=18)
log.grid(row=1, column=0, columnspan=2, pady=10)

flash_btn = ttk.Button(frame, text="Flash")
flash_btn.grid(row=2, column=0, columnspan=2, pady=5)

# ---------------- Flash logic ----------------
class StreamToText:
    def __init__(self, widget):
        self.widget = widget
        self.parts = []   # buffer en RAM (rapide)

    def write(self, s):
        if not s:
            return 0
        self.parts.append(s)
        return len(s)

    def flush(self):
        # Affichage en une seule fois -> beaucoup moins intrusif sur le timing USB
        if not self.parts:
            return
        text = "".join(self.parts)
        self.parts.clear()
        self.widget.insert(tk.END, text)
        self.widget.see(tk.END)
        self.widget.update_idletasks()


class StreamToText2:
    def __init__(self, widget):
        self.widget = widget
        self.buffer = ""

    def write(self, s):
        if not s:
            return 0

        self.buffer += s

        while "\n" in self.buffer:
            line, self.buffer = self.buffer.split("\n", 1)
            self.widget.insert(tk.END, line + "\n")
            self.widget.see(tk.END)
            self.widget.update_idletasks()

        return len(s)

    def flush(self):
        if self.buffer:
            self.widget.insert(tk.END, self.buffer)
            self.widget.see(tk.END)
            self.widget.update_idletasks()
            self.buffer = ""

def build_flash_args(mode):
    args = []
    if mode == "data":
        args += list(OFFSETS["data"])
    elif mode == "application":
        args += list(OFFSETS["application"])
    elif mode == "normal":
        args += list(OFFSETS["application"])
        args += list(OFFSETS["data"])
    elif mode == "full":
        args += list(OFFSETS["bootloader"])
        args += list(OFFSETS["application"])
        args += list(OFFSETS["data"])
    return args

def run_flash():
    log.delete("1.0", tk.END)

    args = [
        "--chip", "esp32s3",
        "--baud", "921600",
        "--before", "usb-reset",   # <<< IMPORTANT
        "--after", "hard-reset",     # <<< IMPORTANT
        "write-flash",
        "--flash-mode", "dio",
        "--flash-freq", "80m",
        "--flash-size", "detect",
    ] + build_flash_args(mode_var.get())


    log.insert(tk.END, "Executing:\npython -m esptool " + " ".join(args) + "\n\n")
    log.update_idletasks()

    old_argv = sys.argv[:]
    sys.argv = ["esptool"] + args

    streamer = StreamToText(log)

    try:
        with contextlib.redirect_stdout(streamer), contextlib.redirect_stderr(streamer):
            try:
                runpy.run_module("esptool.__main__", run_name="__main__")
                streamer.flush()
                messagebox.showinfo("Flash terminé", "Flash terminé avec succès.")
            except SystemExit as e:
                if e.code != 0:
                    messagebox.showerror("Erreur", f"Flash failed (code {e.code})")
    except Exception as e:
        messagebox.showerror("Erreur", str(e))
    finally:
        sys.argv = old_argv
        streamer.flush()

flash_btn.config(command=run_flash)

root.mainloop()
