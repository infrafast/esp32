# Roadmap

## Purpose and tracking rules

This document contains only desired future changes, open issues, work in progress, and pending validation. Current implementation and lasting design decisions belong in [architecture.md](architecture.md); contribution and roadmap maintenance rules are in [AGENT.md](../AGENT.md).

Statuses: **To assess**, **To do**, **In progress**, **To validate**. Give each open request a stable ID, origin, remaining scope, and acceptance criteria. Remove completed, abandoned, or superseded work and its associated notes immediately. If implementation is complete but validation remains, retain only the outstanding validation task. Do not infer completion merely from the presence of a class.

Write all entries in English. Keep only remaining work after each change; use Git history and commit/pull-request descriptions for completed changes and validation evidence. Do not add completion logs or create an archive document. Older unchecked requests remain subject to assessment against current code, rather than assumed missing. Analysis-derived ideas remain proposals until assessed.

## 1. Features, open issues, and pending validation

| ID | Work | Status | Acceptance criteria |
|---|---|---|---|
| MIX-01 | Complete the input-gain strip | To do | Connect reads/writes to the mixer contract, bound and convert values, handle responses; check supported backends and persistence |
| OSC-01 | Assess and complete the generic OSC strip | To assess | Check message library, parsing, serialization, and reception; validate a send/receive scenario |
| MEM-01 | Audit mixer-strip reservations and bindings | To assess | No uninitialized/dangling pointers; release on changes/deletion; handle missing/replaced interfaces; two strips with the same key remain consistent |
| MEM-02 | Validate the extracted ValueList<T> in complete ESP32 firmware builds | To validate | Compile dev and prod against the full Arduino header graph after the extraction; the portable host checks do not cover firmware integration, and no complete ESP32 build has been recorded |
| MEM-03 | Gradually migrate suitable `ivanseidel/LinkedList` usages to `std::vector` | To assess | Inventory all uses and assess ownership, pointer/reference/iterator stability, insertion/removal patterns, worst-case capacity, heap usage and compatibility; migrate incrementally with targeted tests and dev/prod firmware builds; retain linked or fixed-capacity storage where technically justified |
| HW-01 | Compare both TFT/W5500 initialization orders | To validate | dev/prod tests, cold starts and wake-ups, Ethernet and color results recorded; decision documented before generalization |

These items derive from code reviewed for consolidation; they are not an exhaustive firmware audit.

### MEM-03 — progressive standard-container migration

Origin: user request, October 24, 2026. Objective: favor the standard C++ `std::vector<T>` over `ivanseidel/LinkedList` when it improves RAM efficiency, predictable allocation behavior, robustness, and maintainability. This is **not** a blanket replacement mandate.

1. Inventory all `LinkedList` instances (including indirect wrappers), document element type, ownership, mutation and access patterns, current peak sizes, and any stored pointers/references/iterators.
2. Prioritize straightforward indexed/append-oriented collections. Before each change, evaluate whether `std::vector` with an appropriate `reserve()` is compatible with required element and pointer stability; use fixed-capacity storage for allocation-sensitive real-time paths. Reserve only when justified by an established bound or measurement. Check allocation failure behavior and capacity enforcement.
3. Migrate in small, independently verifiable groups while preserving public behavior, serialization/configuration format, and existing framework/application interfaces. Prefer adapting existing abstractions over introducing new files or redundant wrappers.
4. Preserve architecture: reusable, domain-neutral collection code belongs under `lib/Interval`; mixer-specific semantics and bindings remain in `src/mixers`. Keep `Interval.h` glue minimal and justified. Do not move mixer logic into the generic framework.
5. Specifically assess `ValueList<T>` in Interval: its reservation counters, sharing and node/pointer lifetime contract may require stable addresses, so do **not** directly substitute `std::vector` until these invariants are preserved or intentionally redesigned with tests.
6. For each migration batch, include affected files, memory/heap and fragmentation observations where possible, ownership/lifetime checks, regression tests, dev/prod builds, and appropriate hardware validation in the commit or pull-request description. Keep only remaining scope and pending checks in this roadmap. Remove the `LinkedList` dependency only after confirming there are no remaining required uses.

Acceptance: no behavioral regressions, no dangling references after container changes, measured or reasoned memory benefit, no unjustified runtime allocation in critical paths, all relevant tests passing, and roadmap/architecture documentation updated. Track each batch's remaining scope separately and remove completed batches; remove MEM-03 when the migration and required validation are complete.

## 2. Interval library requests to assess

