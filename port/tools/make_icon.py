#!/usr/bin/env python3
"""make_icon.py -- launcher/icon.jpg from the game's own launcher icon.

The APK keeps it as Android adaptive icon layers, 432x432. The middle 72/108 of
them is what a phone shows, so that part is cut out and scaled to the 256x256 JPEG
a Switch icon needs.

usage: make_icon.py <apk> <output.jpg>
"""
import io
import sys
import zipfile

from PIL import Image

z = zipfile.ZipFile(sys.argv[1])


def layer(name):
    return Image.open(io.BytesIO(z.read("res/mipmap-xxxhdpi/%s.png" % name))).convert("RGBA")


icon = layer("ic_launcher_background")
icon.alpha_composite(layer("ic_launcher_foreground"))
size = icon.width
inset = size * 18 // 108
icon = icon.crop((inset, inset, size - inset, size - inset)).convert("RGB")
icon.resize((256, 256), Image.LANCZOS).save(sys.argv[2], "JPEG", quality=93, subsampling=0)
