# Binary Patching & The Surgical Folder Method

Because most emulators hardcode the RetroAchievements server URL (`retroachievements.org`), LAHEE uses binary patching (hex editing) to redirect traffic to the local server.

## The Character-Length Constraint
In a compiled binary, strings are stored in data segments with a fixed length. To replace a URL without corrupting the binary's internal offsets or pointers, the replacement string **must be exactly the same length** as the original, or shorter (padded with null terminators).

## The Surgical Folder Method
LAHEE uses the `/laheer/` subfolder strategy to match the length of official RetroAchievements domains perfectly.

### Mapping Table
| Target | Length | Replacement |
| :--- | :--- | :--- |
| `retroachievements.org` | 21 | `127.0.0.1:8000/laheer` |
| `https://retroachievements.org` | 29 | `http://127.0.0.1:8000/laheer/` |
| `media.retroachievements.org` | 27 | `127.0.0.1:8000/laheer/Badge` |
| `https://media.retroachievements.org` | 35 | `http://127.0.0.1:8000/laheer/Badge/` |

## Patcher Logic (`lahee_patch_ra.py`)
The Python patcher performs a search-and-replace on the emulator binary using these patterns.

### Key Considerations:
1.  **Protocol Correction**: Official RA uses `https`, but LAHEE typically runs on `http`. The patcher replaces `https://` with `http://` while maintaining character count (e.g., by adding a trailing slash).
2.  **Case Sensitivity**: Some emulators are sensitive to the case of the path. LAHEE's routes are designed to handle common variations, but the patcher defaults to `/laheer/`.
3.  **Path Normalization**: The server (`Src/Network.cs`) is configured to handle multiple slashes (e.g., `//dorequest.php`) to accommodate padding added during patching.

## Troubleshooting Patching Issues
- **10-Second Delay**: Often caused by a protocol mismatch (e.g., RetroArch attempting an `https` handshake on an `http` port). The patcher must ensure `https://` is replaced with `http://`.
- **404 Errors**: Ensure the base route in the binary matches the route defined in `Network.cs`.
- **Crashes**: Usually indicate that a string replacement was not length-matched, shifting binary data and breaking pointers.
