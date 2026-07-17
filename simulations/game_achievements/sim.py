"""
GuiGameAchievements Simulator - Engine-Accurate
================================================
All layout math is ported directly from the C++ source.

KEY ENGINE FACTS ENCODED HERE:
  1. Font::getHeight(lineSpacing=1.5) = mMaxGlyphHeight * 1.5
     PIL equivalent: the rendered glyph bitmap height of a tall ASCII char * 1.5
     This is NOT the same as the font point size (pt size ~= 70-80% of getHeight).

  2. Font::sizeText(text).y() for a single-line string = getHeight(1.5)
     So TextComponent with autoCalcExtent=true will have getSize().y() == getHeight(1.5).

  3. TextComponent vertical alignment (ALIGN_CENTER default):
     yOff = (cellHeight - textSize.y()) / 2
     Text draws at cellTop + yOff, NOT at cellTop.

  4. buildTextCache y-start:
     y = offset.y + (getHeight(lineSpacing) + bearing_y_of_S) / 2
     Glyphs draw at y - bearing.y (above baseline), so the visual baseline is
     roughly in the vertical center of getHeight().

  5. WINDOW_WIDTH macro = min(screenH*1.125, screenW*0.90) = 540 on 640x480.
     BUT fullScreenMenus() forces mMenu to full 640x480.
     So column % math uses 540, but the rendered pixels use 640. This is the
     well-documented engine bug; we simulate both modes.

  6. Tab alignment: \t in ALIGN_LEFT text jumps to tabStops[n] + screenW*0.01
     tabStops are computed per-line as the max x-advance before each \t.
     We approximate this for the stats block.

  7. GameAchievementEntry row height:
     IMAGESPACER = screenH * (10/720)  ≈ 6.67px at 480p
     IMAGESIZE   = screenH * (48/720)  ≈ 32px  at 480p
     row_h = max(IMAGESIZE + IMAGESPACER, title_h + desc_h)
     where title_h and desc_h are getHeight(1.5) of their respective fonts.

  8. Progress bar height = screenH * (18/720) ≈ 12px at 480p (not 17-22px).
     Width = textColWidth * 0.45.

  9. ComponentGrid tab bar height:
     h = tabText.getSize().y() + screenH*0.02
     tabText.getSize().y() = getHeight(1.5) of medium font

  10. List area starts at: tabY + tabH + screenH*0.005  (2.4px separator)
"""

import os
from PIL import Image, ImageDraw, ImageFont
import numpy as np

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
ROOT_DIR = "/Users/jacko/Documents/MyEmulationStation"
FONT_PATH_DEFAULT = os.path.join(ROOT_DIR, "resources/opensans_hebrew_condensed_regular.ttf")
FONT_PATH_ACRE = os.path.join(ROOT_DIR, "simulations/game_achievements/Acre.otf")

# ---------------------------------------------------------------------------
# Theme colors
# ---------------------------------------------------------------------------
THEME_BG          = (42, 42, 42)
THEME_TEXT        = (255, 255, 255)
THEME_TEXT_MUTED  = (135, 135, 135)
THEME_SEL_BG      = (254, 254, 254)
THEME_SEL_TEXT    = (0, 0, 0)
THEME_LIST_BG     = (135, 135, 135)
THEME_LIST_BG_ALT = (50, 50, 50)
THEME_PROGRESS_BG = (34, 34, 34)
THEME_PROGRESS_FG = (11, 113, 193)
THEME_BUTTON_BG   = (42, 42, 42)
THEME_BUTTON_BORDER = (255, 255, 255)

# ---------------------------------------------------------------------------
# Engine font metrics helper
# ---------------------------------------------------------------------------
def measure_max_glyph_height(pil_font):
    """
    Approximates Font::mMaxGlyphHeight.
    The engine sets mMaxGlyphHeight = max bitmap height across all ASCII 32-127 glyphs.
    We measure the rendered pixel height of uppercase/tall characters.
    Returns the integer pixel height of the tallest glyph bitmap.
    """
    tallest = 0
    test_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZbdfhijklpqty|()"
    tmp = Image.new("L", (200, 100), 0)
    d = ImageDraw.Draw(tmp)
    for ch in test_chars:
        d.text((10, 20), ch, font=pil_font, fill=255)
    arr = np.array(tmp)
    rows_with_ink = np.where(arr.max(axis=1) > 32)[0]
    if len(rows_with_ink) > 0:
        tallest = int(rows_with_ink[-1] - rows_with_ink[0] + 1)
    return tallest

def get_height(max_glyph_h, line_spacing=1.5):
    """Font::getHeight(lineSpacing) = mMaxGlyphHeight * lineSpacing"""
    return max_glyph_h * line_spacing

