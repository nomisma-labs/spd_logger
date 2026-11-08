# spd_logger

<!-- repo-description:begin -->
`spd_logger` is a synchronous logger for single-threaded C++ applications. It provides an owning
`nom::logger`, the global `nom::main_logger`, `LDEBUG` / `LINFO` / `LWARN` / `LERROR` / `LALWAYS`
macros, and `NOM_LOG` / `NOM_LOG_NOEXCEPT` for an explicit logger. Console or rotating-file output
uses spdlog and fmt, with ordinary C++ configuration types from `base`; it starts no background
threads.
<!-- repo-description:end -->

C++ builds and consumers use CMake 4.4.x, Ninja, GCC 16 or Clang 22, and libstdc++ 16
standard-library module sources. Enable std module detection before the consumer's first
`project()` and call `nom_enable_std_modules(target)` on each compiled consumer target.
See [standard-library module setup](docs/kit/build.md#modules), including package consumers
without the kit. Macro headers remain textual includes before imports.

```cpp
#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    nom::init_logger(nom::main_logger);
    LINFO("Application started with {} connections", 3);
    nom::flush_logger(nom::main_logger);
}
```

Link `nom::spd_logger` and import `nom.spd_logger`. The global wrapper exists before `main()`
and starts empty; initialize it before logging, changing its level or flushing. A disabled logging
macro does not evaluate its message arguments. `LERROR` and `LALWAYS` catch logging exceptions
and fall back to stderr. Read the
[logging guide](docs/spd_logger/logger.md) for configuration, lifecycle and severity semantics,
or run the [small example](apps/logger_example/main.cpp).

## Build

Install the tools listed in [docs/kit/build.md](docs/kit/build.md#toolchain), then:

```bash
git submodule update --init --recursive
./bootstrap.sh                   # fresh builds, tests and checks for every preset but release
cmake --workflow --preset dev    # configure, build and test one preset
build/dev/bin/logger_example     # run the single small example
```

Everyday commands, the presets and what bootstrap checks: [docs/kit/build.md](docs/kit/build.md).

## Docs

Agents start at [AGENTS.md](AGENTS.md), people at [docs/](docs/README.md). Adding a library, an
app, tests or a dependency: [docs/kit/howto.md](docs/kit/howto.md). Releases: [CHANGELOG.md](CHANGELOG.md).
