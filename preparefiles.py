"""esp32 adapter for the reusable Interval web-asset preparation helper."""

from __future__ import annotations

import os
import sys
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parent
INTERVAL_DIR = PROJECT_DIR.parent / "Interval"
sys.path.insert(0, str(INTERVAL_DIR / "scripts"))

from web_assets import prepare_assets, resolve_source_dir  # noqa: E402


def main() -> None:
    # The network checkout is the canonical editable webGUI source used for
    # firmware packaging. The sibling path keeps local Codebase/CI checkouts
    # portable, while WEBGUI_SRC always has highest priority.
    source = resolve_source_dir(
        os.environ.get("WEBGUI_SRC"),
        [
            Path("/Volumes/web/webGUI"),
            PROJECT_DIR.parent / "webGUI",
        ],
    )
    destination = Path(
        os.environ.get("FIRMWARE_WEBAPP_DEST", PROJECT_DIR / "data" / "webapp")
    )
    prepare_assets(source, destination)


if __name__ == "__main__":
    main()
