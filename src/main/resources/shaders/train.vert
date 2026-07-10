#version 450

// Push Constants - 每实例的变换和颜色
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
} push;

// 顶点输入: Position(3f) + Normal(3f) + UV(2f) + Color(4f)
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

// 输出到片段着色器
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragWorldPos;

void main() {
    gl_Position = push.mvp * vec4(inPosition, 1.0);

    fragUV = inUV;
    fragColor = inColor * push.color; // 顶点颜色 × 实例颜色叠加
    fragNormal = mat3(push.mvp) * inNormal; // 简化的法线变换
    fragWorldPos = inPosition;
}