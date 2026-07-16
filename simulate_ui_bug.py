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

# --- Layout math from GuiProfileSelect::onSizeChanged ---
title_bbox = draw.textbbox((0, 0), "SELECT PROFILE", font=font_large)
titleH = (title_bbox[3] - title_bbox[1]) + 8
gap = H * 0.08
gridH = H * 0.30

blockH = titleH + gap + gridH
startY = (H - blockH) / 2.0

title_w = title_bbox[2] - title_bbox[0]
draw.text(((W - title_w) / 2, startY), "SELECT PROFILE", fill=(200, 200, 200, 255), font=font_large)

gridW = W * 0.90
gridX = W * 0.05
gridY = startY + titleH + gap

profiles = ["123", "56789", "7890", "99999"]
cols = len(profiles) + 1
cellW = gridW / cols
cornerSize = 6

for i in range(cols):
    isCreate = (i == cols - 1)

    # Cell origin (relative to grid origin)
    cX = gridX + i * cellW
    cY = gridY
    cW = cellW   # resize=true forces card to fill cell
    cH = gridH

    # --- NinePatch background ---
    npX = cX       # NOT cX - 6
    npY = cY       # NOT cY - 6
    npW = cW + 12
    npH = cH + 12

    if not isCreate:
        edgeColor = (255, 255, 255, 255) if i == 0 else (136, 136, 136, 255)
        centerColor = (34, 34, 34, 255)
        # Draw rounded rectangle to simulate NinePatch with frame.png
        # NinePatch expands by 6px, which forms the 6px thick border!
        draw.rounded_rectangle(
            [npX, npY, npX + npW, npY + npH],
            radius=cornerSize * 2,
            fill=centerColor,
            outline=edgeColor,
            width=6
        )

    # --- Content positioning (relative to card's mSize box, NOT the NinePatch) ---
    avatarSize = (W * 0.13) if isCreate else (W * 0.10)

    text_bbox_s = draw.textbbox((0, 0), "Tg", font=font_small)
    textHeight = (text_bbox_s[3] - text_bbox_s[1]) if not isCreate else 0

    totalHeight = avatarSize + (0 if isCreate else 10 + textHeight)
    cardStartY = (cH - totalHeight) / 2

    avatarX = cX + (cW - avatarSize) / 2
    avatarY = cY + cardStartY

    to_draw = fav_add_img if isCreate else cartridge_img
    to_draw = to_draw.resize((int(avatarSize), int(avatarSize)), Image.Resampling.LANCZOS)
    img.alpha_composite(to_draw, dest=(int(avatarX), int(avatarY)))

    if not isCreate:
        # Text is ALIGN_CENTER within (pad, startY+avatarSize+10) to (cW-pad, ...)
        pad = cW * 0.10
        nameW = cW - pad * 2
        nameX = cX + pad
        nameY = cY + cardStartY + avatarSize + 10

        name_str = profiles[i]
        tb = draw.textbbox((0, 0), name_str, font=font_small)
        tw = tb[2] - tb[0]
        # Center text within the nameW area
        textX = nameX + (nameW - tw) / 2
        draw.text((textX, nameY), name_str, fill=(200, 200, 200, 255), font=font_small)

img.save('/tmp/simulation_bug3.png')
print(f"FONT_SIZE_LARGE={FONT_SIZE_LARGE}, FONT_SIZE_SMALL={FONT_SIZE_SMALL}")
print(f"cellW={cellW:.1f}, gridH={gridH:.1f}, cornerSize={cornerSize}")
print(f"NinePatch extends 12px RIGHT and DOWN from card origin (setPosition(0,0) override)")
print("Saved to /tmp/simulation_bug3.png")
