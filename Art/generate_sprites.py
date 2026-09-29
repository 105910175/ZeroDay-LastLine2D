"""Generates the placeholder-quality-but-stylised game sprites into Art/Source.

Run with:  python Art/generate_sprites.py
Needs:     pip install Pillow

Every sprite is drawn at 4x size and scaled down so the edges are smooth. The look is flat
shapes with a dark outline and a neon accent colour per object, so each thing reads clearly
on top of the city map. Sizes are in pixels and Paper2D shows one pixel as one world unit.
"""

import math
import os
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Source")
SS = 4  # supersampling factor

# Palette (RGBA).
NAVY = (11, 16, 32, 255)
STEEL = (66, 83, 111, 255)
STEEL_DARK = (44, 56, 78, 255)
STEEL_LIGHT = (111, 134, 168, 255)
CYAN = (51, 214, 255, 255)
CYAN_DARK = (20, 120, 160, 255)
MAGENTA = (255, 61, 129, 255)
RED = (255, 74, 61, 255)
ORANGE = (255, 154, 46, 255)
YELLOW = (255, 217, 59, 255)
GREEN = (75, 227, 106, 255)
GREEN_DARK = (32, 130, 60, 255)
SKIN = (120, 214, 138, 255)
SKIN_DARK = (70, 150, 90, 255)
WHITE = (245, 248, 255, 255)
PINK = (255, 105, 140, 255)
BROWN = (150, 96, 52, 255)
BROWN_DARK = (98, 60, 30, 255)


class Canvas:
    """A supersampled RGBA canvas. All coordinates passed in are in final pixels."""

    def __init__(self, w, h):
        self.w, self.h = w, h
        self.img = Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.img)

    def _s(self, v):
        return [c * SS for c in v]

    def ellipse(self, box, fill=None, outline=None, width=0):
        self.d.ellipse(self._s(box), fill=fill, outline=outline, width=int(width * SS))

    def rect(self, box, fill=None, outline=None, width=0, radius=0):
        if radius:
            self.d.rounded_rectangle(self._s(box), radius=radius * SS, fill=fill, outline=outline, width=int(width * SS))
        else:
            self.d.rectangle(self._s(box), fill=fill, outline=outline, width=int(width * SS))

    def poly(self, pts, fill=None, outline=None, width=0):
        flat = [(x * SS, y * SS) for x, y in pts]
        self.d.polygon(flat, fill=fill)
        if outline:
            self.d.line(flat + [flat[0]], fill=outline, width=int(width * SS), joint="curve")

    def line(self, pts, fill, width):
        self.d.line([(x * SS, y * SS) for x, y in pts], fill=fill, width=int(width * SS), joint="curve")

    def glow(self, box, color, blur, alpha=170):
        """A soft glow blob under everything drawn afterwards."""
        layer = Image.new("RGBA", self.img.size, (0, 0, 0, 0))
        ImageDraw.Draw(layer).ellipse(self._s(box), fill=(color[0], color[1], color[2], alpha))
        layer = layer.filter(ImageFilter.GaussianBlur(blur * SS))
        self.img = Image.alpha_composite(self.img, layer)
        self.d = ImageDraw.Draw(self.img)

    def shadow(self, box, alpha=90):
        self.glow(box, (0, 0, 0), 4, alpha)

    def save(self, name):
        os.makedirs(OUT, exist_ok=True)
        out = self.img.resize((self.w, self.h), Image.LANCZOS)
        out.save(os.path.join(OUT, name))
        return out


