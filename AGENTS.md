# AGENTS.md

<!-- repo-description:begin -->
`spd_logger` provides synchronous, owning loggers for single-threaded applications, including
the global `nom::main_logger`. It uses spdlog internally for sinks, fmt for message formatting,
and `base` for common configuration types. The module is `nom.spd_logger`, the CMake target is
`nom::spd_logger`, and `spd_logger/logging.h` supplies `LDEBUG`, `LINFO`, `LWARN`, `LERROR`, `LALWAYS`
and the explicit-logger macros `NOM_LOG` and `NOM_LOG_NOEXCEPT`.
<!-- repo-description:end -->

Read and follow the rules every Nomisma repo shares: @docs/kit/agents.md

Rules for this repo alone follow below.

## This repo

- Read [docs/spd_logger/logger.md](docs/spd_logger/logger.md) for lifecycle, configuration and
  logging semantics. The public namespace is `nom`.
- `src/spd_logger/`: the module exports `logger`, `main_logger`, severity, configuration and
  lifecycle functions. `logging.h` contains only macros; using them requires importing the
  module. Keep spdlog types out of the public interface; fmt remains the formatting dependency.
  `logger_state.cpp` defines `main_logger` once at namespace scope. Do not replace it with a
  function-local static or lazy singleton accessor.
- A default-constructed `logger` is empty and creates no backend or sink. Initialize it with
  `init_logger(logger&, config)` or construct it with a `log_config` before logging, setting its
  level or flushing. The wrapper owns its backend, is neither copyable nor movable, and releases
  resources in its no-throw destructor. Initialization before use is a precondition, checked by
  `assert` only when `NDEBUG` is not defined; release builds have no runtime initialization guard.
- The logger assumes named `log_level` values. Configuration adapters or the application
  framework must accept only supported values; explicit casts producing unnamed values are the
  caller's responsibility and outside the input contract. Conversions defensively fall back to
  `ALWAYS`/spdlog `critical`; this does not define filtering for arbitrary cast values.
- All logger operations run on the calling application thread. Use synchronous `_st` sinks;
  do not add workers, asynchronous queues or periodic flushing. The five severity macros target
  `main_logger`; `NOM_LOG(logger, level, ...)` accepts an explicit logger and evaluates it once.
  It calls `should_log()` before `log_prechecked_level()` so suppressed macro calls do not
  evaluate their message or formatting arguments.
- `logger::log()` checks the threshold before formatting. The public `log_prechecked_level()`
  overloads accept raw messages, format strings with arguments, and single formattable values;
  callers must have checked `should_log()` first. They omit repeated wrapper level checks while the
  backend retains normal filtering. Direct member calls evaluate arguments before entry.
  Direct calls and `NOM_LOG` propagate formatting and type-erasure exceptions; runtime backend
  I/O errors retain their reporting policy and flush failures do not prevent reconfiguration.
- `NOM_LOG_NOEXCEPT` catches the entire call, including argument evaluation; `LERROR` and
  `LALWAYS` use it. Its fallback writes the message or format text and exception text to stderr
  with `fputs`, without allocation, formatting retries or repeated argument evaluation. If no
  message is available, use the source expression. Catch non-standard exceptions too. Keep the
  same initialization and threading preconditions. Internal macro helpers live in `nom::detail`.
- Keep small logging wrappers explicitly `inline` in the module interface: `should_log()`,
  `log()`, `log_prechecked_level()` and the initialization assertion helper. Keep substantive
  formatting and backend writes in the implementation unit. The cached `level_` must match the
  backend threshold: successful initialization and level changes update both, and failed
  reinitialization preserves both.
- `log_config` uses common types from `nom.config_types`. There is no JSON dependency or
  configuration-format mapping. Client adapters own custom config-type registrations.
  `apps/logger_example/main.cpp` is the single small example.
- Keep `logfile_config` and `log_config` as aggregates with
  `std::expected<void, std::string> validate() const`. File validation owns filename and rotation
  rules; `log_config` prefixes nested file errors with `/file`. `init_logger()` validates before
  backend side effects and throws `std::invalid_argument` with the returned message on failure.
- Keep the `LDEBUG`, `LINFO`, `LWARN`, `LERROR` and `LALWAYS` names without short-name aliases.
  They coexist with Catch2's normal `INFO` and `WARN` diagnostics; tests need no prefix workaround.
- Dependencies are `base` v0.5.0, spdlog v1.17.0 and fmt 12.2.0. Link `nom::config_types` and
  `fmt::fmt` publicly, and `spdlog::spdlog` privately; exported packages require `base`,
  `"fmt 12"` and `spdlog`.
- Build spdlog with `SPDLOG_NO_ATOMIC_LEVELS` and `SPDLOG_NO_THREAD_ID` enabled. Keep their
  definitions consistent through spdlog's CMake target; do not define them only on this wrapper.
- `trace_logger` is developed independently. Its overlapping public names are deliberate;
  do not refactor or connect that repository as part of this logger's work.
