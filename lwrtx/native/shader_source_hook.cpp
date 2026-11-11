#include "shader_transform.h"
#include <shadowhook.h>
#include <GLES2/gl2.h>
#include <android/log.h>
#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <jni.h>

#define LWRTX_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LWRTX", __VA_ARGS__)
#define LWRTX_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "LWRTX", __VA_ARGS__)

namespace {
using ShaderSourceFn = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
using LinkProgramFn = void (*)(GLuint);
std::atomic<ShaderSourceFn> g_originalShaderSource{nullptr};
std::atomic<LinkProgramFn> g_originalLinkProgram{nullptr};
std::atomic<float> g_lightX{-0.35f};
std::atomic<float> g_lightY{0.82f};
std::atomic<float> g_lightZ{-0.44f};
std::atomic<float> g_intensity{0.75f};
std::atomic<float> g_ambient{0.35f};
std::atomic<float> g_enabled{1.0f};
std::atomic<bool> g_installed{false};
std::atomic<unsigned int> g_transformedShaders{0};
std::atomic<unsigned int> g_litPrograms{0};
std::mutex g_installMutex;
void* g_shaderHookStub = nullptr;
void* g_linkHookStub = nullptr;

void ApplyUniforms(GLuint program) {
    const GLint enabled = glGetUniformLocation(program, "LWRTX_ENABLED");
    const GLint direction = glGetUniformLocation(program, "LWRTX_LIGHT_DIRECTION");
    const GLint intensity = glGetUniformLocation(program, "LWRTX_LIGHT_INTENSITY");
    const GLint ambient = glGetUniformLocation(program, "LWRTX_AMBIENT");
    if (enabled < 0 && direction < 0 && intensity < 0 && ambient < 0) return;

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgram(program);
    if (direction >= 0) glUniform3f(direction, g_lightX.load(), g_lightY.load(), g_lightZ.load());
    if (intensity >= 0) glUniform1f(intensity, g_intensity.load());
    if (ambient >= 0) glUniform1f(ambient, g_ambient.load());
    if (enabled >= 0) glUniform1f(enabled, g_enabled.load());
    glUseProgram(static_cast<GLuint>(previousProgram));
    g_litPrograms.fetch_add(1, std::memory_order_relaxed);
}

void HookedShaderSource(GLuint shader, GLsizei count,
                        const GLchar* const* strings, const GLint* lengths) {
    ShaderSourceFn original = g_originalShaderSource.load(std::memory_order_acquire);
    if (!original) return;
    if (count <= 0 || !strings) {
        original(shader, count, strings, lengths);
        return;
    }
    GLint shaderType = 0;
    glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
    if (shaderType != GL_VERTEX_SHADER && shaderType != GL_FRAGMENT_SHADER) {
        original(shader, count, strings, lengths);
        return;
    }

    std::string combined;
    for (GLsizei i = 0; i < count; ++i) {
        if (!strings[i]) continue;
        if (lengths && lengths[i] >= 0)
            combined.append(strings[i], static_cast<std::size_t>(lengths[i]));
        else
            combined.append(strings[i], std::strlen(strings[i]));
    }
    const std::string transformed = lwrtx::TransformShaderSource(
        static_cast<unsigned int>(shaderType), combined);
    if (transformed != combined)
        g_transformedShaders.fetch_add(1, std::memory_order_relaxed);
    const GLchar* source = transformed.c_str();
    original(shader, 1, &source, nullptr);
}

void HookedLinkProgram(GLuint program) {
    LinkProgramFn original = g_originalLinkProgram.load(std::memory_order_acquire);
    if (!original) return;
    original(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) ApplyUniforms(program);
}
} // namespace

extern "C" __attribute__((visibility("default")))
int LWRTX_InstallHooks() {
    std::lock_guard<std::mutex> lock(g_installMutex);
    if (g_installed.load(std::memory_order_acquire)) return 0;
    const int initResult = shadowhook_init(SHADOWHOOK_MODE_SHARED, false);
    if (initResult != 0) {
        LWRTX_LOGE("shadowhook_init failed: %d", initResult);
        return initResult;
    }

    void* originalShaderSource = nullptr;
    g_shaderHookStub = shadowhook_hook_sym_name(
        "libGLESv2.so", "glShaderSource",
        reinterpret_cast<void*>(HookedShaderSource), &originalShaderSource);
    if (!g_shaderHookStub || !originalShaderSource) {
        const int error = shadowhook_get_errno();
        LWRTX_LOGE("glShaderSource hook failed: %d %s", error, shadowhook_to_errmsg(error));
        g_shaderHookStub = nullptr;
        return error ? error : -1;
    }
    g_originalShaderSource.store(reinterpret_cast<ShaderSourceFn>(originalShaderSource),
                                 std::memory_order_release);

    void* originalLinkProgram = nullptr;
    g_linkHookStub = shadowhook_hook_sym_name(
        "libGLESv2.so", "glLinkProgram",
        reinterpret_cast<void*>(HookedLinkProgram), &originalLinkProgram);
    if (!g_linkHookStub || !originalLinkProgram) {
        const int error = shadowhook_get_errno();
        LWRTX_LOGE("glLinkProgram hook failed: %d %s", error, shadowhook_to_errmsg(error));
        shadowhook_unhook(g_shaderHookStub);
        g_shaderHookStub = nullptr;
        g_originalShaderSource.store(nullptr, std::memory_order_release);
        return error ? error : -2;
    }
    g_originalLinkProgram.store(reinterpret_cast<LinkProgramFn>(originalLinkProgram),
                                std::memory_order_release);
    g_installed.store(true, std::memory_order_release);
    LWRTX_LOGI("GLES hooks installed");
    return 0;
}

extern "C" __attribute__((visibility("default")))
void LWRTX_SetLighting(float x, float y, float z,
                       float intensity, float ambient, float enabled) {
    g_lightX.store(x);
    g_lightY.store(y);
    g_lightZ.store(z);
    g_intensity.store(intensity);
    g_ambient.store(ambient);
    g_enabled.store(enabled);
}
extern "C" __attribute__((visibility("default")))
int LWRTX_IsHookInstalled() { return g_installed.load(std::memory_order_acquire) ? 1 : 0; }
extern "C" __attribute__((visibility("default")))
unsigned int LWRTX_GetTransformedShaderCount() { return g_transformedShaders.load(); }
extern "C" __attribute__((visibility("default")))
unsigned int LWRTX_GetLitProgramCount() { return g_litPrograms.load(); }

// Runs only when a host Android app loads this library with System.loadLibrary().
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    LWRTX_InstallHooks();
    return JNI_VERSION_1_6;
}
