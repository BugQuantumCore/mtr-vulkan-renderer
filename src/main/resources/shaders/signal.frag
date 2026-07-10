#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 signalColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, fragUV);

    // 如果灯头区域，使用信号颜色（发光效果）
    // 假设UV (0.4-0.6, 0.8-1.0) 是灯头区域
    bool isLight = fragUV.x > 0.4 && fragUV.x < 0.6 && fragUV.y > 0.8;

    vec3 finalColor;
    if (isLight) {
        // 发光效果：信号颜色 + 轻微脉动
        float pulse = 0.9 + 0.1 * sin(fragUV.x * 20.0);
        finalColor = signalColor.rgb * pulse * 1.5; // 增亮
    } else {
        finalColor = texColor.rgb * fragColor.rgb;
    }

    outColor = vec4(finalColor, texColor.a * fragColor.a);
}