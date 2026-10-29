# Interval Footswitch

Interval Footswitch is a programmable hardware controller for mixing consoles and stage equipment. This repository contains the ESP32-S3 firmware, the Interval framework, web configuration resources, and update tools.

## Overview

The footswitch combines three buttons, a TFT display, and Wi-Fi/Ethernet connectivity. Configuration is performed through its web interface. The currently selected application integrates two mixer adapters: Mixing Station and Behringer X/M Series.

The framework also includes MIDI, OSC, and socket interfaces. Availability depends on configuration and the compiled environment; the presence of a class does not guarantee that a feature is complete or validated for live use.

## Hardware and operation

The configured target is the Seeed XIAO ESP32-S3, with LittleFS storage, an ILI9341 display, and W5500 Ethernet. Banks and strips organize the controls displayed and triggered by the buttons.

If the normal STA connection is unavailable, the configuration wizard can
expose an AP for the WebGUI. Selecting an SSID there saves its credentials for
the next normal startup while keeping the configuration AP available; this
workflow does not currently use simultaneous AP+STA operation.

The active application registers an input-to-bus send strip. A gain strip is also registered, but command transmission still needs to be completed: it must not be considered operational.

Web resources are included in `data/webapp/`. Updates package firmware and resources into `.tgz` archives; their preparation and the differences between development and production builds are described in the architecture document.

The editable web client is maintained in a separate webGUI repository. On the
development workstation it is checked out beside this repository as `../webGUI/`. To run the complete local demo,
including the PHP mock API under `post/` and `upload/`, use PHP's built-in
server from the repository root:

```sh
php -S 127.0.0.1:8080 -t ../webGUI
```

A static file server can display the HTML and assets, but it cannot provide the
PHP API endpoints used by the demo. `preparefiles_MACOS.py` rebuilds the
compressed firmware web assets from the external webGUI checkout; set
`WEBGUI_SRC` when using another checkout path.

## Documentation

| Document | Purpose |
|---|---|
| [Architecture](docs/architecture.md) | Technical reference, patterns, adding strips/interfaces, builds, and hardware audit |
| [Roadmap](docs/roadmap.md) | Planned changes, open issues, validation, and request tracking |
| [Agent instructions](AGENT.md) | Contribution rules and documentation organization |

Class diagrams and a browsable API reference can be generated automatically from the C++ code with Doxygen and Graphviz. See [generation and web publication](docs/architecture.md#7-generated-class-reference-and-diagrams) for VS Code, GitHub Actions artifacts, and optional GitHub Pages setup.

These three documents are the project's only working references. This README remains a user-facing overview.

Documentation subfolders contain only attachments and verification materials, including PCB audit illustrations and measurements. Explanations and tracking are centralized in the documents above. All project documentation is maintained in English.
