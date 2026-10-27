# Agent Instructions

## Authorized reference documents

Read before making changes:

1. `docs/architecture.md`: architecture, patterns, and extension procedures.
2. `docs/roadmap.md`: open requests, remaining work, and pending validation.
3. `AGENT.md`: these instructions.

`README.md` is exclusively the high-level user-facing overview. The three files above are the project's only working references; the README is their user-facing entry point.

## Mandatory documentation discipline

- Write and maintain all project documentation in English, including additions, edits, headings, tables, audit findings, and validation records. Preserve code identifiers, paths, filenames, URLs, and literal protocol values. This rule does not require translating upstream dependency documentation or user-interface localization resources.
- Do not create any other content documentation file, regardless of extension: no additional README, separate audit report, plan, guide, project changelog, or parallel instruction file.
- Update these existing files at their exact paths and with their exact casing. Keep the requested singular name `AGENT.md`; do not create a second `AGENTS.md`.
- Place information according to its purpose: user overview in README; operation and stable architectural decisions in architecture; open requests, proposals, and pending tests in roadmap; agent rules here.
- Do not duplicate a complete explanation: link to its reference.
- Subfolders of `docs/` are only for attachments: images, diagrams, captures, and measurement data. Their explanatory content remains in architecture or roadmap. A diagram may be an attachment; a PDF report must not bypass this rule.
- Preserve the existing PCB reproduction script as supporting evidence; do not turn it into a new source of instructions or a report.
- Do not modify or delete documentation shipped with third-party dependencies to enforce this rule. It remains upstream documentation and does not replace the project references.
- This restriction concerns new documentation. Source, configuration, and test files required for a feature remain permitted when authorized by the task. Documentation-only tasks must not change code.

## Mandatory documentation for every code change

- Systematically document every code change, including bug fixes, refactoring, build/configuration changes, and tooling changes, in the appropriate existing reference files as part of the same change.
- Keep `docs/roadmap.md` current for every code change: track only outstanding scope, current status, acceptance criteria, and remaining validation or limitations. Remove completed work as part of the same change rather than appending a completion record. Explain what changed, why, and validation performed in the commit or pull request; keep lasting implementation details in architecture.
- Update `docs/architecture.md` whenever a change affects implementation structure, responsibilities, contracts, data flow, design patterns, extension procedures, configuration, build/update behavior, or an architectural decision.
- Update `README.md` whenever a change affects user-visible capabilities, supported hardware, operation, or limitations. Keep it high level.
- Update `AGENT.md` whenever contribution rules or agent obligations change.
- A code change is not complete until the relevant documentation is current. Do not substitute a separate report, code comments alone, or a commit message for these updates. If a reference does not need an update, do not add redundant content merely to touch the file.

## Keep the roadmap focused on future work

- `docs/roadmap.md` contains only desired future changes, work in progress, unresolved issues, and pending validation. It is not a changelog, archive, audit history, or completion journal.
- Once a request is implemented and its required validation is complete, remove its entry and associated plan, notes, and completed subtasks immediately. Also remove abandoned or superseded requests instead of retaining closed statuses.
- If implementation is complete but required validation remains, replace the implementation request with a concise pending-validation item describing only the outstanding checks and acceptance criteria. Never delete unresolved work merely because implementation is marked done.
- After a partial delivery, retain only the remaining scope. Keep stable IDs for surviving requests and avoid leaving references to deleted entries.
- Keep permanent agent rules in AGENT.md and current behavior/design in architecture; do not duplicate them as roadmap tasks. Preserve useful technical evidence in its existing architectural context when necessary, not as a growing completion archive.
- Use Git history and commit/pull-request descriptions for completed changes and validation evidence. Do not create another history document or move deleted roadmap content into an archive.
- Before every delivery, prune completed and obsolete items throughout the roadmap, including tables, narrative sections, and logs. Do not add a completed entry to record this pruning itself.

## Code comments for human maintainers

- Comment generated or modified code in English so a developer can understand what it does and why the approach was chosen.
- Use concise header comments for classes, functions, or logical sections where they help explain purpose, responsibilities, or important constraints. Add brief inline comments only where a non-obvious step, workaround, assumption, or design decision needs explanation.
- Keep comments proportional to complexity: usually a few lines in a header and occasional inline notes. Avoid long narratives, line-by-line explanations, and comments that merely repeat obvious code.
- Emphasize rationale and maintenance-relevant details, such as ownership, units, protocol behavior, timing constraints, and trade-offs, when applicable.
- Keep comments accurate when changing code. Prefer clear names and simple structure over compensating for unclear code with excessive comments. Code comments complement, but do not replace, the required updates to the project reference documents.


