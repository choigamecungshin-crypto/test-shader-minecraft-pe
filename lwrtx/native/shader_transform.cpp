#include "shader_transform.h"

#include <regex>
#include <string>

namespace lwrtx {
namespace {
bool Has(const std::string& s, const char* token) {
    return s.find(token) != std::string::npos;
}

bool IsGlsl300(const std::string& s) {
    const std::size_t p = s.find("#version");
    if (p == std::string::npos) return false;
    const std::size_t e = s.find('\n', p);
    return s.substr(p, e == std::string::npos ? e : e - p).find("300") != std::string::npos;
}

// GLSL requires #extension directives to precede ordinary declarations.
void InsertHeader(std::string& source, const std::string& code) {
    const std::size_t version = source.find("#version");
    if (version == std::string::npos) {
        source.insert(0, code);
        return;
    }
    std::size_t insertAt = source.find('\n', version);
    if (insertAt == std::string::npos) {
        source += "\n" + code;
        return;
    }
    ++insertAt;
    while (insertAt < source.size()) {
        const std::size_t lineEnd = source.find('\n', insertAt);
        const std::size_t end = lineEnd == std::string::npos ? source.size() : lineEnd;
        const std::string line = source.substr(insertAt, end - insertAt);
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line.compare(first, 10, "#extension") == 0) {
            if (lineEnd == std::string::npos) {
                insertAt = source.size();
                break;
            }
            insertAt = lineEnd + 1;
            continue;
        }
        break;
    }
    source.insert(insertAt, code);
}

bool ReplaceFirst(std::string& s, const std::string& a, const std::string& b) {
    const std::size_t p = s.find(a);
    if (p == std::string::npos) return false;
    s.replace(p, a.size(), b);
    return true;
}

bool ReplaceDiffuseOutput(std::string& s, const std::string& outputName,
                          const std::string& lighting) {
    const std::regex output("\\b" + outputName + R"(\s*=\s*diffuse\s*;)");
    std::smatch match;
    if (!std::regex_search(s, match, output)) return false;
    s.replace(static_cast<std::size_t>(match.position()),
              static_cast<std::size_t>(match.length()), lighting + match.str());
    return true;
}

bool TransformVertex(std::string& s) {
    if (!Has(s, "CHUNK_ORIGIN_AND_SCALE") || !Has(s, "gl_Position")) return false;
    InsertHeader(s, IsGlsl300(s)
        ? "out highp vec3 lwrtxWorldPos;\n"
        : "varying highp vec3 lwrtxWorldPos;\n");
    return ReplaceFirst(s, "gl_Position = pos;",
                        "gl_Position = pos;\n    lwrtxWorldPos = worldPos.xyz;");
}

bool TransformFragment(std::string& s) {
    if (!Has(s, "TEXTURE_0") ||
        (!Has(s, "gl_FragColor") && !Has(s, "FragColor")) ||
        !Has(s, "void main")) return false;

    const bool modern = IsGlsl300(s);
    std::string prefix;
    if (!modern) prefix += "#extension GL_OES_standard_derivatives : enable\n";
    prefix += modern
        ? "in highp vec3 lwrtxWorldPos;\n"
        : "varying highp vec3 lwrtxWorldPos;\n";
    prefix +=
        "uniform highp vec3 LWRTX_LIGHT_DIRECTION;\n"
        "uniform highp float LWRTX_LIGHT_INTENSITY;\n"
        "uniform highp float LWRTX_AMBIENT;\n"
        "uniform highp float LWRTX_ENABLED;\n";
    InsertHeader(s, prefix);

    const std::string lighting =
        "// LWRTX Cornell-inspired direct diffuse lighting.\n"
        "{\n"
        " highp vec3 lwrtxDx = dFdx(lwrtxWorldPos);\n"
        " highp vec3 lwrtxDy = dFdy(lwrtxWorldPos);\n"
        " highp vec3 lwrtxN = normalize(cross(lwrtxDx, lwrtxDy));\n"
        " if (!gl_FrontFacing) lwrtxN = -lwrtxN;\n"
        " highp float lwrtxLen2 = dot(LWRTX_LIGHT_DIRECTION, LWRTX_LIGHT_DIRECTION);\n"
        " highp vec3 lwrtxL = lwrtxLen2 > 0.0001 ? normalize(LWRTX_LIGHT_DIRECTION) : normalize(vec3(-0.35, 0.82, -0.44));\n"
        " highp float lwrtxDiffuse = max(dot(lwrtxN, lwrtxL), 0.0);\n"
        " highp float lwrtxFactor = max(LWRTX_AMBIENT, 0.0) + max(LWRTX_LIGHT_INTENSITY, 0.0) * lwrtxDiffuse;\n"
        " diffuse.rgb *= mix(vec3(1.0), vec3(max(lwrtxFactor, 0.0)), clamp(LWRTX_ENABLED, 0.0, 1.0));\n"
        "}\n";

    // Prefer inserting before Bedrock's fog pass so fog remains the final blend.
    if (ReplaceFirst(s, "#ifdef FOG\n    diffuse.rgb = mix",
                     lighting + "#ifdef FOG\n    diffuse.rgb = mix")) return true;
    // Fallback for shader variants without the canonical fog block.
    if (ReplaceDiffuseOutput(s, "gl_FragColor", lighting)) return true;
    return ReplaceDiffuseOutput(s, "FragColor", lighting);
}
} // namespace

std::string TransformShaderSource(unsigned int type, const std::string& source) {
    std::string out = source;
    bool changed = false;
    if (type == kVertexShader) changed = TransformVertex(out);
    if (type == kFragmentShader) changed = TransformFragment(out);
    return changed ? out : source;
}
} // namespace lwrtx
