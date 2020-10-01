#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 worldPos;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outDepth;

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform lightInformation
{
    vec3 lightPosition;
};

void main() {
    //outColor = vec4(fragColor, 1.0);
    //outColor = vec4(fragTexCoord, 0.0, 1.0);
    vec3 lightDir = normalize(lightPosition - worldPos);
    vec3 normal = normalize(worldNormal);
    float diffuseIntensity = clamp(dot(normal, lightDir), 0.0, 1.0);

    vec4 albedo = texture(texSampler, fragTexCoord);
    outColor = diffuseIntensity * albedo;
    outNormal = vec4(normal, 0.0);
    outDepth = vec4(gl_FragCoord.z, 0.0, 0.0, 0.0);
    //outColor = texture(texSampler, fragTexCoord);
}