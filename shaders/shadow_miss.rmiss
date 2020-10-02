// Miss shader
// shadow shader
#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV float payload;

void main() {
    payload = 1.0f;
    //payload = vec4(0.3922, 0.5843, 0.9294, 1.0);
}