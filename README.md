# Interval Footswitch Application

This repository contains the firmware application for the Interval footswitch.
It uses the reusable [Interval framework](https://github.com/infrafast/Interval)
to provide the common device services and runs the product's mixer-management
features, including support for Mixing Station and Behringer X/M-series
workflows.

You normally use this repository when you want to build or flash the complete
footswitch application. The reusable framework is downloaded automatically by
PlatformIO; it must not be copied into this project.

## Build

Install PlatformIO, connect the target board, then choose an environment:

```sh
pio run -e dev
pio run -e prod
```

The `dev` environment is intended for development and diagnostics. The `prod`
environment is the normal release build. The exact board and build settings
are kept in `platformio.ini`.

To build the framework diagnostic application instead of the mixer application,
enable the `INTERVAL_DIAGNOSE` build flag. The diagnostic entry point is
provided by Interval itself.

## Web interface and resources

The firmware can package the web configuration interface and other resources
from the `data/` directory. The editable web client is maintained separately;
follow the project scripts and the instructions in `platformio.ini` when
refreshing those resources.

## Project boundaries

- This repository contains the product launcher, mixer applications, product
  configuration, translations, and firmware resources.
- The [Interval repository](https://github.com/infrafast/Interval) contains the
  reusable framework, shared hardware services, diagnostic application, and its
  dependency graph.
- Changes to reusable framework behavior belong in Interval. This project
  consumes the released version declared in `platformio.ini`.

## Documentation

| Document | Purpose |
|---|---|
| [Architecture](docs/architecture.md) | Product integration and application-specific design |
| [Roadmap](docs/roadmap.md) | Product work and pending validation |
| [Agent instructions](AGENT.md) | Contribution rules for coding agents |

For the framework's capabilities and development rules, see the
[Interval documentation](https://github.com/infrafast/Interval).
