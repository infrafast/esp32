# Mixer Application Roadmap

This roadmap contains only open work specific to the product application, mixer backends, mixer strips, and their
integration. Framework-wide work (hardware lifecycle, GUI, interfaces, persistence, memory, dependencies and
diagnostics) belongs in the [Interval framework
roadmap](https://github.com/infrafast/Interval/blob/main/docs/roadmap.md).

Completed work is kept in Git history rather than recorded here. Each item below remains open until implementation and
the stated validation are complete.

## Mixer strips and ownership

### MEM-01 — Fix mixer-strip reservation ownership and null bindings

- **Status:** To do. **Priority:** P1.
- Initialize reservation pointers before release, pair every acquisition with
release during selection changes and destruction, and handle missing mixer or allocation failure transactionally.
- Validate repeated strip creation, editing, deletion and interface switching;
shared values must return to their baseline reservation count.

## Mixer lifecycle

### BUG-MIX-01 — Remove stale bindings and release mixer metadata

- **Status:** To do. **Priority:** P2.
- Define teardown for mixer metadata, static bindings and concrete mixer-owned
datasets so destroyed mixers cannot leave stale references.
- Validate repeated create/destroy cycles and reconnection without dangling
bindings or increasing allocations.

## Mixer configuration

### BUG-MIX-02 — Bound restored mixer model names

- **Status:** To do. **Priority:** P1.
- Bound model-name restoration and reject oversized or invalid values without
changing the existing configuration format.
- Validate all supported model names, malformed input and persistence reload.

### IMP-MIX-01 — Size synchronized name storage to actual mixer needs

- **Status:** To assess.
- Measure synchronized channel/bus/effect name lengths for supported mixer
models, then reduce avoidable storage and String copies without truncation.
- Acceptance requires successful synchronization and unchanged strip labels for
every supported model.

## Acceptance

An item is complete only after the mixer behavior, documentation and relevant dev/prod validation are complete.
Framework changes must be redirected to Interval and released through its versioned dependency before being consumed
here.
