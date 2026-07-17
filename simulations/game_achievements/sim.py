import sys
import os
from PIL import Image, ImageDraw, ImageFont


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------
ROOT_DIR = "/Users/jacko/Documents/MyEmulationStation"
FONT_PATH_DEFAULT = os.path.join(ROOT_DIR, "resources/opensans_hebrew_condensed_regular.ttf")
FONT_PATH_ACRE    = os.path.join(ROOT_DIR, "simulations/game_achievements/Acre.otf")

# ---------------------------------------------------------------------------
# Theme colors (approximated from Batocera dark theme)
# ---------------------------------------------------------------------------
THEME_BG           = (42, 42, 42)
THEME_TEXT         = (255, 255, 255)
THEME_TEXT_MUTED   = (135, 135, 135)
THEME_SEL_BG       = (254, 254, 254)
THEME_SEL_TEXT     = (0, 0, 0)
THEME_LIST_BG      = (135, 135, 135)   # selected row highlight
THEME_PROGRESS_BG  = (34, 34, 34)
THEME_PROGRESS_FG  = (11, 113, 193)
THEME_BUTTON_BG    = (42, 42, 42)
THEME_BUTTON_BORDER= (255, 255, 255)


# ---------------------------------------------------------------------------
# App
# ---------------------------------------------------------------------------
class App:
    def __init__(self):
        self.active_tab = 0
        self.screen_w   = 640
        self.screen_h   = 480

        # ---- font sizes from Font.h ----------------------------------------
        # SIZE_* = factor * min(screenW, screenH)
        # getHeight() (line metrics) ~= size * 1.3
        # getLetterHeight() (cap height) ~= size * 0.65
        min_dim = min(self.screen_w, self.screen_h)          # 480
        self.size_mini   = 0.030 * min_dim                   # 14.4
        self.size_small  = 0.035 * min_dim                   # 16.8
        self.size_medium = 0.045 * min_dim                   # 21.6
        self.size_large  = 0.085 * min_dim                   # 40.8

        # PIL truetype() takes point sizes.  On a 480p screen these integer
        # values produce glyph heights closest to the ES FreeType output.
        self._pil_mini   = int(self.size_mini)
        self._pil_small  = int(self.size_small)
        self._pil_medium = int(self.size_medium)
        self._pil_large  = int(self.size_large)

        # ---- derived layout constants (all from C++ source) ----------------
        # MenuComponent.h padding constants (screen_h x factor)
        self.TITLE_WITHSUB_VERT_PADDING = self.screen_h * 0.05      # 24.0
        self.SUBTITLE_VERT_PADDING      = self.screen_h * 0.019     # 9.12
        self.BUTTON_GRID_VERT_PADDING   = self.screen_h * 0.0296296 # 14.22
        self.BUTTON_GRID_HORIZ_PADDING  = self.screen_w * 0.0052083 # 3.33

        # WINDOW_WIDTH macro in GuiGameAchievements.cpp:
        #   min(screenH * 1.125, screenW * 0.90) = min(540, 576) = 540
        self.WINDOW_WIDTH = min(self.screen_h * 1.125, self.screen_w * 0.90)  # 540

        # On R36S fullScreenMenus() == true -> menu fills 640x480
        self.menu_w = self.screen_w   # 640
        self.menu_h = self.screen_h   # 480

        # IMAGESIZE / IMAGESPACER macros
        self.IMAGESIZE   = self.screen_h * (48.0 / 720.0)   # 32.0
        self.IMAGESPACER = self.screen_h * (10.0 / 720.0)   # 6.67

        # ---- fonts ---------------------------------------------------------
        font_to_use = FONT_PATH_ACRE if os.path.exists(FONT_PATH_ACRE) else FONT_PATH_DEFAULT
        print(f"Using font: {font_to_use}")
        try:
            self.font_title      = ImageFont.truetype(font_to_use,       self._pil_large)
            self.font_tabs       = ImageFont.truetype(font_to_use,       self._pil_medium)
            self.font_list_title = ImageFont.truetype(font_to_use,       self._pil_medium)
            self.font_buttons    = ImageFont.truetype(font_to_use,       self._pil_small)
            self.font_stats      = ImageFont.truetype(FONT_PATH_DEFAULT, self._pil_small)
            self.font_list_desc  = ImageFont.truetype(FONT_PATH_DEFAULT, self._pil_small)
        except Exception as e:
            print(f"Font error, using default: {e}")
            self.font_title = self.font_tabs = self.font_list_title = \
            self.font_buttons = self.font_stats = self.font_list_desc = \
                ImageFont.load_default()

        # ---- reference image crops -----------------------------------------
        try:
            real = Image.open(
                "/Users/jacko/.gemini/antigravity-cli/brain/"
                "bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png")
            self.box_art     = real.crop((564, 108, 624, 168))
            self.double_icon = real.crop((14, 280, 74, 339))
            self.triple_icon = real.crop((14, 346, 74, 405))
        except Exception as e:
            print(f"Could not load reference crops: {e}")
            self.box_art = self.double_icon = self.triple_icon = None

    # -----------------------------------------------------------------------
    # _title_height()
    #   Mirrors the TITLE_HEIGHT macro in MenuComponent.h:
    #     letterHeight + TITLE_WITHSUB_VERT_PADDING
    #     + subtitle.size().y + SUBTITLE_VERT_PADDING
    # -----------------------------------------------------------------------
    def _title_height(self):
        # mTitle->getFont()->getLetterHeight() ~= size_large * 0.65
        letter_h = self.size_large * 0.65                            # ~26.5

        # mSubtitle uses TextSmall (SIZE_SMALL) with lineSpacing 1.1.
        # GuiGameAchievements subtitle string has:
        #   3 data lines  (softcore, hardcore, points)
        #   5 padding lines (the "\r\n " spacers at the end of the string)
        # = 8 lines total.
        subtitle_line_h = self.size_small * 1.1                     # 18.48
        subtitle_lines  = 8
        subtitle_h      = subtitle_line_h * subtitle_lines          # 147.84

        return letter_h + self.TITLE_WITHSUB_VERT_PADDING + subtitle_h + self.SUBTITLE_VERT_PADDING

    # -----------------------------------------------------------------------
    # _title_image_iw()
    #   From MenuComponent::setTitleImage() non-replaceTitle branch:
    #     float width = min(screenH, screenW * 0.90)  -> 480
    #     float iw    = TITLE_HEIGHT / width
    # -----------------------------------------------------------------------
    def _title_image_iw(self):
        width = min(self.screen_h, self.screen_w * 0.90)            # 480
        return self._title_height() / width

    # -----------------------------------------------------------------------
    # _tab_h()
    #   From GuiGameAchievements::updateTab():
    #     float h = leftTab->getSize().y() + screenH * 0.02
    #   leftTab is TextComponent(theme->Text.font = SIZE_MEDIUM).
    #   TextComponent::getSize().y() ~= font->getHeight() ~= size_medium * 1.3
    # -----------------------------------------------------------------------
    def _tab_h(self):
        text_comp_h = self.size_medium * 1.3                        # ~28.1
        return text_comp_h + self.screen_h * 0.02                   # ~37.7

    # -----------------------------------------------------------------------
    # _row_height()
    #   From GameAchievementEntry constructor:
    #     int height = max(IMAGESIZE + IMAGESPACER, mText.h + mSubstring.h)
    #   mText      = theme->Text.font      (SIZE_MEDIUM) h ~= size_medium*1.3
    #   mSubstring = theme->TextSmall.font (SIZE_SMALL)  h ~= size_small*1.3
    # -----------------------------------------------------------------------
    def _row_height(self):
        text_h = self.size_medium * 1.3
        desc_h = self.size_small  * 1.3
        from_text = text_h + desc_h
        from_img  = self.IMAGESIZE + self.IMAGESPACER
        return max(from_img, from_text)                             # ~50

    # -----------------------------------------------------------------------
    # draw()
    # -----------------------------------------------------------------------
    def draw(self, state):
        img  = Image.new("RGB", (self.screen_w, self.screen_h), THEME_BG)
        draw = ImageDraw.Draw(img)

        # ---- derived layout values ----------------------------------------
        TITLE_H = self._title_height()       # ~207
        iw      = self._title_image_iw()     # ~0.432
        tab_h   = self._tab_h()              # ~37.7
        row_h   = self._row_height()         # ~50

        # Column widths (fullscreen menu = 640px wide)
        img_col_w = iw * self.menu_w         # ~276  (image column, right side)
        txt_col_w = (1 - iw) * self.menu_w   # ~363  (text/title/subtitle column, left side)

        # ---- HEADER (title + subtitle + box art) --------------------------
        # mTitle is ALIGN_CENTER inside txt_col_w.
        title_text  = "TETRIS"
        title_bbox  = draw.textbbox((0, 0), title_text, font=self.font_title)
        title_tw    = title_bbox[2] - title_bbox[0]
        title_x     = (txt_col_w - title_tw) / 2
        # Small top inset so the cap height sits within the letter-height zone
        title_y     = (self.size_large * 0.65) * 0.1
        draw.text((title_x, title_y), title_text, fill=THEME_TEXT, font=self.font_title)

        # mSubtitle ALIGN_LEFT, left-pad = screenW * 0.012 (set in updateSize())
        sub_pad = self.screen_w * 0.012
        sub_x   = sub_pad
        # Subtitle top = title baseline + TITLE_WITHSUB_VERT_PADDING fraction
        sub_y   = title_y + self.size_large * 0.65 + self.TITLE_WITHSUB_VERT_PADDING * 0.4
        line_h  = self.size_small * 1.1
        draw.text((sub_x, sub_y),              "ACHIEVEMENTS (SOFTCORE): \t 2/37",  fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((sub_x, sub_y + line_h),     "ACHIEVEMENTS (HARDCORE): \t 0/37",  fill=THEME_TEXT_MUTED, font=self.font_stats)
        draw.text((sub_x, sub_y + line_h * 2), "POINTS: \t 6/480",                  fill=THEME_TEXT_MUTED, font=self.font_stats)

        # Title image in the right column.
        # maxSize = 1.3 * iw * menuW wide, TITLE_H tall; centered in img_col_w.
        img_max_w = 1.3 * iw * self.menu_w
        img_col_x = txt_col_w
        if self.box_art:
            ba = self.box_art.copy()
            ba.thumbnail((int(img_max_w), int(TITLE_H)), Image.LANCZOS)
            bx = int(img_col_x + (img_col_w - ba.width)  / 2)
            by = int((TITLE_H  - ba.height) / 2)
            img.paste(ba, (bx, by))
        else:
            draw.rectangle([img_col_x, 0, img_col_x + img_col_w, TITLE_H],
                           fill=(40, 40, 120))

        # ---- PROGRESS BAR -------------------------------------------------
        # From GuiGameAchievements::render():
        #   tabY   = yBase - tabGrid.h - screenH*0.005   (yBase = TITLE_HEIGHT)
        #   progY  = tabY  - prog_h    - screenH*0.01
        #   prog_h = TextSmall.sizeText("A8O\rA8O", 1.1).y() ~= 2 * size_small * 1.1
        #   prog_w = xx * 0.45   where xx = menuW - menuW*iw
        #   prog_x = menuW * 0.04
        prog_h  = 2 * self.size_small * 1.1
        tab_y   = TITLE_H - tab_h - self.screen_h * 0.005
        prog_y  = tab_y - prog_h - self.screen_h * 0.01
        prog_x  = self.menu_w * 0.04
        xx      = self.menu_w - self.menu_w * iw
        prog_w  = xx * 0.45

        if self.active_tab == 0:
            draw.rectangle([prog_x, prog_y, prog_x + prog_w, prog_y + prog_h],
                           fill=THEME_PROGRESS_BG)
            fill_w = prog_w * 0.05   # 5% complete
            if fill_w >= 1:
                draw.rectangle([prog_x, prog_y, prog_x + fill_w, prog_y + prog_h],
                               fill=THEME_PROGRESS_FG)
            pct_text = "5% complete"
            pct_bb   = draw.textbbox((0, 0), pct_text, font=self.font_stats)
            pct_h    = pct_bb[3] - pct_bb[1]
            draw.text((prog_x + prog_w + 6, prog_y + (prog_h - pct_h) / 2),
                      pct_text, fill=THEME_TEXT, font=self.font_stats)

        # ---- TAB BAR ------------------------------------------------------
        # tabGrid->setSize(WINDOW_WIDTH, h) but rendered inside the 640-wide
        # menu transform -> displayed at full menu_w.  Each col = 50% = 320px.
        tab_w   = self.menu_w * 0.5
        tab0_bg = THEME_SEL_BG   if self.active_tab == 0 else (45, 45, 45)
        tab0_fg = THEME_SEL_TEXT if self.active_tab == 0 else THEME_TEXT
        tab1_bg = THEME_SEL_BG   if self.active_tab == 1 else (45, 45, 45)
        tab1_fg = THEME_SEL_TEXT if self.active_tab == 1 else THEME_TEXT

        draw.rectangle([0,     tab_y, tab_w,       tab_y + tab_h], fill=tab0_bg)
        draw.rectangle([tab_w, tab_y, self.menu_w, tab_y + tab_h], fill=tab1_bg)

        for label, fg, x0, x1 in [
            ("ACHIEVEMENTS", tab0_fg, 0,     tab_w),
            ("PLAY HISTORY", tab1_fg, tab_w, self.menu_w),
        ]:
            bb  = draw.textbbox((0, 0), label, font=self.font_tabs)
            lw  = bb[2] - bb[0]
            lh  = bb[3] - bb[1]
            lx  = x0 + ((x1 - x0) - lw) / 2
            ly  = tab_y + (tab_h - lh) / 2
            draw.text((lx, ly), label, fill=fg, font=self.font_tabs)

        # ---- LIST AREA ----------------------------------------------------
        # ComponentList occupies grid row 1, which starts at y = TITLE_HEIGHT.
        # The tabs + progress bar are drawn manually in render() OVER the header
        # zone; they do not push the list down.
        list_y = TITLE_H

        # Icon column width inside each GameAchievementEntry ComponentGrid:
        #   setColWidthPerc(0, (row_h - IMAGESPACER) / WINDOW_WIDTH)
        #   setColWidthPerc(1, IMAGESPACER / WINDOW_WIDTH)
        # These fractions are multiplied by the actual grid width = menu_w=640.
        # WINDOW_WIDTH=540 is the *denominator*, not the grid width.
        icon_col_px   = ((row_h - self.IMAGESPACER) / self.WINDOW_WIDTH) * self.menu_w
        spacer_col_px = (self.IMAGESPACER           / self.WINDOW_WIDTH) * self.menu_w
        # ComponentList also adds ~menu_w*0.022 left/right padding
        list_pad  = self.menu_w * 0.022
        icon_x    = list_pad
        text_x    = list_pad + icon_col_px + spacer_col_px

        # Vertical text position inside a row (from ComponentGrid row height logic):
        #   topPadding = max(0, (height - textH - descH) / height / 2)  (fraction)
        #   Row 0 (top pad), Row 1 (title text), Row 2 (desc text), Row 3 (bottom pad)
        text_h       = self.size_medium * 1.3
        desc_h       = self.size_small  * 1.3
        top_pad_frac = max(0.0, (row_h - text_h - desc_h) / row_h / 2.0)
        text_y_off   = top_pad_frac * row_h
        desc_y_off   = text_y_off + text_h

        if self.active_tab == 0:
            rows = [
                ("Double", "Clear two lines at once - Points: 2",   self.double_icon, True),
                ("Triple", "Clear three lines at once - Points: 3", self.triple_icon, False),
                ("Tetris", "Clear four lines at once - Points: 10", None,             False),
            ]
            for i, (title, desc, icon, selected) in enumerate(rows):
                ry     = list_y + i * row_h
                row_bg = THEME_LIST_BG if selected else THEME_BG
                draw.rectangle([0, ry, self.menu_w, ry + row_h], fill=row_bg)

                if icon:
                    ic = icon.copy()
                    ic_max = int(row_h - self.IMAGESPACER)
                    ic.thumbnail((ic_max, ic_max), Image.LANCZOS)
                    ic_y = int(ry + (row_h - ic.height) / 2)
                    img.paste(ic, (int(icon_x), ic_y))

                title_color = THEME_SEL_TEXT if selected else THEME_TEXT
                desc_color  = THEME_SEL_TEXT if selected else THEME_TEXT_MUTED
                draw.text((text_x, ry + text_y_off), title, fill=title_color, font=self.font_list_title)
                draw.text((text_x, ry + desc_y_off), desc,  fill=desc_color,  font=self.font_list_desc)

        elif self.active_tab == 1:
            msg    = "No play history found"
            bb     = draw.textbbox((0, 0), msg, font=self.font_list_title)
            mw, mh = bb[2]-bb[0], bb[3]-bb[1]
            draw.text(((self.menu_w - mw) / 2, list_y + 40),
                      msg, fill=THEME_TEXT_MUTED, font=self.font_list_title)

        # ---- BUTTON BAR ---------------------------------------------------
        # From MenuComponent: buttonGrid height = btn.h + BUTTON_GRID_VERT_PADDING + 2
        # ButtonComponent height ~= font->getHeight()  (~= size_medium * 1.3)
        # mGrid.setRowHeight(2, getButtonGridHeight())  -> placed at bottom of menu
        btn_font_h  = self.size_medium * 1.3
        btn_h_inner = btn_font_h + self.BUTTON_GRID_VERT_PADDING + 2
        btn_y       = self.menu_h - btn_h_inner

        b_pad_x = self.BUTTON_GRID_HORIZ_PADDING
        labels     = ["LAUNCH", "BACK"]
        btn_widths = [self.font_buttons.getlength(l) + b_pad_x * 2 for l in labels]
        total_bw   = sum(btn_widths) + b_pad_x * (len(labels) - 1)
        bx         = (self.menu_w - total_bw) / 2

        for label, bw in zip(labels, btn_widths):
            draw.rectangle([bx, btn_y, bx + bw, btn_y + btn_h_inner],
                           fill=THEME_BUTTON_BG, outline=THEME_BUTTON_BORDER)
            bb  = draw.textbbox((0, 0), label, font=self.font_buttons)
            lw  = bb[2] - bb[0]
            lh  = bb[3] - bb[1]
            draw.text((bx + (bw - lw) / 2, btn_y + (btn_h_inner - lh) / 2),
                      label, fill=THEME_TEXT, font=self.font_buttons)
            bx += bw + b_pad_x

        # ---- save ---------------------------------------------------------
        os.makedirs("simulations/game_achievements", exist_ok=True)
        out_path = f"simulations/game_achievements/{state}.png"
        img.save(out_path)
        print(f"Saved {out_path}")
        return img


# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
app = App()

app.active_tab = 0
app.draw("state1")   # Achievements tab

app.active_tab = 1
app.draw("state2")   # Play History tab

app.active_tab = 0
app.draw("state3")   # Achievements tab again (for diff comparison)

# Comparison overlay: real | sim | pixel-diff
try:
    from PIL import ImageChops
    real_img = Image.open(
        "/Users/jacko/.gemini/antigravity-cli/brain/"
        "bb358d42-7408-4534-a7f6-4663fd1d2d36/achievements.png").convert("RGB")
    sim_img  = Image.open("simulations/game_achievements/state1.png").convert("RGB")
    diff_img = ImageChops.difference(real_img, sim_img)
    comp     = Image.new("RGB", (real_img.width * 3, max(real_img.height, sim_img.height)))
    comp.paste(real_img, (0, 0))
    comp.paste(sim_img,  (real_img.width, 0))
    comp.paste(diff_img, (real_img.width * 2, 0))
    comp.save("comparison.png")
    print("comparison.png saved (real | sim | diff)")
except Exception as e:
    print(f"Comparison error: {e}")
