# AGENTS.md

<!-- repo-description:begin -->
`spd_logger` provides one global, synchronous logger for single-threaded applications. It uses
spdlog for formatting and sinks, fmt for message formatting, and `base` for common configuration
types. The module is `nom.spd_logger`, the CMake target is `nom::spd_logger`, and
`spd_logger/logging.h` supplies `DEBUG`, `INFO`, `WARN`, `ERROR` and `ALWAYS`.
<!-- repo-description:end -->

Read and follow the rules every Nomisma repo shares: @docs/kit/agents.md

Rules for this repo alone follow below.

## This repo

- Read [docs/spd_logger/logger.md](docs/spd_logger/logger.md) for lifecycle, configuration and
  logging semantics. The public namespace is `nom`.
- `core/spd_logger/`: the module exports severity, configuration and lifecycle functions;
  `logging.h` contains macros and the declaration of the internal global logger object.
  `logger_state.cpp` defines that concrete object at namespace scope in an ordinary translation
  unit. Do not replace it with a function-local static or lazy singleton accessor.
- All logger operations run on the calling application thread. Use synchronous `_st` sinks;
  do not add workers, asynchronous queues or periodic flushing. There is one global logger and
  no explicit logger-argument interface.
- `log_config` uses common types from `nom.config_types`. There is no JSON dependency or
  configuration-format mapping. `apps/logger_example/main.cpp` is the single small example.
- `INFO` and `WARN` collide with Catch2's macros. Tests define `CATCH_CONFIG_PREFIX_MESSAGES`
  before including Catch2 headers and use `CATCH_INFO` / `CATCH_WARN` for their diagnostics.
- Dependencies are `base` v0.1.0, spdlog v1.17.0 and fmt 12.2.0. Link `nom::config_types` and
  `spdlog::spdlog`; exported packages require `base`, `"fmt 12"` and `spdlog`.
- `trace_logger` is developed independently. Its overlapping public names are deliberate;
  do not refactor or connect that repository as part of this logger's work.
