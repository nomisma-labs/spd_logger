# Changelog

Notable changes to this repo. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org); how to release:
[docs/kit/build.md](docs/kit/build.md#versioning).

## [Unreleased]

## [0.1.0] - 2026-10-17

### Added

- One global, eagerly initialized logger with synchronous single-threaded spdlog sinks.
- `DEBUG`, `INFO`, `WARN`, `ERROR` and `ALWAYS` macros with source locations, checked formatting
  and argument suppression below the configured level.
- Console, append-file and size-rotating-file destinations, UTC timestamps, synchronous flush
  and sequential reconfiguration that preserves the logger object's address.
- Plain C++ configuration based on `base`'s `nom.config_types`, without configuration-format
  dependencies or mappings.
- One simple example, behavioral unit tests, and source/installed-package consumer checks.
