// Miss shader
// shadow shader
#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec4 payload;

void main() {
    payload = vec4(1.0, 1.0, 1.0, 1.0);
    //payload = vec4(0.3922, 0.5843, 0.9294, 1.0);
}