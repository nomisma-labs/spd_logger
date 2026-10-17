# Application logging

`spd_logger` provides one global logger for a single-threaded application. Formatting, writing,
rotation and flushing run on the calling thread. The logger owns synchronous spdlog `_st` sinks;
it starts no background workers and does not modify spdlog's default logger or registry.

## Basic use

```cpp
#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    const nom::log_config config{.level = nom::log_level::info};
    nom::init_main_logger(config);
    INFO("Application started");
    DEBUG("Processed {} messages", 42);
    nom::flush_main_logger();
}
```

The module exports configuration, severity and lifecycle functions. Include `logging.h` before
the import to obtain the logging macros. Importing the module alone is sufficient to configure
the logger or use `nom::log_level` and `nom::to_string`.

`DEBUG`, `INFO`, `WARN`, `ERROR` and `ALWAYS` accept a message or a compile-time-checked fmt format
string followed by arguments. A message without formatting arguments is written as-is, including
braces. Suppressed calls do not evaluate their message or formatting arguments. The macros behave
as single statements and capture the calling source file and line.

`NOM_LOG` is an internal implementation helper. The five severity macros above are the supported
logging interface.

The severity order is `debug`, `info`, `warn`, `error`, `always`. The default threshold is `info`;
`set_main_log_level()` changes it immediately. `ALWAYS` is the highest public severity and passes
every supported threshold. It maps to spdlog's `critical`, which is also the label printed in the
output. It does not bypass output failures. Timestamps use UTC with a `Z` suffix.

## Lifetime and threading

The concrete logger object is defined once at namespace scope and initialized with a console
sink before `main()`. Logging from `main()` therefore works before explicit configuration. There
is no function-local static, lazy singleton accessor, or per-call initialization guard. Logging
from other translation units' global constructors or destructors is outside this lifecycle
contract because their initialization/destruction order is not guaranteed.

`init_main_logger()` configures that existing object. Calling it again sequentially flushes the
old destination and replaces its configuration without changing the object's address or adding
duplicate sinks. Configuration validation or replacement-sink construction failures throw and
leave the previous logger usable. Invalid levels, empty file names and zero/out-of-range rotation
sizes throw `std::invalid_argument`; spdlog reports file-open failures with `spdlog::spdlog_ex`.
Runtime output failures use spdlog's error reporting.

All logging, initialization, level changes and flushing must occur on the same application
thread. There are no asynchronous queues, periodic flushers, named loggers or explicit logger
arguments. `flush_main_logger()` flushes synchronously; `ERROR` and `ALWAYS` also flush after
writing. Flush at normal application shutdown. A flush updates the output stream; it is not a
durable-storage synchronization guarantee.

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
nom::init_main_logger(config);
```

The common types come from the `base` repository's `nom.config_types` module. The logger's enum
and aggregates opt in to its configuration traits. This supplies no serialization or field
mapping: configuration is ordinary C++ data, and there is no dependency on `json_config` or an
external configuration format.

The [single example](../../apps/logger_example/main.cpp) initializes a hardcoded configuration,
shows suppressed DEBUG argument evaluation, changes the level and writes a few messages.

## Consuming and checking the library

Use `find_package(spd_logger CONFIG REQUIRED)` and link `nom::spd_logger`, or add the repository
through `FetchContent`. Its package brings the `base`, fmt and spdlog dependencies. The primary
module is `nom.spd_logger`; the macro header is `spd_logger/logging.h`.

Run `./bootstrap.sh` for the repository's standard checks. The logging unit suite covers
filtering, argument evaluation, source locations, file rotation, flushing, reinitialization and
failure preservation. Source-tree and installed-package consumers test both the macro/header
interface and an import-only interface.

Catch2 also defines `INFO` and `WARN`. Tests that include the logging header define
`CATCH_CONFIG_PREFIX_MESSAGES` before Catch2 headers and use `CATCH_INFO` / `CATCH_WARN` for test
diagnostics.

`trace_logger` is independent and unchanged. Its overlapping `nom` API is intentional during
development; this version does not combine both logger libraries in one application.
