import cv2
import numpy as np

# Load the real screenshot
real_img_path = "/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png"
img = cv2.imread(real_img_path)
overlay = img.copy()

# The regions we crop and analyze
regions = {
    "TETRIS": (150, 10, 200, 40, (0, 255, 0)),             # Green
    "Achievements (soft)": (50, 45, 400, 20, (255, 0, 0)),    # Blue
    "Achievements (hard)": (50, 65, 400, 20, (255, 0, 0)),    # Blue
    "Points": (50, 85, 300, 20, (255, 0, 0)),                 # Blue
    "5% complete": (10, 200, 300, 30, (0, 0, 255)),           # Red
    "ACHIEVEMENTS": (20, 230, 300, 30, (255, 255, 0)),        # Cyan
    "PLAY HISTORY": (300, 230, 300, 30, (255, 255, 0)),       # Cyan
    "Double": (60, 270, 200, 30, (255, 0, 255)),              # Magenta
    "Triple": (60, 330, 200, 30, (255, 0, 255)),              # Magenta
    "Tetris": (60, 400, 200, 30, (255, 0, 255)),              # Magenta
    "LAUNCH": (150, 430, 200, 30, (0, 255, 255)),             # Yellow
    "BACK": (250, 430, 200, 30, (0, 255, 255))                # Yellow
}

def extract_word_bounds(gray_img, crop_box, is_dark_text=False):
    x, y, w, h = crop_box
    roi = gray_img[y:y+h, x:x+w]
    if is_dark_text:
        _, thresh = cv2.threshold(roi, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    else:
        _, thresh = cv2.threshold(roi, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    points = cv2.findNonZero(thresh)
    if points is None:
        return None
    bx, by, bw, bh = cv2.boundingRect(points)
    return (x + bx, y + by, bw, bh)

gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

for name, (cx, cy, cw, ch, color) in regions.items():
    # Draw ROI search box as dotted line
    cv2.rectangle(overlay, (cx, cy), (cx + cw, cy + ch), (100, 100, 100), 1)
    
    # Draw actual text bounds found by contour analysis
    is_dark = (name == "ACHIEVEMENTS") # achievements tab is selected and has dark text
    bounds = extract_word_bounds(gray, (cx, cy, cw, ch), is_dark)
    if bounds:
        bx, by, bw, bh = bounds
        cv2.rectangle(overlay, (bx, by), (bx + bw, by + bh), color, 2)
        cv2.putText(overlay, name, (bx, max(12, by - 5)), cv2.FONT_HERSHEY_SIMPLEX, 0.35, color, 1)

# Save result in workspace
cv2.imwrite("simulations/game_achievements/bounds_check.png", overlay)
print("Saved bounds_check.png to simulations/game_achievements/bounds_check.png")