## Doxygen comments and generated diagrams

- Use Doxygen-compatible comments for file, class, and function/API documentation added or revised in project-owned C++ code: `/** ... */` or `///`, with a concise `@brief`. Use `@file` for file headers and `///<` for trailing member documentation when appropriate. Ordinary implementation notes may remain `//` comments.
- Keep the language simple, practical and concise. Explain purpose first and include a concrete use example when useful. Give more detail for domain behavior and non-obvious algorithms: persistence formats and limits, updates, screen/subscreen selection, state-change detection, important events and ownership. Add `@param`, `@return`, `@note` or `@warning` only when they explain meaningful contracts; avoid boilerplate tags.
- Keep documentation attached to the correct declaration and synchronized with signatures and behavior. Do not claim that all existing comments have been converted to Doxygen unless the requested scope has actually been checked. Do not convert unrelated files without a request.
- Reuse `Doxyfile`, `.github/workflows/documentation.yml` and the existing VS Code task. Keep input paths, exclusions and workflow path filters current when changing source layout. See [generated reference operation](docs/architecture.md#7-generated-class-reference-and-diagrams).
- Do not edit or commit generated HTML/SVG files under `build/doxygen/`, or introduce a parallel manually maintained class model. Generated references are disposable outputs and do not replace the authorized working documents.
- For changes affecting documentation generation, run Doxygen with Graphviz when available and inspect representative class pages/diagrams and warnings. Otherwise state the limitation and retain pending validation in the roadmap; do not equate configuration checks or static diagrams with a successful firmware build or runtime validation.

## Reuse-first design and framework boundaries

- Before implementing any change or new feature, inspect the existing capabilities, APIs, classes, and extension points. Apply this order: (1) reuse or compose existing behavior where sufficient; (2) improve or extend an existing abstraction when it falls short; (3) create a new abstraction only when neither option is suitable.
- Prefer the smallest maintainable implementation. Avoid duplicated logic, parallel APIs, unnecessary wrappers, and speculative abstractions. When new code is necessary, make it reusable where the responsibility is genuinely generic, without adding complexity for hypothetical use cases.
- Always preserve the framework/application boundary: reusable, application-independent code and classes belong in `lib/Interval/`; application-specific behavior belongs in user code, for example `src/MixerApplication.h` and `src/mixers/`. Interval must not depend on concrete mixer/application classes, protocol-specific mixer rules, or application configuration. Keep such specialization behind the existing contracts and extension points.
- Keep interface state projection separate from business transitions: workflow methods decide and perform lifecycle changes; `setStateXXX()` methods only publish the resulting framework/GUI state. Do not add service start/stop, discovery, resource reservation, or network ownership side effects to a `setStateXXX()` method. See the detailed contract in [`docs/architecture.md`](docs/architecture.md#state-transition-and-state-projection-rule).
- Prefer object-oriented composition and the existing class contracts. When necessary integration cannot reasonably be expressed within those objects, use `lib/Interval/src/Interval.h` for minimal framework glue, such as shared declarations, callbacks, or integration wiring. Keep domain logic in its owning application or mixer class; do not turn Interval.h into a catch-all implementation file or use it to bypass the framework/application boundary.
- Prefer extending the existing file that owns the responsibility instead of creating a new source/header file. Place generic changes in the appropriate existing Interval library file and domain changes in the appropriate existing application file. Do not merge unrelated responsibilities merely to reduce file count.
- Create a new source/header file only when the design justifies it, for example a distinct cohesive responsibility or a necessary dependency boundary. Record that justification in the open request's roadmap entry while work remains and retain the lasting design rationale in `docs/architecture.md` and the commit description. This exception does not relax the restriction on additional content documentation files.
- While the request is open, briefly record in its roadmap entry the existing capabilities considered, what is reused or extended, and why any new abstraction or file is necessary. Preserve lasting rationale in architecture and the commit description when removing the completed entry. Check this reasoning and the framework/application boundary before completion.

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
- Record validation results and limitations in the commit or pull request. Keep only unresolved limitations and remaining checks in roadmap, removing completed entries. Update architecture when operation or a decision changes, and README when user-facing behavior changes.
- Before delivery, audit the changed-file list: no new documentation outside the four authorized files, no explanatory content moved into subfolders, and no code changes outside the requested scope. Check that all code changes have corresponding documentation updates and that documentation is in English.