# --------------------------------------------------------------------------- threat
def threat():
    """A shambling glitch-zombie seen from above, facing right (+X)."""
    c = Canvas(128, 128)
    c.shadow((14, 26, 116, 104))
    c.glow((30, 20, 110, 108), MAGENTA, 8, 70)  # unstable red-pink aura
    # Arms reaching forward.
    for y in (30, 82):
        c.rect((52, y, 112, y + 18), fill=SKIN, outline=NAVY, width=3, radius=9)
        c.rect((52, y + 9, 112, y + 18), fill=SKIN_DARK, radius=8)
        for k in range(3):  # claws
            cy = y + 3 + k * 6
            c.poly([(110, cy), (124, cy + 3), (110, cy + 6)], fill=WHITE, outline=NAVY, width=2)
    # Torso / shoulders.
    c.ellipse((22, 26, 82, 102), fill=(42, 53, 80, 255), outline=NAVY, width=3)
    c.ellipse((30, 34, 74, 94), fill=(58, 72, 108, 255))
    for i, y in enumerate((48, 60, 72)):  # torn-jacket glitch stripes
        col = MAGENTA if i % 2 == 0 else CYAN
        c.rect((34, y, 70, y + 4), fill=col)
    # Head.
    c.ellipse((52, 40, 100, 88), fill=SKIN, outline=NAVY, width=3)
    c.ellipse((58, 46, 94, 82), fill=(140, 230, 158, 255))
    # Glowing red eyes.
    for y in (54, 72):
        c.glow((84, y - 6, 100, y + 6), RED, 3, 220)
        c.ellipse((86, y - 5, 97, y + 5), fill=RED, outline=NAVY, width=2)
        c.ellipse((90, y - 2, 95, y + 2), fill=WHITE)
    # Glitch blocks around the head.
    for (x, y, w, h, col) in ((44, 30, 10, 5, CYAN), (100, 96, 12, 5, MAGENTA), (96, 24, 8, 4, MAGENTA), (40, 96, 10, 4, CYAN)):
        c.rect((x, y, x + w, y + h), fill=col)
    return c.save("threat.png")


# --------------------------------------------------------------------------- walls
# The wall is drawn taller than before (104px, was 64px) with a thick reinforced footing
# below the main plate, so it reads as a solid barrier rather than a thin line next to the
# player. Paper2D shows one pixel as one world unit, so this directly makes the wall deeper.
def wall_base(c):
    c.shadow((6, 12, 250, 100), 110)
    c.rect((2, 6, 254, 56), fill=STEEL, outline=NAVY, width=3, radius=8)  # main plate (unchanged from before)
    c.rect((6, 30, 250, 52), fill=STEEL_DARK, radius=6)
    for x in range(64, 256, 64):  # panel seams
        c.line([(x, 8), (x, 54)], NAVY, 2)
    for x in range(14, 250, 32):  # rivets
        c.ellipse((x, 42, x + 6, 48), fill=STEEL_LIGHT, outline=NAVY, width=1)
    # Reinforced footing: a thick base course below the plate, so the wall looks chunky and load-bearing.
    c.rect((0, 60, 256, 68), fill=(14, 18, 30, 255))  # dark seam separating plate from footing
    c.rect((2, 68, 254, 102), fill=STEEL_DARK, outline=NAVY, width=3, radius=8)
    for x in range(16, 250, 30):  # footing plates
        c.rect((x, 76, x + 18, 94), fill=STEEL, outline=NAVY, width=2, radius=3)


def wall_intact():
    c = Canvas(256, 104)
    wall_base(c)
    c.glow((10, 8, 246, 26), CYAN, 5, 150)
    c.rect((10, 12, 246, 22), fill=CYAN, outline=CYAN_DARK, width=1, radius=4)  # energy strip
    c.rect((14, 14, 242, 17), fill=WHITE, radius=2)
    return c.save("wall_intact.png")


def wall_damaged():
    c = Canvas(256, 104)
    wall_base(c)
    for (x0, x1) in ((10, 84), (100, 128), (146, 246)):  # flickering energy strip
        c.rect((x0, 12, x1, 22), fill=(230, 160, 70, 255), outline=NAVY, width=1, radius=3)
    for pts in (  # cracks now run all the way down into the footing
        [(70, 6), (78, 22), (66, 34), (80, 50), (68, 74), (78, 96)],
        [(176, 6), (168, 20), (182, 32), (170, 46), (180, 70), (168, 96)],
        [(120, 10), (126, 24), (116, 38), (124, 60), (114, 90)],
    ):
        c.line(pts, NAVY, 4)
        c.line(pts, ORANGE, 2)
    c.rect((198, 26, 226, 44), fill=(0, 0, 0, 0))
    c.poly([(198, 26), (226, 30), (222, 44), (200, 42)], fill=(20, 24, 36, 255), outline=ORANGE, width=2)
    for x in range(8, 250, 24):  # hazard stripe across the footing
        c.poly([(x, 86), (x + 12, 86), (x + 6, 92), (x - 6, 92)], fill=YELLOW)
    return c.save("wall_damaged.png")


