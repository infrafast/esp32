"""Prepare the webGUI files used by the esp32 firmware.

This file is intentionally a small application-level adapter.  The reusable
copy/minify/gzip implementation lives in Interval; this adapter only selects
the webGUI source and the firmware data destination for this product.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parent
INTERVAL_DIR = PROJECT_DIR.parent / "Interval"
sys.path.insert(0, str(INTERVAL_DIR / "scripts"))

from web_assets import prepare_assets, resolve_source_dir  # noqa: E402


def main() -> None:
    # Resolution order:
    #   1. WEBGUI_SRC, when supplied by Codebase/CI or a developer;
    #   2. the reference workstation checkout on the mounted web volume;
    #   3. a sibling checkout for a portable Projects/ layout.
    # Change this list only if the product's workspace convention changes.
    source = resolve_source_dir(
        os.environ.get("WEBGUI_SRC"),
        [
            Path("/Volumes/web/webGUI"),
            PROJECT_DIR.parent / "webGUI",
        ],
    )
    # This is the application-owned LittleFS staging directory.  Override it
    # when packaging for another firmware layout; Interval does not own it.
    destination = Path(
        os.environ.get("FIRMWARE_WEBAPP_DEST", PROJECT_DIR / "data" / "webapp")
    )
    prepare_assets(source, destination)


if __name__ == "__main__":
    main()
