#!/usr/bin/env python3
"""Create a custom start-up image for the Canon PowerShot G1 X Mark III (firmware GM1.00A).

Output: a JPEG (baseline, 640x480, 4:2:0, at most 12179 bytes) and a C file built from it,
which the g1x3_splash build links into the core. The camera shows the image at power-on
when the camera menu "Start-up Image" is set to 2 (background: docs/NOTES_de.md, section "Startbild").

Examples:
  python3 make_splash.py                                      # example image example.png
  python3 make_splash.py my_image.png                         # your own image, black background
  python3 make_splash.py my_image.png --background ffffff --width 400
  python3 make_splash.py my_image.png --logo-from-dump ../dumps/PRIMARY.BIN   # Canon lettering below

The Canon lettering is not in the repository. --logo-from-dump takes it from your own firmware dump
(Canon's start-up image in ROM at 0xE0A741A0, 5182 bytes) and smooths its edges.
Requires: Python 3, Pillow, numpy, opencv-python (pip install pillow numpy opencv-python).
"""
import argparse, io, os, struct, sys
import numpy as np, cv2
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 640, 480
BUF_SIZE = 0x2f93            # RAM buffer for Canon's start-up image = size Canon reports for "Start-up Image: 2"
LIMIT = BUF_SIZE - 100       # headroom
ROM_BASE = 0xE0000000
LOGO_ADDR, LOGO_LEN = 0xE0A741A0, 0x143E   # Canon start-up image (set 0) in the GM1.00A ROM
UP = 8                       # working resolution for smoothing the lettering


def color(s):
    s = s.lstrip('#')
    if len(s) != 6:
        raise argparse.ArgumentTypeError('give the color as RRGGBB, e.g. 000000')
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))


def canon_logo(dump, scale):
    """Canon lettering from your own dump: decode Canon's start-up image, cut out the lettering, smooth the edges."""
    with open(dump, 'rb') as f:
        f.seek(LOGO_ADDR - ROM_BASE)
        jpg = f.read(LOGO_LEN)
    if len(jpg) != LOGO_LEN or jpg[:2] != b'\xff\xd8':
        sys.exit('Dump does not match: no JPEG at 0x%08X (only firmware GM1.00A is supported).' % LOGO_ADDR)
    im = Image.open(io.BytesIO(jpg))
    if im.size != (W, H):
        sys.exit('Dump does not match: Canon start-up image is not 640x480 (only firmware GM1.00A).')
    im.draft('L', im.size)                                   # decode the luma channel (Y) directly
    L = np.asarray(im.convert('L')).astype(np.float32)
    ly, lx = np.nonzero(L > 128)
    L = L[ly.min() - 2:ly.max() + 3, lx.min() - 2:lx.max() + 3]
    Lb = cv2.GaussianBlur(cv2.resize(L, (L.shape[1] * UP, L.shape[0] * UP), interpolation=cv2.INTER_CUBIC),
                          (0, 0), UP * 0.5)
    lw, lh = round(L.shape[1] * scale), round(L.shape[0] * scale)
    return cv2.resize((Lb > 128).astype(np.float32), (lw, lh), interpolation=cv2.INTER_AREA)


