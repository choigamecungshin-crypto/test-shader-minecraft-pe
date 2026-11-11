# LWRTX native shader-source transformer

C++17 shader-source transformer and Android GLES callback for a future external hook installer. It recognizes Bedrock RenderChunk shader strings, adds a Cornell-inspired derivative diffuse term, and passes unrelated shaders through unchanged.

## Android callback

The Android build produces `liblwrtx_gles_hook.so`. An external hook installer must:

1. Resolve the original `glShaderSource` function.
2. Call `LWRTX_SetOriginalShaderSource(original)`.
3. Redirect calls to `LWRTX_glShaderSource`.

This repository does not yet contain that hook installer or a Minecraft library loader. The callback alone does not inject itself.

The runtime must also set `LWRTX_LIGHT_DIRECTION`, `LWRTX_LIGHT_INTENSITY`, `LWRTX_AMBIENT`, and `LWRTX_ENABLED` on each linked RenderChunk program. Uniform/program binding is not implemented yet.

## Host tests

```sh
cmake -S lwrtx/native -B build/lwrtx
cmake --build build/lwrtx --parallel
ctest --test-dir build/lwrtx --output-on-failure
```

Runtime shader capture is still required to confirm whether Minecraft passes raw or preprocessed source to `glShaderSource`.
