#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    // 车站设施使用简单的平面着色
    vec3 normal = normalize(fragNormal);
    float light = max(dot(normal, vec3(0.0, 1.0, 0.0)), 0.0) * 0.5 + 0.5;

    outColor = vec4(texColor.rgb * fragColor.rgb * light, texColor.a * fragColor.a);
    if (outColor.a < 0.01) discard;
}