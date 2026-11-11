#pragma once
#include <string>

namespace lwrtx {
constexpr unsigned int kVertexShader = 0x8B31;
constexpr unsigned int kFragmentShader = 0x8B30;

// Returns the original source unchanged when it is not a recognized
// Bedrock RenderChunk shader or when a required insertion point is absent.
std::string TransformShaderSource(unsigned int shader_type, const std::string& source);
}