Origin: unchecked requests from the former `lib/Interval/README.md`. Assess each against current code; remove requests already implemented and validated.

| ID | Preserved request | Status | Expected validation |
|---|---|---|---|
| INT-01 | Add more templates with examples | To assess | Identify missing templates; integrate examples into authorized files or the relevant code |
| INT-02 | Check memory consumption when deleting MenuSystem | To assess | Measure repeated creation/deletion and recovered memory |
| INT-03 | Implement the menuSystem destructor, delete all MenuComponent objects, then adjust allocation | To assess | Check ownership and current allocator; the historical `realloc()` suggestion is not a prescription without this check |
| INT-04 | Add AP mode for the wizard | To assess | Define the gap relative to existing AccessPoint support; validate entering/leaving the flow |
| INT-05 | Add a wizard screen | To assess | Validate initial setup flow and return to normal configuration |
| INT-06 | Add a profile icon | To assess | Consistent display and selection |
| INT-07 | Add profile loading and saving | To assess | Persistent round trip and read-error handling |
| INT-08 | Make Label float-compatible and move the relevant ArcMeter code into it | To assess | Check shared numeric display, formatting, and boundary values |
| INT-09 | Add icons to menu items | To assess | Consistent rendering in affected menus |
| INT-10 | Add icon and label to the translation file | To assess | Check keys, fallback, and language switching |
| INT-11 | Implement menu-item text | To assess | Display initial values and updates |
| INT-12 | Implement menu-item feedback | To assess | State feedback synchronized with the action |
| INT-13 | Add Chinese multilingual files | To assess | Check key coverage, fonts, and encoding |
| INT-14 | Add/complete Spanish multilingual files | To assess | An `espanol.json.gz` file already exists; check coverage before marking the request complete |

## 3. Maintenance proposals

These items preserve risks and opportunities from `ANALYSE_REPO.md`.

| ID | Proposal | Status | Acceptance criteria |
|---|---|---|---|
| TOOL-01 | Parameterize script source/destination paths | To assess | No mandatory dependency on K: or /Volumes/web/fwupdate; conventions documented in architecture |
| TOOL-02 | Separate packaging from deployment moves | To assess | Deterministic local build, explicit and optional external side effects |
| TOOL-03 | Provide local or optional minification | To assess | Asset preparation possible without an external HTTP service; controlled failure |
| TOOL-04 | Avoid workflow execution on simple script import | To assess | Import without unintended deletion, copying, or network calls |
| DEP-01 | Inventory local patches and dependency references | To assess | Adaptation origin/version/rationale described in architecture; reproducible updates |
| CI-01 | Automatically check dev and prod builds | To assess | Both environments compiled, packaging side effects controlled; no new workflow file in this documentation task |

## 4. PCB 2.0 / 2.2 — pending tests and design proposals

Origin: initial audit dated October 21, 2026, U6 revision dated October 22, examined code `58ba13a`. Findings, pinouts, measurements, illustrations, archive hashes, and method limitations are preserved in [architecture](architecture.md). This documentation audit provides no board-measurement results.

| ID | Action | Status | Expected validation |
|---|---|---|---|
| PCB-01 | Check continuity and solder joints for all three U6 grounds and TFT | To validate | Power-off measurements; inspect battery-pad plating |
| PCB-02 | Test the XIAO GND → lower-right U6 GND jumper | To validate | Same firmware, repeated startups; record Ethernet detection and colors separately |
| PCB-03 | Test the TFT ground return if necessary | To validate | Compare with/without additional return after PCB-02 |
| PCB-04 | Capture local power rail, reset, CS, and SPI | To validate | Startup and wake-up captures, suitable probe reference |
| PCB-05 | Define the next PCB revision from test results | Proposal to validate | Evaluate direct return/ground plane, decoupling, CS states, and damping; do not present any cause as proven |
| PCB-06 | Study a separate W5500 reset only if needed | Proposal to validate | Validate timing and electrical/firmware compatibility |

### Protocol and proposals preserved from the audit

The protocol below is preserved as a historical proposal. When comparing PCBs, fix and record `HW_INIT_TFT_BEFORE_COMPONENTS`: TFT-first order is now enabled in dev. Do not combine a hardware change and a firmware change within the same test.

### 4.1. Priority tests with a fixed firmware version

Make one change at a time and repeat several cold starts and shutdown/wake-up cycles.

