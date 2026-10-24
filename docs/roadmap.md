# Roadmap and Change Tracking

## Purpose and tracking rules

This document is the single reference for requests, open work, validation, and audit results. Stable architecture is described in [architecture.md](architecture.md); contribution rules are in [AGENT.md](../AGENT.md).

Documentation consolidated on October 24, 2026. Previously unchecked items remain historical requests **to be assessed**, without assuming that current code does not already implement them. Analysis-derived ideas remain proposals, not requested or validated features.

Statuses: **To assess**, **To do**, **In progress**, **To validate**, **Done**, **Abandoned**. Each new request receives a stable ID, origin, scope, acceptance criteria, and, on completion, validation evidence and the relevant commit. Do not mark an item “Done” solely because a class exists.

All entries must be written in English. Every code change must be documented here, with the appropriate architecture and user-facing documentation updates, following `AGENT.md`.

## 1. Documentation requests

| ID | Request | Status | Criteria / result |
|---|---|---|---|
| DOC-01 | Separate user overview, architecture, tracking, and agent instructions | Done — documentation | Created README.md, docs/architecture.md, docs/roadmap.md, and AGENT.md; consolidated the three previous project documents |
| DOC-02 | Centralize audit content and retain supporting materials in subfolders | Done — documentation | PCB measurements and findings in architecture; tests and corrections in this roadmap; original supporting materials retained |
| DOC-03 | Check documentation organization with every change | Permanent rule | No new content documentation; update the appropriate reference file |
| DOC-04 | Translate all four project documents into English and require English documentation | Done — documentation | All four Markdown documents translated; identifiers, links, measurements, and historical limitations preserved; English-language rule added to AGENT.md |
| DOC-05 | Require documentation for every code change | Done — documentation | AGENT.md requires roadmap updates for every code change and architecture/README updates according to impact; documentation is part of completion |
| DOC-06 | Require concise code comments for human maintainers | Done — documentation | AGENT.md requires English comments explaining purpose and rationale, using short headers and occasional inline notes without redundant or lengthy narration |
| DOC-07 | Require reuse-first implementation and preserve framework/application boundaries | Done — documentation | AGENT.md and architecture define reuse → extend → create, generic Interval versus application code, limited Interval.h glue, and justified new source files |

### Documentation audit results

- No root README, architecture document, or roadmap existed in the original examined tree.
- `ANALYSE_REPO.md` combined overview, architecture, instructions, and modernization ideas: content redistributed across the four references.
- `lib/Interval/README.md` contained a request list: fully carried over below.
- `docs/audit-pcb-2.0-2.2/README.md` combined findings, tests, and proposals: split between architecture and roadmap, preserving measurements and limitations.
- The historical startup order is no longer universal: `dev` enables TFT before components; `prod` keeps the historical default. The PCB report is identified as historical.
- Consolidated old project documents were removed to avoid competing sources.
- READMEs, changelogs, and documents from distributed dependencies, especially `flash/` (esptool) and third-party libraries, remain upstream documents. They are not project design references and are not removed.
- The PCB subfolder retains PNG illustrations, JSON measurements, and the existing reproduction script. No separate report will be added there.

## 2. Observed features and incomplete areas

| ID | Work | Status | Acceptance criteria |
|---|---|---|---|
| MIX-01 | Complete the input-gain strip | To do | Connect reads/writes to the mixer contract, bound and convert values, handle responses; check supported backends and persistence |
| OSC-01 | Assess and complete the generic OSC strip | To assess | Check message library, parsing, serialization, and reception; validate a send/receive scenario |
| MEM-01 | Audit mixer-strip reservations and bindings | To assess | No uninitialized/dangling pointers; release on changes/deletion; handle missing/replaced interfaces; two strips with the same key remain consistent |
| MEM-02 | Move the generic reservation container into Interval | Done — code; firmware build pending | ValueList<T> moved to lib/Interval/src/Interval.h; mixer structures and wrappers retained in src/mixers/values.h; existing API and behavior preserved; targeted host checks passed |
| HW-01 | Compare both TFT/W5500 initialization orders | To validate | dev/prod tests, cold starts and wake-ups, Ethernet and color results recorded; decision documented before generalization |

