#inject boardname https://community.platformio.org/t/inject-board-name-into-code/21526/2

Import("env")

# PlatformIO executes this pre-script before it resolves lib_deps, so the
# dependency cannot provide these first-stage flags itself. Keep this tiny
# bootstrap synchronized with Interval's configure_usb_mode() profile contract;
# all reusable post-build mechanics remain owned by Interval.
def _has_define(name):
    for define in env.get("CPPDEFINES", []):
        if isinstance(define, (tuple, list)) and define and define[0] == name:
            return True
        if define == name or (isinstance(define, str) and define.startswith(name + "=")):
            return True
    sources = [str(env.get("BUILD_FLAGS", ""))]
    for key in ("CCFLAGS", "CXXFLAGS", "CPPFLAGS"):
        value = env.get(key)
        if isinstance(value, (list, tuple)):
            sources.append(" ".join(map(str, value)))
        elif value:
            sources.append(str(value))
    if any(f"-D{name}" in source or f"-D {name}" in source for source in sources):
        return True
    try:
        flags = str(env.GetProjectOption("build_flags") or "")
        if f"-D{name}" in flags or f"-D {name}" in flags:
            return True
    except Exception:
        pass
    return False


def _set_define(name, value):
    env.ProcessUnFlags([f"-D{name}=0", f"-D{name}=1", f"-D{name}"])
    env.AppendUnique(CPPDEFINES=[(name, value)])


if _has_define("INTERVAL_BUILD_PROD"):
    env.AppendUnique(BUILD_UNFLAGS=[
        "-DARDUINO_USB_MODE=1",
        "-DARDUINO_USB_CDC_ON_BOOT=1",
    ])
    env.ProcessUnFlags(["-DARDUINO_USB_MODE=1", "-DARDUINO_USB_CDC_ON_BOOT=1"])
    env.AppendUnique(CPPDEFINES=["PROD"])
    env.AppendUnique(CPPDEFINES=[
        "CONFIG_IDF_TARGET_ESP32S3",
        "CONFIG_TINYUSB_ENABLED",
        "CONFIG_TINYUSB_MIDI_ENABLED",
        "CFG_TUD_MIDI",
    ])
    _set_define("ARDUINO_USB_MODE", 0)
    _set_define("ARDUINO_USB_CDC_ON_BOOT", 0)
elif _has_define("INTERVAL_BUILD_DEV"):
    env.AppendUnique(BUILD_UNFLAGS=[
        "-DARDUINO_USB_MODE=0",
        "-DARDUINO_USB_CDC_ON_BOOT=0",
    ])
    env.ProcessUnFlags(["-DARDUINO_USB_MODE=0", "-DARDUINO_USB_CDC_ON_BOOT=0"])
    _set_define("ARDUINO_USB_MODE", 1)
    _set_define("ARDUINO_USB_CDC_ON_BOOT", 1)
else:
    raise RuntimeError("Select INTERVAL_BUILD_DEV or INTERVAL_BUILD_PROD")
