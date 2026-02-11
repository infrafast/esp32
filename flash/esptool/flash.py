import sys
import subprocess

cmd = [
    sys.executable, "-m", "esptool",
    "--chip", "esp32s3",
    "--baud", "921600",
    "--before", "default-reset",
    "--after", "hard-reset",
    "write-flash",
    "--flash-mode", "dio",
    "--flash-freq", "80m",
    "--flash-size", "detect",
    "0x410000",
    "littlefs.bin"
]

print("Executing:", " ".join(cmd))
sys.exit(subprocess.call(cmd))
