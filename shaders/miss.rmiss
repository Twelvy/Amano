// Miss shader
#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadEXT vec4 payload;

void main() {
    payload = vec4(0.0, 0.0, 0.0, 0.0);
    //payload = vec4(0.3922, 0.5843, 0.9294, 1.0);
}