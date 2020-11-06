//////////////////////////////////////////////////////
// CONSTANTS
//////////////////////////////////////////////////////
const float PI = 3.14159265358979;
const float INV_PI = 0.31830988618;

const vec3 FACE_BASES[6*2] = {
    // X face
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    // -X face
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    // Y face
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0),
    // -Y face
    vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0),
    // Z face
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    // -Z face
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0)
};

// cubemap arrangement
//        _____
//       |     |
//       |  Y  |
//  _____|_____|___________
// |     |     |     |     |
// | -X  |  Z  |  X  | -Z  |
// |_____|_____|_____|_____|
//       |     |
//       | -Y  |
//       |_____|

// this simulates a cube of size 1 at the origin
const vec3 FACES_MAIN_DIRECTION[6 * 3] = {
    // X face: u is -z, v is -y
    vec3(0.0,  0.0, -1.0),
    vec3(0.0, -1.0,  0.0),
    vec3(0.5,  0.0,  0.0),
    // -X face: u is z, v is -y
    vec3( 0.0,  0.0, 1.0),
    vec3( 0.0, -1.0, 0.0),
    vec3(-0.5,  0.0, 0.0),
    // Y face: u is x, v is z
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 0.5, 0.0),
    // -Y face: u is x, v is -z
    vec3(1.0,  0.0,  0.0),
    vec3(0.0,  0.0, -1.0),
    vec3(0.0, -0.5,  0.0),
    // Z face: u is x, v is -y
    vec3(1.0,  0.0, 0.0),
    vec3(0.0, -1.0, 0.0),
    vec3(0.0,  0.0, 0.5),
    // -Z face: u is -x, v is -y
    vec3(-1.0,  0.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  0.0, -0.5)
};

//////////////////////////////////////////////////////
// FUNCTIONS
//////////////////////////////////////////////////////

// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-20-gpu-based-importance-sampling
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Get the ith Hammersley point of a set of count
vec2 hammersley2D(uint i, uint count) {
    return vec2(float(i)/float(count), radicalInverse_VdC(i));
}

vec3 hemisphereSample_uniform(float u, float v) {
    float phi = v * 2.0 * PI;
    float cosTheta = 1.0 - u;
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
 }

void buildBase(vec3 N, out vec3 U, out vec3 V, int faceIndex) {
    U = FACE_BASES[2 * faceIndex];
    V = FACE_BASES[2 * faceIndex + 1];

    V = normalize(cross(N, U));
    U = normalize(cross(V, N));
}

vec3 pixelToDirection(ivec3 pixel, float size) {
    vec2 uv = (vec2(pixel.xy) + vec2(0.5, 0.5)) / vec2(size, size) - vec2(0.5, 0.5);  // [-0.5, 0.5] range
    vec3 direction =   uv.x * FACES_MAIN_DIRECTION[3 * pixel.z]
                     + uv.y * FACES_MAIN_DIRECTION[3 * pixel.z + 1]
                     + FACES_MAIN_DIRECTION[3 * pixel.z + 2];
    return normalize(direction);
}