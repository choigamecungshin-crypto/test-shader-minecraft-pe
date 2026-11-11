# LWRTX — Minecraft Bedrock Android graphics experiment

LWRTX is an experimental native C++ / GLSL graphics project targeting Minecraft Bedrock 1.26.60.4 on Android ARM64, with GLES as the first backend.

The Cornell Box shader is used as a lighting reference and adapted to the Bedrock RenderChunk shader interface.

## Included

- Cornell Box GLSL reference shaders.
- Experimental Bedrock RenderChunk vertex/fragment shader pair.
- C++17 RenderChunk shader-source transformer.
- Android GLES inline hooks using ByteDance ShadowHook.
- Uniform initialization after successful program linking.
- GitHub Actions host tests and Android ARM64 runtime bundle.

## Current status

The native runtime is buildable. It is **not yet an installable Minecraft Graphics Mode**: the two libraries must be loaded inside the Minecraft process by a host/launcher integration. No official Minecraft process loader, rootless external injector, in-game graphics-mode menu, or device runtime validation is included.

The current shader adds Cornell-inspired direct diffuse lighting and ambient contribution. It is **not** ray tracing, path tracing, global illumination, or reflections.

## Build

Host tests:

```sh
cmake -S lwrtx/native -B build/lwrtx -DCMAKE_BUILD_TYPE=Release
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

Android ARM64: run the GitHub Actions workflow. It downloads ShadowHook 2.0.1 and uploads `lwrtx-runtime-arm64-v8a` containing both required shared libraries.

## Source layout

- `lwrtx/native/` — C++ transform, GLES hooks, tests.
- `lwrtx/shaders/reference/` — Cornell Box reference.
- `lwrtx/shaders/bedrock/` — experimental Bedrock RenderChunk shaders.
- `.github/workflows/` — CI and Android ARM64 build.
