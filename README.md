# LWRTX — Minecraft Bedrock Android graphics experiment

LWRTX is an experimental native C++ / GLSL graphics project targeting Minecraft Bedrock 1.26.60.4 on Android ARM64, with GLES as the first backend.

The Cornell Box shader is used as a lighting reference and adapted to the Bedrock RenderChunk shader interface.

## Included

- Cornell Box GLSL reference shaders.
- Experimental Bedrock RenderChunk vertex/fragment shader pair.
- C++17 RenderChunk shader-source transformer.
- Android GLES hooks using ByteDance ShadowHook.
- Automatic hook installation when the runtime is loaded as a native dependency.
- Bedrock `.brarchive` decoder/encoder and graphics-settings UI patcher.
- APK inspection and ARM64 build scripts.

## Current integration status

The Android ARM64 runtime builds successfully and host regression tests pass.

The APK build pipeline is hosted in this source repository. It downloads only the original Minecraft 1.26.60.4 APK from the Release in `choigamecungshin-crypto/test`; all LWRTX source changes, CI, compilation, APK patching, signing, and build artifacts remain in this repository.

The current UI patch changes the visible **Deferred** graphics-mode label to **LWRTX**. It deliberately reuses Minecraft's existing `#graphics_mode_radio_deferred` binding. This exposes an LWRTX-labelled choice in the existing graphics-mode dropdown, but it is an alias of the native Deferred mode, **not a new native GraphicsMode enum value**. Adding a distinct engine mode still requires modifying the native graphics-mode selection and state logic.

The APK patch adds `liblwrtx_gles_hook.so` as a `DT_NEEDED` dependency of `libminecraftpe.so`, and packages `libshadowhook.so` alongside it. A constructor installs the GLES hooks when Android's linker loads the dependency. The APK is re-signed with a temporary test key; it does not retain the original Play signing certificate. PairIP/license protection is left untouched, so the re-signed APK may reject launch. No DRM or license checks are bypassed.

The current shader adds Cornell-inspired direct diffuse lighting and ambient contribution. It is **not** ray tracing, path tracing, global illumination, or reflections. The patched APK has not yet been validated on a physical Android device.

## Build and test

Host tests:

```sh
cmake -S lwrtx/native -B build/lwrtx -DCMAKE_BUILD_TYPE=Release
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

Android ARM64 runtime: run the **LWRTX Android ARM64 build** workflow.

Before running the workflow, add an Actions secret named `TEST_REPO_READ_TOKEN` in this repository. It must be a fine-grained token with read-only Contents access to the private `choigamecungshin-crypto/test` repository. The token is used only to download the original Release asset; it is never written into the APK or uploaded as an artifact.

APK inspection and experimental APK build: run **LWRTX Bedrock APK build** in this repository. The workflow downloads the pinned original APK from the `test` repository Release, verifies its SHA-256, runs host tests, builds the ARM64 runtime, patches and signs the APK, then uploads the APK, runtime libraries, and inspection report as artifacts here.

## Source layout

- `lwrtx/native/` — C++ shader transform, GLES hooks, and tests.
- `lwrtx/shaders/reference/` — Cornell Box reference shaders.
- `lwrtx/shaders/bedrock/` — experimental Bedrock RenderChunk shaders.
- `tools/` — APK inspection, `.brarchive` codecs, UI patcher, and APK patcher.
- `.github/workflows/` — CI and Android ARM64 build.
