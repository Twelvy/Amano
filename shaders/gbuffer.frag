#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    outAlbedo = texture(texSampler, fragTexCoord);
    outNormal = vec4(normalize(worldNormal), 0.0);
}