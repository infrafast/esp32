"""esp32 integration hook for the reusable Interval build helpers.

PlatformIO does not execute extra scripts shipped by a library automatically.
This file is intentionally a thin consumer adapter: the implementation lives
in the resolved Interval package, while esp32 supplies only the product
project/data location and release destination through the PlatformIO env.
"""

Import("env")

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path


def _load_interval_build_helpers():
    project_libdeps = Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    environment_name = str(env.get("PIOENV", ""))
    candidates = [
        project_libdeps / environment_name / "Interval" / "scripts" / "platformio_build.py",
        project_libdeps / "Interval" / "scripts" / "platformio_build.py",
    ]
    for helper_path in candidates:
        if helper_path.exists():
            spec = spec_from_file_location("interval_platformio_build", helper_path)
            module = module_from_spec(spec)
            spec.loader.exec_module(module)
            return module
    raise FileNotFoundError(
        "Interval build helpers not found in the resolved PlatformIO dependency"
    )


_load_interval_build_helpers().register_artifact_packaging(env)
