# Comprehensive Development & Debugging Workflow

You must strictly follow these behavioral guardrails during all tasks:

## 1. Regressions
If told a feature worked recently, treat it as a regression from a recent change, not a missing feature. Before proposing a fix, diff against the last known-good commit/tag for the affected files to find the actual change, rather than describing the issue in the abstract.

## 2. Long-running tasks
Don't make the user poll you for CI/build/test status. Use whatever async or background execution mechanism this tool actually provides to check status without blocking, and report back once it completes. If no such mechanism is available for a given task, say so explicitly instead of going idle.

## 3. Failed experiments
Before trying a new approach, remove the failed one. Commit or stash current work first, then revert only the specific files you changed (`git checkout -- <file>`). Don't run `git reset --hard` unless git status is confirmed clean — it discards uncommitted work beyond just the failed attempt.

## 4. Unverified APIs
Before writing code against an undocumented or third-party API, search for existing docs or reference implementations first. If none can be found, say so explicitly and label the implementation as unverified rather than presenting a guess with full confidence. **When integrating with web APIs (like RetroAchievements), NEVER arbitrarily spoof HTTP headers (like User-Agent) without verifying the exact format expected by the server via web search. Servers may use strict format checks (e.g. `App/Version (OS) Core/Version`) and may silently inject corrupt data, dummy achievements (e.g., ID 101000001 "Outdated Emulator"), or trigger IP blocks in response to guessed or malformed headers.**

## 5. Cross-system risk
When a change could affect an adjacent system (e.g. online/offline sync, shared state), trace the actual dependency graph in the code before claiming it's safe. State which files/modules were checked — a verbal assurance alone isn't sufficient.

## 6. Evidence over inference
Before declaring a bug fixed, run the code, reproduce the issue, or check logs. "This should work because..." is not sufficient to close an issue.

## 7. Data Flow Debugging
When debugging data serialization, proxy servers, or API responses, STOP theorizing in the abstract. Immediately dump and inspect the literal raw input data and output data (e.g., using a quick Python script or `curl`) before reading the code. Many "complex" bugs are instantly solved by looking at the actual payloads.

## 8. Context integrity
Long sessions can lose track of earlier context even within a large context window. Every ~15 turns, or before a complex multi-file task, summarize current state and decisions in 5–10 lines and confirm accuracy before continuing.

## 9. UI Framework Constraints
Never assume a UI framework natively supports real-time layout updates or data-binding. Always verify the rendering lifecycle by searching the codebase (e.g., looking for `removeRow` or `clear` methods). If dynamic behavior is required, test it, and implement async reloads (like `postToUiThread`) instead of assuming an exit/reopen is acceptable.

## 13. EmulationStation GUI Invariants
When creating or modifying `GuiComponent` subclasses, you must adhere to these rules:
- **Constructor Lifecycle**: Always initialize and instantiate all child components (grids, components, backgrounds) BEFORE calling `setSize()`. Calling `setSize()` triggers `onSizeChanged()`, which will fail or compute size `(0, 0)` if the child objects are still null.
- **Centering & Positioning**: Custom menus or popups do not center automatically. Calculate center offsets and call `setPosition()` at the end of the constructor using `Renderer::getScreenWidth()` and `Renderer::getScreenHeight()`.
- **Theme Integration**: Integrate with the active theme. Fetch `ThemeData::getMenuTheme()` and use its styles (e.g., `theme->Background.color`, `theme->Text.color`, theme fonts) instead of hardcoding raw color hex values or font levels.
- **Resolution Independence**: Never use absolute pixel bounds for items (e.g., card size `140x180`). Calculate dimensions relative to the screen dimensions (e.g., `Renderer::getScreenWidth() * 0.2f`). Remember that testing is run on the R36S device which uses a low-resolution 480p screen (640x480), so layout elements must scale down elegantly without text clipping or overlaps.
- **Safe Deletion**: Never call `delete this` directly inside the `update()` loop. For deferred or automatic GUI closures (like menu bypasses), wrap the deletion inside UI thread dispatching: `mWindow->postToUiThread([this]() { ... delete this; });`.
- **ComponentListRow Spacing**: `ComponentListRow` does not automatically inject horizontal margins or spacing between elements. Always insert an empty spacer component (e.g. `GuiComponent` with size `Renderer::getScreenWidth() * 0.015f` to `0.02f`) between adjacent elements in a row.
- **Avatar & Icon Sizing**: Default menu icons scale to `theme->Text.font->getLetterHeight() * 1.25f`. For prominent visual components (e.g., profile avatars in headers), scale up to `2.5f` times the letter height.

## 10. Deployment & R36S Device Sync Workflow
When required to build and deploy changes to the R36S device, use the following lock-free sequence to upload the binary without encountering "Text file busy" errors:
1. **Push**: Commit and push changes to the active branch (e.g. `attempt2`) to trigger the GitHub Actions (GHA) build.
2. **Build**: Monitor the GHA build (`gh run list --branch <branch>`). Average build duration is **211 seconds (~3.5 minutes)**. When waiting for builds, schedule a single timer for 210 seconds. If build not done yet, wait 20 seconds more. Once complete, download the artifact locally: 
   `rm -rf /tmp/es-artifact && gh run download <run_id> --name emulationstation-r36s --dir /tmp/es-artifact`
3. **Upload**: Upload the new binary to the `/tmp` directory on the device via SCP (which avoids lock issues since `/tmp/emulationstation` is not running):
   `scp -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no /tmp/es-artifact/EmulationStation/emulationstation ark@192.168.18.20:/tmp/emulationstation`
4. **Deploy**: Move the binary to its destination on the ROMs partition (which unlinks the running file safely) and make it executable:
   `ssh -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20 "mv /tmp/emulationstation /roms/EmulationStation/emulationstation && chmod +x /roms/EmulationStation/emulationstation"`
5. **Restart**: The user can restart EmulationStation via the UI menu, or you can trigger a restart remotely by killing the process:
   `ssh -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20 "killall emulationstation"`

## 11. Smart Session & Context Management (Keep Agent Smart)
- **Limit Scope**: Limit sessions to 3–10 files. Avoid editing files > 500 lines if possible.
- **Plan First**: Output file scope and plan before execution.
- **Context Target**: Keep context usage under 10–20%. Monitor via `/context` in Antigravity CLI.
- **Handoff Workflow**: After small cluster of work, write progress to [HANDOFF.md](file:///Users/jacko/Documents/MyEmulationStation/HANDOFF.md) and stop. Next agent session resumes from there.

## 12. Capturing Device Screenshots
When visual verification is required on the R36S device, capture the framebuffer directly over SSH:
- **Policy**: Do not capture screenshots automatically after every restart or deploy unless layout debugging is needed or explicitly requested. EmulationStation restarts often return to default screens (e.g., Profile Selection), so automatic screenshots may miss target menus. Use screenshots intentionally when the system is in the correct state.
1. **Capture and Download**: Run the following command sequence to record 1 frame from `/dev/fb0` and copy it to the local system:
   `ssh -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20 "sudo ffmpeg -y -f fbdev -i /dev/fb0 -vframes 1 /tmp/screenshot.png" && scp -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20:/tmp/screenshot.png /tmp/screen.png`
2. **Inspect**: Use the `view_file` tool on `/tmp/screen.png` to review the rendered screen layout.
