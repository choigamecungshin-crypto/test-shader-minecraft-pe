# LWRTX — Cornell-inspired Bedrock lighting prototype

LWRTX targets Minecraft Bedrock 1.26.60.4 on Android ARM64. The Cornell Box shader is the lighting reference; Bedrock RenderChunk GLSL is the compatibility target.

## Implemented source

- Cornell Box GLSL reference pair.
- Experimental RenderChunk shader pair retaining vanilla texture, lightmap, alpha-test, seasons, and fog paths.
- C++17 shader-source transformer for RenderChunk shaders.
- Android GLES callback library source for a future external `glShaderSource` hook.
- Host-side C++ tests and Android ARM64 GitHub Actions build.

## Integration status

The native callback is **not self-installing**. Minecraft does not load or call LWRTX yet. A verified hook installer/library-load path, runtime shader capture, and code to bind LWRTX uniforms to the correct linked RenderChunk programs remain unimplemented. There is no in-game Graphics Mode entry or toggle.

The current lighting is direct diffuse plus ambient. It is not ray tracing, path tracing, global illumination, or reflections.

## Validation

```sh
cmake -S lwrtx/native -B build/lwrtx -DCMAKE_BUILD_TYPE=Release
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

See `native/README.md` for Android callback integration constraints.
