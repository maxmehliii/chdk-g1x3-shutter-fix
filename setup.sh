#!/bin/bash
# setup.sh - run once: fetch the CHDK source and build the build environment (Docker image "chdkbuild").
#
#   ./setup.sh
#
# Checks out CHDK trunk at exactly the revision this port was developed and tested against (r6358)
# into ./chdk. The g1x3 port itself (folder port/) is copied in later by build.sh.
# Requires: subversion (svn), docker (macOS: e.g. Colima or Docker Desktop, started beforehand).
set -euo pipefail
cd "$(dirname "$0")"

CHDK_REV=6358
CHDK_URL=https://subversion.assembla.com/svn/chdk/trunk

command -v svn >/dev/null    || { echo "svn is missing (macOS: brew install subversion, Debian/Ubuntu: apt install subversion)"; exit 1; }
command -v docker >/dev/null || { echo "docker is missing (macOS: brew install colima docker && colima start)"; exit 1; }
docker info >/dev/null 2>&1  || { echo "Docker is not running (macOS with Colima: colima start)"; exit 1; }

if [ ! -d chdk ]; then
    echo "Checking out CHDK trunk r$CHDK_REV into ./chdk ..."
    svn checkout -q -r "$CHDK_REV" "$CHDK_URL" chdk
fi
rev=$(svn info --show-item revision chdk 2>/dev/null || echo "?")
[ "$rev" = "$CHDK_REV" ] || echo "WARNING: ./chdk is revision $rev, tested is r$CHDK_REV (svn update -r $CHDK_REV chdk)"

if ! docker image inspect chdkbuild >/dev/null 2>&1; then
    echo "Building Docker image chdkbuild (official CHDK Dockerfile: gcc 5.4.1, capstone 4.0.2) ..."
    docker build -t chdkbuild chdk/tools/docker/chdkbuild
fi
echo "Done. Next: ./build.sh final   (or: ./build.sh splash)"
