# R36S ArkOS Filesystem Analysis Notes

This document provides a detailed breakdown of findings from analyzing the system structure of the R36S console running ArkOS (`darkosre-r36`), based on the system crawl log at `/Volumes/EASYROMS/ports/filesystem_structure.txt`.

---

## 1. System & Kernel Specifications

* **Distribution Hostname:** `darkosre-r36` (ArkOS custom rebuild/distro for the R36S)
* **Kernel Version:** `Linux darkosre-r36 4.4.189 #1 SMP Mon Apr 27 15:15:32 AEST 2026 aarch64 GNU/Linux`
* **CPU Architecture:** `aarch64` (64-bit ARM)

---

## 2. Partition & Mount Table Layout

The system divides storage between three main partitions on the micro SD card:

| Partition Device | Mount Point | Filesystem | Size | Use % | Mount Options / Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `/dev/mmcblk0p1` | `/boot` | `vfat` (FAT32) | 99M | 47% | Bootloader assets, device tree files (DTBs), kernel image, etc. |
| `/dev/mmcblk0p2` | `/` | `btrfs` | 11G | 49% | Main Linux filesystem. Mounted with `ssd`, `noacl`, `space_cache`. |
| `/dev/mmcblk0p3` | `/roms` | `exfat` | 105G | 1% | **EASYROMS** partition. Mounted with `noatime`, `fmask=0000`, `dmask=0000` (which ensures all files are executable by default). |
| `/dev/mmcblk0p3` | `/opt/system/Tools` | `exfat` | 105G | 1% | Re-mounted directory of the ROMs partition where system scripts/utilities are mapped. |

> [!NOTE]
> Because `/roms` is mounted with `fmask=0000` and `dmask=0000` under `exfat`, executables placed on the SD card (e.g., custom EmulationStation binaries) can be executed directly from `/roms/EmulationStation/emulationstation` without needing standard Linux `chmod +x` file permissions.

---

## 3. Important Executables & Path Directories

### EmulationStation Binaries
* **Main Binary Path:** `/usr/bin/emulationstation/emulationstation`
* **Original Bootloader Wrapper:** `/usr/bin/emulationstation/emulationstation.sh`
* **Backup Binary Wrapper:** `/usr/bin/emulationstation/emulationstation.bak_wrapper` (if modified/installed by setup scripts)

### RetroArch Binaries
* **64-bit Core Path:** `/usr/local/bin/retroarch`
* **32-bit Core Path:** `/usr/local/bin/retroarch32`

---

## 4. Key Configuration Directories & Files

### EmulationStation
* **Global Systems Configuration:** `/etc/emulationstation/es_systems.cfg` (Defines the command line flags, platforms, and folders for emulators)
* **Global Input Configuration:** `/etc/emulationstation/es_input.cfg`
* **Global Themes Directory:** `/etc/emulationstation/themes/`
* **User Settings & States:**
  * Config Folder: `/home/ark/.emulationstation/`
  * Main Settings File: `/home/ark/.emulationstation/es_settings.cfg`
  * Controller Mapping: `/home/ark/.emulationstation/es_input.cfg`
  * Last Input Config: `/home/ark/.emulationstation/es_last_input.cfg`
  * Theme Choice File: `/home/ark/.emulationstation/themesettings`

### RetroArch
* **64-bit Main Configuration:** `/home/ark/.config/retroarch/retroarch.cfg`
* **32-bit Main Configuration:** `/home/ark/.config/retroarch32/retroarch.cfg`
* **Gamepad Autoconfigs (udev):** `/home/ark/.config/retroarch/autoconfig/udev/`
  * Example gamepad profile: `GO-Advance Gamepad.cfg`

---

## 5. Helpful Notes for Redirection / Hooking

1. **Remounting Root Filesystem (`/`)**:
   * To overwrite or modify files in `/usr/bin/emulationstation/`, the root filesystem must be remounted read-write.
   * Standard command: `mount -o remount,rw /` or using the explicit device mapping: `mount -o remount,rw /dev/mmcblk0p2 /`.

2. **Resources Redirect**:
   * Running custom EmulationStation binaries from `/roms/EmulationStation/` changes the canonical folder base. 
   * Make sure to bundle the complete `resources` directory (e.g. `resources/fonts/`, `resources/graphics/` containing `frame.png`) alongside the binary under `/roms/EmulationStation/` to avoid rendering bugs like transparent menus or input re-mappings resetting.

3. **Log Outputs**:
   * ArkOS script executions run from `/roms/ports/` redirect outputs to logs. For system debugging, write log outputs to `/roms/ports/es_log.txt`.
