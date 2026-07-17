import numpy as np
from PIL import Image

r_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
s_img = Image.open("/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/state1.png").convert("RGB")

r = np.array(r_img)
s = np.array(s_img)

def find_text_bounds(arr):
    # Find white pixels (text is mostly white [255,255,255] or close)
    white_mask = (arr[:,:,0] > 200) & (arr[:,:,1] > 200) & (arr[:,:,2] > 200)
    
    ys, xs = np.where(white_mask)
    if len(ys) > 0:
        return np.min(xs), np.max(xs), np.min(ys), np.max(ys)
    return 0,0,0,0

print("White text bounds:")
print("Real:", find_text_bounds(r))
print("Sim:", find_text_bounds(s))
