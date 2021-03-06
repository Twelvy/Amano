#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec4 worldPos4 = ubo.model * vec4(inPosition, 1.0);
    vec4 worldNormal4 = ubo.model * vec4(inNormal, 0.0); // should be inverse transpose but we only have translation + rotation
    worldNormal = normalize(worldNormal4.xyz);

    gl_Position = ubo.proj * ubo.view * worldPos4;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}