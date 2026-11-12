#!/usr/bin/env python3
"""Eigenes Startbild fuer die Canon PowerShot G1 X Mark III (Firmware GM1.00A) erzeugen.

Ergebnis: ein JPEG (baseline, 640x480, 4:2:0, hoechstens 12179 Byte) und daraus eine C-Datei,
die der Build g1x3_startbild in den Kern einbindet. Die Kamera zeigt das Bild beim Einschalten,
wenn im Kamera-Menue "Startbild" auf 2 steht (Hintergrund: docs/NOTES.md, Abschnitt "Startbild").

Beispiele:
  python3 make_startbild.py                                   # Beispielbild beispiel.png
  python3 make_startbild.py mein_bild.png                     # eigenes Bild, Hintergrund schwarz
  python3 make_startbild.py mein_bild.png --hintergrund ffffff --breite 400
  python3 make_startbild.py mein_bild.png --logo-aus-dump ../dumps/PRIMARY.BIN   # Canon-Schriftzug darunter

Der Canon-Schriftzug ist nicht im Repository. --logo-aus-dump holt ihn aus dem eigenen Firmware-Dump
(Canons Startbild im ROM bei 0xE0A741A0, 5182 Byte) und glaettet seine Kanten.
Benoetigt: Python 3, Pillow, numpy, opencv-python (pip install pillow numpy opencv-python).
"""
import argparse, io, os, struct, sys
import numpy as np, cv2
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
W, H = 640, 480
BUF_SIZE = 0x2f93            # RAM-Puffer fuer Canons Startbild = Groesse, die Canon bei "Startbild: 2" meldet
LIMIT = BUF_SIZE - 100       # Reserve
ROM_BASE = 0xE0000000
LOGO_ADDR, LOGO_LEN = 0xE0A741A0, 0x143E   # Canon-Startbild (Satz 0) im ROM von GM1.00A
UP = 8                       # Arbeitsaufloesung fuer das Glaetten des Schriftzugs


def farbe(s):
    s = s.lstrip('#')
    if len(s) != 6:
        raise argparse.ArgumentTypeError('Farbe als RRGGBB angeben, z. B. 000000')
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))


def canon_logo(dump, skala):
    """Canon-Schriftzug aus dem eigenen Dump: Canons Startbild decodieren, Schrift ausschneiden, Kanten glaetten."""
    with open(dump, 'rb') as f:
        f.seek(LOGO_ADDR - ROM_BASE)
        jpg = f.read(LOGO_LEN)
    if len(jpg) != LOGO_LEN or jpg[:2] != b'\xff\xd8':
        sys.exit('Dump passt nicht: an 0x%08X liegt kein JPEG (nur Firmware GM1.00A wird unterstuetzt).' % LOGO_ADDR)
    im = Image.open(io.BytesIO(jpg))
    if im.size != (W, H):
        sys.exit('Dump passt nicht: Canons Startbild hat nicht 640x480 (nur Firmware GM1.00A).')
    im.draft('L', im.size)                                   # direkt den Helligkeitskanal (Y) decodieren
    L = np.asarray(im.convert('L')).astype(np.float32)
    ly, lx = np.nonzero(L > 128)
    L = L[ly.min() - 2:ly.max() + 3, lx.min() - 2:lx.max() + 3]
    Lb = cv2.GaussianBlur(cv2.resize(L, (L.shape[1] * UP, L.shape[0] * UP), interpolation=cv2.INTER_CUBIC),
                          (0, 0), UP * 0.5)
    lw, lh = round(L.shape[1] * skala), round(L.shape[0] * skala)
    return cv2.resize((Lb > 128).astype(np.float32), (lw, lh), interpolation=cv2.INTER_AREA)


