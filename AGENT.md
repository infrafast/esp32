# Agent Instructions

## Authorized reference documents

Read before making changes:

1. `docs/architecture.md`: architecture, patterns, and extension procedures.
2. `docs/roadmap.md`: requests, progress, audits, and validation.
3. `AGENT.md`: these instructions.

`README.md` is exclusively the high-level user-facing overview. The three files above are the project's only working references; the README is their user-facing entry point.

## Mandatory documentation discipline

- Write and maintain all project documentation in English, including additions, edits, headings, tables, audit findings, and validation records. Preserve code identifiers, paths, filenames, URLs, and literal protocol values. This rule does not require translating upstream dependency documentation or user-interface localization resources.
- Do not create any other content documentation file, regardless of extension: no additional README, separate audit report, plan, guide, project changelog, or parallel instruction file.
- Update these existing files at their exact paths and with their exact casing. Keep the requested singular name `AGENT.md`; do not create a second `AGENTS.md`.
- Place information according to its purpose: user overview in README; operation and stable architectural decisions in architecture; requests, proposals, tests, and dated results in roadmap; agent rules here.
- Do not duplicate a complete explanation: link to its reference.
- Subfolders of `docs/` are only for attachments: images, diagrams, captures, and measurement data. Their explanatory content remains in architecture or roadmap. A diagram may be an attachment; a PDF report must not bypass this rule.
- Preserve the existing PCB reproduction script as supporting evidence; do not turn it into a new source of instructions or a report.
- Do not modify or delete documentation shipped with third-party dependencies to enforce this rule. It remains upstream documentation and does not replace the project references.
- This restriction concerns new documentation. Source, configuration, and test files required for a feature remain permitted when authorized by the task. Documentation-only tasks must not change code.

## Mandatory documentation for every code change

- Systematically document every code change, including bug fixes, refactoring, build/configuration changes, and tooling changes, in the appropriate existing reference files as part of the same change.
- Always update `docs/roadmap.md` with the request or existing tracking ID, what changed and why, current status, validation performed, and any remaining limitations. Record the relevant commit reference when available.
- Update `docs/architecture.md` whenever a change affects implementation structure, responsibilities, contracts, data flow, design patterns, extension procedures, configuration, build/update behavior, or an architectural decision.
- Update `README.md` whenever a change affects user-visible capabilities, supported hardware, operation, or limitations. Keep it high level.
- Update `AGENT.md` whenever contribution rules or agent obligations change.
- A code change is not complete until the relevant documentation is current. Do not substitute a separate report, code comments alone, or a commit message for these updates. If a reference does not need an update, do not add redundant content merely to touch the file.

## Code comments for human maintainers

- Comment generated or modified code in English so a developer can understand what it does and why the approach was chosen.
- Use concise header comments for classes, functions, or logical sections where they help explain purpose, responsibilities, or important constraints. Add brief inline comments only where a non-obvious step, workaround, assumption, or design decision needs explanation.
- Keep comments proportional to complexity: usually a few lines in a header and occasional inline notes. Avoid long narratives, line-by-line explanations, and comments that merely repeat obvious code.
- Emphasize rationale and maintenance-relevant details, such as ownership, units, protocol behavior, timing constraints, and trade-offs, when applicable.
- Keep comments accurate when changing code. Prefer clear names and simple structure over compensating for unclear code with excessive comments. Code comments complement, but do not replace, the required updates to the project reference documents.

## Before and during a change

- Check current code and effective options; an old report does not prove that the described behavior still exists.
- Register the request in roadmap with an ID, origin, status, and validation criteria. Distinguish user requirements, technical proposals, hypotheses, and findings.
- Identify whether the change belongs to the Interval framework, an interface, a strip, a mixer backend, or build tools.
- Keep `setup()` declarative and routine `app.refresh()` processing non-blocking.
- Follow the `DeviceInterface`, `MixerBase`, and `MixerDevice<Transport>` contracts, skeletons, and datasets described in architecture. Do not bypass the mixer abstraction by casting to a particular backend.
- Preserve persistent identifiers, serialization, and index conversions. Document any required migration.
- Pair event subscriptions and value reservations with their corresponding unsubscription/release; check null pointers, selection changes, interface replacement, and destruction.
- Use bounded buffers and check sizes, allocations, and lifetimes. Do not start a global container migration without a request and measurements.
- Respect local dependency patches; do not replace a bundled library without examining their rationale.
- Treat `platformio.ini` as the environment reference. In particular, check USB differences and TFT/W5500 initialization order.
- Inspect script effects before execution: packaging, moves, deletions, and network calls. Tooling changes must make side effects explicit and avoid mandatory machine-specific paths.

## Verification and completion

- Choose checks appropriate to the change. For firmware/build changes, check the affected environments; for scripts, inspect effects from a clean workspace; for mixer/strip changes, check synchronization, shared values, and reconnection.
- For documentation-only changes, check links, paths, consistency, and information preservation; do not claim to have compiled or tested hardware.
- Do not copy an incomplete strip as though it were a validated implementation. Consult open roadmap items.
- Record results, limitations, and commit references in roadmap. Update architecture when operation or a decision changes, and README when user-facing behavior changes.
- Before delivery, audit the changed-file list: no new documentation outside the four authorized files, no explanatory content moved into subfolders, and no code changes outside the requested scope. Check that all code changes have corresponding documentation updates and that documentation is in English.
