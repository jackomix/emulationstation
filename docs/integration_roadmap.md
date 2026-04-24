# Amber-ES / LAHEE Integration Roadmap

This document outlines the strategy for merging the capabilities of LAHEE (Local Achievements) into the Amber-ES (EmulationStation) frontend.

## Goals
1.  **Offline Visibility**: View achievement progress in ES without an internet connection.
2.  **Unified Management**: Configure local RA sets and users directly from the ES menu.
3.  **Local Metadata**: Use LAHEE's local JSON data to enrich the ES gamelist display.

## Planned Changes

### Phase 1: Redirection (The "Proxy" Phase)
- **Goal**: Make Amber-ES point to the local LAHEE server.
- **Tasks**:
    - Modify es-app/src/RetroAchievements.cpp to use a configurable base URL.
    - Add RetroAchievementsServerURL to the ES Settings menu.
    - Test compatibility with LAHEE's API responses.

### Phase 2: User Synchronization
- **Goal**: Automatically sync the ES login with LAHEE.
- **Tasks**:
    - When a user logs in via ES, ensure the session is registered in LAHEE's UserManager.
    - Handle the AutoSessionOnSingleUser mode for zero-configuration setups.

### Phase 3: Enhanced UI
- **Goal**: Surface local-specific data in the ES UI.
- **Tasks**:
    - Display "Local Only" achievement tags.
    - Show the presence of modified or custom achievement sets.
    - Integrate the "Fetch" command into the ES UI to download sets from the real RA server into LAHEE's data folder.

## Integration Benefits
By combining these two projects, the user gets a seamless "Console-like" experience on handhelds like the R36S, where achievements work identically whether they are online or offline, and the UI remains consistent across the frontend and the emulator.
