python3 -m venv venv
source venv/bin/activate
python3 -m pip install --upgrade pyinstaller esptool pyserial
pyinstaller --noconfirm --clean --windowed --onedir --name "ESP32 Flasher" \
  --hidden-import esptool.__main__ \
  --collect-all esptool \
  --add-data "bootloader.bin:." \
  --add-data "firmware.bin:." \
  --add-data "littlefs.bin:." \
  flashEXE.py

