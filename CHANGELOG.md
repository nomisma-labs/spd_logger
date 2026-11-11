# Changelog

Notable changes to this repo. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and versions follow [Semantic Versioning](https://semver.org); how to release:
[docs/kit/build.md](docs/kit/build.md#versioning).

## [Unreleased]

## [0.9.0] - 2026-11-11

### Changed

- License project-owned code and documentation under MIT, retaining
  Alexey Tkachenko's copyright. Third-party dependencies retain their own licenses.
- Synchronize cmake_template v0.14.0 and record the repository license policy;
  the shared kit retains MIT independently of the project selection.
- Include the project license in installed artifacts alongside the applicable
  toolchain and dependency notices.
- Update internal dependency pins to `base v0.5.0`.

## [0.8.0] - 2026-11-08

### Added

- `NOM_LOG_NOEXCEPT` catches argument evaluation and formatting/logging exceptions, with an
  allocation-free stderr fallback containing the message or source expression and exception text.

### Changed

- `LERROR` and `LALWAYS` use `NOM_LOG_NOEXCEPT`; other logging macros retain exception propagation.

## [0.7.0] - 2026-11-08

### Changed

- Move library sources from `core/` to `src/` and synchronize the shared build kit with
  cmake_template v0.13.0. Module, target, public include and installed paths remain unchanged.
- Pin base v0.4.0 so standalone and composed builds use the same source layout and kit.

### Upgrade

- Source consumers must adopt cmake_template v0.13.0 and migrate their own library sources
  to `src/` alongside the dependency update. Recreate build trees and compilation databases.

## [0.6.0] - 2026-11-07

### Changed

- Declare `log_config::level` as `cfg_defaulted<log_level>` so configuration
  adapters can preserve its `info` default using the common per-field policy.
  Explicit aggregate initialization uses `.level = {log_level::info}`; the
  wrapper provides access to the underlying enum. No JSON dependency is added.

## [0.5.0] - 2026-10-28

### Changed

- Sync cmake_template v0.12.0, use `IMPORT_STD` in target helpers, and suppress
  experimental-feature warnings through the shared presets while retaining other warnings.
- Pin base v0.3.0 for standalone and composed builds.

## [0.4.1] - 2026-10-28

### Changed

- Update the `base` submodule to v0.2.1 so standalone and composed builds use the same
  standard-library module support and common configuration types.
- Sync the shared build kit to cmake_template v0.11.2 and adopt `import std;` in C++
  modules and consumers, retaining macro and third-party header boundaries.
  Include the follow-up standard-library documentation correction from template commit `8ad7a4e`.
- Require CMake 4.4.x and enable standard-library module detection before `project()`
  in both the repository and source/installed-package consumer projects.

## [0.4.0] - 2026-10-27

### Changed

- Update the `base` dependency to v0.2.0.
- Sync the shared build infrastructure with cmake_template v0.11.1; `nom_app()` uses the
  supplied target name without a project prefix.

## [0.3.0] - 2026-10-18

### Changed

- Rename the global logging macros to `LDEBUG`, `LINFO`, `LWARN`, `LERROR` and `LALWAYS`.
  Update calls to the previous `DEBUG`, `INFO`, `WARN`, `ERROR` and `ALWAYS` names; the logging
  header no longer defines them, so Catch2 diagnostics work without prefix configuration.

## [0.2.0] - 2026-10-18

### Added

- Owning `nom::logger` instances for global and local logging, with direct logging methods
  and an explicit-logger `NOM_LOG` macro.
- Configuration validation through `std::expected<void, std::string> validate() const`, with
  field paths in validation errors.

### Changed

- Replace the global logger lifecycle functions with `init_logger`, `set_log_level` and
  `flush_logger`, each taking a logger reference. The global wrapper starts empty and requires
  explicit initialization.
- Hide spdlog behind the module API and require an explicit `import nom.spd_logger` when using
  the macro-only logging header. Configuration adapters own type registrations.
- Inline threshold checks and use `log_prechecked_level` in macros to avoid a repeated wrapper
  check while keeping suppressed arguments unevaluated.
- Disable spdlog atomic levels and thread-ID capture for single-threaded applications.
- Propagate formatting exceptions and keep flush failures nonfatal during reconfiguration.

### Fixed

- Reject embedded NUL characters in file names before initialization side effects.
- Render the public severity labels, including `ALWAYS`, consistently in log output.

## [0.1.1] - 2026-10-17

### Fixed

- Initialize install directories locally so dependency licenses stay inside the installation
  prefix when another Nomisma repository has already loaded the shared CMake kit.

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
