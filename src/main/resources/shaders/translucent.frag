#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    // 半透明渲染（车窗、信号灯辉光等）
    outColor = texColor * fragColor;

    // 预乘Alpha
    outColor.rgb *= outColor.a;
}