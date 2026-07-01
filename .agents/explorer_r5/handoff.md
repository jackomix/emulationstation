# Handoff Report: R5 Log Redirection Investigation

## 1. Observation
We observed the following definitions and behaviors in the codebase:
- In `es-app/src/main.cpp:61`, the application initializes the executable path using `argv[0]`:
  ```cpp
  Paths::setExePath(argv[0]);
  ```
- In `es-core/src/Paths.cpp:310-317`, `Paths::setExePath` canonicalizes the input path and extracts its parent directory (which is stored in a static global `exePath`):
  ```cpp
  void Paths::setExePath(const std::string& _path)
  {
  	std::string path = Utils::FileSystem::getCanonicalPath(_path);
  	if (Utils::FileSystem::isRegularFile(path))
  		path = Utils::FileSystem::getParent(path);

  	exePath = Utils::FileSystem::getGenericPath(path);
  }
  ```
- In `es-core/src/Log.cpp:46-50`, `Log::init()` constructs the log file path:
  ```cpp
  #ifdef _ENABLEAMBERELEC
  	auto logPath = Paths::getLogPath() + "/es_log.txt";
  #else	
      auto logPath = Paths::getUserEmulationStationPath() + "/es_log.txt";
  #endif
  ```
- In `setup_boot_hook.sh:31-33`, the test environment launches the binary at:
  ```bash
  /roms/ports/es_test/emulationstation "$@"
  ```
- In `es-app/src/ApiSystem.cpp` (lines 264, 293, 323, 325) and `es-core/src/utils/Platform.cpp:149`, other parts of the codebase also access the log directory using `Paths::getLogPath()`.

---

## 2. Logic Chain
1. When running the application via `/roms/ports/es_test/emulationstation`, `argv[0]` contains the binary path `/roms/ports/es_test/emulationstation`.
2. `Paths::setExePath(argv[0])` canonicalizes this to `/roms/ports/es_test/emulationstation`.
3. Since it is a regular file, it extracts the parent directory `/roms/ports/es_test` and saves it in `exePath`.
4. Any calls to `Paths::getExePath()` will return `/roms/ports/es_test`.
5. Under default behavior, `Paths::Paths()` constructor initializes `mLogPath` to `/storage/.emulationstation/logs` (when `_ENABLEAMBERELEC` is defined) or `/home/ark/.emulationstation` otherwise.
6. Thus, the log file `es_log.txt` is written to one of these system/home folders rather than the custom SD card partition `/roms/ports`.
7. By checking if `Paths::getExePath() == "/roms/ports/es_test"`, we can reliably detect that the binary is running from the test path `/roms/ports/es_test/emulationstation`.
8. Once detected, we can override `mLogPath` to `/roms/ports` and override `logPath` in `Log::init()` to `/roms/ports/es_log.txt`.

---

## 3. Caveats
- **Assumption of Directory Existence and Write Access**: We assume that `/roms/ports` directory is present and is a writable partition. On standard ArkOS / AmberElec installations, this is the case since it is the exFAT partition where ports and games are stored.
- **Binary Name**: If the binary name is changed to something other than `emulationstation` (e.g. `emulationstation-test`), the parent path checking logic `Paths::getExePath() == "/roms/ports/es_test"` will still hold true, which is desirable and robust.

---

## 4. Conclusion
The root cause is that EmulationStation initializes its log path (`mLogPath`) based on static OS configurations and the user's home folder, ignoring the directory where the binary is executed.

We recommend the following fix strategy:
1. Detect that the binary is running from the test folder by checking if `Paths::getExePath() == "/roms/ports/es_test"`.
2. Override `mLogPath` in the `Paths::Paths()` constructor to `/roms/ports`. This ensures other tools writing auxiliary logs (e.g. scrapers, system processes) also output to the `/roms/ports/` partition.
3. Override `logPath` in `Log::init()` to `/roms/ports/es_log.txt` when the execution directory matches the test path, which handles all build configuration flag variations.

A proposed `.patch` has been written to:
`/Users/jacko/Documents/myEmulationStation/.agents/explorer_r5/r5_log_redirection.patch`

---

## 5. Verification Method
To verify this change:
1. Apply the patch `r5_log_redirection.patch` to the codebase.
2. Build the project:
   ```bash
   mkdir -p build && cd build
   cmake ..
   make
   ```
3. Copy the compiled `emulationstation` binary to `/roms/ports/es_test/emulationstation`.
4. Run the binary from that path:
   ```bash
   /roms/ports/es_test/emulationstation
   ```
5. Check if the log file is generated successfully at:
   `/roms/ports/es_log.txt`
6. Run the binary from another path (e.g. `/usr/bin/emulationstation`) and verify it writes its log to the default path (e.g. `~/.emulationstation/es_log.txt` or `~/.emulationstation/logs/es_log.txt`).
