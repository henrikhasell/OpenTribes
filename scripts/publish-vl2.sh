#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
name=TestMission
vl2="$root/build/${name}.vl2"
classic="${TRIBES2_DIR:-$HOME/.wine/drive_c/Dynamix/Tribes2/GameData}/Classic"

[ -f "$vl2" ] || { echo "no $vl2; run scripts/build-vl2.sh first" >&2; exit 1; }

mkdir -p "$classic"
find "$classic" -type f -name '*.dso' -delete
cp "$vl2" "$classic/"
echo "$classic/$(basename "$vl2")"
