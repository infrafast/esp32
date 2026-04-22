# Repository Analysis (Reference Guide)

## 1) What this repository is

This is a **PlatformIO + Arduino** firmware project targeting the **Seeed XIAO ESP32-S3**. It combines:

- Embedded application runtime (C++ in `src/` + `lib/Interval/`)
- A bundled/compressed web UI served from LittleFS (`data/webapp/`)
- OTA/update packaging helpers (Python scripts hooked into PlatformIO)
- Mixer-control domain logic (Mixing Station + Behringer X/M protocols)

In practice, this repository behaves as a **hardware controller + networked mixer integration platform** with local UI and update tooling.

---

## 2) Build and environment model

### Platform and board

- PlatformIO platform pin: `espressif32@6.4.0`
- Framework: `arduino`
- Board: `seeed_xiao_esp32s3`
- Filesystem: `littlefs`

These are declared in `platformio.ini` and are central to reproducible builds.

### Environments

The project provides two primary PlatformIO environments:

- `env:dev`
  - debug-oriented (`-D DEBUG=1`)
  - runs pre/post scripts (`prescript.py`, `gzfw_to_data.py`, `make_tgz.py`)
- `env:prod`
  - release-oriented (`-D PROD=1`)
  - USB mode flags for MIDI use-case (`ARDUINO_USB_MODE=0`, `ARDUINO_USB_CDC_ON_BOOT=0`)

### Build-time scripts and side effects

- `prescript.py`: injects TFT and setup defines before compilation.
- `gzfw_to_data.py`: compresses generated firmware binary (`.bin -> .bin.gz`) into `data/` after build.
- `make_tgz.py`: creates a versioned `.tgz` archive from `data/` and moves artifacts to `/Volumes/web/fwupdate`.

**Important operational implication:** build output is not only a firmware binary; it also produces deployable update assets.

---

## 3) Runtime entrypoints and startup flow

### Entrypoint

- `src/main.cpp` includes `MixerApplication.h` (current active entry path).

### Active app bootstrap

- `src/MixerApplication.h`
  - enables network path via `#define USE_NETWORK`
  - includes `Interval.h`
  - creates mixer implementations (`MixingStation`, `XMSeries`)
  - creates strip skeletons (`input2mixbus`, `inputGain`) and registers datasets
  - calls `app.init()` in `setup()` and `app.refresh()` in `loop()`

### Alternate test harness

- `src/TestApplication.h` exists as a minimal app path (`app.init()/app.refresh()`), but is not selected by `main.cpp` currently.

---

## 4) Core architecture map

## 4.1 Core framework layer (`lib/Interval/src`)

`Interval` is the local framework that provides most of the reusable infrastructure:

- interface abstraction and lifecycle
- dataset and state synchronization machinery
- networking integrations (conditional on compile flags)
- filesystem/update helpers
- app-level orchestration (`app` object used by the entrypoint)

`lib/Interval/src/Interval.h` is a broad integration header and indicates this framework owns substantial system behavior.

## 4.2 Domain layer (`src/mixers`)

Mixer-specific behavior is implemented as app-level domain extensions:

- `mixer.h`
  - defines `MixerBase`, synchronized datasets (`DataSetSynced`), identity/definition structures, and shared constants.
- `m_mstation.h`
  - implementation for Mixing Station protocol/routes over socket transport.
- `m_xmseries.h`
  - implementation for X/M series behavior over OSC transport, discovery workflow, protocol file loading.
- `s_mixer.h`
  - strip mixin system binding UI strips to active mixer behavior.
- `values.h`
  - shared value-reservation containers (`OneIndexValueList`, `TwoIndexValueList`) with reference-count-like reservation semantics.

### Architectural pattern in use

The code strongly uses:

- interface-driven polymorphism (virtual methods on mixer abstractions)
- template composition (`StripMixerMixin`, `MixerDevice<Transport>` patterns)
- shared mutable state by pointer to propagate updates between UI strips and mixer backends

---

## 5) Data, web UI, and update pipeline

### Web asset storage

`data/webapp/` stores compressed web resources (`*.gz`) plus a few static binaries (e.g., fonts/images).

### Asset preparation tooling

`preparefiles.py` defines a manual pipeline:

