#!/usr/bin/env python3
import os
import shutil

# Targeted paths but also searching recursively in common locations
SEARCH_DIRS = [
    "/usr/bin",
    "/usr/local/bin",
    "/opt/retroarch",
    "/roms/tools",
    "/roms/bin",
    "."
]

def find_targets():
    found = set()
    known = [
        "/usr/bin/retroarch",
        "/usr/bin/retroarch32",
        "/opt/retroarch/bin/retroarch",
        "/opt/retroarch/bin/retroarch32",
        "retroarch",
        "retroarch32"
    ]
    for k in known:
        if os.path.exists(k):
            found.add(k)
            
    for d in SEARCH_DIRS:
        if not os.path.exists(d):
            continue
        print(f"Searching for retroarch binaries in {d}...")
        for root, dirs, files in os.walk(d):
            if "games" in dirs: dirs.remove("games")
            if "saves" in dirs: dirs.remove("saves")
            
            for file in files:
                if file.startswith("retroarch") and not file.endswith((".cfg", ".txt", ".sh", ".bak", ".lpl", ".so")):
                    found.add(os.path.join(root, file))
    return list(found)

def patch_file(path):
    print(f"Checking {path}...")
    try:
        with open(path, "rb") as f:
            data = f.read()
    except Exception as e:
        print(f"  Error reading file: {e}")
        return False
        
    any_replaced = False
    new_data = data

    # v14: The "Surgical Folder" Method
    # We use a subfolder /laheer/ to match the character length perfectly.
    
    # 21 chars: retroachievements.org
    # 21 chars: 127.0.0.1:8000/laheer
    
    # 29 chars: https://retroachievements.org
    # 29 chars: http://127.0.0.1:8000/laheer/
    
    # 27 chars: media.retroachievements.org
    # 27 chars: 127.0.0.1:8000/laheer/badge
    
    PATTERNS = [
        # Protocol-prefixed media URLs to avoid TLS handshake hang on HTTP port
        (b"https://media.retroachievements.org", b"http://127.0.0.1:8000/laheer/Badge/"), # 35
        (b"http://media.retroachievements.org",  b"http://127.0.0.1:8000/laheer/Badge"),  # 34

        (b"https://retroachievements.org", b"http://127.0.0.1:8000/laheer/"),
        (b"http://retroachievements.org",  b"http://127.0.0.1:8000/laheer"),
        (b"media.retroachievements.org",   b"127.0.0.1:8000/laheer/Badge"),
        (b"retroachievements.org",         b"127.0.0.1:8000/laheer"),
        
        # Fix already bad patches (https:// on port 8000)
        (b"https://127.0.0.1:8000/laheer/Badge", b"http://127.0.0.1:8000/laheer/Badge/"), # 35
        (b"https://127.0.0.1:8000/laheer/badge", b"http://127.0.0.1:8000/laheer/Badge/"), # 35
    ]
    
    # Sort by length descending to match longest first
    PATTERNS.sort(key=lambda x: len(x[0]), reverse=True)

    for old, new in PATTERNS:
        if old in new_data:
            count = new_data.count(old)
            print(f"  Found {count} instances of: {old.decode()}")
            
            if len(old) != len(new):
                # This should not happen with our calculated mapping
                print(f"  [ERROR] Length mismatch: {old.decode()} ({len(old)}) vs {new.decode()} ({len(new)})")
                continue
                
            print(f"  Replacing with: {new.decode()}")
            new_data = new_data.replace(old, new)
            any_replaced = True
            
    if any_replaced:
        bak_path = path + ".bak"
        if not os.path.exists(bak_path):
            print(f"  Creating backup at {bak_path}")
            shutil.copy2(path, bak_path)
            
        with open(path, "wb") as f:
            f.write(new_data)
        os.chmod(path, 0o755)
        print(f"  Successfully patched {path}!")
        return True
    
    return False

if __name__ == "__main__":
    print("LAHEE RetroArch Nuclear Patcher (v14 - Surgical Folder Mode) starting...")
    targets = find_targets()
    print(f"Found {len(targets)} potential binaries to check.")
    
    patched_count = 0
    for t in targets:
        if patch_file(t):
            patched_count += 1
            
    print(f"\nPatch Complete. Total files patched: {patched_count}")
