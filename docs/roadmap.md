# Roadmap

## Purpose and tracking rules

This document contains only desired future changes, open issues, work in progress, and pending validation. Current implementation and lasting design decisions belong in [architecture.md](architecture.md); contribution and roadmap maintenance rules are in [AGENT.md](../AGENT.md).

Statuses: **To assess**, **To do**, **In progress**, **To validate**. Give each open request a stable ID, origin, remaining scope, and acceptance criteria. Remove completed, abandoned, or superseded work and its associated notes immediately. If implementation is complete but validation remains, retain only the outstanding validation task. Do not infer completion merely from the presence of a class.

Write all entries in English. Keep only remaining work after each change; use Git history and commit/pull-request descriptions for completed changes and validation evidence. Do not add completion logs or create an archive document. Older unchecked requests remain subject to assessment against current code, rather than assumed missing. Analysis-derived ideas remain proposals until assessed.

## Bugs to fix

Open memory-safety and ownership corrections identified by static inspection of [the source revision](https://github.com/infrafast/esp32/tree/679bafe8a8461f34d67096db16b2685835ae7c67). No firmware build, runtime leak measurement, browser heap profile or hardware validation was performed for this audit. Paths/line numbers below refer to that revision, not to future edits.

Each ID is an actionable request: for example, **implement BUG-WEB-01**. Existing IDs are preserved when a previous request already covers the defect. **P1**: address first (memory corruption, repeatable leaks, or unsafe error paths); **P2**: next (bounded growth, teardown or conditional lifecycle defects). Priority is not a claim that every trigger occurs during normal use. All fixes must preserve the Interval/application separation and existing APIs where possible, include targeted regression checks and dev/prod builds when available, and be removed here once implemented and validated.

### Core / buffers

#### BUG-CORE-01 — Make ByteBuffer allocation failure safe

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/Interval.h:818](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Interval.h#L818) — ByteBuffer::write/shrink assign realloc directly to buffer and update capacity before success; write then calls memcpy without checking allocation or size arithmetic. OOM can lose the previous allocation and dereference null.
- **Requested change:** Use a temporary allocation result, checked size arithmetic and bounded growth; retain the original buffer/state on failure and propagate a short-write/error to compression callers.
- **Acceptance:** Inject initial/growth/shrink allocation failures and overflow-sized requests; no lost allocation, invalid access or false success.

### Interfaces / configuration

#### NET-CONFIG-01 — Validate deferred STA credentials from the configuration AP

- **Status:** To validate.
- **Origin:** User-requested fallback workflow when Ethernet and saved STA networks are unavailable.
- **Implemented scope:** `setWifi` now persists the selected SSID/password while a dedicated configuration AP remains active; the normal STA applies the saved values on the next startup.
- **Remaining validation:** On hardware, start the AP from the wizard, connect a phone, select an SSID, submit credentials, confirm the AP/WebGUI remains reachable, reboot, and verify STA association. Also check empty-SSID rejection and persistence-write failure handling.
- **Acceptance:** The AP session is not interrupted by `setWifi`; after reboot the selected SSID is attempted with the saved password; normal non-AP Wi-Fi configuration keeps its existing immediate-connection behavior.

#### BUG-IF-01 — Bound network and OSC property parsing

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/Interfaces.h:881](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Interfaces.h#L881) — NetworkDevice::restoreProperty and NetworkOSC::restoreProperty (interfaces/i_osc.h:276) scan string properties into fixed inputStr arrays using address patterns without a destination-width bound.
- **Requested change:** Reuse existing property APIs with bounded extraction, complete-match validation and explicit invalid-input rejection.
- **Acceptance:** Oversized IP/ping properties and malformed inputs are rejected without stack writes outside buffers; valid saved configurations still load.

### Interfaces / lifecycle

#### IMP-IF-BLE-01 — Provide a real BLEMIDI start/stop lifecycle

- **Status:** To assess.
- **Observation:** `BLEMIDIInterface` currently forces `state.enabled = true` and `state.switchable = false` because the selected `lathoub/Arduino-BLE-MIDI` ESP32 backend does not implement a real shutdown. The transport exposes `begin()`/`end()`, but `BLEMIDI_ESP32::end()` only prints a message; the NimBLE backend's `end()` is empty as well. The classic ESP32 disconnect callback also restarts advertising. See the [upstream ESP32 backend](https://github.com/lathoub/Arduino-BLE-MIDI/blob/master/src/hardware/BLEMIDI_ESP32.h), [NimBLE backend](https://github.com/lathoub/Arduino-BLE-MIDI/blob/master/src/hardware/BLEMIDI_ESP32_NimBLE.h), and [transport wrapper](https://github.com/lathoub/Arduino-BLE-MIDI/blob/master/src/BLEMIDI_Transport.h).
- **Rationale:** Exposing the generic enable switch today would claim that BLE is disabled while the BLE server/advertising and global ESP32 Bluetooth resources could remain active. A generic `BLEDevice::deinit()` is global to the BLE stack and must not be introduced without checking coexistence with other Bluetooth users (including `BluetoothSerial`) and the Arduino ESP32 core lifecycle.
- **Requested change:** Assess and implement a backend-aware, idempotent BLEMIDI teardown/restart path, including advertising/server/resource ownership and callback safety. Only then restore `state.switchable = true` and normal enable/disable workflow behavior.
- **Acceptance:** Enabling starts one BLE-MIDI instance; disabling stops advertising and releases or explicitly retains documented resources; re-enabling works repeatedly on the XIAO ESP32-S3; callbacks cannot target a stopped instance; no other Bluetooth service is disrupted.

#### BUG-IF-02 — Unregister destroyed network services

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/Interfaces.h:450](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Interfaces.h#L450) — NetWorkService::addToInventory appends raw pointers; no corresponding removal exists. DeviceInterface::~DeviceInterface removes only interfaceRepo membership. Deleting a network-service object leaves inventory pointing to freed storage.
- **Requested change:** Define teardown through existing classes: release capacity, stop subscriptions/tasks, and remove inventory entries before destruction. This is a teardown-path defect; current startup instances are mostly long-lived.
- **Acceptance:** Repeated create/destroy followed by unlockWaitingEnabledServices performs no stale access and inventory/budget return to baseline.

### GUI / menus

#### INT-03 — Release rebuilt menu trees with explicit ownership

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/APPManager.h:3570](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L3570) — refreshBankMenu deletes MenuSystem and allocates new MenuItems; buildLangMenu repeats the pattern. Bundled Menu/MenuSystem declare no destructors freeing their realloc-backed component array or allocated children. Rebuilds lose those allocations.
- **Requested change:** Fix existing menu ownership and teardown; distinguish owned children from borrowed/shared entries such as back_menu_item. Preserve local library adaptations; do not blindly delete every child.
- **Acceptance:** Repeat bank/language menu rebuilds at least 500 times; no increasing live allocations, dangling labels or double-free of shared menu items.

