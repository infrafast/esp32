"""esp32 integration hook for the reusable Interval build helpers.

PlatformIO does not execute extra scripts shipped by a library automatically.
This file is intentionally a thin consumer adapter: the implementation lives
in the resolved Interval package, while esp32 supplies only the product
project/data location and release destination through the PlatformIO env.
"""

Import("env")

from interval_platformio import load_interval_build_helpers


load_interval_build_helpers(env).register_artifact_packaging(env)
