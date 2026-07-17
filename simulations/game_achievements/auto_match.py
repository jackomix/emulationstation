import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import json

real_img_path = "/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png"
font_path = "/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/Acre.otf"

real_gray = cv2.imread(real_img_path, cv2.IMREAD_GRAYSCALE)

def extract_word_bounds(gray_img, crop_box, is_dark_text=False):
    x, y, w, h = crop_box
    roi = gray_img[y:y+h, x:x+w]
    
    if is_dark_text:
        # text is dark on light bg
        _, thresh = cv2.threshold(roi, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    else:
        # text is light on dark bg
        _, thresh = cv2.threshold(roi, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
        
    points = cv2.findNonZero(thresh)
    if points is None:
        return None
    
    bx, by, bw, bh = cv2.boundingRect(points)
    # Return absolute coordinates
    return (x + bx, y + by, bw, bh)

def find_best_font_size(text, target_width):
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
        
        diff = abs(sim_w - target_width)
        if diff < best_diff:
            best_diff = diff
            best_size = size
            
    return best_size

# 1. Define rough crop boxes to search inside
# (X, Y, W, H, is_dark_text, literal_text)
elements_to_find = {
    "TETRIS": (200, 5, 100, 40, False, "TETRIS"),
    "Achievements (softcore):": (100, 40, 300, 30, False, "Achievements (softcore):"),
    "Achievements (hardcore):": (100, 60, 300, 30, False, "Achievements (hardcore):"),
    "Points:": (100, 80, 100, 30, False, "Points:"),
    "5% complete": (30, 205, 120, 30, False, "5% complete"),
    "ACHIEVEMENTS": (50, 235, 150, 30, True, "ACHIEVEMENTS"),
    "PLAY HISTORY": (320, 235, 150, 30, False, "PLAY HISTORY"),
    "Double": (60, 265, 100, 30, False, "Double"),
    "Triple": (60, 325, 100, 30, False, "Triple"),
    "Tetris": (60, 400, 100, 30, False, "Tetris"),
    "LAUNCH": (200, 430, 100, 30, False, "LAUNCH"),
    "BACK": (280, 430, 100, 30, False, "BACK")
}

optimized_config = {}

for name, (cx, cy, cw, ch, is_dark, text) in elements_to_find.items():
    bounds = extract_word_bounds(real_gray, (cx, cy, cw, ch), is_dark)
    if bounds:
        x, y, w, h = bounds
        font_size = find_best_font_size(text, w)
        optimized_config[name] = {
            "x": int(x),
            "y": int(y),
            "w": int(w),
            "h": int(h),
            "font_size": int(font_size)
        }
    else:
        print(f"Failed to find bounds for {name}")

# Print code to easily copy-paste into sim.py
print("\n--- OPTIMIZED PARAMETERS ---")
for k, v in optimized_config.items():
    print(f"{k:<25}: X={v['x']:<3} Y={v['y']:<3} | Font Size={v['font_size']}")