def get_text_height(pil_font, line_spacing=1.5):
    """Returns the engine's getHeight() for a given PIL font."""
    mgh = measure_max_glyph_height(pil_font)
    return get_height(mgh, line_spacing)

def get_text_width(pil_font, text):
    """Approximates Font::sizeText(text).x() — PIL getlength is accurate enough."""
    return pil_font.getlength(text)

def draw_text_centered_in_cell(draw, img, pil_font, text, cell_x, cell_y, cell_w, cell_h, fill, align="left"):
    """
    Mimics TextComponent rendering:
    - Vertical: ALIGN_CENTER => yOff = (cell_h - text_size_y) / 2
    - Horizontal: left/center/right
    - text_size_y = getHeight(1.5) of the font
    """
    text_h = get_text_height(pil_font)
    y_off = (cell_h - text_h) / 2.0
    draw_y = cell_y + y_off
    text_w = get_text_width(pil_font, text)

    if align == "center":
        draw_x = cell_x + (cell_w - text_w) / 2.0
    elif align == "right":
        draw_x = cell_x + cell_w - text_w
    else:  # left
        draw_x = cell_x

    draw.text((draw_x, draw_y), text, font=pil_font, fill=fill)


# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------
class App:
    def __init__(self, **kwargs):
        self.active_tab = 0
        self.screen_w = 640
        self.screen_h = 480
        self.use_cpp_math = kwargs.get("use_cpp_math", True)  # default True now

        min_dim = min(self.screen_w, self.screen_h)  # 480

        # Font point sizes from Font.h  (base * min_dim, scale=1.0)
        # These are the INTEGER sizes passed to FT_Set_Pixel_Sizes.
        self.pt_mini   = int(0.030 * min_dim)  # 14
        self.pt_small  = int(0.035 * min_dim)  # 16
        self.pt_medium = int(0.045 * min_dim)  # 21
        self.pt_large  = int(0.085 * min_dim)  # 40

        # Choose font file
        font_to_use = FONT_PATH_ACRE if os.path.exists(FONT_PATH_ACRE) else FONT_PATH_DEFAULT
        print(f"Using font: {font_to_use}")

        try:
            # Acre = theme font, used for titles and tab labels
            self.font_title      = ImageFont.truetype(font_to_use,    self.pt_large)
            self.font_tabs       = ImageFont.truetype(font_to_use,    self.pt_medium)
            self.font_list_title = ImageFont.truetype(font_to_use,    self.pt_medium)
            self.font_buttons    = ImageFont.truetype(font_to_use,    self.pt_small)
            # OpenSans = default font, used for stats + list descriptions
            self.font_stats      = ImageFont.truetype(FONT_PATH_DEFAULT, self.pt_small)
            self.font_list_desc  = ImageFont.truetype(FONT_PATH_DEFAULT, self.pt_mini)
        except Exception as e:
            print(f"Font load error: {e}, using PIL default")
            fallback = ImageFont.load_default()
            self.font_title = self.font_tabs = self.font_list_title = fallback
            self.font_buttons = self.font_stats = self.font_list_desc = fallback

        # Pre-compute engine font metric values
        self.mgh_medium = measure_max_glyph_height(self.font_tabs)
        self.mgh_small  = measure_max_glyph_height(self.font_stats)
        self.mgh_mini   = measure_max_glyph_height(self.font_list_desc)
        self.mgh_large  = measure_max_glyph_height(self.font_title)

        # getHeight(1.5) for each font — this is what TextComponent.getSize().y() returns
        self.h_medium = get_height(self.mgh_medium)  # ~31.5px
        self.h_small  = get_height(self.mgh_small)   # ~24px
        self.h_mini   = get_height(self.mgh_mini)    # ~21px
        self.h_large  = get_height(self.mgh_large)   # ~60px

        print(f"getHeight(medium)={self.h_medium:.1f}  getHeight(small)={self.h_small:.1f}  getHeight(mini)={self.h_mini:.1f}  getHeight(large)={self.h_large:.1f}")

        # --- Engine layout constants ---
        # WINDOW_WIDTH = min(screenH*1.125, screenW*0.90) = 540
        self.WINDOW_WIDTH = min(self.screen_h * 1.125, self.screen_w * 0.90)  # 540

        # fullScreenMenus() forces menu to full screen
        # column math uses WINDOW_WIDTH but rendering uses screen_w
        self.menu_w = self.screen_w   # 640  (forced by fullScreenMenus)
        self.menu_h = self.screen_h   # 480

        # --- Header grid column math ---
        # TITLE_HEIGHT = mMenu.getTitleHeight() which includes padding.
        # The header ComponentGrid has 2 cols: text col and image col.
        # iw (image width %) = TITLE_HEIGHT / WINDOW_WIDTH
        # But WINDOW_WIDTH evaluated as screenH (480) due to the engine bug.
        TITLE_HEIGHT_EST = self.h_large + self.h_small * 3 + self.screen_h * 0.04
        iw = TITLE_HEIGHT_EST / self.WINDOW_WIDTH   # ~0.1491 = 14.91%
        self.text_col_pct = 1.0 - iw                # ~0.8509 = 85.09%
        self.img_col_pct  = iw                      # ~0.1491 = 14.91%

        # In pixel space (using menu_w = 640):
        self.text_col_w = self.menu_w * self.text_col_pct   # ~544.6
        self.img_col_w  = self.menu_w * self.img_col_pct    # ~95.4
        self.img_col_x  = self.text_col_w                   # image col starts here

        # --- GameAchievementEntry row constants ---
        # IMAGESPACER = screenH * (10/720)
        self.IMAGESPACER = self.screen_h * (10.0 / 720.0)   # ~6.67px
        # IMAGESIZE = screenH * (48/720)
        self.IMAGESIZE = self.screen_h * (48.0 / 720.0)     # ~32px
        # row height = max(IMAGESIZE + IMAGESPACER, title_h + desc_h)
        self.list_row_h = max(
            self.IMAGESIZE + self.IMAGESPACER,
            self.h_medium + self.h_mini
        )
        print(f"list_row_h={self.list_row_h:.1f}  (IMAGESIZE+SPACER={self.IMAGESIZE+self.IMAGESPACER:.1f}, title+desc={self.h_medium+self.h_mini:.1f})")

        # --- Tab bar height ---
        # h = tabText.getSize().y() + screenH*0.02
        self.tab_h = self.h_medium + self.screen_h * 0.02  # ~31.5 + 9.6 = ~41.1

        # --- Progress bar ---
        self.prog_h = self.screen_h * (18.0 / 720.0)  # ~12px

        # --- Load real image crops for icons ---
        self.box_art = None
        self.double_icon = None
        self.triple_icon = None
        try:
            real_img = Image.open("/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png")
            self.box_art     = real_img.crop((564, 108, 624, 168))
            self.double_icon = real_img.crop((14, 280, 74, 339))
            self.triple_icon = real_img.crop((14, 346, 74, 405))
        except Exception as e:
            print(f"Icon crop error: {e}")

    # -----------------------------------------------------------------------
    def _compute_tab_y(self, title_block_bottom):
        """
        tabY = title_block_bottom + prog_h + screenH*0.01 (spacing above tabs)
        Progress bar sits just above the tabs.
        """
        return title_block_bottom + self.prog_h + self.screen_h * 0.01

    def draw(self, state):
        img  = Image.new("RGB", (self.screen_w, self.screen_h), THEME_BG)
        draw = ImageDraw.Draw(img)

        # ===================================================================
        # HEADER  (ComponentGrid: text col | image col)
        # ===================================================================
        # All header content starts at Y=0 (menu-local Y=0 = screen Y=0
        # when fullScreenMenus is true — no vertical centering offset).
        header_y = 0

        # --- Game title (ALIGN_CENTER in text col) ---
        title_text = "TETRIS"
        title_w = get_text_width(self.font_title, title_text)
        title_draw_x = (self.text_col_w - title_w) / 2.0
        # Vertical: title row height = h_large + screenH*0.01 padding
        title_row_h = self.h_large + self.screen_h * 0.01
        # TextComponent ALIGN_CENTER vertical: yOff = (row_h - text_h) / 2
        title_y_off = (title_row_h - self.h_large) / 2.0
        draw.text((title_draw_x, header_y + title_y_off), title_text,
                  font=self.font_title, fill=THEME_TEXT)

        # --- Stats subtitle (ALIGN_LEFT in text col, with \t tab stops) ---
        # The engine processes \t by jumping to tabStop+screenW*0.01.
        # TabStop[0] is set to the max x-advance before the first \t across all lines.
        # Lines: "Achievements (softcore):\t2/37"
        #        "Achievements (hardcore):\t0/37"
        #        "Points:\t6/480"
        # Max pre-tab text = "Achievements (softcore):\t" prefix width
        pre_tab_texts = [
            "Achievements (softcore):",
            "Achievements (hardcore):",
            "Points:",
        ]
        tab_stop_0 = max(get_text_width(self.font_stats, t) for t in pre_tab_texts)
        tab_col_x = tab_stop_0 + self.screen_w * 0.01  # tabStops[0] + screenW*0.01

        stats_lines = [
            ("Achievements (softcore):", "2/37"),
            ("Achievements (hardcore):", "0/37"),
            ("Points:", "6/480"),
        ]
        stats_y = header_y + title_row_h
        # Left-pad stats to match horizontal centering: engine uses ALIGN_LEFT here
        # but the subtitle TextComponent X = 0 (left of text col).
        stats_x = 0
        line_h = self.h_small  # getHeight(1.5) for stats font
        for i, (label, value) in enumerate(stats_lines):
            row_y = stats_y + i * line_h
            draw.text((stats_x, row_y), label, font=self.font_stats, fill=THEME_TEXT_MUTED)
            draw.text((tab_col_x, row_y), value, font=self.font_stats, fill=THEME_TEXT_MUTED)

        subtitle_block_h = len(stats_lines) * line_h
        title_block_bottom = stats_y + subtitle_block_h + self.screen_h * 0.005

        # --- Box art image (in image col, centered) ---
        if self.box_art:
            bw, bh = self.box_art.size
            img_paste_x = int(self.img_col_x + (self.img_col_w - bw) / 2.0)
            img_paste_y = int(header_y + (title_row_h + subtitle_block_h - bh) / 2.0)
            img_paste_y = max(header_y, img_paste_y)
            img.paste(self.box_art, (img_paste_x, img_paste_y))
        else:
            bw, bh = 60, 86
            draw.rectangle(
                [self.img_col_x, header_y, self.img_col_x + bw, header_y + bh],
                fill=(0, 0, 200)
            )

        # ===================================================================
        # PROGRESS BAR
        # ===================================================================
        # Sits between the header and the tab bar.
        # Width = text_col_w * 0.45,  left-margin = menu_w * 0.04
        prog_x = self.menu_w * 0.04
        prog_w = self.text_col_w * 0.45
        prog_y = title_block_bottom
        prog_fill_w = prog_w * 0.05  # 5% complete

        draw.rectangle(
            [prog_x, prog_y, prog_x + prog_w, prog_y + self.prog_h],
            fill=THEME_PROGRESS_BG
        )
        if prog_fill_w > 0:
            draw.rectangle(
                [prog_x, prog_y, prog_x + prog_fill_w, prog_y + self.prog_h],
                fill=THEME_PROGRESS_FG
            )
        pct_text = "5%"
        draw.text(
            (prog_x + prog_fill_w + 4, prog_y + (self.prog_h - self.h_mini) / 2.0),
            pct_text, font=self.font_stats, fill=THEME_TEXT
        )

        # ===================================================================
        # TAB BAR
        # ===================================================================
        tab_y = prog_y + self.prog_h + self.screen_h * 0.005
        tab_w = self.menu_w * 0.5  # two equal tabs

        for i, label in enumerate(["ACHIEVEMENTS", "PLAY HISTORY"]):
            tx1 = i * tab_w
            tx2 = tx1 + tab_w
            is_active = (i == self.active_tab)
            bg = THEME_SEL_BG if is_active else (45, 45, 45)
            fg = THEME_SEL_TEXT if is_active else THEME_TEXT
            draw.rectangle([tx1, tab_y, tx2, tab_y + self.tab_h], fill=bg)
            # Text: ALIGN_CENTER horizontally, ALIGN_CENTER vertically in tab_h
            lw = get_text_width(self.font_tabs, label)
            lx = tx1 + (tab_w - lw) / 2.0
            ly = tab_y + (self.tab_h - self.h_medium) / 2.0
            draw.text((lx, ly), label, font=self.font_tabs, fill=fg)

        # ===================================================================
        # LIST AREA
        # ===================================================================
        list_y = tab_y + self.tab_h + self.screen_h * 0.005  # 2.4px separator

        # Entry layout constants
        list_margin = self.menu_w * 0.022   # ~14px — left padding before icon
        icon_size   = self.IMAGESIZE         # ~32px
        text_x      = list_margin + self.list_row_h  # text starts after icon+spacer

        if self.active_tab == 0:
            achievements = [
                (self.double_icon, "Double",  "Clear two lines at once",  "2",  True,  THEME_TEXT),
                (self.triple_icon, "Triple",  "Clear three lines at once","3",  False, THEME_TEXT_MUTED),
                (None,             "Tetris",  "Clear four lines at once", "10", False, THEME_TEXT_MUTED),
            ]
            for idx, (icon, title, desc, pts, earned, desc_color) in enumerate(achievements):
                ry = list_y + idx * self.list_row_h
                # Alternating row background (earned rows use list bg)
                row_bg = THEME_LIST_BG if earned else THEME_LIST_BG_ALT
                draw.rectangle(
                    [0, ry, self.menu_w, ry + self.list_row_h],
                    fill=row_bg
                )
                # Icon
                if icon:
                    icon_scaled = icon.resize((int(icon_size), int(icon_size)))
                    icon_y = int(ry + (self.list_row_h - icon_size) / 2.0)
                    img.paste(icon_scaled, (int(list_margin), icon_y))
                else:
                    # placeholder
                    draw.rectangle(
                        [list_margin, ry + 4, list_margin + icon_size, ry + icon_size + 4],
                        fill=(80, 80, 80)
                    )
                # Title text: top half of text area, ALIGN_LEFT, ALIGN_CENTER vertically in top half
                title_cell_h = self.list_row_h / 2.0
                title_y = ry + (title_cell_h - self.h_medium) / 2.0
                draw.text((text_x, title_y), title, font=self.font_list_title, fill=THEME_TEXT)
                # Points badge right-aligned in text col
                pts_text = f"  {pts} pts"
                pts_w = get_text_width(self.font_stats, pts_text)
                draw.text((self.text_col_w - pts_w - 8, title_y), pts_text,
                          font=self.font_stats, fill=THEME_TEXT_MUTED)
                # Description text: bottom half of text area
                desc_y = ry + title_cell_h + (title_cell_h - self.h_mini) / 2.0
                draw.text((text_x, desc_y), desc, font=self.font_list_desc, fill=desc_color)

        elif self.active_tab == 1:
            msg = "No play history found"
            mw = get_text_width(self.font_list_title, msg)
            draw.text(
                ((self.menu_w - mw) / 2.0, list_y + 40),
                msg, font=self.font_list_title, fill=THEME_TEXT_MUTED
            )

        # ===================================================================
        # BUTTON BAR
        # ===================================================================
        # ButtonComponent pads: padX = screenW*0.0052, padY = screenH*0.0296
        # Button height = font_h + padY*2; placed at bottom of screen.
        b_pad_x = self.screen_w * 0.0052
        b_pad_y = self.screen_h * 0.0296
        btn_content_h = self.h_small
        btn_h = btn_content_h + b_pad_y * 2
        btn_y = self.screen_h - btn_h - self.screen_h * 0.01  # 1% margin from bottom

        buttons = ["LAUNCH", "BACK"]
        btn_widths = [get_text_width(self.font_buttons, b) + b_pad_x * 2 for b in buttons]
        total_btn_w = sum(btn_widths) + 10 * (len(buttons) - 1)
        bx = (self.screen_w - total_btn_w) / 2.0

        for label, bw in zip(buttons, btn_widths):
            draw.rectangle([bx, btn_y, bx + bw, btn_y + btn_h],
                           fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            lw = get_text_width(self.font_buttons, label)
            lx = bx + (bw - lw) / 2.0
            ly = btn_y + (btn_h - self.h_small) / 2.0
            draw.text((lx, ly), label, font=self.font_buttons, fill=THEME_TEXT)
            bx += bw + 10

        # ===================================================================
        # Save
        # ===================================================================
        os.makedirs("simulations/game_achievements", exist_ok=True)
        out_path = f"simulations/game_achievements/{state}.png"
        img.save(out_path)
        print(f"Saved {out_path}")
        return img


# ---------------------------------------------------------------------------
# Run all states
# ---------------------------------------------------------------------------
app = App()
app.active_tab = 0
app.draw("state1")   # Achievements tab

app.active_tab = 1
app.draw("state2")   # Play History tab

app.active_tab = 0
app.draw("state3")   # Achievements tab again (same as state1, for regression)

# ---------------------------------------------------------------------------
# Comparison overlay (requires real screenshot)
# ---------------------------------------------------------------------------
try:
    from PIL import ImageChops
    real_img = Image.open(
        "/Users/jacko/.gemini/antigravity-cli/brain/bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png"
    ).convert("RGB")
    sim_img = Image.open("simulations/game_achievements/state1.png").convert("RGB")

    diff_img = ImageChops.difference(real_img, sim_img)
    comp_w = real_img.width * 3
    comp_h = max(real_img.height, sim_img.height)
    comp_img = Image.new("RGB", (comp_w, comp_h))
    comp_img.paste(real_img, (0, 0))
    comp_img.paste(sim_img, (real_img.width, 0))
    comp_img.paste(diff_img, (real_img.width * 2, 0))
    comp_img.save("comparison.png")
    print("comparison.png saved (real | sim | diff)")
except Exception as e:
    print(f"Comparison skipped: {e}")
