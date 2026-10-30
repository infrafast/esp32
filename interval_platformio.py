"""Loader for build helpers shipped by the resolved Interval package."""

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path


def load_interval_build_helpers(env):
    """Load Interval tooling without assuming a checkout or host path."""
    project_libdeps = Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    environment_name = str(env.get("PIOENV", ""))
    candidates = (
        project_libdeps / environment_name / "Interval" / "scripts" / "platformio_build.py",
        project_libdeps / "Interval" / "scripts" / "platformio_build.py",
    )
    for helper_path in candidates:
        if helper_path.exists():
            spec = spec_from_file_location("interval_platformio_build", helper_path)
            module = module_from_spec(spec)
            spec.loader.exec_module(module)
            return module
    raise FileNotFoundError("Interval build helpers not found in the resolved dependency")
