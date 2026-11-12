# LWRTX — Minecraft Bedrock Android graphics experiment

LWRTX is an experimental native C++ / GLSL graphics project targeting Minecraft Bedrock 1.26.60.4 on Android ARM64, with GLES as the first backend.

The Cornell Box shader is used as a lighting reference and adapted to the Bedrock RenderChunk shader interface.

## Included source

- Cornell Box GLSL reference shaders.
- Experimental Bedrock RenderChunk vertex/fragment shader pair.
- C++17 RenderChunk shader-source transformer.
- Android GLES hooks using ByteDance ShadowHook.
- Host-side regression tests.
- Bedrock `.brarchive` decoder/encoder and graphics-settings UI patcher.
- APK inspection and patching helper scripts used by the separate APK build workspace.

## Current integration status

The Android ARM64 runtime is built by the separate private `choigamecungshin-crypto/test` build workspace. This repository is the LWRTX source repository; the APK build workflow is not hosted here.

The APK patch changes the visible **Deferred** graphics-mode label to **LWRTX**. It deliberately reuses Minecraft's existing `#graphics_mode_radio_deferred` binding. This exposes an LWRTX-labelled choice in the existing graphics-mode dropdown, but it is an alias of the native Deferred mode, **not a new native GraphicsMode enum value**.

The APK patch adds `liblwrtx_gles_hook.so` as a `DT_NEEDED` dependency of `libminecraftpe.so`, and packages `libshadowhook.so` alongside it. The package ID is changed from `com.mojang.minecraftpe` to `com.mojang.minecraftpe.test` so the experimental build has a separate Android application identity.

The APK is re-signed with a temporary test key; it does not retain the original Play signing certificate. PairIP/license protection is left untouched, so the re-signed APK may reject launch. No DRM or license checks are bypassed.

The current shader adds Cornell-inspired direct diffuse lighting and ambient contribution. It is **not** ray tracing, path tracing, global illumination, or reflections. The patched APK has not yet been validated on a physical Android device.

## Host tests

```sh
cmake -S lwrtx/native -B build/lwrtx -DCMAKE_BUILD_TYPE=Release
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

## Source layout

- `lwrtx/native/` — C++ shader transform, GLES hooks, and tests.
- `lwrtx/shaders/reference/` — Cornell Box reference shaders.
- `lwrtx/shaders/bedrock/` — experimental Bedrock RenderChunk shaders.
- `tools/` — APK inspection, `.brarchive` codecs, UI patcher, and APK patcher.
