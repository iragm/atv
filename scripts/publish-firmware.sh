#!/usr/bin/env bash
set -euo pipefail
# The release tag stays fixed; its .bin asset is updated after CI passes.
tag="firmware-latest"
file="quad-controller-d1-mini.bin"
if ! gh release view "$tag" >/dev/null 2>&1; then
  gh release create "$tag" --target "$GITHUB_SHA" --title 'Latest passing firmware' --notes 'Continuously updated from passing main builds. Keep the ATV stopped during OTA.' --latest=false
fi
gh release upload "$tag" "$file" --clobber