These items derive from code reviewed for consolidation; they are not an exhaustive firmware audit.

## 3. Historical Interval library requests

Origin: all unchecked items in the former `lib/Interval/README.md`. No completion is inferred here.

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

## 4. Maintenance from repository analysis

These items preserve risks and opportunities from `ANALYSE_REPO.md`.

| ID | Proposal | Status | Acceptance criteria |
|---|---|---|---|
| TOOL-01 | Parameterize script source/destination paths | To assess | No mandatory dependency on K: or /Volumes/web/fwupdate; conventions documented in architecture |
| TOOL-02 | Separate packaging from deployment moves | To assess | Deterministic local build, explicit and optional external side effects |
| TOOL-03 | Provide local or optional minification | To assess | Asset preparation possible without an external HTTP service; controlled failure |
| TOOL-04 | Avoid workflow execution on simple script import | To assess | Import without unintended deletion, copying, or network calls |
| DEP-01 | Inventory local patches and dependency references | To assess | Adaptation origin/version/rationale described in architecture; reproducible updates |
| CI-01 | Automatically check dev and prod builds | To assess | Both environments compiled, packaging side effects controlled; no new workflow file in this documentation task |

## 5. PCB 2.0 / 2.2 audit — test tracking

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

### 5.1. Priority tests with a fixed firmware version

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

### 5.2. Proposed corrections for a future PCB compatible with the code

1. **Restore a direct ground return to U6 first.** Widely connect the common power/XIAO-area ground to lower-right U6 GND as in 2.0, then provide a continuous ground plane stitched with vias to the peripherals. Keep all three U6 GND pins connected to the plane and provide a TFT return close to SPI. Stop depending on a single mechanical slot for peripheral ground return. Ideally, jumper A-B should confirm this hypothesis before manufacturing a new board.

2. **Provide local decoupling.** Place footprints near module supply inputs with short GND connections: typically 100 nF and a few µF. Determine fitted values after identifying the modules, power switch, and their stability and startup-current constraints. Randomly adding a large capacitor can also degrade rail rise.

3. **Define startup CS states.** Provide a pull-up, typically 10 kΩ, on each CS to the **switched peripheral 3.3 V rail**, subject to resistors already present on the modules. This preserves the active-LOW selections expected by the code. Reset also needs a defined idle level; check its existing bias before adding a resistor. Avoid pulling an unpowered peripheral toward a permanent rail.

