#version 300 es
precision highp float;

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 fragColor;

uniform vec3 uLight;
uniform vec3 uCamera;
uniform vec4 uColor;
uniform sampler2D uTex;
uniform int uUseTex;

void main() {
    vec4 base = uColor;
    if (uUseTex == 1) base *= texture(uTex, vUV);

    vec3 N = normalize(vNormal);
    vec3 toLight = uLight - vPos;
    float distanceToLight = length(toLight);
    vec3 L = toLight / max(distanceToLight, 0.0001);

    float diffuse = max(dot(N, L), 0.0);
    float attenuation = 1.0 /
        (1.0 + 0.08 * distanceToLight +
         0.025 * distanceToLight * distanceToLight);

    vec3 V = normalize(uCamera - vPos);
    vec3 H = normalize(L + V);
    float specular = pow(max(dot(N, H), 0.0), 24.0) * 0.12;

    vec3 color = base.rgb * (0.22 + 1.6 * diffuse * attenuation) + vec3(specular);
    color = color / (1.0 + color);
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    fragColor = vec4(color, base.a);
}
