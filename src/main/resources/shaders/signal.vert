#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 signalColor; // 信号灯颜色 (R/G/Y)
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec4 signalColor;

void main() {
    gl_Position = push.mvp * vec4(inPosition, 1.0);
    fragUV = inUV;
    fragColor = inColor;
    fragNormal = inNormal;
    signalColor = push.signalColor;
}