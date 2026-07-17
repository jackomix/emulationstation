import numpy as np
from PIL import Image, ImageDraw, ImageFont

# Load real image
img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
arr = np.array(img)

# Isolate the word "ACHIEVEMENTS" tab. It's located around Y=230-245.
# Let's crop to Y=230:250, X=75:200 and find the exact white pixel bounds.
crop = arr[230:250, 75:200]
white_mask = (crop[:,:,0] > 200) & (crop[:,:,1] > 200) & (crop[:,:,2] > 200)
ys, xs = np.where(white_mask)

if len(ys) == 0:
    print("Could not find text in crop.")
    exit(1)

real_h = np.max(ys) - np.min(ys) + 1
real_w = np.max(xs) - np.min(xs) + 1
print(f"Real 'ACHIEVEMENTS' pixel size: {real_w}x{real_h}")

font_path = "/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/Acre.otf"

# Binary/linear search for the best font size
best_size = 10
best_diff = 999
best_w, best_h = 0, 0

for size in range(10, 40):
    font = ImageFont.truetype(font_path, size)
    # Render text on a blank canvas to measure exact pixel bounds
    test_img = Image.new("RGB", (200, 50), "black")
    draw = ImageDraw.Draw(test_img)
    draw.text((10, 10), "ACHIEVEMENTS", font=font, fill="white")
    
    t_arr = np.array(test_img)
    t_mask = t_arr[:,:,0] > 128
    t_ys, t_xs = np.where(t_mask)
    if len(t_ys) == 0:
        continue
        
    sim_h = np.max(t_ys) - np.min(t_ys) + 1
    sim_w = np.max(t_xs) - np.min(t_xs) + 1
    
    diff = abs(sim_h - real_h) + abs(sim_w - real_w)
    if diff < best_diff:
        best_diff = diff
        best_size = size
        best_w = sim_w
        best_h = sim_h

print(f"Best PIL font size for Tabs: {best_size} (renders at {best_w}x{best_h})")

# Do the same for list title, e.g. "Double" at Y=280:300, X=80:160
crop2 = arr[280:300, 80:160]
white_mask2 = (crop2[:,:,0] > 200) & (crop2[:,:,1] > 200) & (crop2[:,:,2] > 200)
ys2, xs2 = np.where(white_mask2)
if len(ys2) > 0:
    r_h2 = np.max(ys2) - np.min(ys2) + 1
    r_w2 = np.max(xs2) - np.min(xs2) + 1
    print(f"Real 'Double' pixel size: {r_w2}x{r_h2}")
    
    best_size2 = 10
    best_diff2 = 999
    
    for size in range(10, 40):
        font = ImageFont.truetype(font_path, size)
        test_img = Image.new("RGB", (200, 50), "black")
        draw = ImageDraw.Draw(test_img)
        draw.text((10, 10), "Double", font=font, fill="white")
        t_arr = np.array(test_img)
        t_mask = t_arr[:,:,0] > 128
        t_ys, t_xs = np.where(t_mask)
        if len(t_ys) == 0: continue
        sim_h = np.max(t_ys) - np.min(t_ys) + 1
        sim_w = np.max(t_xs) - np.min(t_xs) + 1
        diff = abs(sim_h - r_h2) + abs(sim_w - r_w2)
        if diff < best_diff2:
            best_diff2 = diff
            best_size2 = size
    print(f"Best PIL font size for List Title: {best_size2}")
else:
    print("Could not find 'Double'")

# Do the same for stats "Points:" at Y=90:105, X=200:270
crop3 = arr[90:105, 200:270]
white_mask3 = (crop3[:,:,0] > 150) & (crop3[:,:,1] > 150) & (crop3[:,:,2] > 150)
ys3, xs3 = np.where(white_mask3)
if len(ys3) > 0:
    r_h3 = np.max(ys3) - np.min(ys3) + 1
    r_w3 = np.max(xs3) - np.min(xs3) + 1
    print(f"Real 'Points:' pixel size: {r_w3}x{r_h3}")
    
    best_size3 = 10
    best_diff3 = 999
    
    for size in range(10, 40):
        font = ImageFont.truetype(font_path, size)
        test_img = Image.new("RGB", (200, 50), "black")
        draw = ImageDraw.Draw(test_img)
        draw.text((10, 10), "Points:", font=font, fill="white")
        t_arr = np.array(test_img)
        t_mask = t_arr[:,:,0] > 128
        t_ys, t_xs = np.where(t_mask)
        if len(t_ys) == 0: continue
        sim_h = np.max(t_ys) - np.min(t_ys) + 1
        sim_w = np.max(t_xs) - np.min(t_xs) + 1
        diff = abs(sim_h - r_h3) + abs(sim_w - r_w3)
        if diff < best_diff3:
            best_diff3 = diff
            best_size3 = size
    print(f"Best PIL font size for Stats: {best_size3}")
else:
    print("Could not find 'Points:'")