| Order | Test | Useful result |
|---|---|---|
| 1 | With power off, measure XIAO GND ↔ each of the three U6 GND pins ↔ TFT GND. Check their solder joints and inter-layer continuity at the negative battery pad | An open circuit or abnormal resistance provides a direct hardware cause |
| 2 | With power off, install an insulated ground wire, as direct as possible, between **XIAO GND and lower-right U6 GND**: points A-B in the illustration in architecture. Repeat startups with the same firmware | If Ethernet becomes detectable again, the removed connection is strongly implicated; if colors also recover, a common electrical origin is more likely |
| 3 | Retain A-B, then add a direct XIAO → TFT ground return (A-C) if needed | Distinguishes improvement specific to U6 from additional display sensitivity; record Ethernet and colors separately |
| 4 | Use an oscilloscope to measure 3.3 V **between U6's local 3V3 and GND pins**, then at the TFT power terminals, throughout startup | A voltage drop or incorrect rise points toward supply, switching, or decoupling |
| 5 | Capture GPIO5/rail, U6 RST, U6 CS, TFT CS, and SCLK with a short local ground reference | Check reset release before the first Ethernet frame, the later 20 ms LOW pulse under historical order, and CS states during the other peripheral's exchanges |
| 6 | If TFT is removable, disconnect it with power off and test U6 alone; since W5500 is soldered, start with tests that require no desoldering | Ethernet recovery indicates interaction through power, reset, or SPI, without identifying which one by itself |
| 7 | If power/reset/CS are correct, observe SCLK and MOSI at the receivers and try a series resistor near the source | A reproducible improvement confirms sensitivity to signal-edge quality |

For the bare W5500, specified VDD is **2.97 to 3.63 V**, relative to its local GND. If the module includes a regulator or another supply arrangement, also measure the voltage actually applied to the IC. [Source: W5500, §5.3](https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf).

Use a short probe connection or a suitable differential measurement: common instrument grounds can themselves create an additional return and hide the fault being investigated.

For incorrect colors, check whether TFT pixel-format and memory-control commands are received correctly: initialization writes `PIXFMT (0x3A) = 0x55`, then `MADCTL (0x36) = 0x08`; rotation later rewrites `MADCTL`. An incorrect color does not prove a display-mode change: a lost bit, incorrectly received command, or corrupted pixel data remains possible. Using the same XIAO and display makes a different controller or persistent configuration much less convincing.

### 4.2. Proposed corrections for a future PCB compatible with the code

1. **Restore a direct ground return to U6 first.** Widely connect the common power/XIAO-area ground to lower-right U6 GND as in 2.0, then provide a continuous ground plane stitched with vias to the peripherals. Keep all three U6 GND pins connected to the plane and provide a TFT return close to SPI. Stop depending on a single mechanical slot for peripheral ground return. Ideally, jumper A-B should confirm this hypothesis before manufacturing a new board.

2. **Provide local decoupling.** Place footprints near module supply inputs with short GND connections: typically 100 nF and a few µF. Determine fitted values after identifying the modules, power switch, and their stability and startup-current constraints. Randomly adding a large capacitor can also degrade rail rise.

3. **Define startup CS states.** Provide a pull-up, typically 10 kΩ, on each CS to the **switched peripheral 3.3 V rail**, subject to resistors already present on the modules. This preserves the active-LOW selections expected by the code. Reset also needs a defined idle level; check its existing bias before adding a resistor. Avoid pulling an unpowered peripheral toward a permanent rail.

4. **Provide SPI damping.** Add series-resistor footprints on SCLK and MOSI near the XIAO. Try, for example, 22–47 Ω, adjusted from observed waveforms; this is not a value guaranteed by the Gerbers. For MISO, the source is W5500. Footprints on CS/DC are useful if measurements reveal disturbed edges. Placement near the source follows the series-termination principles described in the [TI TIDA-060045 SPI design guide](https://www.ti.com/lit/pdf/tiduf78).

5. **Limit unused branches toward U7.** Alternative U6/U7 footprints create bus branches. Provide selectable connections or 0 Ω resistors to isolate the unpopulated footprint without changing firmware GPIOs. On this board, retain U6 and the display. Keep a ground reference close to signals.

6. **Address shared reset separately if captures justify it.** Under historical initialization order, firmware physically resets W5500 during TFT initialization after the first Ethernet initialization. A separate W5500 power-on reset circuit can avoid this, but requires proper validation of its delay, rail ramp, and Ethernet procedure after ESP32 reset. Do not simply add a large capacitor on GPIO44 or drive a supervisor against the XIAO push-pull output. This remains a design option, not a proven fix for the regression.

Priorities 1–5 preserve the audited pinout, polarities, and firmware. Their effectiveness against the fault must be validated by the tests above; priority 6 requires further study.
