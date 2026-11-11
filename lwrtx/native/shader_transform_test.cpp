#include "shader_transform.h"

#include <cassert>
#include <string>

int main() {
    const std::string vertex =
        "#version 300 es\n"
        "#define CHUNK_ORIGIN_AND_SCALE 1\n"
        "void main(){ vec3 worldPos=vec3(1.0); vec4 pos=vec4(0.0); "
        "gl_Position = pos; }\n";
    const std::string v = lwrtx::TransformShaderSource(lwrtx::kVertexShader, vertex);
    assert(v.find("out highp vec3 lwrtxWorldPos") != std::string::npos);
    assert(v.find("lwrtxWorldPos = worldPos.xyz") != std::string::npos);

    const std::string fragment300 =
        "#version 300 es\n"
        "#define TEXTURE_0 1\n"
        "void main(){ vec4 diffuse=vec4(1.0); "
        "gl_FragColor=diffuse; }\n";
    const std::string f300 = lwrtx::TransformShaderSource(lwrtx::kFragmentShader, fragment300);
    assert(f300.find("in highp vec3 lwrtxWorldPos") != std::string::npos);
    assert(f300.find("dFdx(lwrtxWorldPos)") != std::string::npos);
    assert(f300.find("LWRTX_LIGHT_INTENSITY") != std::string::npos);
    assert(f300.find("diffuse.rgb *= mix") != std::string::npos);
    assert(f300.find("void main") < f300.find("LWRTX Cornell-inspired"));

    const std::string fragmentFog =
        "#version 100\r\n"
        "varying vec4 fogColor;\r\n"
        "uniform sampler2D TEXTURE_0;\r\n"
        "void main(){ vec4 diffuse=vec4(1.0);\r\n"
        "#ifdef FOG\r\n"
        "\tdiffuse.rgb = mix( diffuse.rgb, fogColor.rgb, fogColor.a );\r\n"
        "#endif\r\n"
        "    gl_FragColor = diffuse;\r\n"
        "}\r\n";
    const std::string ffog = lwrtx::TransformShaderSource(lwrtx::kFragmentShader, fragmentFog);
    const std::size_t lightingAt = ffog.find("LWRTX Cornell-inspired");
    const std::size_t fogAt = ffog.find("#ifdef FOG\r\n\tdiffuse.rgb = mix");
    assert(lightingAt != std::string::npos && fogAt != std::string::npos);
    assert(lightingAt < fogAt);
    assert(ffog.find("void main") < lightingAt);

    const std::string fragment100 =
        "#version 100\n"
        "#extension GL_EXT_shader_texture_lod : enable\n"
        "precision mediump float;\n"
        "uniform sampler2D TEXTURE_0;\n"
        "void main(){ vec4 diffuse=vec4(1.0); "
        "gl_FragColor = diffuse; }\n";
    const std::string f100 = lwrtx::TransformShaderSource(lwrtx::kFragmentShader, fragment100);
    assert(f100.find("#extension GL_OES_standard_derivatives : enable") != std::string::npos);
    assert(f100.find("varying highp vec3 lwrtxWorldPos") != std::string::npos);
    assert(f100.find("#extension GL_OES_standard_derivatives : enable") >
           f100.find("#extension GL_EXT_shader_texture_lod : enable"));
    assert(f100.find("diffuse.rgb *= mix") != std::string::npos);

    const std::string unrelated = "#version 300 es\nvoid main(){}\n";
    assert(lwrtx::TransformShaderSource(lwrtx::kFragmentShader, unrelated) == unrelated);
    return 0;
}
