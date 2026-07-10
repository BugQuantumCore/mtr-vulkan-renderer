#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in float fragDistance;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 0) uniform UniformBuffer {
    mat4 viewProj;
    mat4 model;
    vec4 lightDirection;
    vec4 ambientColor;
    vec4 cameraPos;
} ubo;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    // 简单光照
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(-ubo.lightDirection.xyz);
    float diffuse = max(dot(normal, lightDir), 0.0) * 0.7 + 0.3;

    vec3 finalColor = texColor.rgb * fragColor.rgb * diffuse;

    // 距离淡出（远处的轨道逐渐消失）
    float maxDist = 128.0;
    float fade = 1.0 - smoothstep(maxDist * 0.7, maxDist, fragDistance);

    outColor = vec4(finalColor, texColor.a * fragColor.a * fade);

    if (outColor.a < 0.01) discard;
}