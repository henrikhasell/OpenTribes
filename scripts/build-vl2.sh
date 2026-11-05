#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
name=TestMission
src="$root/${name}"
out="$root/build/${name}.vl2"

mkdir -p "$root/build"
rm -f "$out"
(cd "$src" && zip -qrXD "$out" . -x '*.dso')
echo "$out"