#### BUG-GUI-01 — Preserve menu contents when realloc fails

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/arduino-menusystem/MenuSystem.cpp:152](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/arduino-menusystem/MenuSystem.cpp#L152) — Menu::add_component overwrites _menu_components with realloc's return. On failure the old array is lost while component count remains nonzero; the caller's new item is also not accepted explicitly.
- **Requested change:** Make growth transactional and report insertion failure with a clear ownership contract; coordinate with INT-03.
- **Acceptance:** Injected allocation failure leaves existing menu navigable and recoverable, with no lost array or orphaned newly created item.

### Widgets / rendering

#### BUG-WDG-01 — Handle sprite allocation failures consistently

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/GUIComponents.h:345](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/GUIComponents.h#L345) — Widget::createSprite does not check createSprite's pixel-buffer result before recording dimensions; CompositeWidget::refresh (1695) dereferences a child's sprite without checking it. GUIManager rotation buffers are also unchecked (GUIManager.h:244).
- **Requested change:** Use the existing Widget allocation/cleanup path to report failure and skip rendering safely; guard child and rotated buffers, preserving retry behavior.
- **Acceptance:** Force pixel-buffer allocation failures at initial creation and resize; no invalid access or blank-success state; rendering recovers after memory becomes available.

### Components / background tasks

#### BUG-CMP-01 — Release task parameters when task creation fails

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/HWComponents.h:2108](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/HWComponents.h#L2108) — asyncPing allocates PingParameters but ignores xTaskCreate's result; only the worker frees params. checkAndInstallBinaryFromFile (Interval.h:2306–2328) similarly hands allocated File/StreamParams to a task without checking creation.
- **Requested change:** Retain caller ownership until task creation succeeds; on failure close/free all owned objects and restore flags/UI state.
- **Acceptance:** Inject task creation failure into both paths repeatedly; no leaked parameters/files, stuck update state or incorrect transfer of ownership.

### Components / HTTP connection

#### BUG-CMP-02 — Restore connection state on task creation failure

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/HWComponents.h:2144](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/HWComponents.h#L2144) — asyncConnect sets isClientConnecting=true and returns true without checking xTaskCreatePinnedToCore. Failure leaves a pending state with no worker to clear it.
- **Requested change:** Check task creation, roll back pending state and report failure through the existing API; avoid adding a parallel connection state machine.
- **Acceptance:** Injected failure returns failure, permits a later retry and leaves client ownership and connection flags consistent.

### Strips / serialization

#### BUG-STR-01 — Correct strip property formatting and bounded restoration

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/Strips.h:848](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Strips.h#L848) — OneSelectStrip/TwoSelectStrip pass sizeof(buf) to snprintf although buf is a pointer, truncating serialized text. Their %d scans write through uint8_t* select IDs, although %d expects int*; text scans are unbounded.
- **Requested change:** Use explicit buffer capacities, scan numeric IDs into correctly typed temporaries, validate ranges, and bound string extraction through existing property handling.
- **Acceptance:** Round-trip long valid text and minimum/maximum IDs; malformed/oversized input cannot overwrite neighboring members or truncate silently.

### Strips / MIDI and OSC

#### BUG-STR-02 — Make message-library strip parsing and formatting memory safe

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/strips/s_midi.h:349](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/strips/s_midi.h#L349) — SysexStrip::serializeProperties passes an Arduino String object to a %s vararg instead of c_str(); NetworkOSC serialization has a similar String argument (i_osc.h:271). SysEx/OSC key restoration uses unbounded %s into fixed arrays (s_midi.h:359; s_osc.h:83); TextStrip parsing is also unbounded (s_utils.h:36).
- **Requested change:** Replace unsafe varargs and bound all affected extraction paths, reusing existing serialization/property facilities.
- **Acceptance:** Warnings/format checks where applicable plus oversized key/text and valid library round trips produce no invalid memory access.

### Mixers / strips

#### MEM-01 — Fix mixer-strip reservation ownership and null bindings

- **Status:** To do. **Priority:** P1.
- **Evidence:** [src/mixers/s_mixer.h:110](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/src/mixers/s_mixer.h#L110) — SendStrip reservation pointers are uninitialized before first unreserve; its destructor is empty and does not release reservations. Reserve may return null but is dereferenced immediately. Focus/visibility callbacks also dereference boundInterface without guarding it (42–47).
- **Requested change:** Initialize pointers; pair acquisitions/releases across property changes, destruction and interface replacement; handle missing mixer and allocation failure transactionally, preserving shared-value pointers.
- **Acceptance:** Repeated strip create/edit/delete and interface switches return live reservation counts to baseline; same-key strips stay coherent; null/OOM/focus paths do not crash.

### Mixers / lifecycle

#### BUG-MIX-01 — Remove stale bindings and release owned mixer metadata on teardown

- **Status:** To do. **Priority:** P2.
- **Evidence:** [src/mixers/mixer.h:182](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/src/mixers/mixer.h#L182) — MixerBase stores six new DataSetSynced objects via MixerDevice and holds a static binding registry, but its destructor is empty and there is no unbind. Concrete mixers allocate mixerDef without a teardown owner. Destruction leaks metadata and leaves binding pointers stale; normal startup mixers are long-lived.
- **Requested change:** Establish teardown ownership for datasets/definitions and remove bindings, coordinating with generic dataset/template registries and surviving strips.
- **Acceptance:** Repeated creation/destruction leaves no live owned datasets/definitions or bindings; lookup of a deleted interface returns no mixer and performs no stale access.

### Mixers / configuration

#### BUG-MIX-02 — Bound restored mixer model names

- **Status:** To do. **Priority:** P1.
- **Evidence:** [src/mixers/m_xmseries.h:198](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/src/mixers/m_xmseries.h#L198) — XMSeries::restoreProperty reads /model/%s into char model[64] with no width restriction.
- **Requested change:** Bound model extraction and validate full input against the supported model names before modifying state.
- **Acceptance:** A model name over 63 characters is rejected without stack corruption; valid model selection and reload remain compatible.

### Data / repositories

#### BUG-DATA-01 — Remove destroyed parameter objects from the template registry

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h:287](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/DataManagement.h#L287) — SerializableParameters registers this in jsonInputRepo; its destructor deletes owned datasets but never removes itself from jsonTemplateRepo, whose serialization dereferences every stored pointer.
- **Requested change:** Add symmetric registry removal in the existing repository/destructor paths, respecting destruction order.
- **Acceptance:** Create/destroy parameter providers then serialize templates repeatedly: no stale entries, registry growth or freed-object reads.

### Data / polymorphic ownership

#### BUG-DATA-02 — Make base-pointer dataset destruction valid

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h:109](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/DataManagement.h#L109) — DataSet has virtual methods but a non-virtual destructor. SerializableParameters deletes derived ConfigDataSet instances through DataSet*, which is undefined behavior even if current derived cleanup is simple.
- **Requested change:** Make the existing owning base-pointer contract safe and audit related ownership without deleting borrowed Address objects.
- **Acceptance:** Destroy each owned dataset through its actual ownership path; derived cleanup and repository removal each run once, without undefined behavior.

#### BUG-DATA-03 — Make DataProvider polymorphic destruction safe

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DataProvider declares virtual methods but no virtual destructor; deleting a derived provider through DataProvider* would be undefined behavior. Such deletion has not been demonstrated in active callers.
- **Requested change:** Add a virtual default destructor and audit the actual creation/deletion paths without transferring ownership to DynamicDataSet.
- **Acceptance:** Derived providers can be destroyed through their documented owning path with exactly one derived/base cleanup; dev/prod builds remain compatible.

#### BUG-DATA-04 — Enforce DynamicDataSet provider lifetime

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DynamicDataSet retains a non-owning DataProvider*; null checks do not protect against a destroyed provider. The constructor also dereferences dp to derive the dataset name without validating it.
- **Requested change:** Specify and enforce provider-before-dataset lifetime or explicit deregistration/invalidation within the existing classes; ensure constructor prerequisites are checked or made impossible to violate. Avoid adding shared ownership unless a real caller requires it.
- **Acceptance:** Create/destroy providers and associated datasets in supported orders, then serialize or query the repository: no use-after-free, invalid constructor dereference or stale registration; ownership remains unambiguous.

#### BUG-DATA-05 — Guard repository lookups and dataset access

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DataRepository::getDataSet(int) permits negative indices, lookups dereference ds->address without checking, DynamicDataSet::getSize() dereferences address, and SerializableParameters::getValuePtr() assumes repo is valid.
- **Requested change:** Validate indices and nullable objects at access boundaries, or formally enforce non-null construction invariants; preserve existing lookup behavior for valid registrations.
- **Acceptance:** Negative/out-of-range lookup, missing address/repository, and unavailable provider fail safely without invalid memory accesses; normal dataset serialization still works.

#### BUG-DATA-06 — Use size-safe iteration for growing registries

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DataRepository lookup/serialization and JsonTemplateRepository serialization use uint8_t counters while traversing std::vector-backed registries. With more than 255 entries, the counter may wrap and fail to terminate.
- **Requested change:** Use size_t or the container's size type for all vector iterations; check index conversions at interfaces that genuinely require narrower types.
- **Acceptance:** Synthetic registries containing 0, 255, 256 and more entries are traversed exactly once per entry, with no wraparound or hanging.

### Webapp / firmware HTTP server

#### BUG-WEB-01 — Match the HTTP file-buffer allocator and deallocator

- **Status:** To do. **Priority:** P1.
- **Evidence:** [lib/Interval/src/APPManager.h:2536](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L2536) — sendFileInChunks allocates byte[] using new(std::nothrow) but frees it with free(); it also reads into the buffer without checking a null allocation.
- **Requested change:** Use matching deallocation or existing scoped ownership, check allocation and read/write errors, and preserve tested Ethernet chunk behavior.
- **Acceptance:** Repeated file downloads and injected OOM produce no allocator mismatch, invalid writes or leaked open file; content remains correct over Wi-Fi/Ethernet.

### Webapp / firmware upload

#### BUG-WEB-02 — Avoid leaking StreamParams on ordinary uploads

- **Status:** To do. **Priority:** P2.
- **Evidence:** [lib/Interval/src/APPManager.h:2438](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L2438) — upload handler allocates StreamParams before dispatch. TGZ and firmware consumers delete it; the ordinary streamToFile branch does not use or delete it, leaking one object per ordinary upload.
- **Requested change:** Allocate parameters only in consuming branches or use scoped caller ownership; never delete the borrowed HTTP request stream.
- **Acceptance:** Repeated ordinary uploads return allocation counts to baseline; TGZ/firmware paths retain exactly one owner and no double deletion.

### Webapp / browser

#### BUG-WEB-03 — Bound the browser console log

- **Status:** To do. **Priority:** P2.
- **Evidence:** [data/webapp/index.html.gz](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/data/webapp/index.html.gz) — Decoded logWebConsole appends every message to consoleLog.value without any retention limit; _l logs routine socket/JSON traffic as well. Long sessions accumulate an unbounded string in browser memory.
- **Requested change:** Add a maximum line/character budget and trim oldest entries; preserve useful diagnostics and distinguish browser RAM from ESP32 heap.
- **Acceptance:** A long synthetic logging session stays within the configured log budget and retains the newest messages without degrading interaction.

## Improvements

These are optimization or design assessments, not demonstrated memory savings or blanket rewrite instructions. Profile the relevant workload first; retain current behavior unless a measured or clearly reasoned improvement justifies changing it. Reuse existing facilities and files. The MEM-03 migration remains incremental and must not invalidate shared pointers.

### Core / memory instrumentation

#### IMP-CORE-01 — Establish a repeatable memory baseline

- **Status:** To assess.
- **Observation:** [lib/Interval/src/APPManager.h:1864](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L1864) — Existing heap display reports heap size but does not establish repeatable per-operation allocation trends.
- **Scope and acceptance:** Extend existing diagnostics with free/minimum heap, largest free block, PSRAM where present and task stack high-water marks; compare after warm-up across edits, reconnects, menus and uploads. Reuse diagnostics rather than adding a separate service.

### Core / buffers

#### IMP-CORE-02 — Make ByteBuffer and dispatcher ownership explicit

- **Status:** To assess.
- **Observation:** [lib/Interval/src/Interval.h:818](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Interval.h#L818) — ByteBuffer and EventDispatcher own raw allocations but do not explicitly prohibit copying; no active unsafe copy was established in this review.
- **Scope and acceptance:** After BUG-CORE-01, delete unsupported copies or implement appropriate move semantics in existing classes; inspect callers and compile both environments. Avoid presenting potential copies as an observed runtime leak.

### Interfaces / OSC queue

#### IMP-IF-01 — Bound queued OSC memory and reduce per-message allocations

- **Status:** To assess.
- **Observation:** [lib/Interval/src/interfaces/i_osc.h:87](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/interfaces/i_osc.h#L87) — enqueue allocates a packet plus a linked node; no message/byte capacity limit exists. Dequeue is latency-throttled, so producer bursts can outpace it.
- **Scope and acceptance:** Define queue message/byte budgets and observable backpressure/drop/coalescing semantics preserving action order; measure heap and latency before considering a pool. Burst and disconnected tests must stay within budget.

### GUI / render loop

#### IMP-GUI-01 — Reuse sprites and rotation scratch storage where beneficial

- **Status:** To assess.
- **Observation:** [lib/Interval/src/GUIManager.h:217](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/GUIManager.h#L217) — GUIManager deletes a rendered widget sprite each pass; rotation allocates a temporary sprite. FromToWidget and composites also discard sprites, so base caching alone cannot remove allocation churn.
- **Scope and acceptance:** Coordinate retention/eviction in existing GUI and Widget classes; measure peak RAM, largest block and frame time. Retaining every sprite is not automatically better; cap any cache and preserve visual output.

### GUI / menus

#### INT-02 — Measure memory after menu ownership fixes

- **Status:** To assess.
- **Observation:** [lib/Interval/src/APPManager.h:1760](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L1760) — Existing request to measure MenuSystem deletion is retained and linked to concrete ownership defects INT-03 / BUG-GUI-01.
- **Scope and acceptance:** After those fixes, stress language/bank menu rebuilds, name changes and navigation; verify stable live allocations and largest block after warm-up, with no borrowed-label lifetime regressions.

### Widgets / composition

#### IMP-WDG-01 — Reduce temporary child allocations during widget drawing

- **Status:** To assess.
- **Observation:** [lib/Interval/src/GUIComponents.h:1286](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/GUIComponents.h#L1286) — RingMeter/TextArea/Toggle paths construct and destroy temporary Label/NumericLabel objects during rendering.
- **Scope and acceptance:** Profile these paths, then reuse member/stack render helpers where lifetimes allow; use existing drawing APIs and measure memory/frame-time benefit before broad changes.

### Components / network and tasks

#### IMP-CMP-01 — Bound task concurrency and verify cancellation ownership

- **Status:** To assess.
- **Observation:** [lib/Interval/src/HWComponents.h:1821](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/HWComponents.h#L1821) — Network paths create 4/8 KiB task stacks and may forcibly delete socket-open workers. Discovery also creates tasks; static reading alone does not prove cancellation leaks inside dependencies.
- **Scope and acceptance:** Measure stack high-water marks and concurrent task count; prefer existing task reuse/cooperative stop where justified. Stress Wi-Fi/Ethernet switching and teardown before resizing stacks or changing ownership.

### Strips / events

#### IMP-STR-01 — Avoid unnecessary event JSON deep copies

- **Status:** To assess.
- **Observation:** [lib/Interval/src/Strips.h:264](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/Strips.h#L264) — Strip::onEvent copies the event JsonDocument before processAction, multiplying allocations for every listener.
- **Scope and acceptance:** Review handler mutation/lifetime contracts, then reuse a borrowed const view where safe or copy only when needed; no retained view beyond event lifetime. Verify identical multi-strip behavior and compare allocation counts.

### Mixers / synchronized metadata

#### IMP-MIX-01 — Size synchronized name storage against actual needs

- **Status:** To assess.
- **Observation:** [src/mixers/mixer.h:277](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/src/mixers/mixer.h#L277) — MixerDevice eagerly creates six named datasets with compile-time maxima for every backend, even before model detection.
- **Scope and acceptance:** Measure fixed array/string and populated-name costs; inspect DataSetSynced storage and pointer users before lazy/model-sized allocation. Preserve name pointer stability, discovery and synchronization semantics.

### Data / descriptors

#### IMP-DATA-01 — Clarify Address ownership and permanent registry budgets

- **Status:** To assess.
- **Observation:** [lib/Interval/src/DataManagement.h:302](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/DataManagement.h#L302) — Numerous datasets receive new Address objects while others borrow global addresses; ownership is not explicit. Many are startup-lifetime allocations, not proven recurring leaks.
- **Scope and acceptance:** Document owned versus borrowed descriptors in existing classes, avoid duplicate descriptors and consider member/static storage where appropriate; enable teardown only with a valid ownership model and virtual destruction where needed.

#### IMP-DATA-02 — Reduce DataProvider API String copies

- **Status:** To assess.
- **Observation:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DataProvider::getSize/getValuePtr and DataRepository::getDataSet accept String arguments by value, potentially copying address/provider strings on repeated lookups.
- **Scope and acceptance:** Inventory overrides and callers, then consider const String& inputs consistently where it reduces heap churn without changing the public data contract. Measure allocation count/latency for repeated GUI/strip lookups and compile dev/prod.

#### IMP-DATA-03 — Clarify empty datasets and returned-pointer stability

- **Status:** To assess.
- **Observation:** [lib/Interval/src/DataManagement.h](../lib/Interval/src/DataManagement.h) — DataSet::serializeContent() treats getSize()==0 as a request for getValuePtr(0) and serializes a scalar, conflating an empty list with a scalar dataset. getValuePtr returns a borrowed const String* whose validity during mutation/reallocation is not specified.
- **Scope and acceptance:** Inventory existing WebApp, ConfigDataSet, synchronized-mixer and strip consumers before changing JSON shape. Document or minimally refine scalar/collection semantics and pointer validity, including std::vector reallocation and concurrent refresh; preserve existing persisted/wire formats or provide explicit compatibility tests. Validate zero/one/multiple entries and mutation during consumption.

### Webapp / firmware responses

#### IMP-WEB-01 — Reduce response serialization and compression peak RAM

- **Status:** To assess.
- **Observation:** [lib/Interval/src/APPManager.h:3412](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/lib/Interval/src/APPManager.h#L3412) — httpPost builds a JSON String and a compressed ByteBuffer concurrently; several serialization paths use additional JsonDocuments/Strings.
- **Scope and acceptance:** Profile maximum configuration/library responses, then reuse existing streaming/serialization facilities or bounded scratch space where compatible with gzip and Ethernet constraints; validate wire-format compatibility and peak heap.

### Webapp / browser queues and lifecycle

#### IMP-WEB-02 — Bound browser queues and verify listener/timer cleanup

- **Status:** To assess.
- **Observation:** [data/webapp/index.html.gz](https://github.com/infrafast/esp32/blob/679bafe8a8461f34d67096db16b2685835ae7c67/data/webapp/index.html.gz) — Decoded code pushes into messageQueue/httpQueue/jsonCallbackQueue and retries HTTP work; repeated bank loading attaches an anonymous dragend listener to .draggable, while shutdown clears several but not every resource.
- **Scope and acceptance:** Assess queue budgets/backpressure, offline bursts and retained DOM references with browser heap snapshots. Make repeated initialization idempotent and disposal explicit where profiling confirms retention; do not assume detached-node listeners alone are leaks.

### Collections / staged migration

#### MEM-03 — progressive standard-container migration

Origin: user request, October 24, 2026. Objective: favor the standard C++ `std::vector<T>` over `ivanseidel/LinkedList` when it improves RAM efficiency, predictable allocation behavior, robustness, and maintainability. This is **not** a blanket replacement mandate.

1. Inventory all `LinkedList` instances (including indirect wrappers), document element type, ownership, mutation and access patterns, current peak sizes, and any stored pointers/references/iterators.
2. Prioritize straightforward indexed/append-oriented collections. Before each change, evaluate whether `std::vector` with an appropriate `reserve()` is compatible with required element and pointer stability; use fixed-capacity storage for allocation-sensitive real-time paths. Reserve only when justified by an established bound or measurement. Check allocation failure behavior and capacity enforcement.
3. Migrate in small, independently verifiable groups while preserving public behavior, serialization/configuration format, and existing framework/application interfaces. Prefer adapting existing abstractions over introducing new files or redundant wrappers.
4. Preserve architecture: reusable, domain-neutral collection code belongs under `lib/Interval`; mixer-specific semantics and bindings remain in `src/mixers`. Keep `Interval.h` glue minimal and justified. Do not move mixer logic into the generic framework.
5. Specifically assess `ValueList<T>` in Interval: its reservation counters, sharing and node/pointer lifetime contract may require stable addresses, so do **not** directly substitute `std::vector` until these invariants are preserved or intentionally redesigned with tests.
6. For each migration batch, include affected files, memory/heap and fragmentation observations where possible, ownership/lifetime checks, regression tests, dev/prod builds, and appropriate hardware validation in the commit or pull-request description. Keep only remaining scope and pending checks in this roadmap. Remove the `LinkedList` dependency only after confirming there are no remaining required uses.

Acceptance: no behavioral regressions, no dangling references after container changes, measured or reasoned memory benefit, no unjustified runtime allocation in critical paths, all relevant tests passing, and roadmap/architecture documentation updated. Track each batch's remaining scope separately and remove completed batches; remove MEM-03 when the migration and required validation are complete.


## Generated class documentation — pending validation

| ID | Work | Status | Acceptance criteria |
|---|---|---|---|
| DOC-01 | Validate Doxygen/Graphviz generation and optional web publication | To validate | Confirm automatic site refresh after a subsequent source change, including updated Strip, Bank and DataProvider descriptions. |

Origin: user request, October 25, 2026. Existing code declarations/comments are reused; dedicated Doxygen, workflow and VS Code configuration files supply generation and execution without changing firmware or adding a parallel UML model. Remaining validation covers the deployed site refresh after a source change. Generation, SVG presence and Pages deployment have passed Actions; no hardware validation is implied.

## Other features and pending validation

| ID | Work | Status | Acceptance criteria |
|---|---|---|---|
| MIX-01 | Complete the input-gain strip | To do | Connect reads/writes to the mixer contract, bound and convert values, handle responses; check supported backends and persistence |
| OSC-01 | Assess and complete the generic OSC strip | To assess | Check message library, parsing, serialization, and reception; validate a send/receive scenario |
| MEM-02 | Validate the extracted ValueList<T> in complete ESP32 firmware builds | To validate | Compile dev and prod against the full Arduino header graph after the extraction; the portable host checks do not cover firmware integration, and no complete ESP32 build has been recorded |
| HW-01 | Compare both TFT/W5500 initialization orders | To validate | dev/prod tests, cold starts and wake-ups, Ethernet and color results recorded; decision documented before generalization |

These items derive from code reviewed for consolidation; they are not an exhaustive firmware audit.


## Other Interval library requests to assess

Origin: unchecked requests from the former `lib/Interval/README.md`. Assess each against current code; remove requests already implemented and validated.

| ID | Preserved request | Status | Expected validation |
|---|---|---|---|
| INT-01 | Add more templates with examples | To assess | Identify missing templates; integrate examples into authorized files or the relevant code |
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

## Other maintenance proposals

These items preserve risks and opportunities from `ANALYSE_REPO.md`.

| ID | Proposal | Status | Acceptance criteria |
|---|---|---|---|
| TOOL-01 | Parameterize script source/destination paths | Done | Source defaults to sibling `../webGUI`; packaging defaults to sibling `../fwupdate`, with environment overrides |
| TOOL-02 | Separate packaging from deployment moves | To assess | Deterministic local build, explicit and optional external side effects |
| TOOL-03 | Provide local or optional minification | To assess | Asset preparation possible without an external HTTP service; controlled failure |
| TOOL-04 | Avoid workflow execution on simple script import | To assess | Import without unintended deletion, copying, or network calls |
| DEP-01 | Inventory local patches and dependency references | To assess | Adaptation origin/version/rationale described in architecture; reproducible updates |
| CI-01 | Automatically check dev and prod builds | To assess | Both environments compiled, packaging side effects controlled; no new workflow file in this documentation task |

## PCB 2.0 / 2.2 — pending tests and design proposals

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

The protocol below is preserved as a historical proposal. The experimental
TFT-first initialization has been removed; current firmware uses the
historical components-before-TFT order. Do not combine a hardware change and
a firmware change within the same test.

### Priority tests with a fixed firmware version

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

### Proposed corrections for a future PCB compatible with the code

1. **Restore a direct ground return to U6 first.** Widely connect the common power/XIAO-area ground to lower-right U6 GND as in 2.0, then provide a continuous ground plane stitched with vias to the peripherals. Keep all three U6 GND pins connected to the plane and provide a TFT return close to SPI. Stop depending on a single mechanical slot for peripheral ground return. Ideally, jumper A-B should confirm this hypothesis before manufacturing a new board.

2. **Provide local decoupling.** Place footprints near module supply inputs with short GND connections: typically 100 nF and a few µF. Determine fitted values after identifying the modules, power switch, and their stability and startup-current constraints. Randomly adding a large capacitor can also degrade rail rise.

3. **Define startup CS states.** Provide a pull-up, typically 10 kΩ, on each CS to the **switched peripheral 3.3 V rail**, subject to resistors already present on the modules. This preserves the active-LOW selections expected by the code. Reset also needs a defined idle level; check its existing bias before adding a resistor. Avoid pulling an unpowered peripheral toward a permanent rail.

4. **Provide SPI damping.** Add series-resistor footprints on SCLK and MOSI near the XIAO. Try, for example, 22–47 Ω, adjusted from observed waveforms; this is not a value guaranteed by the Gerbers. For MISO, the source is W5500. Footprints on CS/DC are useful if measurements reveal disturbed edges. Placement near the source follows the series-termination principles described in the [TI TIDA-060045 SPI design guide](https://www.ti.com/lit/pdf/tiduf78).

5. **Limit unused branches toward U7.** Alternative U6/U7 footprints create bus branches. Provide selectable connections or 0 Ω resistors to isolate the unpopulated footprint without changing firmware GPIOs. On this board, retain U6 and the display. Keep a ground reference close to signals.

6. **Address shared reset separately if captures justify it.** Under historical initialization order, firmware physically resets W5500 during TFT initialization after the first Ethernet initialization. A separate W5500 power-on reset circuit can avoid this, but requires proper validation of its delay, rail ramp, and Ethernet procedure after ESP32 reset. Do not simply add a large capacitor on GPIO44 or drive a supervisor against the XIAO push-pull output. This remains a design option, not a proven fix for the regression.

Priorities 1–5 preserve the audited pinout, polarities, and firmware. Their effectiveness against the fault must be validated by the tests above; priority 6 requires further study.