1. Purge destination folder
2. Minify web assets (CSS/JS/HTML via remote API calls)
3. Copy + gzip selected filetypes

### Pipeline concerns

The script currently includes hardcoded/local assumptions:

- hardcoded source path (`K:`)
- Windows-specific mapping helper usage (`map_folder.bat`)
- network dependency on external minifier endpoints (`toptal.com` APIs)
- auto-execution of `workflow()` on import

This means the asset pipeline is powerful but not portable by default.

### Firmware/update packaging

- `gzfw_to_data.py` creates compressed firmware artifact in `data/`.
- `make_tgz.py` tars `data/`, names archive using `_V`, and moves outputs to external destination.

This effectively couples build and deployment staging in one process.

---

## 6) Dependency strategy

The project mixes three dependency sources:

1. **PlatformIO registry libs**
2. **Git URL dependencies pinned in `lib_deps`**
3. **Vendored/locally patched libs in `lib/`** (e.g., `Ethernet`, `mWebSockets`, `Interval`, `TFT_eSPI`)

This hybrid approach is practical for embedded patching, but increases maintenance overhead when upstream versions change.

---

## 7) Practical coding-guide reference (derived from repository behavior)

These guidelines reflect what the repository currently *expects* to work well.

### 7.1 Entrypoint and app lifecycle

- Keep `setup()` lightweight and declarative: instantiate interfaces/strips, then call `app.init()`.
- Keep main loop non-blocking: rely on `app.refresh()` and per-module monitor methods.

### 7.2 Mixer/domain extensions

- New mixer integrations should follow `MixerBase` contracts (request/setter symmetry).
- Use dataset abstractions (`DataSet`, `DataSetSynced`) rather than ad-hoc state channels.
- Prefer reservable shared value containers for strip-to-strip coherence (`values.h` pattern).

### 7.3 Memory and embedded constraints

- Prefer bounded/static buffers for parsing/transport paths where feasible.
- Guard dynamic allocation paths and clean up reference/reservation usage explicitly.
- Avoid heavy blocking logic in network monitoring methods.

### 7.4 Build script hygiene

- Any new script should be deterministic in CI and avoid machine-specific absolute paths.
- Keep side effects explicit (artifact moves, deletes, uploads).
- Gate optional deploy behavior by environment variables when possible.

### 7.5 Configuration management

- Treat `platformio.ini` as the source of truth for environment behavior.
- Keep `dev` and `prod` macro differences intentional and documented next to usage.

---

## 8) Repository hotspots (high-impact change areas)

1. `lib/Interval/src/Interval.h` and adjacent Interval internals
   - broad effect radius across runtime behavior.
2. `src/mixers/*`
   - protocol behavior and UI synchronization logic.
3. `platformio.ini`
   - can alter toolchain/framework behavior dramatically.
4. `prescript.py`, `gzfw_to_data.py`, `make_tgz.py`, `preparefiles.py`
   - can break build/deploy pipelines if changed carelessly.

---

## 9) Risk register and modernization opportunities

### Current risks

- Portability risk due to hardcoded paths and OS-specific assumptions in helper scripts.
- Reliability risk from external HTTP minifier services in asset preparation.
- Complexity risk from mixed dependency sourcing and local patch drift.

### High-value improvements

- Parameterize script paths and destination directories.
- Add local minification fallback (or make minification optional/offline-friendly).
- Document expected local deployment path conventions.
- Add lightweight CI checks for both `env:dev` and `env:prod` compile validity.

---

## 10) Quick onboarding checklist for contributors

1. Read `platformio.ini` fully before making build/runtime assumptions.
2. Validate whether your change touches runtime core (`Interval`) or mixer domain (`src/mixers`).
3. Build with the target environment (`dev` or `prod`) that matches your intended behavior.
4. If touching scripts, test from a clean workspace and confirm side effects.
5. If touching mixer logic, verify dataset sync and strip value propagation patterns remain consistent.

---

## 11) Final summary

This repository is a **custom embedded application stack** with a strong internal framework (`Interval`), specialized mixer protocol adapters, and an integrated web/update packaging pipeline. Its main strengths are domain-focused architecture and practical tooling; its main challenges are portability and dependency/script maintainability. As a coding reference, contributors should prioritize consistency with the existing app lifecycle, mixer abstraction contracts, and build artifact flow.
