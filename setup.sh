#!/bin/bash
# setup.sh - einmalig: CHDK-Quelltext holen und die Build-Umgebung (Docker-Image "chdkbuild") bauen.
#
#   ./setup.sh
#
# Holt CHDK trunk in genau der Revision, gegen die dieser Port entwickelt und getestet wurde (r6358),
# nach ./chdk. Der g1x3-Port selbst (Ordner port/) wird erst von build.sh dazukopiert.
# Voraussetzungen: subversion (svn), docker (macOS: z. B. Colima oder Docker Desktop, vorher starten).
set -euo pipefail
cd "$(dirname "$0")"

CHDK_REV=6358
CHDK_URL=https://subversion.assembla.com/svn/chdk/trunk

command -v svn >/dev/null    || { echo "svn fehlt (macOS: brew install subversion, Debian/Ubuntu: apt install subversion)"; exit 1; }
command -v docker >/dev/null || { echo "docker fehlt (macOS: brew install colima docker && colima start)"; exit 1; }
docker info >/dev/null 2>&1  || { echo "Docker laeuft nicht (macOS mit Colima: colima start)"; exit 1; }

if [ ! -d chdk ]; then
    echo "Hole CHDK trunk r$CHDK_REV nach ./chdk ..."
    svn checkout -q -r "$CHDK_REV" "$CHDK_URL" chdk
fi
rev=$(svn info --show-item revision chdk 2>/dev/null || echo "?")
[ "$rev" = "$CHDK_REV" ] || echo "WARNUNG: ./chdk ist Revision $rev, getestet ist r$CHDK_REV (svn update -r $CHDK_REV chdk)"

if ! docker image inspect chdkbuild >/dev/null 2>&1; then
    echo "Baue Docker-Image chdkbuild (offizielles CHDK-Dockerfile: gcc 5.4.1, capstone 4.0.2) ..."
    docker build -t chdkbuild chdk/tools/docker/chdkbuild
fi
echo "Fertig. Weiter mit: ./build.sh final   (oder: ./build.sh startbild)"
