# TODO

## Flush and emergency logging

- [ ] Make synchronous flush-after-write an explicit, tested guarantee for `LALWAYS` and
  `LERROR`. The backend already uses `flush_on(spdlog::level::err)`, which covers both levels;
  preserve that behavior through configuration changes and both console and file sinks.
  Filtered messages need not flush. Flush makes buffered output visible; it does not imply
  durable-storage synchronization (`fsync`).
- [ ] Add `LFLUSH()` as a convenience macro for `nom::flush_logger(nom::main_logger)`.
  Keep the existing initialization and calling-thread requirements; do not add automatic
  logger initialization or background flushing.
- [ ] Add `FATAL(...)` for emergency logging and termination. Write the diagnostic with
  `fprintf(stderr, ...)` and `fflush(stderr)`, also attempt `LALWAYS` and
  `flush_logger(main_logger)`, then call `std::terminate()`. Write the emergency output first
  so formatting or backend failures cannot prevent it. Exceptions from application logging
  must not escape or prevent termination; formatting failure needs a minimal fallback message.
  Preserve the call-site location and avoid evaluating arguments twice.
- [ ] Define safe `FATAL` behavior before logger initialization: retain emergency stderr
  output and termination without accessing an empty backend. Ordinary logging keeps its
  existing initialization precondition. Document the emergency path's threading constraints.
- [ ] Add subprocess tests proving that ERROR/ALWAYS output is flushed before abrupt exit,
  `LFLUSH()` publishes buffered messages, and `FATAL` writes to both configured output and
  stderr before terminating. Cover logger/formatting failures and the uninitialized-logger case.
