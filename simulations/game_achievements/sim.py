import sys
import os
from PIL import Image, ImageDraw, ImageFont

# Path helpers
ROOT_DIR = "/Users/jacko/Documents/MyEmulationStation"
FONT_PATH_DEFAULT = os.path.join(ROOT_DIR, "resources/opensans_hebrew_condensed_regular.ttf")
# Pointing to the specific theme font based on es_settings.cfg and theme.xml
FONT_PATH_ACRE = os.path.join(ROOT_DIR, "simulations/game_achievements/Acre.otf")

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
    def __init__(self, **kwargs):
        self.active_tab = 0
        self.image_size = [60, 60] 
        self.screen_w = 640
        self.screen_h = 480
        
        self.use_cpp_math = kwargs.get("use_cpp_math", False)
        
        # --- C++ Text Sizing Logic Ported to Python ---
        min_dim = min(self.screen_w, self.screen_h)
        
        # Override scale to 1.0 based on visual evidence
        self.menu_font_scale = 1.0

        # Font size definitions from Font.h (base * scale)
        self.size_mini = int((0.030 * min_dim) * self.menu_font_scale)   # ~14px
        self.size_small = int((0.035 * min_dim) * self.menu_font_scale)  # ~16px
        self.size_medium = int((0.045 * min_dim) * self.menu_font_scale) # ~21px
        self.size_large = int((0.085 * min_dim) * self.menu_font_scale)  # ~40px
        
        # Load real image to crop icons
        try:
            self.real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png")
            self.box_art = self.real_img.crop((564, 108, 624, 168))
            self.double_icon = self.real_img.crop((14, 280, 74, 339))
            self.triple_icon = self.real_img.crop((14, 346, 74, 405))
        except Exception as e:
            print(f"Error cropping icons: {e}")
            self.box_art = None
            self.double_icon = None
            self.triple_icon = None
            
        # Try loading the specific theme font, fallback to default if missing
        font_to_use = FONT_PATH_ACRE if os.path.exists(FONT_PATH_ACRE) else FONT_PATH_DEFAULT
        print(f"Using font: {font_to_use}")

        try:
            self.font_title = ImageFont.truetype(font_to_use, self.size_large)
            self.font_tabs = ImageFont.truetype(font_to_use, self.size_medium)
            self.font_list_title = ImageFont.truetype(font_to_use, self.size_medium)
            self.font_buttons = ImageFont.truetype(font_to_use, self.size_small)
            
            # Use OpenSans for stats, progress text, and list descriptions
            self.font_stats = ImageFont.truetype(FONT_PATH_DEFAULT, self.size_small)
            self.font_list_desc = ImageFont.truetype(FONT_PATH_DEFAULT, self.size_small)
        except Exception as e:
            print(f"Error loading fonts, falling back to default: {e}")
            self.font_title = ImageFont.load_default()
            self.font_stats = ImageFont.load_default()
            self.font_tabs = ImageFont.load_default()
            self.font_list_title = ImageFont.load_default()
            self.font_list_desc = ImageFont.load_default()
            self.font_buttons = ImageFont.load_default()

    def draw(self, state):
        img = Image.new("RGB", (self.screen_w, self.screen_h), THEME_BG)
        draw = ImageDraw.Draw(img)
        
        # --- TRUE C++ MACROS ---
        win_w = min(self.screen_h * 1.125, self.screen_w * 0.90)  # 540
        
        if self.use_cpp_math:
            # The ENGINE BUG (Proven by C++ compilation):
            # width macro evaluated to 480.
            # Initial TITLE_HEIGHT was 71.576
            # iw = 71.576 / 480 = 0.1491
            # Col 0 (Text) = 85.09%. Col 1 (Image) = 14.91%.
            # The grid gets forced to 640px wide by the fullScreenMenus() early return.
            actual_menu_w = self.screen_w
            
            text_col_w = actual_menu_w * 0.8509
            
            # Title is ALIGN_CENTER in text_col_w (width 544.5)
            # Center is 272.25. For width 65, left X = 272.25 - 32.5 = 239.75
            hx = (text_col_w / 2) - 32.5
            
            # Stats are ALIGN_CENTER in text_col_w
            # Assuming stats text width ~180px, sub_x = 272.25 - 90 = 182.25
            sub_x = (text_col_w / 2) - 90
            
            # Image is in Col 1 (Starts at 544.5, width 95.5).
            # ComponentGrid centers it if resize=false. Center is 592.25.
            # Minus half box art width (30.5) = 561.75
            img_x = 561.75
        else:
            menu_x = (self.screen_w - win_w) / 2  # 50 (incorrect baseline)
            hx = 239
            sub_x = 199
            img_x = 58 + 551.12
            
        hy = 0
        
        draw.text((hx, hy + 6), "TETRIS", fill=THEME_TEXT, font=self.font_title)
        
        sub_y_base = hy + 49.1
        draw.text((sub_x, sub_y_base + 6), "Achievements (softcore): \t 2/37", fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((sub_x, sub_y_base + 28), "Achievements (hardcore): \t 0/37", fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((sub_x, sub_y_base + 50), "Points: \t 6/480", fill=THEME_TEXT_MUTED, font=self.font_stats)
        
        img_y = hy + 94.46
        if self.box_art:
            img.paste(self.box_art, (int(img_x), int(img_y)))
        else:
            draw.rectangle([img_x, img_y, img_x + 61.19, img_y + 87.42], fill=(0,0,255))
            
        # --- PROGRESS BAR ---
        if self.use_cpp_math:
            # We know Col 0 (text column) is 85.09% of 640.
            # 640 * 0.8509 = 544.576
            prog_x = actual_menu_w * 0.04
            prog_y = 228.372 - 17.6 - (self.screen_h * 0.01) 
            # Width uses text column width scaled by percent
            prog_w = (544.576) * 0.45
            prog_h = 17.6
        else:
            prog_x, prog_y = 25.6, 201.572
            prog_w = 122
            prog_h = 22
            
        if self.active_tab == 0:
            draw.rectangle([prog_x, prog_y, prog_x+prog_w, prog_y+prog_h], fill=THEME_PROGRESS_BG)
            draw.rectangle([prog_x, prog_y, prog_x+10, prog_y+prog_h], fill=THEME_PROGRESS_FG)
            draw.text((prog_x + 30, prog_y + 3), "5% complete", fill=THEME_TEXT, font=self.font_stats)
            
        # --- TABS ---
        tab_y = 228.372
        if self.use_cpp_math:
            tab_w = actual_menu_w * 0.5
            tab0_x1 = 0
            tab0_x2 = tab_w
            tab1_x1 = tab_w
            tab1_x2 = actual_menu_w
        else:
            tab_w = win_w * 0.5
            tab0_x1, tab0_x2 = 0, 270
            tab1_x1, tab1_x2 = 270, 540
            
        tab0_bg = THEME_SEL_BG if self.active_tab == 0 else (45, 45, 45)
        tab0_fg = THEME_SEL_TEXT if self.active_tab == 0 else THEME_TEXT
        draw.rectangle([tab0_x1, tab_y, tab0_x2, tab_y+45.6], fill=tab0_bg)
        draw.text((tab0_x1 + 70, tab_y + 12), "ACHIEVEMENTS", fill=tab0_fg, font=self.font_tabs)
        
        tab1_bg = THEME_SEL_BG if self.active_tab == 1 else (45, 45, 45)
        tab1_fg = THEME_SEL_TEXT if self.active_tab == 1 else THEME_TEXT
        draw.rectangle([tab1_x1, tab_y, tab1_x2, tab_y+45.6], fill=tab1_bg)
        draw.text((tab1_x1 + 60, tab_y + 12), "PLAY HISTORY", fill=tab1_fg, font=self.font_tabs)
        
        # --- LIST AREA ---
        yBase = tab_y + 45.6 + 2.4 # 276.372
        
        # C++ List Layout Math
        if self.use_cpp_math:
            actual_menu_w = self.screen_w
            list_margin = actual_menu_w * 0.022 # Approx 14px
            image_spacer = self.screen_h * (10.0 / 720.0)
            row_height = 71.77
            image_max_size = row_height - image_spacer
            text_x = 0 + list_margin + row_height
            row_w = actual_menu_w
            row_start_x = 0
            icon_x = list_margin
        else:
            list_margin = 14  
            image_spacer = self.screen_h * (10.0 / 720.0)
            row_height = 71.77 
            image_max_size = row_height - image_spacer
            text_x = list_margin + row_height
            row_w = self.screen_w
            row_start_x = 0
            icon_x = list_margin
        
        if self.active_tab == 0:
            # Row 1
            draw.rectangle([row_start_x, yBase, row_start_x+row_w, yBase+66], fill=THEME_LIST_BG)
            if self.double_icon and self.image_size[0] > 0:
                img.paste(self.double_icon, (int(icon_x), int(yBase) + 4))
            draw.text((text_x, yBase + 6), "Double", fill=THEME_TEXT, font=self.font_list_title)
            draw.text((text_x, yBase + 36 + 6), "Clear two lines at once - Points: 2", fill=THEME_TEXT, font=self.font_list_desc)
            
            # Row 2
            row2_y = yBase + 66
            if self.triple_icon and self.image_size[0] > 0:
                img.paste(self.triple_icon, (int(icon_x), int(row2_y) + 4))
            draw.text((text_x, row2_y + 6), "Triple", fill=THEME_TEXT, font=self.font_list_title)
            draw.text((text_x, row2_y + 36 + 6), "Clear three lines at once - Points: 3", fill=THEME_TEXT_MUTED, font=self.font_list_desc)
            
            # Row 3
            row3_y = row2_y + 66
            draw.text((text_x, row3_y + 6), "Tetris", fill=THEME_TEXT, font=self.font_list_title)
            
        elif self.active_tab == 1:
            draw.text((260, 310), "No play history found", fill=THEME_TEXT_MUTED, font=self.font_list_title)
            
        # --- BUTTONS ---
        if self.use_cpp_math:
            btn_y = 435
            b_pad_x = self.screen_w * 0.0052
            b_pad_y = self.screen_h * 0.0296
            
            b1_w = self.font_buttons.getlength("LAUNCH") + (b_pad_x * 2)
            b2_w = self.font_buttons.getlength("BACK") + (b_pad_x * 2)
            total_w = b1_w + b2_w + 10
            
            b1_x = (self.screen_w - total_w) / 2
            b2_x = b1_x + b1_w + 10
            
            draw.rectangle([b1_x, btn_y, b1_x+b1_w, btn_y+35], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            draw.text((b1_x + b_pad_x, btn_y + b_pad_y - 8), "LAUNCH", fill=THEME_TEXT, font=self.font_buttons)
            
            draw.rectangle([b2_x, btn_y, b2_x+b2_w, btn_y+35], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            draw.text((b2_x + b_pad_x, btn_y + b_pad_y - 8), "BACK", fill=THEME_TEXT, font=self.font_buttons)
        else:
            draw.rectangle([228, 435, 324, 470], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            draw.text((248, 443), "LAUNCH", fill=THEME_TEXT, font=self.font_buttons)
            
            draw.rectangle([328, 435, 412, 470], fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            draw.text((353, 443), "BACK", fill=THEME_TEXT, font=self.font_buttons)
        
        os.makedirs("simulations/game_achievements", exist_ok=True)
        img.save(f"simulations/game_achievements/{state}.png")
        return img

app = App()
app.draw("state1") # Initial achievements

app.active_tab = 1
app.image_size = [0, 0] 
app.draw("state2")

app.active_tab = 0
app.draw("state3") 

# Create difference overlay
try:
    from PIL import ImageChops
    real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
    sim_img = Image.open("simulations/game_achievements/state1.png").convert("RGB")
    
    diff_img = ImageChops.difference(real_img, sim_img)
    comp_img = Image.new("RGB", (real_img.width * 3, max(real_img.height, sim_img.height)))
    comp_img.paste(real_img, (0, 0))
    comp_img.paste(sim_img, (real_img.width, 0))
    comp_img.paste(diff_img, (real_img.width * 2, 0))
    
    comp_img.save("comparison.png")
    print("comparison.png with diff generated.")
except Exception as e:
    print(f"Error making comparison: {e}")
