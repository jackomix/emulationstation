#!/usr/bin/env python3
import os
import shutil

# Match the search directories used in the patcher
SEARCH_DIRS = [
    "/usr/bin",
    "/usr/local/bin",
    "/opt/retroarch",
    "/roms/tools",
    "/roms/bin",
    "."
]

def find_backups():
    found = []
    # Search recursively for .bak files of retroarch binaries
    for d in SEARCH_DIRS:
        if not os.path.exists(d):
            continue
        print(f"Searching for backups in {d}...")
        for root, dirs, files in os.walk(d):
            # Prune directories to speed up search
            if "games" in dirs: dirs.remove("games")
            if "saves" in dirs: dirs.remove("saves")
            
            for file in files:
                # Only look for backups of the actual binaries
                if file.startswith("retroarch") and file.endswith(".bak"):
                    # Strict check: the original name (without .bak) must not be a config/script/etc
                    original_name = file[:-4]
                    if original_name.endswith((".cfg", ".txt", ".sh", ".lpl", ".so", ".png", ".zip")):
                        continue
                        
                    bak_path = os.path.join(root, file)
                    original_path = os.path.join(root, original_name)
                    if os.path.exists(original_path):
                        found.append((bak_path, original_path))
    return found

def unpatch_file(bak_path, original_path):
    print(f"Restoring {original_path} from backup...")
    try:
        shutil.copy2(bak_path, original_path)
        os.chmod(original_path, 0o755)
        # We keep the backup just in case, or should we delete it? 
        # Usually it's cleaner to delete it if unpatching is successful.
        os.remove(bak_path)
        print(f"  Successfully unpatched {original_path}!")
        return True
    except Exception as e:
        print(f"  Error restoring {original_path}: {e}")
        return False

if __name__ == "__main__":
    print("LAHEE RetroArch Nuclear Unpatcher starting...")
    targets = find_backups()
    
    if not targets:
        print("No RetroArch backups found. Nothing to unpatch.")
    else:
        print(f"Found {len(targets)} backup(s).")
        unpatched_count = 0
        for bak_path, original_path in targets:
            if unpatch_file(bak_path, original_path):
                unpatched_count += 1
        
        print(f"\nNuclear Unpatch Complete. Total files restored: {unpatched_count}")
