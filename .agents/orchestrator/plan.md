# Project Plan: EmulationStation R36S console refactoring

## Architecture
- Custom fork of EmulationStation for the R36S handheld console.
- HomeView: Custom dashboard screen with 5 cards/tiles (Browse, Achievements, Profiles, Settings, Continue Playing) and top widgets (avatar, clock, battery).
- SystemView: Carousel of emulation systems.
- GuiMenu: Main menu opened by Start button.
- Logger: System logging mechanism.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Exploration & Code Audit | Check existing implementation of HomeView, input routing, and logging in the codebase. Run git status/diff. | None | PLANNED |
| 2 | Refine HomeView UI (R1) | Ensure all 5 cards and widgets render cleanly at R36S resolution and dynamic theme sizes are respected. | Milestone 1 | PLANNED |
| 3 | Input & Navigation (R2) | Fix Start Button unlocking and L/R shoulder bumper navigation to strictly cycle between Home and System Views. | Milestone 1 | PLANNED |
| 4 | Compilation & Logging (R4) | Redirection of log path to `/roms/ports/es_log.txt`. Ensure the codebase compiles cleanly. | Milestone 1 | PLANNED |
| 5 | Documentation (R3) | Create detailed `.md` files explaining original vs refactored implementation, component map, and input logic. | Milestones 2-4 | PLANNED |
| 6 | Verification & Audit | Peer review, challenger validation, and Forensic Audit verification of all requirements. | Milestones 2-5 | PLANNED |

## Code Layout
- `es-app/src/views/HomeView.cpp` / `HomeView.h`: Custom Home View implementation.
- `es-app/src/views/ViewController.cpp` / `ViewController.h`: System View carousel and main navigation.
- `es-core/src/Window.cpp` / `Window.h`: Gui stack and input routing.
- `es-core/src/Log.cpp` / `Log.h`: Logger functionality.
- `es-core/src/Paths.cpp` / `Paths.h`: Paths configuration.

## Interface Contracts
- Standard EmulationStation view transitions and rendering.
- Logger output path override behavior.
