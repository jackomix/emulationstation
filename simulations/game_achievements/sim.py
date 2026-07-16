import sys
import os
from PIL import Image, ImageDraw, ImageFont

# Path helper
ROOT_DIR = "/Users/jacko/Documents/MyEmulationStation"
FONT_PATH = os.path.join(ROOT_DIR, "resources/opensans_hebrew_condensed_regular.ttf")

# EmulationStation color theme approximation
THEME_BG = (42, 42, 42)
THEME_TEXT = (255, 255, 255)
THEME_TEXT_MUTED = (135, 135, 135)
THEME_SEL_BG = (254, 254, 254)
THEME_SEL_TEXT = (0, 0, 0)
THEME_LIST_BG = (135, 135, 135)
THEME_PROGRESS_BG = (34, 34, 34)
THEME_PROGRESS_FG = (11, 113, 193)
THEME_BUTTON_BG = (42, 42, 42)
THEME_BUTTON_BORDER = (255, 255, 255)

class App:
    def __init__(self):
        self.active_tab = 0
        self.image_size = [60, 60] # Simulated normal size
        self.screen_w = 640
        self.screen_h = 480
        
        # Load real image to crop icons
        try:
            self.real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png")
            # Crop box art
            self.box_art = self.real_img.crop((564, 108, 624, 168))
            # Crop double icon
            self.double_icon = self.real_img.crop((14, 280, 74, 339))
            # Crop triple icon
            self.triple_icon = self.real_img.crop((14, 346, 74, 405))
        except Exception as e:
            print(f"Error cropping icons: {e}")
            self.box_art = None
            self.double_icon = None
            self.triple_icon = None
            
        # Try loading fonts
        try:
            # We'll use different font sizes for different texts
            self.font_title = ImageFont.truetype(FONT_PATH, 18)
            self.font_stats = ImageFont.truetype(FONT_PATH, 14)
            self.font_tabs = ImageFont.truetype(FONT_PATH, 16)
            self.font_list_title = ImageFont.truetype(FONT_PATH, 16)
            self.font_list_desc = ImageFont.truetype(FONT_PATH, 12)
            self.font_buttons = ImageFont.truetype(FONT_PATH, 14)
        except Exception as e:
            print(f"Error loading fonts: {e}")
            self.font_title = ImageFont.load_default()
            self.font_stats = ImageFont.load_default()
            self.font_tabs = ImageFont.load_default()
            self.font_list_title = ImageFont.load_default()
            self.font_list_desc = ImageFont.load_default()
            self.font_buttons = ImageFont.load_default()

    def draw(self, state):
        img = Image.new("RGB", (self.screen_w, self.screen_h), THEME_BG)
        draw = ImageDraw.Draw(img)
        
        # --- UI LAYOUT MATH (R36S 640x480) ---
        TITLE_VERT_PADDING = self.screen_h * 0.0637 # 30.576
        TITLE_WITHSUB_VERT_PADDING = self.screen_h * 0.024 # 11.52
        SUBTITLE_VERT_PADDING = self.screen_h * 0.015 # 7.2
        title_letter_height = 18
        
        # Subtitle lines and height
        subtitle_lines = 8
        subtitle_line_height = 21 # approx 14 * 1.5
        subtitle_h = subtitle_lines * subtitle_line_height
        
        TITLE_HEIGHT = title_letter_height + TITLE_WITHSUB_VERT_PADDING + subtitle_h + SUBTITLE_VERT_PADDING
        
        # Window bounds
        window_w = min(self.screen_h, self.screen_w * 0.90) # 480
        window_x = (self.screen_w - window_w) / 2 # 80
        
        # --- HEADER ---
        # Title
        title_y = window_x + TITLE_WITHSUB_VERT_PADDING # Relative to window? No, just estimating Y
        draw.text((self.screen_w/2 - 30, title_y), "TETRIS", fill=THEME_TEXT, font=self.font_title)
        
        # Subtitle (Stats)
        sub_y_base = title_y + title_letter_height + 5
        draw.text((147, sub_y_base), "Achievements (softcore):", fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((341, sub_y_base), "2/37", fill=THEME_TEXT_MUTED, font=self.font_stats)
        
        draw.text((144, sub_y_base + subtitle_line_height), "Achievements (hardcore):", fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((345, sub_y_base + subtitle_line_height), "0/37", fill=THEME_TEXT_MUTED, font=self.font_stats)
        
        draw.text((210, sub_y_base + subtitle_line_height*2), "Points:", fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((270, sub_y_base + subtitle_line_height*2), "6/480", fill=THEME_TEXT_MUTED, font=self.font_stats)
        
        # Title Image (Box Art)
        if self.box_art:
            img.paste(self.box_art, (int(window_x + window_w - 60 - self.screen_w * 0.012), int(title_y)))
        else:
            draw.rectangle([564, 108, 624, 168], fill=(0,0,255))
            
        # --- PROGRESS BAR ---
        tab_y = TITLE_HEIGHT - 30 - self.screen_h * 0.005
        prog_y = tab_y - 15 - self.screen_h * 0.01
        
        if self.active_tab == 0:
            draw.rectangle([window_x + window_w*0.04, prog_y, window_x + window_w*0.04 + window_w*0.45, prog_y + 15], fill=THEME_PROGRESS_BG)
            draw.rectangle([window_x + window_w*0.04, prog_y, window_x + window_w*0.04 + 20, prog_y + 15], fill=THEME_PROGRESS_FG)
            draw.text((window_x + window_w*0.04 + 60, prog_y), "5% complete", fill=THEME_TEXT, font=self.font_stats)
            
        # --- TABS ---
        tab0_bg = THEME_SEL_BG if self.active_tab == 0 else (45, 45, 45)
        tab0_fg = THEME_SEL_TEXT if self.active_tab == 0 else THEME_TEXT
        draw.rectangle([window_x, tab_y, window_x + window_w/2, tab_y + 30], fill=tab0_bg)
        draw.text((window_x + 60, tab_y + 5), "ACHIEVEMENTS", fill=tab0_fg, font=self.font_tabs)
        
        tab1_bg = THEME_SEL_BG if self.active_tab == 1 else (45, 45, 45)
        tab1_fg = THEME_SEL_TEXT if self.active_tab == 1 else THEME_TEXT
        draw.rectangle([window_x + window_w/2, tab_y, window_x + window_w, tab_y + 30], fill=tab1_bg)
        draw.text((window_x + window_w/2 + 60, tab_y + 5), "PLAY HISTORY", fill=tab1_fg, font=self.font_tabs)
        
        # --- LIST AREA ---
        list_y = TITLE_HEIGHT
        draw.line([window_x, list_y, window_x + window_w, list_y], fill=(255, 255, 255))
        
        if self.active_tab == 0:
            draw.rectangle([window_x, list_y + 1, window_x + window_w, list_y + 65], fill=THEME_LIST_BG)
            if self.double_icon and self.image_size[0] > 0:
                img.paste(self.double_icon, (int(window_x + 14), int(list_y + 4)))
            draw.text((window_x + 88, list_y + 10), "Double", fill=THEME_TEXT, font=self.font_list_title)
            draw.text((window_x + 88, list_y + 35), "Clear two lines at once - Points: 2", fill=THEME_TEXT, font=self.font_list_desc)
            
            draw.line([window_x, list_y + 66, window_x + window_w, list_y + 66], fill=(255, 255, 255))
            
            if self.triple_icon and self.image_size[0] > 0:
                img.paste(self.triple_icon, (int(window_x + 14), int(list_y + 70)))
            draw.text((window_x + 88, list_y + 77), "Triple", fill=THEME_TEXT, font=self.font_list_title)
            draw.text((window_x + 88, list_y + 102), "Clear three lines at once - Points: 3", fill=THEME_TEXT_MUTED, font=self.font_list_desc)
            
            draw.line([window_x, list_y + 132, window_x + window_w, list_y + 132], fill=(255, 255, 255))
            draw.text((window_x + 88, list_y + 143), "Tetris", fill=THEME_TEXT, font=self.font_list_title)
            
        elif self.active_tab == 1:
            draw.text((window_x + 160, list_y + 34), "No play history found", fill=THEME_TEXT_MUTED, font=self.font_list_title)
            
        # --- BUTTONS ---
        btn_y = self.screen_h * 0.901 - 35
        draw.rectangle([window_x + 148, btn_y, window_x + 244, btn_y + 35], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
        draw.text((window_x + 168, btn_y + 8), "LAUNCH", fill=THEME_TEXT, font=self.font_buttons)
        
        draw.rectangle([window_x + 248, btn_y, window_x + 332, btn_y + 35], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
        draw.text((window_x + 273, btn_y + 8), "BACK", fill=THEME_TEXT, font=self.font_buttons)
            
        img.save(f"simulations/game_achievements/{state}.png")

app = App()
app.draw("state1") # Initial achievements

app.active_tab = 1
app.image_size = [0, 0] # Emulate WebImageComponent size collapsing to 0x0
app.draw("state2")

app.active_tab = 0
app.draw("state3") # Return to achievements - icon collapsed

# Create difference overlay
try:
    from PIL import ImageChops
    real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
    sim_img = Image.open("simulations/game_achievements/state1.png").convert("RGB")
    
    h_real = real_img.height
    w_real = real_img.width
    
    # Calculate difference
    diff_img = ImageChops.difference(real_img, sim_img)
    
    # Side-by-side of original side-by-side + diff
    comp_w = w_real * 3
    comp_h = max(h_real, sim_img.height)
    
    comp_img = Image.new("RGB", (comp_w, comp_h))
    comp_img.paste(real_img, (0, 0))
    comp_img.paste(sim_img, (w_real, 0))
    comp_img.paste(diff_img, (w_real * 2, 0))
    
    comp_img.save("comparison.png")
    print("comparison.png with diff generated.")
except Exception as e:
    print(f"Error making comparison: {e}")