4. **Provide SPI damping.** Add series-resistor footprints on SCLK and MOSI near the XIAO. Try, for example, 22–47 Ω, adjusted from observed waveforms; this is not a value guaranteed by the Gerbers. For MISO, the source is W5500. Footprints on CS/DC are useful if measurements reveal disturbed edges. Placement near the source follows the series-termination principles described in the [TI TIDA-060045 SPI design guide](https://www.ti.com/lit/pdf/tiduf78).

5. **Limit unused branches toward U7.** Alternative U6/U7 footprints create bus branches. Provide selectable connections or 0 Ω resistors to isolate the unpopulated footprint without changing firmware GPIOs. On this board, retain U6 and the display. Keep a ground reference close to signals.

6. **Address shared reset separately if captures justify it.** Under historical initialization order, firmware physically resets W5500 during TFT initialization after the first Ethernet initialization. A separate W5500 power-on reset circuit can avoid this, but requires proper validation of its delay, rail ramp, and Ethernet procedure after ESP32 reset. Do not simply add a large capacitor on GPIO44 or drive a supervisor against the XIAO push-pull output. This remains a design option, not a proven fix for the regression.

Priorities 1–5 preserve the audited pinout, polarities, and firmware. Their effectiveness against the fault must be validated by the tests above; priority 6 requires further study.

## 6. Validation log

### October 24, 2026 — documentation consolidation

Reviewed project documents and code integration points: active application, interfaces, strips, datasets, mixers, reservations, build options, and hardware order. Performed documentation checks for paths, content distribution, and preservation of PCB audit content.

Commit: [8fcc973](https://github.com/infrafast/esp32/commit/8fcc973c459fa41ad7b91444512ced18e7704615).

This intervention changed neither firmware, PCB routing, nor third-party libraries. No builds or hardware measurements were performed for the reorganization. Open features and corrections above are therefore not declared validated.

### October 24, 2026 — English documentation and agent obligations

Translated README.md, docs/architecture.md, docs/roadmap.md, and AGENT.md into English. Preserved filenames, identifiers, links, audit measurements, archive hashes, request IDs, statuses, and technical limitations. Normalized the nested PCB audit heading numbering to fit its host documents.

Added mandatory English documentation and documentation requirements for every code change. Roadmap must track every code change; architecture and README must be updated according to implementation and user impact, and AGENT.md when contribution rules change.

Validation: checked the four-file scope, link-target preservation, tracking IDs, measurement values, and archive hashes. No source files, build settings, dependency documentation, attachments, or hardware were changed; no builds or hardware tests were run.

### October 24, 2026 — concise code-comment rule

Added DOC-06 at the user's request. AGENT.md now requires concise English comments in generated or modified code, explaining what it does and why, primarily in headers and occasionally inline for non-obvious details. Comments must stay accurate and support human maintenance without duplicating obvious code or becoming lengthy narratives.

Validation: reviewed the rule against the requested level of detail and existing documentation obligations. Only AGENT.md and docs/roadmap.md were changed; no code or new files were created.

### October 24, 2026 — reuse-first design and framework boundaries

At the user's request, checked the existing rules: they already required established contracts and responsibility identification, but did not explicitly mandate the reuse → extend → create order or source-file creation policy.

Added DOC-07. AGENT.md now requires inspecting existing capabilities before implementation, prioritizing reuse and extension, preserving generic Interval versus application-specific code, using Interval.h only for necessary minimal framework glue when object-based integration is unsuitable, and preferring existing responsibility-owning files. New source/header files require a design rationale recorded with the reuse assessment in roadmap. The architecture documents the same dependency direction and extension policy.

Validation: checked consistency with existing documentation restrictions, class contracts, and the current separation of Interval and mixer code. Only AGENT.md, docs/architecture.md, and docs/roadmap.md were changed; no source files were created or modified. No build or hardware test was needed for this documentation change.

### October 24, 2026 — MEM-02: separate generic storage from mixer values

At the user's request, moved the existing generic `ValueList<T>` template into `lib/Interval/src/Interval.h`, before framework integration includes. Added explicit standard includes for `std::nothrow` and fixed-width integers. Kept `OneIndexValue`, `TwoIndexValue`, `OneIndexValueList`, and `TwoIndexValueList` in `src/mixers/values.h`, which explicitly includes `Interval.h`.

Reuse assessment: reused the existing linked-node implementation and matcher/initializer extension points unchanged. No new container, abstraction, source file, or dependency was introduced. The generic template depends only on the record's default construction and reservation counter; mixer keys and payload semantics remain in application code. Condensed the moved template's comments and translated touched example comments into English.

Validation: verified a single template definition and unchanged template code after removing comments/whitespace. Compiled the extracted template with the actual mixer structures/wrappers using C++11, warnings as errors, AddressSanitizer, and UndefinedBehaviorSanitizer. Targeted runtime checks passed for generic non-mixer records, one/two-index sharing, reservation counts, unrelated insertion/removal pointer stability, head/middle/final release, clear, and destruction. LeakSanitizer could not run in this environment; reran successfully with leak detection disabled. Checked declaration placement and the existing Interval include guards/dependency direction.

Limitations: PlatformIO is not installed here, so no complete dev/prod ESP32 build or hardware test was performed. The host check compiles extracted portable code, not the full Arduino header graph. Existing reservation-counter limits and caller lifetime/error-handling concerns are unchanged. Only the two existing source headers and architecture/roadmap documentation were modified; no repository files were created.
