# Application logging

`spd_logger` provides owning `nom::logger` objects and the global `nom::main_logger` for a
single-threaded application. Formatting, writing, rotation and flushing run on the calling
thread. Each initialized logger owns its backend and synchronous spdlog `_st` sink; it starts
no background workers and does not modify spdlog's default logger or registry.

## Basic use

```cpp
#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    const nom::log_config config{.level = nom::log_level::info};
    nom::init_logger(nom::main_logger, config);
    LINFO("Application started");
    LDEBUG("Processed {} messages", 42);
    nom::flush_logger(nom::main_logger);
}
```

The module exports `logger`, `main_logger`, configuration, severity and lifecycle functions.
`logging.h` contains only macros; include it before importing the module, and use both for
logging. Importing the module alone is sufficient to construct or configure a logger or use
`nom::log_level` and `nom::to_string`. The public interface exposes no spdlog types. fmt remains
the formatting dependency; application-defined fmt formatters can include the relevant fmt
headers before the module import.

`LDEBUG`, `LINFO`, `LWARN`, `LERROR` and `LALWAYS` accept a message or a compile-time-checked fmt
format string followed by arguments, and write to `nom::main_logger`. A message without formatting
arguments is written as-is, including braces. Suppressed calls do not evaluate their message
or formatting arguments. The macros behave as single statements and capture the calling source
file and line.

`NOM_LOG(logger, level, ...)` provides the same behavior for an explicit logger and a
`nom::log_level`. It evaluates the logger expression once, even when the message is suppressed:

```cpp
nom::logger audit{nom::log_config{.level = nom::log_level::info}};
NOM_LOG(audit, nom::log_level::info, "Audit logger ready");
nom::flush_logger(audit);
```

Direct `logger::log(level, location, ...)` calls check the threshold before formatting. Both
`log()` and the public `log_prechecked_level()` have overloads for a raw message, a checked fmt
format string with arguments, and a single formattable value. Pass `nom::log_location::current()`
to capture the direct caller's source location. Ordinary function arguments are evaluated before
either member is entered, even when the message will be suppressed.

`log_prechecked_level()` requires the caller to have accepted the logger and level with
`should_log()`. It omits repeated wrapper level checks; the backend still applies its normal filtering.
`NOM_LOG` performs that check before evaluating the message arguments and calls
`log_prechecked_level()` when enabled.

The severity order is `debug`, `info`, `warn`, `error`, `always`. The default threshold is `info`;
`set_log_level(logger, level)` changes it immediately. `ALWAYS` is the highest public severity
and passes every supported threshold. Output uses the public labels `DEBUG`, `INFO`, `WARN`,
`ERROR` and `ALWAYS`; the underlying spdlog level mapping is an implementation detail. `ALWAYS`
does not bypass output failures. Timestamps use UTC with a `Z` suffix.

The logger assumes named `log_level` values. Configuration adapters or the application framework
must accept only supported values. Explicit casts producing unnamed values are the caller's
responsibility and outside the input contract. Conversions defensively fall back to `ALWAYS`
or spdlog's `critical`; this does not define filtering for arbitrary cast values.

## Lifetime and threading

`nom::main_logger` is defined once at namespace scope and starts empty, with no backend or sink.
A default-constructed `nom::logger` has the same empty state. Call `init_logger(logger, config)`
before logging, changing its level or flushing; the default configuration selects console output.
Alternatively, `nom::logger logger{config}` constructs and initializes a local logger. Using an
empty logger violates a precondition. An assertion diagnoses this only in builds without
`NDEBUG`; release builds provide no runtime initialization guard. Initialize every logger before
use in every build. There is no automatic initialization or lazy console allocation.

The wrapper is neither copyable nor movable. Its destructor releases its owned resources without
throwing, including when the wrapper was never initialized. Logging through `main_logger` from
other translation units' global constructors or destructors is outside the lifecycle contract
because their initialization/destruction order is not guaranteed.

