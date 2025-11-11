# Debugging Recipes for Mechanica Imperii

Updated: January 19, 2026

This guide captures the workflows we use when diagnosing issues across the game. It covers
breakpoint setup, log filtering, the new verbose diagnostics macros, ImGui overlays, and
post-crash symbol collection.

---

## 1. Breakpoints & Editor Configuration

1. **Entry-point guard** – set an initial breakpoint in `SDL_main` to ensure the crash handler is
   initialized before any subsystems boot. This guarantees crash dumps include breadcrumbs.
2. **Subsystem entry** – recommended breakpoint locations:
   * `core::threading::ThreadedSystemManager::Update()` to inspect per-system threading state.
   * `game::gameplay::GameSystemsIntegration::Initialize()` when verifying cross-system wiring.
   * `core::diagnostics::CrashHandler::InitializeCrashHandling()` to confirm the dump directory.
3. **Conditional breakpoints** – in Visual Studio and CLion, watch `core::logging::GetGlobalLogLevel()`
   to pause when log verbosity changes at runtime. This helps reproduce log filtering issues.

> Tip: add `.vscode/launch.json` or the CLion Run Configuration to preload
> `${workspaceFolder}/build/bin/mechanica_imperii` with the working directory set to the repo root.

---

## 2. Logging & Filtering

### Level control

* Use `core::logging::SetGlobalLogLevel()` in a debugger console or temporary code to bump verbosity
  (`Trace`, `Debug`, `Info`, `Warn`, `Error`, `Critical`, `Off`).
* All subsystems now log through either `CORE_LOG_*` macros or the streaming helpers
  (`CORE_STREAM_INFO`, `CORE_STREAM_ERROR`, etc.). They respect the global level and unify the
  timestamped output format.

### Verbose diagnostics & tracing

CMake exposes three toggles:

```bash
cmake -DENABLE_VERBOSE_DIAGNOSTICS=ON \
      -DENABLE_MESSAGE_BUS_TRACE=ON \
      -DENABLE_ECS_LIFECYCLE_TRACE=ON ..
```

* `ENABLE_VERBOSE_DIAGNOSTICS` gates all optional tracing blocks.
* `ENABLE_MESSAGE_BUS_TRACE` emits `Trace` logs for every queue dispatch in the message bus.
* `ENABLE_ECS_LIFECYCLE_TRACE` reports entity creation/destruction from `EntityManager`.

### Filtering streams

* To reduce noise during long sessions, pipe stdout/stderr through `grep` or `less`:

  ```bash
  ./bin/mechanica_imperii 2>&1 | rg "\[TradeSystem\]"
  ```

* The log prefix is `[TIMESTAMP][LEVEL][System] message`. Combine with `rg --pcre2 "\[ERROR\]\[.*Save"`
  to isolate save-system failures.

### File logging & rotation

* The application enables `core::logging::EnableFileSink` during bootstrap. Logs are written to
  `logs/mechanica.log` with a 10&nbsp;MB rotation threshold and up to five historical files.
* Override defaults via environment variables:
  * `MECHANICA_LOG_PATH` – absolute or relative destination for the active log file.
  * `MECHANICA_LOG_MAX_SIZE` – rotation threshold in bytes (e.g., `2097152` for 2&nbsp;MB).
  * `MECHANICA_LOG_MAX_FILES` – number of archived files to retain (set to `0` to truncate instead of rotate).
  * `MECHANICA_LOG_LEVEL` – case-insensitive log level (`trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`).
* Use `core::logging::Flush()` before collecting logs mid-run and `core::logging::DisableFileSink()` if you need to
  opt out temporarily (the global mutex keeps this thread-safe).

---

## 3. ImGui Debug Overlays

* Press `F12` (default binding) to toggle the developer HUD. This renders the
  `ui::PerformanceWindow`, `ui::PopulationInfoWindow`, and the trade diagnostics overlay.
* Add custom overlays by instantiating `ui::Toast::Show()` or extending the ImGui block in
  `apps/main.cpp` after the `RenderUI()` call.
* When hunting layout bugs, run the build with `-DENABLE_VERBOSE_DIAGNOSTICS=ON` and set a
  breakpoint in `ui::Toast::Show`. The call stack highlights which system triggered the overlay.

---

## 4. Crash Dumps & Symbol Generation

### Crash handler basics

* `core::diagnostics::InitializeCrashHandling` is invoked from `SDL_main`. Dumps and breadcrumbs are
  written under `<build>/bin/crash_dumps` by default.
* Use `core::diagnostics::AppendCrashBreadcrumb("Entered economic update loop")` to append
  context that will be flushed on the next crash.

### Post-mortem workflow

1. Reproduce the crash and note the dump filename printed by `CrashHandler`.
2. Symbols are copied to `<build>/symbols/` after each build.
   * Windows: PDB files from `/Zi` + `/DEBUG:FULL` linking.
   * Linux: split debug objects generated through `objcopy --only-keep-debug` with build IDs.
3. Load the dump:
   * **Windows** – open the `.dmp` in WinDbg or Visual Studio. Ensure the symbol path contains
     `<build>/symbols`.
   * **Linux** – run `gdb ./bin/mechanica_imperii <dump.log>`. GDB automatically picks up the
     `.debug` companion via the GNU debuglink.
4. Share repro steps with `ENABLE_VERBOSE_DIAGNOSTICS=ON` so the team receives trace-level context.

### Ensuring reproducibility

* Document the exact CMake preset/options used (especially diagnostic toggles) in bug reports.
* Attach the breadcrumb text file (same basename as the dump with `.breadcrumbs.txt`).
* Capture the ImGui overlay state by toggling screenshots (Print Screen on Windows or `gnome-screenshot`).

---

## 5. Quick Reference

| Feature                         | Command / Location                                   |
|---------------------------------|------------------------------------------------------|
| Toggle global log level         | `core::logging::SetGlobalLogLevel(LogLevel::Debug);` |
| Enable verbose tracing          | `cmake -DENABLE_VERBOSE_DIAGNOSTICS=ON ...`          |
| Crash dump directory            | `<build>/bin/crash_dumps/`                           |
| Symbol output                   | `<build>/symbols/`                                   |
| Message bus trace macro         | `CORE_TRACE_MESSAGE_BUS(event, topic, payload)`      |
| ECS lifecycle trace macro       | `CORE_TRACE_ECS_LIFECYCLE(action, id, name)`         |
| Stream logging helper           | `CORE_STREAM_INFO("System") << value`               |
| Enable rotating log file        | `core::logging::EnableFileSink(options, &error)`     |
| Flush log streams to disk       | `core::logging::Flush();`                            |

## 6. Rendering Performance Profiling

1. Configure and build the test harness:

   ```bash
   cmake -S . -B build -DBUILD_TESTS=ON
   cmake --build build --target test_rendering_profile
   ```

2. From `build/tests`, run the synthetic benchmark directly or wrap it with `perf`:

   ```bash
   perf stat ./test_rendering_profile
   ```

   The executable seeds 5k synthetic provinces, repositions the camera 50 times, and reports both total and per-iteration
   culling times so you can compare optimizations across branches.

Use these recipes as a living document—add new workflows whenever a debugging session uncovers a repeatable trick.

