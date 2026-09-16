"""Draw a new typography specimen with the project's actual free font files."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
FONTS = HERE.parents[2] / 'assets' / 'fonts'
W, H = 1672, 941
INK = '#07364B'
CREAM = '#FFF7E6'
im = Image.new('RGB', (W, H), '#072D40')
d = ImageDraw.Draw(im)

def font(file, size):
    face = ImageFont.truetype(str(FONTS / file), size)
    if file == 'Nunito-Variable.ttf':
        face.set_variation_by_name('SemiBold')
    return face

body_file = 'Nunito-Variable.ttf'
body = lambda size: font(body_file, size)

def text(x, y, value, face, fill=INK, center=False, stroke=0, shadow=False):
    box = d.textbbox((0, 0), value, font=face, stroke_width=stroke)
    width = box[2] - box[0]
    left = x - width / 2 if center else x
    pos = (left - box[0], y - box[1])
    if shadow:
        d.text((pos[0], pos[1] + 5), value, font=face, fill=INK,
               stroke_width=stroke, stroke_fill=INK)
    d.text(pos, value, font=face, fill=fill, stroke_width=stroke, stroke_fill=INK)

text(48, 33, 'FISHIUS / TYPOGRAPHY', body(19), '#76DEE4')
text(45, 76, 'Free fonts. Big personality.', font('LuckiestGuy-Regular.ttf', 54), CREAM)
text(48, 143, 'These samples use the actual font files already included in the project.', body(25), '#CAE6E8')

specs = [
    ('Luckiest Guy', 'LuckiestGuy-Regular.ttf', 'Closest heading match',
     'Big titles and celebrations.', 'Apache 2.0 licence'),
    ('Lilita One', 'LilitaOne-Regular.ttf', 'Clearer mixed-case labels',
     'Buttons, fish names and prices.', 'SIL Open Font License 1.1'),
    ('Nunito SemiBold', body_file, 'Readable supporting text',
     'Descriptions, settings and stats.', 'SIL Open Font License 1.1'),
]
card_w = 509
for index, (name, file, detail, usage, licence) in enumerate(specs):
    x, y = 48 + index * 533, 202
    right, bottom = x + card_w, 684
    d.rounded_rectangle((x, y+7, right, bottom+7), radius=24, fill='#001E2D')
    d.rounded_rectangle((x, y, right, bottom), radius=24, fill=CREAM, outline='#76D7E2', width=3)
    text(x+24, y+22, name, body(30))
    text(x+24, y+66, detail, body(19), '#3A6876')
    d.rounded_rectangle((x+12, y+110, right-12, y+385), radius=18, fill='#37CCE6')
    text(x+card_w/2, y+137, 'Coins & Pearls', font(file, 48), '#FFFFFF', True, 3, True)
    text(x+card_w/2, y+210, '2,500 / 90 / $4.99', font(file, 35), '#FFFFFF', True, 2, True)
    bx, by = x+45, y+294
    d.rounded_rectangle((bx, by+5, right-45, by+66), radius=16, fill='#12632A')
    d.rounded_rectangle((bx, by, right-45, by+61), radius=16, fill='#56D82B', outline='#1C6E2F', width=3)
    d.rounded_rectangle((bx+5, by+4, right-50, by+26), radius=12, fill='#9CEF66')
    text(x+card_w/2, by+13, 'OPEN SHOP', font(file, 31), '#FFFFFF', True, 2, True)
    text(x+24, y+409, usage, body(22))
    text(x+24, y+447, licence, body(17), '#426D79')

d.rounded_rectangle((48, 728, 1624, 864), radius=21, fill='#0D5D6A', outline='#3DBDC5', width=2)
text(74, 749, 'Recommended combination', font('LilitaOne-Regular.ttf', 32), CREAM)
text(74, 798, 'Luckiest Guy headings + Lilita One controls + Nunito descriptions.', body(25), '#DCF8F3')
text(74, 835, 'White fill, navy outline, short shadow. Keep small body text plain.', body(18), '#C1E6E2')
text(50, 895, 'Free to use in the game with their licence notices. No custom font file was made.', body(20), '#ADCDD2')
im.save(HERE / '09-free-fonts.png')
print(HERE / '09-free-fonts.png')
