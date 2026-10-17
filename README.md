# spd_logger

<!-- repo-description:begin -->
`spd_logger` is a synchronous logger for single-threaded C++ applications. It provides one
global logger, `DEBUG` / `INFO` / `WARN` / `ERROR` / `ALWAYS` macros, console or rotating-file
output, and ordinary C++ configuration. It uses spdlog and fmt, with common configuration types
from `base`; it starts no background threads.
<!-- repo-description:end -->

```cpp
#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    nom::init_main_logger();
    INFO("Application started with {} connections", 3);
    nom::flush_main_logger();
}
```

Link `nom::spd_logger` and import `nom.spd_logger`. The logger object exists before `main()`;
initialization configures it. A disabled log call does not evaluate its arguments. Read the
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