def wall_breached():
    c = Canvas(256, 104)
    c.shadow((6, 12, 250, 100), 110)
    for (x0, x1) in ((2, 92), (164, 254)):  # two broken halves with a gap, full height
        c.rect((x0, 6, x1, 100), fill=STEEL_DARK, outline=NAVY, width=3, radius=8)
    c.poly([(80, 6), (96, 6), (88, 22), (98, 34), (86, 62), (98, 80), (86, 100), (72, 100)], fill=STEEL_DARK, outline=NAVY, width=3)
    c.poly([(176, 6), (160, 6), (168, 22), (158, 34), (170, 62), (158, 80), (170, 100), (184, 100)], fill=STEEL_DARK, outline=NAVY, width=3)
    c.glow((92, 8, 164, 100), RED, 8, 190)  # warning glow in the gap
    c.rect((100, 10, 156, 96), fill=(60, 10, 14, 220), radius=6)
    for pts in ([(104, 44), (116, 30), (128, 46)], [(130, 48), (142, 34), (152, 50)], [(112, 76), (124, 64), (134, 80)]):  # rubble
        c.poly(pts, fill=STEEL, outline=NAVY, width=2)
    for x in (108, 128, 148):  # warning stripes
        c.poly([(x, 12), (x + 8, 12), (x + 14, 20), (x + 6, 20)], fill=RED)
    c.rect((10, 12, 80, 22), fill=(90, 96, 110, 255), outline=NAVY, width=1, radius=3)  # dead strip (top)
    c.rect((176, 12, 246, 22), fill=(90, 96, 110, 255), outline=NAVY, width=1, radius=3)
    c.rect((10, 76, 80, 86), fill=(90, 96, 110, 255), outline=NAVY, width=1, radius=3)  # dead strip (footing)
    c.rect((176, 76, 246, 86), fill=(90, 96, 110, 255), outline=NAVY, width=1, radius=3)
    return c.save("wall_breached.png")


# --------------------------------------------------------------------------- stations
def station_power():
    c = Canvas(160, 160)
    c.shadow((10, 14, 152, 150), 110)
    c.rect((10, 10, 150, 148), fill=STEEL, outline=NAVY, width=4, radius=16)
    c.rect((18, 18, 142, 140), fill=STEEL_DARK, radius=12)
    for (x, y) in ((22, 22), (130, 22), (22, 130), (130, 130)):
        c.ellipse((x, y, x + 8, y + 8), fill=STEEL_LIGHT, outline=NAVY, width=1)
    c.glow((30, 30, 130, 130), YELLOW, 10, 150)
    c.ellipse((34, 32, 126, 124), fill=(30, 38, 58, 255), outline=YELLOW, width=4)  # turbine housing
    for k in range(6):  # turbine blades
        a = k * math.pi / 3
        cx, cy = 80, 78
        c.poly([(cx, cy), (cx + math.cos(a) * 40, cy + math.sin(a) * 40), (cx + math.cos(a + 0.5) * 34, cy + math.sin(a + 0.5) * 34)], fill=(60, 74, 104, 255))
    c.poly([(86, 40), (66, 82), (80, 82), (72, 118), (98, 70), (84, 70)], fill=YELLOW, outline=NAVY, width=3)  # bolt
    return c.save("station_power.png")


