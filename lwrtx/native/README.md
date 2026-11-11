# LWRTX native runtime

The Android ARM64 build produces two runtime libraries:

- `liblwrtx_gles_hook.so`: C++ shader transformer, GLES hooks and uniform setup.
- `libshadowhook.so`: ShadowHook runtime dependency (MIT, ByteDance).

## Runtime behavior

When the host Android app loads `liblwrtx_gles_hook.so` with `System.loadLibrary()`, `JNI_OnLoad` attempts to install inline hooks for:

- `libGLESv2.so!glShaderSource`: transforms recognized Bedrock RenderChunk vertex/fragment shader strings.
- `libGLESv2.so!glLinkProgram`: after successful link, initializes the four LWRTX uniforms if they exist in the program.

The C exports are:

- `LWRTX_InstallHooks()` — explicitly retry hook installation; returns 0 on success.
- `LWRTX_IsHookInstalled()` — returns 1 only after both hooks are installed.
- `LWRTX_SetLighting(x, y, z, intensity, ambient, enabled)` — sets values used when a later program is linked.

Default lighting values are direction (-0.35, 0.82, -0.44), intensity 0.75, ambient 0.35, enabled 1.0.

## Critical integration limitation

This library does **not** load itself into Minecraft. It must be loaded inside the Minecraft process by a legitimate host/launcher integration (for example, a modified app that calls `System.loadLibrary`). A standalone APK containing these libraries does not automatically attach to the official Minecraft process. No rootless external injector or in-game Graphics Mode menu is included.

The hook targets and shader variants still require runtime validation on Minecraft Bedrock 1.26.60.4. If the game resolves GLES entry points through a different dispatch path, the symbol hooks may not see its calls. The build succeeding is not proof that the game accepts the modified shader.
