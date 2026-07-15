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

## 7. Context integrity
Long sessions can lose track of earlier context even within a large context window. Every ~15 turns, or before a complex multi-file task, summarize current state and decisions in 5–10 lines and confirm accuracy before continuing.

## 8. UI Framework Constraints
Never assume a UI framework natively supports real-time layout updates or data-binding. Always verify the rendering lifecycle by searching the codebase (e.g., looking for `removeRow` or `clear` methods). If dynamic behavior is required, test it, and implement async reloads (like `postToUiThread`) instead of assuming an exit/reopen is acceptable.

## 9. Deployment & R36S Device Sync Workflow
When required to build and deploy changes to the R36S device, use the following lock-free sequence to upload the binary without encountering "Text file busy" errors:
1. **Push**: Commit and push changes to the active branch (e.g. `attempt2`) to trigger the GitHub Actions (GHA) build.
2. **Build**: Monitor the GHA build (`gh run list --branch <branch>`). Once complete, download the artifact locally: 
   `gh run download <run_id> --name emulationstation-r36s --dir /tmp/es-artifact`
3. **Upload**: Upload the new binary to the `/tmp` directory on the device via SCP (which avoids lock issues since `/tmp/emulationstation` is not running):
   `scp -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no /tmp/es-artifact/EmulationStation/emulationstation ark@192.168.18.20:/tmp/emulationstation`
4. **Deploy**: Move the binary to its destination on the ROMs partition (which unlinks the running file safely) and make it executable:
   `ssh -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20 "mv /tmp/emulationstation /roms/EmulationStation/emulationstation && chmod +x /roms/EmulationStation/emulationstation"`
5. **Restart**: The user can restart EmulationStation via the UI menu, or you can trigger a restart remotely by killing the process:
   `ssh -i ~/.ssh/id_ed25519_antigravity -o StrictHostKeyChecking=no ark@192.168.18.20 "killall emulationstation"`

## 10. Smart Session & Context Management (Keep Agent Smart)
- **Limit Scope**: Limit sessions to 3–10 files. Avoid editing files > 500 lines if possible.
- **Plan First**: Output file scope and plan before execution.
- **Context Target**: Keep context usage under 10–20%. Monitor via `/context` in Antigravity CLI.
- **Handoff Workflow**: After small cluster of work, write progress to [HANDOFF.md](file:///Users/jacko/Documents/MyEmulationStation/HANDOFF.md) and stop. Next agent session resumes from there.

