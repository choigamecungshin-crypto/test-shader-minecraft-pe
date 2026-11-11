#include "shader_transform.h"

#include <GLES2/gl2.h>
#include <atomic>
#include <cstring>
#include <string>

namespace {
using ShaderSourceFn = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
std::atomic<ShaderSourceFn> g_original{nullptr};
}

extern "C" __attribute__((visibility("default")))
void LWRTX_SetOriginalShaderSource(void* function) {
    g_original.store(reinterpret_cast<ShaderSourceFn>(function), std::memory_order_release);
}

extern "C" __attribute__((visibility("default")))
void LWRTX_glShaderSource(GLuint shader, GLsizei count,
                          const GLchar* const* strings, const GLint* lengths) {
    ShaderSourceFn original = g_original.load(std::memory_order_acquire);
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
    const GLchar* source = transformed.c_str();
    original(shader, 1, &source, nullptr);
}
