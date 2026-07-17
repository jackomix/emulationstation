import os
from PIL import Image, ImageChops
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from sim import App

os.makedirs("simulations/game_achievements/comparison_tests", exist_ok=True)

try:
    real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
except:
    real_img = None
    print("Warning: Real image not found, will not generate diffs.")

combinations = [
    {
        "name": "cheats_on_hardcoded",
        "use_cpp_math": False
    },
    {
        "name": "true_cpp_math_enabled",
        "use_cpp_math": True
    }
]

for params in combinations:
    name = params.pop("name")
    app = App(**params)
    sim_img = app.draw("state1_temp")
    
    filename = f"simulations/game_achievements/comparison_tests/comp_{name}.png"
    
    if real_img:
        diff_img = ImageChops.difference(real_img, sim_img)
        comp_img = Image.new("RGB", (real_img.width * 3, max(real_img.height, sim_img.height)))
        comp_img.paste(real_img, (0, 0))
        comp_img.paste(sim_img, (real_img.width, 0))
        comp_img.paste(diff_img, (real_img.width * 2, 0))
        comp_img.save(filename)
    else:
        sim_img.save(filename)
        
    print(f"Generated {filename}")
