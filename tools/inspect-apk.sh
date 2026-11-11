#!/usr/bin/env bash
set -euo pipefail

APK="${1:?Usage: inspect-apk.sh <minecraft.apk> [output-dir]}"
OUT="${2:-report}"
APKANALYZER="${APKANALYZER:-}"

for tool in unzip readelf strings file sha256sum; do
  command -v "$tool" >/dev/null 2>&1 || {
    echo "Missing required tool: $tool" >&2
    exit 2
  }
done

if [ -z "$APKANALYZER" ]; then
  APKANALYZER="$(find "${ANDROID_HOME:-/nonexistent}/cmdline-tools" -type f -name apkanalyzer -print -quit 2>/dev/null || true)"
fi

mkdir -p "$OUT/native"
test -s "$APK"
unzip -Z1 "$APK" > "$OUT/apk-file-list.txt"

{
  echo "LWRTX APK integration reconnaissance"
  echo "APK: $APK"
  echo "Size: $(stat -c%s "$APK") bytes"
  echo "SHA-256: $(sha256sum "$APK" | cut -d' ' -f1)"
  echo
  echo "=== APK entries relevant to graphics and runtime ==="
  grep -Ei '(^|/)(libminecraftpe[.]so|[^/]*(graphics|render|option|setting|resource)[^/]*|AndroidManifest[.]xml)$|^assets/' "$OUT/apk-file-list.txt" | head -1000 || true
  echo
  echo "=== ARM64 shared libraries ==="
  grep -E '^lib/arm64-v8a/.*[.]so$' "$OUT/apk-file-list.txt" || true
} > "$OUT/apk-overview.txt"

if [ -n "$APKANALYZER" ] && [ -x "$APKANALYZER" ]; then
  "$APKANALYZER" manifest print "$APK" > "$OUT/manifest.txt"
  "$APKANALYZER" manifest application-id "$APK" > "$OUT/application-id.txt" || true
  "$APKANALYZER" manifest min-sdk "$APK" > "$OUT/min-sdk.txt" || true
  "$APKANALYZER" manifest target-sdk "$APK" > "$OUT/target-sdk.txt" || true
else
  echo "apkanalyzer unavailable; binary AndroidManifest.xml is included in the APK." > "$OUT/manifest-analysis-unavailable.txt"
fi

LIB_PATH="$(grep -Fxm1 'lib/arm64-v8a/libminecraftpe.so' "$OUT/apk-file-list.txt" || true)"
if [ -z "$LIB_PATH" ]; then
  LIB_PATH="$(grep -Em1 '^lib/arm64-v8a/[^/]*minecraft[^/]*[.]so$' "$OUT/apk-file-list.txt" || true)"
fi

if [ -n "$LIB_PATH" ]; then
  echo "$LIB_PATH" > "$OUT/native/library-path.txt"
  unzip -p "$APK" "$LIB_PATH" > "$OUT/native/libminecraftpe.so"
  test -s "$OUT/native/libminecraftpe.so"
  {
    echo "Native library path: $LIB_PATH"
    echo "Native library size: $(stat -c%s "$OUT/native/libminecraftpe.so")"
    echo
    echo "=== File identification ==="
    file "$OUT/native/libminecraftpe.so"
    echo
    echo "=== ELF header ==="
    readelf -h "$OUT/native/libminecraftpe.so"
    echo
    echo "=== Needed libraries ==="
    readelf -d "$OUT/native/libminecraftpe.so" | grep NEEDED || true
    echo
    echo "=== Graphics/menu strings (ASCII) ==="
    strings -a "$OUT/native/libminecraftpe.so" | grep -Ei 'Vibrant Visuals|Ray Tracing|Graphics Mode|graphics mode|rendering|shader|raytrac|vibrant|deferred|lighting' | head -1000 || true
    echo
    echo "=== Graphics/menu strings (UTF-16LE) ==="
    strings -el "$OUT/native/libminecraftpe.so" | grep -Ei 'Vibrant Visuals|Ray Tracing|Graphics Mode|graphics mode|rendering|shader|raytrac|vibrant|deferred|lighting' | head -1000 || true
    echo
    echo "=== Dynamic symbols related to graphics ==="
    readelf -Ws "$OUT/native/libminecraftpe.so" | grep -Ei 'shader|render|graphics|option|setting|gles|egl|vulkan' | head -700 || true
  } > "$OUT/native-graphics-strings.txt"
  rm -f "$OUT/native/libminecraftpe.so"
else
  echo "No ARM64 Minecraft native library found in APK." > "$OUT/native-graphics-strings.txt"
fi

# Extract the vanilla settings-section and localization archives so the
# graphics-mode UI structure can be inspected without unpacking the full APK.
mkdir -p "$OUT/ui-archives"
for entry in \
  "assets/assets/resource_packs/vanilla/__brarchive/ui/settings_sections.brarchive" \
  "assets/assets/resource_packs/vanilla/__brarchive/texts.brarchive"; do
  if grep -Fxq "$entry" "$OUT/apk-file-list.txt"; then
    name="$(basename "$entry")"
    unzip -p "$APK" "$entry" > "$OUT/ui-archives/$name"
    {
      echo "Archive: $entry"
      file "$OUT/ui-archives/$name"
      echo "Size: $(stat -c%s "$OUT/ui-archives/$name") bytes"
      echo "Magic:"
      od -An -tx1 -N64 "$OUT/ui-archives/$name"
      echo "Readable strings:"
      strings -a "$OUT/ui-archives/$name" | grep -Ei 'graphics|ray.?trac|vibrant|deferred|video|settings|mode' | head -250 || true
    } >> "$OUT/ui-archives/summary.txt"
  fi
done

{
  echo "APK_SIZE=$(stat -c%s "$APK")"
  echo "APK_SHA256=$(sha256sum "$APK" | cut -d' ' -f1)"
  echo "LIB_PATH=$LIB_PATH"
} > "$OUT/build-metadata.txt"
