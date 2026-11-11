# LWRTX — Minecraft Bedrock Android graphics experiment

LWRTX is an experimental native C++ / GLSL graphics project targeting Minecraft Bedrock 1.26.60.4 on Android ARM64, with GLES as the first backend.

The Cornell Box shader is used as a lighting reference and adapted to the Bedrock RenderChunk shader interface.

## Current scope

- Cornell Box GLSL reference shaders.
- Experimental Bedrock RenderChunk vertex/fragment shader pair.
- C++17 shader-source transformer for Bedrock RenderChunk GLSL.
- Android GLES `glShaderSource` callback library.
- GitHub Actions host tests and Android ARM64 build artifact.

## Important status

This is a development prototype, **not yet an installable Minecraft Graphics Mode**. The callback library does not install its own hook or load itself into Minecraft. A verified Android hook installer, runtime shader-source capture, and RenderChunk program uniform binding are still required. No in-game graphics-mode menu or toggle is implemented.

The current shader adds a Cornell-inspired direct diffuse term and ambient contribution. It is **not** ray tracing, path tracing, global illumination, or reflections.

## Build locally

Host tests:

```sh
cmake -S lwrtx/native -B build/lwrtx -DCMAKE_BUILD_TYPE=Release
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

Android ARM64 shared callback:

```sh
cmake -S lwrtx/native -B build/android-arm64 \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-24 \
  -DLWRTX_BUILD_TESTS=OFF \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/android-arm64 --parallel
```

The Android workflow uploads `liblwrtx_gles_hook.so` as the artifact `lwrtx-gles-hook-arm64-v8a` when the build succeeds.

## Source layout

- `lwrtx/native/` — C++ transform, GLES callback, and tests.
- `lwrtx/shaders/reference/` — Cornell Box reference.
- `lwrtx/shaders/bedrock/` — experimental Bedrock RenderChunk shaders.
- `.github/workflows/` — CI and Android ARM64 build.

## Next integration gate

Capture the exact source strings passed to `glShaderSource` on the target Minecraft build, validate the transformed shader with a GLSL compiler/device, then implement and verify the native hook and uniform binding.
