"""esp32 integration hook for the reusable Interval build helpers.

PlatformIO does not execute extra scripts shipped by a library automatically.
This file is intentionally a thin consumer adapter: the implementation lives
in the resolved Interval package, while esp32 supplies only the product
project/data location and release destination through the PlatformIO env.
"""

Import("env")

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path


project_libdeps = Path(env.subst("$PROJECT_LIBDEPS_DIR"))
environment_name = str(env.get("PIOENV", ""))
helper_candidates = (
    project_libdeps / environment_name / "Interval" / "scripts" / "platformio_build.py",
    project_libdeps / "Interval" / "scripts" / "platformio_build.py",
)
for helper_path in helper_candidates:
    if helper_path.exists():
        spec = spec_from_file_location("interval_platformio_build", helper_path)
        module = module_from_spec(spec)
        spec.loader.exec_module(module)
        module.register_artifact_packaging(env)
        break
else:
    raise FileNotFoundError("Interval build helpers not found in the resolved dependency")
