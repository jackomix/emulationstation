from PIL import Image, ImageDraw, ImageFont

W, H = 640, 480
img = Image.new('RGBA', (W, H), color=(40, 40, 40, 255))
draw = ImageDraw.Draw(img)

FONT_SIZE_LARGE = int(0.105 * min(W, H))  # ~50 to match FreeType visual weight
FONT_SIZE_SMALL = int(0.045 * min(W, H))  # ~21 to match FreeType visual weight

try:
    font_large = ImageFont.truetype('./resources/ubuntu_condensed.ttf', FONT_SIZE_LARGE)
    font_small = ImageFont.truetype('./resources/ubuntu_condensed.ttf', FONT_SIZE_SMALL)
except:
    font_large = ImageFont.load_default()
    font_small = ImageFont.load_default()

cartridge_img = Image.open('/tmp/cartridge.png').convert("RGBA")
fav_add_img = Image.open('/tmp/fav_add.png').convert("RGBA")

# --- FIX 1: Layout math ---
title_bbox = draw.textbbox((0, 0), "SELECT PROFILE", font=font_large)
titleH = (title_bbox[3] - title_bbox[1]) + 8
gap = H * 0.08
gridH = H * 0.30

blockH = titleH + gap + gridH
# --- FIX 6: Optical Centering ---
# The math centers the bounding box, but the heavy visual weight of the cards at the bottom 
# pulls the center of gravity down. Shifting it up slightly makes it look centered to the human eye.
startY = (H - blockH) / 2.0 - (H * 0.02)

title_w = title_bbox[2] - title_bbox[0]
draw.text(((W - title_w) / 2, startY), "SELECT PROFILE", fill=(200, 200, 200, 255), font=font_large)

# --- FIX 2: Grid uses 85% width with gaps, resize=false ---
profiles = ["123", "56789", "7890", "99999"]
cols = len(profiles) + 1
cornerSize = 6

# Cards are sized to 85% of an equal cell, with the remaining 15% as gaps
# This is what resize=false would give us — cards don't expand to fill
totalCols = cols
gridW = W * 0.90
gridX = W * 0.05
gridY = startY + titleH + gap

cellW = gridW / totalCols
cardW = cellW * 0.85
cardGap = cellW * 0.15

cardH = gridH

for i in range(cols):
    isCreate = (i == cols - 1)

    # Position of the card itself (logical box)
    cX = gridX + i * cellW + cardGap / 2
    cY = gridY
    cW = cardW
    cH = cardH

    # --- FIX 3: NinePatch rendered symmetrically at (-cornerSize, -cornerSize) ---
    # fitTo sets pos=(-6,-6), size=(cW+12, cH+12) — setPosition(0,0) is REMOVED in the fix
    npX = cX - cornerSize
    npY = cY - cornerSize
    npW = cW + cornerSize * 2
    npH = cH + cornerSize * 2

    if not isCreate:
        edgeColor = (255, 255, 255, 255) if i == 0 else (136, 136, 136, 255)
        centerColor = (34, 34, 34, 255)
        draw.rounded_rectangle(
            [npX, npY, npX + npW, npY + npH],
            radius=cornerSize * 2,
            fill=centerColor,
            outline=edgeColor,
            width=6
        )

    # --- FIX 4: Content is centered in the FULL NinePatch visual area (npW x npH) ---
    avatarSize = (W * 0.13) if isCreate else (W * 0.10)

    text_bbox_s = draw.textbbox((0, 0), "Tg", font=font_small)
    textHeight = (text_bbox_s[3] - text_bbox_s[1]) if not isCreate else 0

    if isCreate:
        # The plus icon has no text, so just perfectly center it vertically in the card
        avatarX = npX + (npW - avatarSize) / 2
        avatarY = npY + (npH - avatarSize) / 2
    else:
        # Cartridge + text
        totalHeight = avatarSize + 10 + textHeight
        avatarX = npX + (npW - avatarSize) / 2
        avatarY = npY + (npH - totalHeight) / 2

    to_draw = fav_add_img if isCreate else cartridge_img
    to_draw = to_draw.resize((int(avatarSize), int(avatarSize)), Image.Resampling.LANCZOS)
    img.alpha_composite(to_draw, dest=(int(avatarX), int(avatarY)))

    if not isCreate:
        pad = cW * 0.10
        nameW = cW - pad * 2
        nameY = avatarY + avatarSize + 10

        name_str = profiles[i]
        tb = draw.textbbox((0, 0), name_str, font=font_small)
        tw = tb[2] - tb[0]
        textX = npX + (npW - tw) / 2
        draw.text((textX, nameY), name_str, fill=(200, 200, 200, 255), font=font_small)

img.save('/tmp/simulation_fixed.png')
print("Saved to /tmp/simulation_fixed.png")
