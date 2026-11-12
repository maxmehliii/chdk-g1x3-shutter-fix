#!/bin/bash
# build.sh - baut eine DISKBOOT.BIN fuer die G1 X Mark III (Firmware GM1.00A).
#
#   ./build.sh final       Ausloeser-Ersatz (S1 halten + Q/SET = S2), sonst alles wie bei Canon
#   ./build.sh startbild   wie final, zusaetzlich eigenes Startbild (vorher startbild/make_startbild.py)
#   ./build.sh stufe_ab | stufe_c | stufe_c2 | stufe_d    die Teststufen aus der Entwicklung (docs/NOTES.md)
#
# Ergebnis: out/DISKBOOT.BIN. Voraussetzung: ./setup.sh wurde ausgefuehrt, Docker laeuft.
set -euo pipefail
cd "$(dirname "$0")"
ROOT=$(pwd)

ziel=${1:-final}
dir=g1x3_$ziel
[ -d "port/$dir" ] || { echo "Unbekanntes Ziel '$ziel'. Moeglich: final, startbild, stufe_ab, stufe_c, stufe_c2, stufe_d"; exit 1; }
[ -d chdk ] || { echo "./chdk fehlt - zuerst ./setup.sh ausfuehren"; exit 1; }

# Port in den CHDK-Baum legen (nur neue Ordner, CHDK selbst bleibt unveraendert)
cp -R port/. chdk/

if [ "$ziel" = startbild ] && [ ! -f chdk/g1x3_startbild/startbild_jpg.c ]; then
    echo "Noch kein Startbild erzeugt - nehme startbild/beispiel.png"
    python3 startbild/make_startbild.py
fi

run() { docker run --rm --user="$(id -u):$(id -g)" -v "$ROOT/chdk:/srv/src" chdkbuild "$@"; }
run make -C "$dir" clean >/dev/null
run make -C "$dir"

mkdir -p out
cp "chdk/$dir/DISKBOOT.BIN" out/DISKBOOT.BIN
echo
echo "Fertig: out/DISKBOOT.BIN ($ziel)"
if command -v shasum >/dev/null; then shasum -a 256 out/DISKBOOT.BIN; else sha256sum out/DISKBOOT.BIN; fi
