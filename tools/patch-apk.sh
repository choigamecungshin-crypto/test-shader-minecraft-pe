#!/usr/bin/env bash
set -euo pipefail

APK="${1:?Usage: patch-apk.sh <original.apk> <runtime-dir> <output.apk> <keystore>}"
RUNTIME_DIR="${2:?Missing runtime directory}"
OUTPUT_APK="${3:?Missing output APK path}"
KEYSTORE="${4:?Missing signing keystore}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ZIPALIGN="${ZIPALIGN:-zipalign}"
APKSIGNER="${APKSIGNER:-apksigner}"
APKTOOL="${APKTOOL:-apktool}"
AAPT="${AAPT:-aapt}"
PACKAGE_OLD="com.mojang.minecraftpe"
PACKAGE_NEW="com.mojang.minecraftpe.test"
PATCH_NEEDED="liblwrtx_gles_hook.so"
SETTINGS_ARCHIVE="assets/assets/resource_packs/vanilla/__brarchive/ui/settings_sections.brarchive"

for tool in unzip zip patchelf sha256sum readelf python3; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "Missing required tool: $tool" >&2
    exit 2
  }
done
command -v "$APKTOOL" >/dev/null 2>&1 || {
  echo "Missing required tool: $APKTOOL" >&2
  exit 2
}
command -v "$AAPT" >/dev/null 2>&1 || {
  echo "Missing required tool: $AAPT" >&2
  exit 2
}
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

# Rebuild only the manifest layer with apktool's no-resource/no-dex mode.
# This changes the Android application ID without unpacking/recompiling the
# enormous Bedrock resource table or dex payload.
"$APKTOOL" d --force --no-src --no-res "$APK" -o "$WORK/decoded"
MANIFEST="$WORK/decoded/AndroidManifest.xml"
test -s "$MANIFEST"
python3 - "$MANIFEST" "$PACKAGE_OLD" "$PACKAGE_NEW" <<'PY'
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
old, new = sys.argv[2], sys.argv[3]
text = path.read_text(encoding="utf-8")
needle = f'package="{old}"'
replacement = f'package="{new}"'
count = text.count(needle)
if count != 1:
    raise SystemExit(f"Expected one manifest package={old!r}, found {count}")
path.write_text(text.replace(needle, replacement, 1), encoding="utf-8")
PY
"$APKTOOL" b "$WORK/decoded" -o "$WORK/renamed.apk"
"$AAPT" dump badging "$WORK/renamed.apk" | grep -F "package: name='$PACKAGE_NEW'"
echo "Package name changed: $PACKAGE_OLD -> $PACKAGE_NEW"

unzip -p "$WORK/renamed.apk" lib/arm64-v8a/libminecraftpe.so > "$ROOT/lib/arm64-v8a/libminecraftpe.so"
unzip -p "$WORK/renamed.apk" "$SETTINGS_ARCHIVE" > "$WORK/settings_sections.original.brarchive"
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
cp "$WORK/renamed.apk" "$WORK/unsigned.apk"
zip -q -d "$WORK/unsigned.apk" \
  "lib/arm64-v8a/libminecraftpe.so" \
  "$SETTINGS_ARCHIVE" \
  'META-INF/*.SF' \
  'META-INF/*.RSA' \
  'META-INF/*.DSA' \
  'META-INF/MANIFEST.MF'

(
  cd "$ROOT"
  zip -q -1 "$WORK/unsigned.apk" \
    "lib/arm64-v8a/libminecraftpe.so" \
    "lib/arm64-v8a/liblwrtx_gles_hook.so" \
    "lib/arm64-v8a/libshadowhook.so" \
    "$SETTINGS_ARCHIVE"
)

"$ZIPALIGN" -f 4 "$WORK/unsigned.apk" "$WORK/aligned.apk"
"$APKSIGNER" sign \
  --ks "$KEYSTORE" \
  --ks-key-alias lwrtx \
  --ks-pass pass:android \
  --key-pass pass:android \
  --out "$OUTPUT_APK" \
  "$WORK/aligned.apk"

"$ZIPALIGN" -c -v 4 "$OUTPUT_APK"
"$APKSIGNER" verify --verbose --print-certs "$OUTPUT_APK"
"$AAPT" dump badging "$OUTPUT_APK" | grep -F "package: name='$PACKAGE_NEW'"
unzip -Z1 "$OUTPUT_APK" | grep -E '^lib/arm64-v8a/(libminecraftpe|liblwrtx_gles_hook|libshadowhook)[.]so$'

{
  echo "APK_SHA256=$(sha256sum "$OUTPUT_APK" | cut -d' ' -f1)"
  echo "APK_SIZE=$(stat -c%s "$OUTPUT_APK")"
  echo "PACKAGE_NAME=$PACKAGE_NEW"
  echo "INTEGRATION=LWRTX GLES hook added as DT_NEEDED dependency"
  echo "UI_MODE=LWRTX label aliases the existing Deferred graphics mode"
  echo "NATIVE_LIBS=liblwrtx_gles_hook.so and libshadowhook.so included"
  echo "SIGNING=temporary test key; this is not the original Play signing certificate"
  echo "PAIRIP=untouched; runtime launch may reject a re-signed package"
} > "${OUTPUT_APK}.metadata.txt"

echo "Created experimental patched APK: $OUTPUT_APK"
