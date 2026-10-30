# Mixer Application Architecture

This document describes only the product application assembled in this repository. Shared hardware, GUI, interface
lifecycle, persistence, i18n contracts, dependency ownership, and diagnostic behavior are documented in the [Interval
framework architecture](https://github.com/infrafast/Interval/blob/main/docs/architecture.md).

## Application composition

`src/main.cpp` is the firmware entry point. It includes `Interval.h` once and selects one of two application modes:

- `INTERVAL_DIAGNOSE`: instantiate `Diagnose` and forward `setup()`/`loop()`;
- normal mode: instantiate the mixer backends, register product strips, call
`app.init()` from `setup()`, and call `app.refresh()` from `loop()`.

The application owns mixer profiles, product strips, product icons, translations and packaged resources. It does not
duplicate Interval's shared hardware or framework classes.

## Mixer backends

`MixingStation` and `XMSeries` are product adapters built on the generic Interval interface and network contracts. Their
protocol-specific behavior belongs in `src/mixers/`:

- `mixer.h` contains shared mixer-domain values and the `MixerBase` contract;
- `m_mstation.h` implements the Mixing Station backend;
- `m_xmseries.h` implements the X/M-series backend;
- `s_mixer.h` contains mixer-specific strips and constructor callbacks;
- `values.h` contains shared mixer selection/value reservations.

`MixerDevice<Transport>` keeps protocol-independent mixer operations in `MixerBase` and selects the transport at compile
time. Do not cast an arbitrary interface to a concrete mixer; resolve it through the existing association and base
contracts.

## Mixer strips and shared values

Product strips use the generic `Strip`, `OneSelectStrip`, `TwoSelectStrip` and `StripMixerMixin` types supplied by
Interval. A strip skeleton must register a stable type identifier and its property addresses in the same order used by
the persisted configuration.

Mixer strips may share values through the reservation wrappers in `values.h`. Every reservation must be released on
replacement and destruction, and every nullable mixer or interface binding must be checked before use.

Visibility and focus callbacks may request mixer values, but periodic work must remain bounded and non-blocking. A
disconnected or non-ready interface must not receive live requests.

## Application lifecycle

Keep `setup()` declarative: construct backends, register strips and initialize Interval. Keep `loop()` limited to the
existing non-blocking application refresh. Network retries, interface monitoring, GUI updates, persistence and hardware
services belong to the framework lifecycle and must not be re-created in the mixer application.

## Adding a mixer feature

1. Confirm that the behavior is mixer-specific; generic behavior belongs in Interval. 2. Reuse `MixerBase`,
`MixerDevice<Transport>`, existing datasets and shared value reservations before introducing a new abstraction. 3.
Preserve protocol index conversions, model limits, request/set pairs and synchronization with the web configuration
interface. 4. Register new strips and addresses from `main.cpp` in a stable order. 5. Validate creation, save/reload,
selection changes, disconnection, reconnection, visibility/focus and teardown paths.

## Build boundary

`platformio.ini` selects Interval through the relative sibling dependency `symlink://../Interval` for local workspace
development and supplies only board/product build settings. The symlink keeps the application on the live sibling
checkout instead of a stale copied package. Release consumers should replace it with a versioned Git/package reference.
Interval owns its transitive libraries, including the official TFT_eSPI package. The TFT
pinout and SPI frequency are defined in `Interval/src/HWManager.h`; Interval's build hook derives the generated TFT
setup from that header, so the application must not redeclare those dependencies or settings. Framework dependency or
hardware lifecycle changes belong in the Interval repository; mixer and product workflow changes belong here.

`platformio.ini` contains only the consumer's board, paths and profile choice. `INTERVAL_BUILD_DEV` and
`INTERVAL_BUILD_PROD` are the only build-mode choices made here; the Interval library hook maps them to the required
USB/compiler settings, injects the application Git revision, and registers the packaging workflow. This project supplies
the `data/` directory and optional `FWUPDATE_DEST` location. All paths are derived from PlatformIO's project
environment, so the same configuration works from a checkout, Codebase, CI, or another host.

The generic web-asset script is provided by the resolved Interval dependency. `webgui_src` and `firmware_webapp_dest` in
this file select the application paths; `WEBGUI_SRC` and `FIRMWARE_WEBAPP_DEST` remain supported overrides for Codebase
and CI.

The application's `${PROJECT_DIR}/data` directory is the complete LittleFS staging tree. The Interval preparation script
copies framework resources from `Interval/data/`, while product resources such as mixer definitions remain in this
repository. WebGUI source files remain in the independent webGUI repository; minified/gzipped copies are generated under
`data/webapp/` before `uploadfs`.
