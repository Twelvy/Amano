// Closest hit shader
// shadow shader
#version 460
#extension GL_NV_ray_tracing : require

layout(location = 0) rayPayloadInNV vec4 payload;

void main() {
    payload = vec4(0.0, 0.0, 0.0, 0.0);
}