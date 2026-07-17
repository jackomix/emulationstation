import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont

# Load real image in grayscale
real_img_path = "/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png"
sim_img_path = "/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/state1.png"
font_path = "/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/Acre.otf"

real_gray = cv2.imread(real_img_path, cv2.IMREAD_GRAYSCALE)
sim_gray = cv2.imread(sim_img_path, cv2.IMREAD_GRAYSCALE)

def get_tight_bbox(gray_img, roi):
    x, y, w, h = roi
    crop = gray_img[y:y+h, x:x+w]
    
    # Use Otsu's thresholding to separate text from background
    # This works whether text is light-on-dark or dark-on-light
    _, thresh = cv2.threshold(crop, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    
    # We want text to be white. If the border is white, it means the background was light and text dark.
    # Check corners to see what the background color is.
    bg_color = int(thresh[0,0]) + int(thresh[0,-1]) + int(thresh[-1,0]) + int(thresh[-1,-1])
    if bg_color > 255 * 2: # Background is white (light bg, dark text)
        thresh = cv2.bitwise_not(thresh) # Invert so text is white
        
    points = cv2.findNonZero(thresh)
    if points is None: return None
    bx, by, bw, bh = cv2.boundingRect(points)
    
    return (x + bx, y + by, bw, bh)

regions = {
    "TETRIS": (150, 10, 200, 40),
    "Achievements (soft)": (50, 45, 400, 20),
    "Achievements (hard)": (50, 65, 400, 20),
    "Points": (50, 85, 300, 20),
    "5% complete": (10, 200, 300, 30),
    "ACHIEVEMENTS": (20, 230, 300, 30),
    "PLAY HISTORY": (300, 230, 300, 30),
    "Double": (60, 270, 200, 30),
    "Triple": (60, 330, 200, 30),
    "Tetris": (60, 400, 200, 30),
    "LAUNCH": (150, 430, 200, 30),
    "BACK": (250, 430, 200, 30)
}

print(f"{'Element':<20} | {'Real (X,Y,W,H)':<20} | {'Sim (X,Y,W,H)':<20} | {'DX,DY,DW,DH'}")
print("-" * 85)

for name, roi in regions.items():
    r_bbox = get_tight_bbox(real_gray, roi)
    s_bbox = get_tight_bbox(sim_gray, roi)
    
    if r_bbox and s_bbox:
        rx, ry, rw, rh = r_bbox
        sx, sy, sw, sh = s_bbox
        dx, dy, dw, dh = sx-rx, sy-ry, sw-rw, sh-rh
        print(f"{name[:20]:<20} | {str(r_bbox):<20} | {str(s_bbox):<20} | {dx:>3},{dy:>3},{dw:>3},{dh:>3}")
    else:
        print(f"{name[:20]:<20} | {'None':<20} | {'None':<20} | N/A")

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

print("\nBest PIL Font Sizes:")
for name, roi in regions.items():
    r_bbox = get_tight_bbox(real_gray, roi)
    if r_bbox:
        rx, ry, rw, rh = r_bbox
        bs = find_best_font_size(name, rw)
        print(f"{name:<20}: Best Size = {bs} (Target width={rw})")
