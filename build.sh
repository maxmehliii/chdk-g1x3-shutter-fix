#!/bin/bash
# build.sh - builds a DISKBOOT.BIN for the G1 X Mark III (firmware GM1.00A).
#
#   ./build.sh final       shutter replacement (hold S1 + Q/SET = S2), everything else as Canon
#   ./build.sh splash      like final, plus a custom start-up image (run splash/make_splash.py first)
#   ./build.sh stage_ab | stage_c | stage_c2 | stage_d    the test stages from development (docs/NOTES_de.md)
#
# Output: out/DISKBOOT.BIN. Requires: ./setup.sh has been run, Docker is running.
set -euo pipefail
cd "$(dirname "$0")"
ROOT=$(pwd)

target=${1:-final}
dir=g1x3_$target
[ -d "port/$dir" ] || { echo "Unknown target '$target'. Possible: final, splash, stage_ab, stage_c, stage_c2, stage_d"; exit 1; }
[ -d chdk ] || { echo "./chdk is missing - run ./setup.sh first"; exit 1; }

# Copy the port into the CHDK tree (new folders only, CHDK itself stays unchanged)
cp -R port/. chdk/

if [ "$target" = splash ] && [ ! -f chdk/g1x3_splash/splash_jpg.c ]; then
    echo "No start-up image generated yet - using splash/example.png"
    python3 splash/make_splash.py
fi

run() { docker run --rm --user="$(id -u):$(id -g)" -v "$ROOT/chdk:/srv/src" chdkbuild "$@"; }
run make -C "$dir" clean >/dev/null
run make -C "$dir"

mkdir -p out
cp "chdk/$dir/DISKBOOT.BIN" out/DISKBOOT.BIN
echo
echo "Done: out/DISKBOOT.BIN ($target)"
if command -v shasum >/dev/null; then shasum -a 256 out/DISKBOOT.BIN; else sha256sum out/DISKBOOT.BIN; fi
