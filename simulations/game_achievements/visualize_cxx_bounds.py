from PIL import Image, ImageDraw

def draw_boxes():
    img = Image.open('/tmp/screen.png')
    draw = ImageDraw.Draw(img)

    # mMenu
    # pos: 0,0 size: 640,480

    # Header Grid (offset X=58.28, Y=0 based on centering 523.44 + 61.19 = 584.6)
    # Actually, MenuComponent is fullscreen 640x480.
    # mTitleImage is at 551.12. If right padded by 0.012*640 (7.68), total width is 551.12 + 61.19 + 7.68 = 620.
    # Let's assume HeaderGrid is at X=10 (from entry_x in previous log).
    hx, hy = 58, 0

    # mTitle
    t_x, t_y = hx + 0, hy + 0
    draw.rectangle([t_x, t_y, t_x + 523.44, t_y + 49.1], outline="yellow", width=2)
    
    # mSubtitle
    s_x, s_y = hx + 0, hy + 49.1
    draw.rectangle([s_x, s_y, s_x + 523.44, s_y + 227.25], outline="orange", width=2)
    
    # mTitleImage
    img_x, img_y = hx + 551.12, hy + 94.46
    draw.rectangle([img_x, img_y, img_x + 61.19, img_y + 87.42], outline="purple", width=2)

    # TabGrid
    tab_y = 228.372
    draw.rectangle([0, tab_y, 0+540, tab_y+45.6], outline="red", width=2)

    # Progress
    prog_x, prog_y = 25.6, 201.572
    draw.rectangle([prog_x, prog_y, prog_x+122.177, prog_y+22], outline="green", width=2)

    # ComponentList start Y (yBase)
    yBase = 228.372 + 45.6 + 2.4

    # Entry 1
    entry_x = 10
    entry_y = yBase
    draw.rectangle([entry_x, entry_y, entry_x+620, entry_y+66], outline="blue", width=2)
    
    # Text 1
    text_x = entry_x + 75.7778
    text_y = entry_y + 0
    draw.rectangle([text_x, text_y, text_x+544.222, text_y+36], outline="magenta", width=1)
    
    # Substring 1
    sub_x = entry_x + 75.7778
    sub_y = entry_y + 36
    draw.rectangle([sub_x, sub_y, sub_x+544.222, sub_y+30], outline="cyan", width=1)

    # Entry 2
    entry_y += 66
    draw.rectangle([entry_x, entry_y, entry_x+620, entry_y+66], outline="blue", width=2)
    
    # Text 2
    text_x = entry_x + 75.7778
    text_y = entry_y + 0
    draw.rectangle([text_x, text_y, text_x+544.222, text_y+36], outline="magenta", width=1)
    
    # Substring 2
    sub_x = entry_x + 75.7778
    sub_y = entry_y + 36
    draw.rectangle([sub_x, sub_y, sub_x+544.222, sub_y+30], outline="cyan", width=1)

    # Entry 3
    entry_y += 66
    draw.rectangle([entry_x, entry_y, entry_x+620, entry_y+66], outline="blue", width=2)

    img.save('/Users/jacko/Documents/MyEmulationStation/simulations/game_achievements/cxx_bounds_check.png')
    print("Saved bounding boxes to cxx_bounds_check.png")

if __name__ == "__main__":
    draw_boxes()