def crate(c, x, y, w, h):
    c.rect((x, y, x + w, y + h), fill=BROWN, outline=NAVY, width=3, radius=4)
    c.rect((x + 4, y + 4, x + w - 4, y + h - 4), fill=(178, 120, 70, 255), radius=3)
    c.line([(x + 4, y + h / 2), (x + w - 4, y + h / 2)], BROWN_DARK, 3)
    c.line([(x + w / 2, y + 4), (x + w / 2, y + h - 4)], BROWN_DARK, 3)


def station_food():
    c = Canvas(160, 160)
    c.shadow((10, 14, 152, 150), 110)
    c.rect((8, 8, 152, 148), fill=(30, 66, 44, 255), outline=NAVY, width=4, radius=16)
    c.rect((16, 16, 144, 140), fill=(38, 92, 58, 255), radius=12)
    crate(c, 22, 24, 62, 52)
    crate(c, 90, 30, 50, 46)
    crate(c, 36, 84, 66, 50)
    c.ellipse((108, 86, 140, 118), fill=RED, outline=NAVY, width=3)  # apple
    c.ellipse((114, 90, 124, 100), fill=(255, 140, 130, 255))
    c.poly([(124, 84), (136, 78), (132, 90)], fill=GREEN, outline=NAVY, width=2)  # leaf
    c.line([(124, 86), (124, 94)], NAVY, 3)
    return c.save("station_food.png")


def station_medical():
    c = Canvas(160, 160)
    c.shadow((10, 14, 152, 150), 110)
    c.rect((8, 8, 152, 148), fill=(200, 230, 240, 255), outline=NAVY, width=4, radius=18)
    c.rect((16, 16, 144, 140), fill=WHITE, radius=14)
    c.rect((16, 16, 144, 34), fill=CYAN, radius=10)  # tent roof strip
    c.glow((44, 44, 116, 116), RED, 8, 120)
    c.rect((66, 44, 94, 116), fill=RED, outline=NAVY, width=3, radius=5)  # cross
    c.rect((44, 66, 116, 94), fill=RED, outline=NAVY, width=3, radius=5)
    c.rect((69, 47, 91, 113), fill=RED)
    c.rect((47, 69, 113, 91), fill=RED)
    for (x, y) in ((24, 124), (128, 124)):
        c.ellipse((x - 6, y - 6, x + 6, y + 6), fill=(180, 200, 214, 255), outline=NAVY, width=2)
    return c.save("station_medical.png")


# --------------------------------------------------------------------------- projectile
def bolt():
    c = Canvas(64, 24)
    c.glow((4, 2, 62, 22), CYAN, 4, 200)
    c.poly([(2, 12), (30, 7), (30, 17)], fill=(51, 214, 255, 140))  # fading tail
    c.ellipse((26, 4, 62, 20), fill=CYAN, outline=NAVY, width=2)
    c.ellipse((34, 8, 58, 16), fill=WHITE)
    return c.save("bolt.png")


# --------------------------------------------------------------------------- pickups
def pickup_base(c, accent):
    c.shadow((6, 10, 60, 60), 120)
    c.glow((4, 4, 60, 60), accent, 5, 140)
    c.ellipse((6, 6, 58, 58), fill=(24, 30, 50, 235), outline=accent, width=3)


def pickup_intel():
    c = Canvas(64, 64)
    pickup_base(c, CYAN)
    c.rect((17, 17, 47, 47), fill=CYAN_DARK, outline=NAVY, width=3, radius=4)
    c.rect((22, 22, 42, 42), fill=CYAN, radius=3)
    for i in range(4):  # pins
        p = 21 + i * 7
        for (x0, y0, x1, y1) in ((p, 11, p + 3, 17), (p, 47, p + 3, 53), (11, p, 17, p + 3), (47, p, 53, p + 3)):
            c.rect((x0, y0, x1, y1), fill=STEEL_LIGHT)
    c.line([(26, 34), (32, 34), (32, 27), (38, 27)], NAVY, 2)  # circuit traces
    c.line([(26, 38), (36, 38)], NAVY, 2)
    c.ellipse((36, 33, 40, 37), fill=WHITE)
    return c.save("pickup_intel.png")


