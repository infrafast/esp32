# Interval Footswitch Application

This repository contains the firmware application for the Interval footswitch.
It uses the reusable [Interval framework](https://github.com/infrafast/Interval)
to provide the common device services and runs the product's mixer-management
features, including support for Mixing Station and Behringer X/M-series
workflows.

You normally use this repository when you want to build or flash the complete
footswitch application. The reusable framework is downloaded automatically by
PlatformIO; it must not be copied into this project.

## Build

Install PlatformIO, connect the target board, then choose an environment:

```sh
pio run -e dev
pio run -e prod
```

The `dev` environment is intended for development and diagnostics. The `prod`
environment is the normal release build. The exact board and build settings
are kept in `platformio.ini`.

The application selects only the `INTERVAL_BUILD_DEV` or
`INTERVAL_BUILD_PROD` profile. Interval supplies the corresponding USB/CDC or
TinyUSB/MIDI compiler configuration.

To build the diagnostic application instead of the mixer application, enable
the `INTERVAL_DIAGNOSE` build flag. The launcher instantiates the reusable
`Diagnose` class provided by Interval.

## Web interface and resources

The firmware can package the web configuration interface and other resources
from the `data/` directory. The generic compression, archive and build-hook
mechanics are provided by the installed Interval release; this repository
owns only the product resources and their release destination. The editable
web client is maintained separately.

The repository asset preparation scripts use the versioned
`infrafast/webGUI` checkout. On the reference workstation the canonical source
is `/Volumes/web/webGUI`; the sibling layout below is the portable fallback:

```text
Projects/
├── Interval/
└── esp32/
```

Set `WEBGUI_SRC` and `FIRMWARE_WEBAPP_DEST` to override the source and
destination in Codebase, CI, or another workspace. The source checkout is
maintained separately at https://github.com/infrafast/webGUI.

The generic script is shipped by Interval. Run it from this project with:

```sh
python3 ../Interval/scripts/preparefiles.py --project .
```

It reads `webgui_src` and `firmware_webapp_dest` from this project's
`platformio.ini`. For one-off changes, use `WEBGUI_SRC` and
`FIRMWARE_WEBAPP_DEST`; keep the minify/compression implementation in
Interval.

The whole `${PROJECT_DIR}/data` directory is the application's LittleFS
staging area. Mixer definitions remain product-owned here. The Interval asset
preparation script copies framework resources into this tree and generates
`data/webapp/` from the independent webGUI checkout. Do not copy Interval C++
sources into it. Run the preparation command before `pio run --target uploadfs`;
generated `.gz` files under `data/webapp/` are build artifacts and should not
be edited as if they were webGUI sources.

## Project boundaries

- This repository contains the product launcher, mixer applications, product
  configuration, translations, and firmware resources.
- The [Interval repository](https://github.com/infrafast/Interval) contains the
  reusable framework, shared hardware services, diagnostic class, and its
  dependency graph. This project follows the `main` branch for rolling
  framework updates; run `pio pkg update` when PlatformIO has retained an
  older cached checkout.
- Changes to reusable framework behavior belong in Interval. This project
  consumes the branch or release reference declared in `platformio.ini`.

## Documentation

| Document | Purpose |
|---|---|
| [Architecture](docs/architecture.md) | Product integration and application-specific design |
| [Roadmap](docs/roadmap.md) | Product work and pending validation |
| [Agent instructions](AGENT.md) | Contribution rules for coding agents |

For the framework's capabilities and development rules, see the
[Interval documentation](https://github.com/infrafast/Interval).
