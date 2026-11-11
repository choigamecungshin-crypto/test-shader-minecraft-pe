#!/usr/bin/env bash
set -euo pipefail

APK="${1:?Usage: patch-apk.sh <original.apk> <runtime-dir> <output.apk> <keystore>}"
RUNTIME_DIR="${2:?Missing runtime directory}"
OUTPUT_APK="${3:?Missing output APK path}"
KEYSTORE="${4:?Missing signing keystore}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ZIPALIGN="${ZIPALIGN:-zipalign}"
APKSIGNER="${APKSIGNER:-apksigner}"
PATCH_NEEDED="liblwrtx_gles_hook.so"
SETTINGS_ARCHIVE="assets/assets/resource_packs/vanilla/__brarchive/ui/settings_sections.brarchive"

for tool in unzip zip patchelf sha256sum readelf; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "Missing required tool: $tool" >&2
    exit 2
  }
done
test -x "$ZIPALIGN"
test -x "$APKSIGNER"
test -s "$APK"
test -s "$RUNTIME_DIR/liblwrtx_gles_hook.so"
test -s "$RUNTIME_DIR/libshadowhook.so"
test -s "$KEYSTORE"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
ROOT="$WORK/root"
mkdir -p "$ROOT/lib/arm64-v8a" "$ROOT/assets/assets/resource_packs/vanilla/__brarchive/ui"

unzip -p "$APK" lib/arm64-v8a/libminecraftpe.so > "$ROOT/lib/arm64-v8a/libminecraftpe.so"
unzip -p "$APK" "$SETTINGS_ARCHIVE" > "$WORK/settings_sections.original.brarchive"
test -s "$ROOT/lib/arm64-v8a/libminecraftpe.so"
test -s "$WORK/settings_sections.original.brarchive"

python3 "$SCRIPT_DIR/patch-lwrtx-ui.py" \
  "$WORK/settings_sections.original.brarchive" \
  "$ROOT/$SETTINGS_ARCHIVE"

cp "$RUNTIME_DIR/liblwrtx_gles_hook.so" "$ROOT/lib/arm64-v8a/"
cp "$RUNTIME_DIR/libshadowhook.so" "$ROOT/lib/arm64-v8a/"

# The constructor in liblwrtx_gles_hook installs the GLES hooks when Android's
# linker loads this DT_NEEDED dependency alongside libminecraftpe.so.
if ! readelf -d "$ROOT/lib/arm64-v8a/libminecraftpe.so" | grep -Fq "$PATCH_NEEDED"; then
  patchelf --add-needed "$PATCH_NEEDED" "$ROOT/lib/arm64-v8a/libminecraftpe.so"
fi
readelf -d "$ROOT/lib/arm64-v8a/libminecraftpe.so" | grep -F "$PATCH_NEEDED"

mkdir -p "$(dirname "$OUTPUT_APK")"
cp "$APK" "$WORK/unsigned.apk"
zip -q -d "$WORK/unsigned.apk" \
  "lib/arm64-v8a/libminecraftpe.so" \
  "$SETTINGS_ARCHIVE"

(
  cd "$ROOT"
  zip -q -0 "$WORK/unsigned.apk" \
    "lib/arm64-v8a/libminecraftpe.so" \
    "lib/arm64-v8a/liblwrtx_gles_hook.so" \
    "lib/arm64-v8a/libshadowhook.so" \
    "$SETTINGS_ARCHIVE"
)

"$ZIPALIGN" -f -p 4 "$WORK/unsigned.apk" "$WORK/aligned.apk"
"$APKSIGNER" sign \
  --ks "$KEYSTORE" \
  --ks-key-alias lwrtx \
  --ks-pass pass:android \
  --key-pass pass:android \
  --out "$OUTPUT_APK" \
  "$WORK/aligned.apk"

"$ZIPALIGN" -c -p -v 4 "$OUTPUT_APK"
"$APKSIGNER" verify --verbose --print-certs "$OUTPUT_APK"
unzip -Z1 "$OUTPUT_APK" | grep -E '^lib/arm64-v8a/(libminecraftpe|liblwrtx_gles_hook|libshadowhook)[.]so$'

{
  echo "APK_SHA256=$(sha256sum "$OUTPUT_APK" | cut -d' ' -f1)"
  echo "APK_SIZE=$(stat -c%s "$OUTPUT_APK")"
  echo "INTEGRATION=LWRTX GLES hook added as DT_NEEDED dependency"
  echo "UI_MODE=LWRTX label aliases the existing Deferred graphics mode"
  echo "SIGNING=temporary test key; this is not the original Play signing certificate"
  echo "PAIRIP=untouched; runtime launch may reject a re-signed package"
} > "${OUTPUT_APK}.metadata.txt"

echo "Created experimental patched APK: $OUTPUT_APK"