def main():
    ap = argparse.ArgumentParser(description='Create a custom start-up image for g1x3_splash.')
    ap.add_argument('image', nargs='?', default=os.path.join(HERE, 'example.png'),
                    help='image file (PNG/JPEG, transparency allowed). Default: example.png')
    ap.add_argument('--background', type=color, default=(0, 0, 0), help='background color RRGGBB (default 000000)')
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--scale', type=float, help='scale the image by this factor (1 = original size)')
    g.add_argument('--width', type=int, help='scale the image to this width in pixels')
    ap.add_argument('--logo-from-dump', metavar='PRIMARY.BIN', help='put the Canon lettering from your own firmware dump below the image')
    ap.add_argument('--logo-scale', type=float, default=0.75, help='size of the lettering relative to the original (default 0.75)')
    ap.add_argument('--logo-color', type=color, help='color of the lettering RRGGBB (default: white on dark, black on light)')
    ap.add_argument('--gap', type=int, default=40, help='gap between image and lettering in pixels (default 40)')
    ap.add_argument('--out-c', default=os.path.join(HERE, '..', 'chdk', 'g1x3_splash', 'splash_jpg.c'),
                    help='C file for the build (default ../chdk/g1x3_splash/splash_jpg.c)')
    ap.add_argument('--out-jpg', default=os.path.join(HERE, 'splash.jpg'), help='preview JPEG (default splash.jpg)')
    a = ap.parse_args()

    bg = np.array(a.background, np.float32)
    src = Image.open(a.image).convert('RGBA')
    rgba = np.asarray(src).astype(np.float32)
    al = rgba[:, :, 3:4] / 255
    fig = (rgba[:, :, :3] * al + bg * (1 - al)).round().astype(np.uint8)      # transparency -> background

    logo = canon_logo(a.logo_from_dump, a.logo_scale) if a.logo_from_dump else None
    lh = (logo.shape[0] + a.gap) if logo is not None else 0
    if a.scale:
        s = a.scale
    elif a.width:
        s = a.width / fig.shape[1]
    else:                                                                      # fit, 20 px margin
        s = min((W - 40) / fig.shape[1], (H - 40 - lh) / fig.shape[0])
    w, h = round(fig.shape[1] * s), round(fig.shape[0] * s)
    if w > W or h + lh > H:
        sys.exit('Image too large for 640x480 (%dx%d + lettering %d px). Use a smaller --scale or --width.' % (w, h, lh))
    if (w, h) != (fig.shape[1], fig.shape[0]):
        fig = np.asarray(Image.fromarray(fig).resize((w, h), Image.LANCZOS))

    can = np.zeros((H, W, 3), np.uint8)
    can[:] = a.background
    top = (H - (h + lh)) // 2                                                  # image + gap + lettering centered
    can[top:top + h, (W - w) // 2:(W - w) // 2 + w] = fig
    if logo is not None:
        dark = sum(a.background) < 384
        col = np.array(a.logo_color or ((255, 255, 255) if dark else (0, 0, 0)), np.float32)
        lw_, lh_ = logo.shape[1], logo.shape[0]
        y0, x0 = top + h + a.gap, (W - lw_) // 2
        reg = can[y0:y0 + lh_, x0:x0 + lw_].astype(np.float32)
        t = logo[:, :, None]
        can[y0:y0 + lh_, x0:x0 + lw_] = (reg * (1 - t) + col * t).round().astype(np.uint8)
    can = Image.fromarray(can)

    def encode(q):
        b = io.BytesIO()
        can.save(b, 'JPEG', quality=q, subsampling=2, optimize=True, progressive=False)
        return b.getvalue()

    ok = [q for q in range(5, 96) if len(encode(q)) <= LIMIT]
    if not ok:
        sys.exit('Image does not fit into %d bytes even at quality 5. Use less detail or a smaller image.' % LIMIT)
    q = max(ok)
    jpg = encode(q)

    # Check: baseline (SOF0), 640x480, 3 components, Y 2x2 / Cb,Cr 1x1, ends with EOI (like Canon's own image)
    i, sof = 2, None
    while i < len(jpg):
        m, n = jpg[i + 1], struct.unpack('>H', jpg[i + 2:i + 4])[0]
        if m in (0xC0, 0xC1, 0xC2):
            sof = (m, jpg[i + 4:i + 2 + n])
        if m == 0xDA:
            break
        i += 2 + n
    assert sof and sof[0] == 0xC0, 'not a baseline JPEG'
    p = sof[1]
    assert struct.unpack('>HH', p[1:5]) == (H, W) and p[5] == 3
    assert [p[7], p[10], p[13]] == [0x22, 0x11, 0x11]
    assert jpg[:2] == b'\xff\xd8' and jpg[-2:] == b'\xff\xd9'
    assert len(jpg) <= BUF_SIZE

    open(a.out_jpg, 'wb').write(jpg)
    os.makedirs(os.path.dirname(os.path.abspath(a.out_c)), exist_ok=True)
    with open(a.out_c, 'w') as f:
        f.write('// GENERATED by splash/make_splash.py - do not edit by hand\n')
        f.write('// %s, JPEG baseline 640x480 4:2:0, quality %d, %d bytes (buffer: %d)\n'
                % (os.path.basename(a.image), q, len(jpg), BUF_SIZE))
        f.write('const unsigned int splash_len = %d;\n' % len(jpg))
        f.write('const unsigned char splash_jpg[%d] __attribute__((aligned(4))) = {\n' % len(jpg))
        for k in range(0, len(jpg), 16):
            f.write('    ' + ','.join('0x%02x' % c for c in jpg[k:k + 16]) + ',\n')
        f.write('};\n')
    print('quality %d, %d bytes -> %s, %s' % (q, len(jpg), a.out_jpg, a.out_c))


if __name__ == '__main__':
    main()
