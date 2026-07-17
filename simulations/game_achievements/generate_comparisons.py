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
        "name": "all_fixes_on",
        "font_scale": 1.0, 
        "header_align": "center_subtitle", 
        "y_offset": 6,
        "title_scale": 0.8,
        "use_default_font": True,
        "fix_tab_centering": True,
        "fix_padding": True,
        "fix_progress_height": True
    },
    {
        "name": "all_fixes_off",
        "font_scale": 1.0, 
        "header_align": "grid", 
        "y_offset": 6,
        "title_scale": 1.0,
        "use_default_font": False,
        "fix_tab_centering": False,
        "fix_padding": False,
        "fix_progress_height": False
    },
    {
        "name": "fixes_on_but_title_large",
        "font_scale": 1.0, 
        "header_align": "center_subtitle", 
        "y_offset": 6,
        "title_scale": 1.0,
        "use_default_font": True,
        "fix_tab_centering": True,
        "fix_padding": True,
        "fix_progress_height": True
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
