import numpy as np
from PIL import Image, ImageDraw, ImageFont

real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
sim_img = Image.open("/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/state1.png").convert("RGB")

r_arr = np.array(real_img)
s_arr = np.array(sim_img)

font_path = "/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/Acre.otf"

def find_text_bounds(arr, y1, y2, x1, x2, thresh, invert=False):
    crop = arr[y1:y2, x1:x2]
    if invert:
        # Looking for dark text on light background
        mask = (crop[:,:,0] < thresh) & (crop[:,:,1] < thresh) & (crop[:,:,2] < thresh)
    else:
        # Looking for light text on dark background
        mask = (crop[:,:,0] > thresh) & (crop[:,:,1] > thresh) & (crop[:,:,2] > thresh)
        
    ys, xs = np.where(mask)
    if len(ys) == 0:
        return None
    return (x1 + np.min(xs), y1 + np.min(ys), np.max(xs) - np.min(xs) + 1, np.max(ys) - np.min(ys) + 1)

elements = {
    "TETRIS": (10, 40, 200, 400, 200, False),
    "Achievements (softcore):": (45, 60, 100, 300, 150, False),
    "Achievements (hardcore):": (65, 80, 100, 300, 150, False),
    "Points:": (85, 105, 180, 300, 150, False),
    "5% complete": (205, 225, 30, 200, 200, False),
    "ACHIEVEMENTS": (230, 250, 75, 250, 150, True),  # Active tab, text is dark
    "PLAY HISTORY": (230, 250, 300, 500, 150, False),
    "Double": (270, 290, 70, 200, 150, True), # Active row, text is dark
    "Triple": (340, 360, 70, 200, 150, False),
    "Tetris": (405, 425, 70, 200, 150, False),
    "LAUNCH": (430, 455, 200, 300, 150, False),
    "BACK": (430, 455, 300, 450, 150, False)
}

print(f"{'Element':<25} | {'Real (X, Y, W, H)':<20} | {'Sim (X, Y, W, H)':<20} | {'DX, DY, DW, DH'}")
print("-" * 90)

for name, (y1, y2, x1, x2, thresh, invert) in elements.items():
    real_bounds = find_text_bounds(r_arr, y1, y2, x1, x2, thresh, invert)
    sim_bounds = find_text_bounds(s_arr, y1, y2, x1, x2, thresh, invert)
    
    r_str = f"{real_bounds}" if real_bounds else "None"
    s_str = f"{sim_bounds}" if sim_bounds else "None"
    
    if real_bounds and sim_bounds:
        rx, ry, rw, rh = real_bounds
        sx, sy, sw, sh = sim_bounds
        dx, dy, dw, dh = sx - rx, sy - ry, sw - rw, sh - rh
        diff_str = f"{dx:>3}, {dy:>3}, {dw:>3}, {dh:>3}"
    else:
        diff_str = "N/A"
        
    print(f"{name[:25]:<25} | {r_str:<20} | {s_str:<20} | {diff_str}")

# Find best font size for a given element
def find_best_font_size(text, target_w):
    best_size = 10
    best_diff = 999
    for size in range(10, 50):
        font = ImageFont.truetype(font_path, size)
        test_img = Image.new("RGB", (300, 50), "black")
        draw = ImageDraw.Draw(test_img)
        draw.text((10, 10), text, font=font, fill="white")
        t_arr = np.array(test_img)
        t_mask = t_arr[:,:,0] > 128
        t_ys, t_xs = np.where(t_mask)
        if len(t_ys) == 0: continue
        sim_w = np.max(t_xs) - np.min(t_xs) + 1
        diff = abs(sim_w - target_w)
        if diff < best_diff:
            best_diff = diff
            best_size = size
    return best_size

print("\nBest PIL font sizes based on width:")
for name, (y1, y2, x1, x2, thresh, invert) in elements.items():
    real_bounds = find_text_bounds(r_arr, y1, y2, x1, x2, thresh, invert)
    if real_bounds:
        _, _, rw, _ = real_bounds
        best_size = find_best_font_size(name, rw)
        print(f"{name:<25}: Best size = {best_size}")