def main():
    ap = argparse.ArgumentParser(description='Eigenes Startbild fuer g1x3_startbild erzeugen.')
    ap.add_argument('bild', nargs='?', default=os.path.join(HERE, 'beispiel.png'),
                    help='Bilddatei (PNG/JPEG, Transparenz erlaubt). Standard: beispiel.png')
    ap.add_argument('--hintergrund', type=farbe, default=(0, 0, 0), help='Hintergrundfarbe RRGGBB (Standard 000000)')
    g = ap.add_mutually_exclusive_group()
    g.add_argument('--skala', type=float, help='Bild um diesen Faktor skalieren (1 = Originalgroesse)')
    g.add_argument('--breite', type=int, help='Bild auf diese Breite in Pixeln skalieren')
    ap.add_argument('--logo-aus-dump', metavar='PRIMARY.BIN', help='Canon-Schriftzug aus dem eigenen Firmware-Dump darunter setzen')
    ap.add_argument('--logo-skala', type=float, default=0.75, help='Groesse des Schriftzugs relativ zum Original (Standard 0.75)')
    ap.add_argument('--logo-farbe', type=farbe, help='Farbe des Schriftzugs RRGGBB (Standard: weiss auf dunkel, schwarz auf hell)')
    ap.add_argument('--abstand', type=int, default=40, help='Abstand Bild -> Schriftzug in Pixeln (Standard 40)')
    ap.add_argument('--ausgabe-c', default=os.path.join(HERE, '..', 'chdk', 'g1x3_startbild', 'startbild_jpg.c'),
                    help='C-Datei fuer den Build (Standard ../chdk/g1x3_startbild/startbild_jpg.c)')
    ap.add_argument('--ausgabe-jpg', default=os.path.join(HERE, 'startbild.jpg'), help='Vorschau-JPEG (Standard startbild.jpg)')
    a = ap.parse_args()

    bg = np.array(a.hintergrund, np.float32)
    src = Image.open(a.bild).convert('RGBA')
    rgba = np.asarray(src).astype(np.float32)
    al = rgba[:, :, 3:4] / 255
    fig = (rgba[:, :, :3] * al + bg * (1 - al)).round().astype(np.uint8)      # Transparenz -> Hintergrund

    logo = canon_logo(a.logo_aus_dump, a.logo_skala) if a.logo_aus_dump else None
    lh = (logo.shape[0] + a.abstand) if logo is not None else 0
    if a.skala:
        s = a.skala
    elif a.breite:
        s = a.breite / fig.shape[1]
    else:                                                                      # passend einpassen, Rand 20 px
        s = min((W - 40) / fig.shape[1], (H - 40 - lh) / fig.shape[0])
    w, h = round(fig.shape[1] * s), round(fig.shape[0] * s)
    if w > W or h + lh > H:
        sys.exit('Bild ist zu gross fuer 640x480 (%dx%d + Schriftzug %d px). --skala oder --breite kleiner waehlen.' % (w, h, lh))
    if (w, h) != (fig.shape[1], fig.shape[0]):
        fig = np.asarray(Image.fromarray(fig).resize((w, h), Image.LANCZOS))

    can = np.zeros((H, W, 3), np.uint8)
    can[:] = a.hintergrund
    top = (H - (h + lh)) // 2                                                  # Bild + Abstand + Schriftzug mittig
    can[top:top + h, (W - w) // 2:(W - w) // 2 + w] = fig
    if logo is not None:
        dunkel = sum(a.hintergrund) < 384
        col = np.array(a.logo_farbe or ((255, 255, 255) if dunkel else (0, 0, 0)), np.float32)
        lw_, lh_ = logo.shape[1], logo.shape[0]
        y0, x0 = top + h + a.abstand, (W - lw_) // 2
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
        sys.exit('Bild passt selbst mit Qualitaet 5 nicht in %d Byte. Weniger Details oder kleiner waehlen.' % LIMIT)
    q = max(ok)
    jpg = encode(q)

    # Pruefen: baseline (SOF0), 640x480, 3 Komponenten, Y 2x2 / Cb,Cr 1x1, endet mit EOI (wie Canons eigenes Bild)
    i, sof = 2, None
    while i < len(jpg):
        m, n = jpg[i + 1], struct.unpack('>H', jpg[i + 2:i + 4])[0]
        if m in (0xC0, 0xC1, 0xC2):
            sof = (m, jpg[i + 4:i + 2 + n])
        if m == 0xDA:
            break
        i += 2 + n
    assert sof and sof[0] == 0xC0, 'kein baseline-JPEG'
    p = sof[1]
    assert struct.unpack('>HH', p[1:5]) == (H, W) and p[5] == 3
    assert [p[7], p[10], p[13]] == [0x22, 0x11, 0x11]
    assert jpg[:2] == b'\xff\xd8' and jpg[-2:] == b'\xff\xd9'
    assert len(jpg) <= BUF_SIZE

    open(a.ausgabe_jpg, 'wb').write(jpg)
    os.makedirs(os.path.dirname(os.path.abspath(a.ausgabe_c)), exist_ok=True)
    with open(a.ausgabe_c, 'w') as f:
        f.write('// ERZEUGT von startbild/make_startbild.py - nicht von Hand aendern\n')
        f.write('// %s, JPEG baseline 640x480 4:2:0, Qualitaet %d, %d Byte (Puffer: %d)\n'
                % (os.path.basename(a.bild), q, len(jpg), BUF_SIZE))
        f.write('const unsigned int startbild_len = %d;\n' % len(jpg))
        f.write('const unsigned char startbild_jpg[%d] __attribute__((aligned(4))) = {\n' % len(jpg))
        for k in range(0, len(jpg), 16):
            f.write('    ' + ','.join('0x%02x' % c for c in jpg[k:k + 16]) + ',\n')
        f.write('};\n')
    print('Qualitaet %d, %d Byte -> %s, %s' % (q, len(jpg), a.ausgabe_jpg, a.ausgabe_c))


if __name__ == '__main__':
    main()