def pickup_patch():
    c = Canvas(64, 64)
    pickup_base(c, ORANGE)
    cx = cy = 32
    pts = []
    for k in range(16):  # gear outline
        a0 = k * math.pi / 8
        r = 21 if k % 2 == 0 else 15
        pts += [(cx + math.cos(a0) * r, cy + math.sin(a0) * r), (cx + math.cos(a0 + math.pi / 16) * r, cy + math.sin(a0 + math.pi / 16) * r)]
    c.poly(pts, fill=ORANGE, outline=NAVY, width=3)
    c.ellipse((22, 22, 42, 42), fill=(24, 30, 50, 255), outline=NAVY, width=2)
    c.ellipse((27, 27, 37, 37), fill=STEEL_LIGHT)
    return c.save("pickup_patch.png")


def pickup_food():
    c = Canvas(64, 64)
    pickup_base(c, GREEN)
    c.rect((15, 19, 49, 49), fill=GREEN_DARK, outline=NAVY, width=3, radius=5)
    c.rect((19, 23, 45, 45), fill=GREEN, radius=4)
    c.ellipse((23, 26, 41, 44), fill=RED, outline=NAVY, width=2)  # apple
    c.ellipse((26, 29, 31, 34), fill=(255, 150, 140, 255))
    c.poly([(33, 24), (42, 19), (38, 28)], fill=(40, 170, 70, 255), outline=NAVY, width=2)
    return c.save("pickup_food.png")


def pickup_power():
    c = Canvas(64, 64)
    pickup_base(c, YELLOW)
    c.rect((29, 13, 35, 19), fill=STEEL_LIGHT, outline=NAVY, width=2)  # terminal
    c.rect((19, 18, 45, 50), fill=(70, 60, 20, 255), outline=NAVY, width=3, radius=5)
    c.rect((23, 22, 41, 46), fill=YELLOW, radius=3)
    c.poly([(34, 24), (27, 35), (32, 35), (29, 44), (38, 32), (33, 32)], fill=NAVY)  # bolt
    return c.save("pickup_power.png")


def pickup_medical():
    c = Canvas(64, 64)
    pickup_base(c, PINK)
    c.rect((14, 18, 50, 50), fill=WHITE, outline=NAVY, width=3, radius=6)
    c.rect((26, 12, 38, 20), fill=STEEL_LIGHT, outline=NAVY, width=2, radius=2)  # handle
    c.rect((29, 24, 35, 44), fill=RED)
    c.rect((23, 30, 41, 38), fill=RED)
    return c.save("pickup_medical.png")


# --------------------------------------------------------------------------- HUD icon
def icon_survivor():
    c = Canvas(48, 48)
    c.glow((6, 6, 42, 42), GREEN, 5, 110)
    c.ellipse((16, 6, 32, 22), fill=SKIN, outline=NAVY, width=3)
    c.rect((10, 24, 38, 44), fill=(46, 120, 200, 255), outline=NAVY, width=3, radius=9)
    return c.save("icon_survivor.png")


ALL = [threat, wall_intact, wall_damaged, wall_breached, station_power, station_food, station_medical, bolt,
       pickup_intel, pickup_patch, pickup_food, pickup_power, pickup_medical, icon_survivor]


def contact_sheet(images):
    """Writes a labelled sheet on a map-coloured background so the art can be checked at a glance."""
    pad = 16
    width = 1100
    x, y, row_h = pad, pad, 0
    placed = []
    for name, im in images:
        if x + im.width + pad > width:
            x, y, row_h = pad, y + row_h + pad, 0
        placed.append((name, im, x, y))
        x += im.width + pad
        row_h = max(row_h, im.height)
    sheet = Image.new("RGBA", (width, y + row_h + pad), (150, 140, 175, 255))
    for name, im, px, py in placed:
        sheet.alpha_composite(im, (px, py))
    sheet.save(os.path.join(OUT, "_contact_sheet.png"))


if __name__ == "__main__":
    made = [(fn.__name__, fn()) for fn in ALL]
    contact_sheet(made)
    print("wrote %d sprites to %s" % (len(made), OUT))