`init_logger()` configures the supplied wrapper. It calls `config.validate()` before flushing
the existing backend or constructing its replacement, and throws `std::invalid_argument` with
the returned error message on validation failure. Calling it again sequentially flushes the old
destination and replaces its configuration without changing the wrapper's identity or adding
duplicate sinks. Configuration validation or backend construction failures throw and leave the
previous setup usable, or leave an initially empty wrapper empty. Backend initialization
failures, including file-open failures, throw `std::runtime_error`. Runtime output
and flush failures are reported to stderr without throwing to the application or preventing
reconfiguration. Formatting and type-erasure exceptions, including exceptions from custom
formatters, propagate unchanged. Synchronous I/O can still stall the calling thread; this error
policy does not provide nonblocking I/O.

All logging, initialization, level changes and flushing must occur on the same application
thread. There are no asynchronous queues or periodic flushers. `flush_logger(logger)` flushes
synchronously; messages at `error` and `always` also flush after writing. Flush at normal
application shutdown. A flush updates the output stream; it is not a durable-storage
synchronization guarantee.

The spdlog build enables `SPDLOG_NO_ATOMIC_LEVELS` and `SPDLOG_NO_THREAD_ID`, removing atomic
severity/flush thresholds and thread-ID capture. These definitions are propagated by spdlog's
CMake target to its consumers and exported package. A parent project supplying an already
configured spdlog target must enable both options before configuring that target. This does not
remove the disabled-backtrace atomic check, the backend error handler's mutex, or synchronization
inside C stream flushing.

Give independent file loggers distinct destinations. Separate rotating sinks must not manage
the same file, because each sink tracks rotation state independently.

## Configuration

`log_config` contains a `log_level level` and an optional `logfile_config file`. An absent file
selects the console. A file configuration contains a `cfg_string name` and an optional
`cfg_uint rotation_size` measured in bytes. File output appends; when rotation is enabled it
retains up to ten older files in addition to the active file.

```cpp
const nom::log_config config{
    .level = nom::log_level::debug,
    .file = nom::logfile_config{
        .name = "application.log",
        .rotation_size = nom::cfg_uint{1024 * 1024}}};
nom::init_logger(nom::main_logger, config);
```

Both configuration structures remain aggregates and expose
`std::expected<void, std::string> validate() const`. `logfile_config::validate()` rejects an empty
name, embedded NUL characters, and a supplied rotation size that is zero or exceeds `size_t`'s range.
Errors identify `/name` or `/rotation_size`. `log_config::validate()` validates an optional file
when present and prefixes its error with `/file`.
Validation does not open log files or construct a backend, so applications can call it before
initializing services:

```cpp
const nom::log_config config{.file = nom::logfile_config{.name = ""}};
const auto result = config.validate();
// !result; result.error() == "/file/name must not be empty"
```

The common types come from the `base` repository's `nom.config_types` module. Logger configuration
is ordinary C++ data. Configuration adapters in client repositories own type registration,
field mapping and format-specific validation; this library registers no custom configuration
types and has no dependency on `json_config` or an external configuration format.

The [single example](../../apps/logger_example/main.cpp) initializes a hardcoded configuration,
shows suppressed `LDEBUG` argument evaluation, changes the level and writes a few messages.

## Consuming and checking the library

Use `find_package(spd_logger CONFIG REQUIRED)` and link `nom::spd_logger`, or add the repository
through `FetchContent`. Its package brings the `base`, fmt and spdlog dependencies. The primary
module is `nom.spd_logger`; the macro header is `spd_logger/logging.h`.

Run `./bootstrap.sh` for the repository's standard checks. The logging unit suite covers
filtering, argument evaluation, source locations, file rotation, flushing, reinitialization and
failure preservation. Source-tree and installed-package consumers test both the macro/header
interface and an import-only interface.

The logger's `LINFO` and `LWARN` macros coexist with Catch2's normal `INFO` and `WARN` diagnostics.
Tests can include both libraries without a prefix workaround. The logger provides no short-name
macro aliases.

`trace_logger` is independent and unchanged. Its overlapping `nom` API is intentional during
development; this version does not combine both logger libraries in one application.
