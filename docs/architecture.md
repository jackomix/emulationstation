# Amber-ES Architecture Overview

Amber-ES is a fork of EmulationStation (specifically the version used by AmberElec). It follows the standard EmulationStation architecture but includes various enhancements and service integrations.

## Core Structure

### 1. es-core (Core Engine)
The foundation of the application, handling low-level tasks:
- Window: The main application window and GUI stack management.
- InputManager: Processes controller and keyboard input.
- ThemeData: Parses and manages XML-based themes.
- Settings: Handles configuration persistence (typically es_settings.cfg).
- Log: Global logging utility.

### 2. es-app (Application Logic)
Contains the higher-level logic specific to game management and UI:
- SystemData: Represents an emulated system (e.g., NES, SNES) and its games.
- FileData: Represents a single game/file entry in a system.
- Gamelist: Manages the XML-based game metadata files (gamelist.xml).
- guis/: Contains the logic for various screens (Menus, Scrapers, Settings).

## Service Integrations

### 1. RetroAchievements (es-app/src/RetroAchievements.cpp)
Integration with the RetroAchievements.org API.
- RetroAchievements Class: Handles API communication, authentication, and data parsing.
- UI Components: GuiRetroAchievements, GuiRetroAchievementsSettings, and GuiGameAchievements provide the user interface for viewing progress and configuring the service.
- API Communication: Uses HttpReq for making network requests.

### 2. Embedded HTTP Server (es-app/src/services/)
Amber-ES includes a local HTTP server (HttpServerThread.cpp) and an API (HttpApi.cpp).
- Functionality: Likely used for remote control, status monitoring, or integration with external tools (similar to Batocera's API).

## Key Components for LAHEE Integration
To integrate LAHEE (Local Achievements), the following areas are of primary interest:
- RetroAchievements.cpp: The hardcoded https://retroachievements.org URLs will need to be made configurable or redirected to the local LAHEE server.
- Settings.cpp: New settings for Local RA Server or RA Server URL should be added here.
- GuiRetroAchievementsSettings.cpp: UI to toggle between Official and Local RA servers.
- HttpApi.cpp: Potential for ES to talk directly to LAHEE's management API.
