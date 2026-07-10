#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;

layout(location = 0) out vec4 outColor;

// 纹理采样器
layout(binding = 1) uniform sampler2D texSampler;

// Uniform (全局光照参数)
layout(binding = 0) uniform UniformBuffer {
    mat4 viewProj;
    mat4 model;
    vec4 lightDirection; // xyz=方向, w=强度
    vec4 ambientColor;   // 环境光
    vec4 cameraPos;      // 相机位置
} ubo;

void main() {
    // 采样纹理
    vec4 texColor = texture(texSampler, fragUV);

    // 简单的Phong光照
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(-ubo.lightDirection.xyz);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float ambient = ubo.ambientColor.w;

    vec3 lighting = vec3(ambient + diffuse * ubo.lightDirection.w);

    // 最终颜色: 纹理 × 顶点颜色 × 光照
    outColor = vec4(texColor.rgb * fragColor.rgb * lighting, texColor.a * fragColor.a);

    // 丢弃完全透明的像素
    if (outColor.a < 0.01) {
        discard;
    }
}