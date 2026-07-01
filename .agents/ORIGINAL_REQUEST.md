# Original User Request

## 2026-06-19T03:04:06Z

A complete top-to-bottom rewrite and refactor of the custom HomeView layout, input handling flow, and shoulder-button navigation logic in EmulationStation for the R36S console, alongside detailed architectural documentation mapping the system components.

Working directory: /Users/jacko/Documents/myEmulationStation
Integrity mode: development

## Requirements

### R1. Complete HomeView UI Reimplementation
Reimplement `HomeView` rendering and layout from the ground up. Ensure that it correctly calculates bounds and coordinates so that:
- The background gradient, user avatar, clock/battery widgets, profile name, and all 5 dashboard cards (Browse, Achievements, Profiles, Settings, Continue Playing) render cleanly at the target R36S resolution.
- Dynamic theme layouts and sizes are respected without elements rendering off-screen, overlapping, or appearing blank.

### R2. Refactored Input Handling & View Navigation
Overhaul input processing inside `Window`, `ViewController`, and related files:
- **Start Button**: Ensure pressing the Start button opens the main menu (`GuiMenu`) immediately from both Home View and System View without being swallowed, blocked, or locked by input culling/locking states.
- **Shoulder Buttons (L/R Bumpers)**: Reimplement shoulder button navigation so that bumpers *only* switch between the Home Menu and the Systems View. They must not transition to the Game List view or cause a crash.

### R3. Comprehensive Architectural Documentation (.md files)
Create detailed documentation in the repository (as multiple `.md` files) explaining:
- The original vs. refactored implementation of `HomeView`, input routing, and navigation logic.
- An integration/connection map showing how `Window`, `ViewController`, `HomeView`, `SystemView`, and `InputConfig` interact.
- Diagrams or step-by-step logic flows for input events.

### R4. Compilation & Port Logging
Ensure that the codebase builds cleanly. When running, the log redirection logic must write execution logs to `/roms/ports/es_log.txt` to facilitate debugging on the target console.

## Acceptance Criteria

### Home View & Navigation Correctness
- [ ] EmulationStation compiles cleanly without errors.
- [ ] Home View renders all dashboard elements (avatar, cards, clock/battery widgets) correctly.
- [ ] Pressing the Start button on either Home View or System View displays the Main Menu (`GuiMenu`).
- [ ] Pressing the L/R bumpers cycles strictly between the Home Menu and the Systems View, and never opens the Game List.
- [ ] Log output is written to `/roms/ports/es_log.txt`.

### Documentation Files
- [ ] Multiple detailed `.md` files are created in the workspace detailing the original code, the working code, and how they connect.
