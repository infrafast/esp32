# Interval Footswitch Firmware

This repository contains the product firmware for the Interval footswitch. It
combines the Seeed XIAO ESP32-S3, TFT display, W5500 Ethernet, Wi-Fi, three
analog ladder buttons, one power button, LittleFS resources, and the web
configuration interface.

The product application uses the reusable
[`infrafast/Interval`](https://github.com/infrafast/Interval) framework to
provide the mixer-management application, including Mixing Station and
Behringer X/M-series workflows. The framework is consumed through the
versioned dependency in `platformio.ini`; do not copy it into this repository's
`lib/` directory.

Interval also provides the framework-owned `DiagnoseApplication`, selected by
the `INTERVAL_DIAGNOSE` build flag. It offers a minimal hardware diagnostic
mode without duplicating diagnostic application code in this product project.

## Build and operation

The configured target is the Seeed XIAO ESP32-S3 with Arduino, LittleFS, an
ILI9341 display, and W5500 Ethernet. Development and production environments
have different USB/debug behavior; inspect `platformio.ini` before flashing.

```sh
pio run -e dev
pio run -e prod
```

The firmware serves the compressed resources in `data/webapp/`. The editable
web client is maintained in the separate webGUI checkout. On the development
workstation it is normally available as `../webGUI/`; use `WEBGUI_SRC` when
running `preparefiles_MACOS.py` from another location. A static server is
enough for asset inspection, while the PHP demo server is needed for the mock
API under `post/` and `upload/`.

The development build also prepares firmware/resource archives. See
[`docs/architecture.md`](docs/architecture.md) for build side effects,
packaging, hardware initialization, and update workflows.

## Repository boundaries

- `src/` contains the product launcher, mixer applications, adapters, and
  product-specific behavior. The reusable diagnostic application belongs to
  Interval.
- `data/` contains firmware-owned resources, translations, and packaged web
  assets.
- `lib/` contains only project-local third-party libraries or intentional
  patches; the Interval framework is an external PlatformIO dependency.

Framework behavior must be changed in the Interval repository. The current
consumer tag is `v0.2.2`; after a new Interval release is tagged, update the
exact tag in `platformio.ini` and commit that consumer change. Keep all project
paths relative or PlatformIO-resolved so
the firmware also builds in Codebase and other environments.

## Documentation

| Document | Purpose |
|---|---|
| [Architecture](docs/architecture.md) | Firmware structure, product workflows, hardware/build details, and integration with Interval |
| [Roadmap](docs/roadmap.md) | Product work, PCB findings, open firmware/webGUI items, and pending validation |
| [Agent instructions](AGENT.md) | Contribution rules and documentation discipline |

The Interval framework has its own matching documentation split in its
repository. These documents describe only this firmware product and link to
the framework documentation when implementation details belong there.
